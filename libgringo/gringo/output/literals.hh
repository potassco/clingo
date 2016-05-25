// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#ifndef _GRINGO_OUTPUT_LITERALS_HH
#define _GRINGO_OUTPUT_LITERALS_HH

#include <gringo/terms.hh>
#include <gringo/domain.hh>
#include <gringo/intervals.hh>
#include <gringo/output/aggregates.hh>
#include <gringo/output/theory.hh>

namespace Gringo { namespace Output {

// {{{1 declaration of PredicateAtom

struct PredicateAtom {
public:
    // {{{2 Atom interface

    // Constructs a valid atom without uid and generation.
    PredicateAtom(Symbol value)
    : value_(value)
    , uid_(0)
    , fact_(false)
    , generation_(0)
    , external_(false)
    , delayed_(false) { }

    // Functions that have to be implemented by all atoms.

    // Atoms can be marked as facts.
    // Atoms that are not monotone (like some aggregate atoms) are not considered
    // monotone in the recursive case because their fact status can still change
    // during grounding
    bool fact() const { return fact_; }
    // Defined and undefined atoms are distinguished.
    // Only recursion through negative literals can lead to undefined atoms.
    // Such atoms must not be imported in indices.
    // They can be defined later though, in which case they have to be imported.
    // Example: a :- not b.  b :- not a.
    bool defined() const {
        return generation_ > 0;
    }
    // The generation of the atom. This value is used by indices to determine
    // what is new and old.
    Id_t generation() const {
        assert(defined());
        return generation_ - 1;
    }
    void setGeneration(unsigned x) {
        generation_ = x + 1;
    }
    void markDelayed() {
        delayed_ = 1;
    }
    bool delayed() const {
        return delayed_;
    }
    // Returns the value associated with the atom.
    operator Symbol const &() const {
        return value_;
    }
    // }}}2

    // The functions below are not necessary for domain elements.
    // This are additional functions in symbolic atoms.

    void setFact(bool x) {
        fact_ = x;
    }
    // These functions are used to handle external atoms.
    // The sole reason for this function is to workaround a problem with libclasp,
    // which might find equivalences between external and non-external atoms.
    // In such a case it is not possible to determine which atom is the external
    // one using the clasp API. Note that the external status is never unset;
    // only the solver knows about this.
    bool isExternal() const {
        return external_;
    }
    void setExternal(bool x) {
        external_ = x;
    }
    // The uid of an atom is used in various outputs.
    bool hasUid() const {
        return uid_ > 0;
    }
    void setUid(unsigned x) {
        assert(!hasUid());
        uid_ = x + 1;
    }
    void resetUid(unsigned x) {
        assert(hasUid());
        uid_ = x + 1;
    }
    Id_t uid() const {
        assert(hasUid());
        return uid_ - 1;
    }
    void unmarkDelayed() {
        delayed_ = 0;
    }
private:
    Symbol value_;
    uint32_t uid_ : 31;
    uint32_t fact_ : 1;
    uint32_t generation_ : 30;
    uint32_t external_ : 1;
    uint32_t delayed_ : 1;
};

// {{{1 declaration of TheoryAtom

using TheoryElementVec = std::vector<Id_t>;

class TheoryAtom {
public:
    TheoryAtom(TheoryAtom &&) = default;
    TheoryAtom(TheoryAtom const &) = default;
    TheoryAtom &operator=(TheoryAtom &&) = default;
    TheoryAtom &operator=(TheoryAtom const &) = default;
    ~TheoryAtom() noexcept = default;
    // {{{2 Atom interface
    TheoryAtom(Symbol value)
    : value_(value)
    , enqueued_(false)
    , delayed_(false)
    , recursive_(true)
    , initialized_(false)
    , translated_(false)
    , simplified_(false) { }
    bool fact() const { return false; }
    bool defined() const { return generation_ > 0; }
    Id_t generation() const { assert(defined()); return generation_ - 1; }
    void setGeneration(unsigned x) { generation_ = x + 1; }
    void markDelayed() { delayed_ = 1; }
    bool delayed() const { return delayed_; }
    operator Symbol const &() const { return value_; }
    // }}}2
    bool initialized() { return initialized_; }
    void init(TheoryAtomType type, Id_t name, Id_t op, Id_t guard) {
        initialized_ = true;
        type_ = type;
        name_ = name;
        op_ = op;
        guard_ = guard;
    }
    void setRecursive(bool recursive) { recursive_ = recursive; }
    bool recursive() const { return recursive_; }
    TheoryAtomType type() const { return type_; }
    Id_t name() const { return name_; }
    Id_t op() const { return op_; }
    Id_t guard() const { return guard_; }
    bool hasGuard() const { return guard_ != InvalidId; }
    TheoryElementVec const &elems() const { return elems_; }
    void setEnqueued(bool enqueued) { enqueued_ = enqueued; }
    bool enqueued() const { return enqueued_; }
    bool translated() const { return translated_; }
    void setTranslated() { translated_ = true; }
    LiteralId lit() const { return lit_; }
    void setLit(LiteralId lit) { lit_ = lit; }
    void accumulate(Id_t elemId) { elems_.emplace_back(elemId); }
    void simplify(TheoryData const &data);
private:
    Symbol value_;
    LiteralId lit_;
    TheoryElementVec elems_;
    Id_t name_ = InvalidId;
    Id_t op_ = InvalidId;
    Id_t guard_ = InvalidId;
    Id_t generation_ = 0;
    TheoryAtomType type_;
    uint8_t enqueued_ : 1;
    uint8_t delayed_ : 1;
    uint8_t recursive_ : 1;
    uint8_t initialized_ : 1;
    uint8_t translated_ : 1;
    uint8_t simplified_ : 1;
};

// {{{1 declaration of functions to work with aggregates

int clamp(int64_t x);
bool neutral(SymVec const &tuple, AggregateFunction fun, Location const &loc);
int toInt(IntervalSet<Symbol>::LBound const &x);
int toInt(IntervalSet<Symbol>::RBound const &x);
Symbol getWeight(AggregateFunction fun, SymVec const &x);
Symbol getWeight(AggregateFunction fun, IteratorRange<SymVec::const_iterator> rng);

// {{{1 declaration of AggregateAtomRange

union ValInt {
    ValInt() { }
    ValInt(ValInt const &b) : rep(b.rep) { }
    Symbol val;
    int64_t num;
    uint64_t rep;
};

struct AggregateAtomRange {
    void init(AggregateFunction fun, DisjunctiveBounds &&bounds);
    void accumulate(SymVec const &tuple, bool fact, bool remove);
    Interval range() const;
    PlainBounds plainBounds();
    bool satisfiable() const { return bounds.intersects(range()); }
    bool fact() const { return bounds.contains(range()); }
    int64_t &intMin() { return min.num; }
    int64_t &intMax() { return max.num; }
    Symbol &valMin() { return min.val; }
    Symbol &valMax() { return max.val; }
    int64_t const &intMin() const { return min.num; }
    int64_t const &intMax() const { return max.num; }
    Symbol const &valMin() const { return min.val; }
    Symbol const &valMax() const { return max.val; }

    AggregateFunction fun = AggregateFunction::COUNT;
    DisjunctiveBounds bounds;
    ValInt min;
    ValInt max;
};

// {{{1 declaration of BodyAggregateAtom

struct BodyAggregateElements_ {
private:
    class TupleOffset;
    class ClauseOffset;

public:
    void accumulate(DomainData &data, TupleId tuple, LitVec &lits, bool &inserted, bool &fact, bool &remove);
    // NOTE: expensive (linear)
    BodyAggregateElements elems() const;

private:
    std::pair<uint64_t &, bool> insertTuple(uint64_t to);
    template <class F>
    void visitClause(F f);

private:
    HashSet<uint64_t> tuples_;
    std::vector<uint32_t> conditions_;
};

class BodyAggregateAtom {
public:
    BodyAggregateAtom(BodyAggregateAtom &&) = default;
    BodyAggregateAtom(BodyAggregateAtom const &) = delete;
    BodyAggregateAtom &operator=(BodyAggregateAtom &&) = default;
    BodyAggregateAtom &operator=(BodyAggregateAtom const &) = delete;
    ~BodyAggregateAtom() noexcept;
    // {{{2 Atom interface
    BodyAggregateAtom(Symbol value) : data_(gringo_make_unique<Data>(value)) { }
    bool fact() const { return data_->fact && (data_->monotone || !data_->recursive); }
    bool defined() const { return data_->generation > 0; }
    unsigned generation() const { assert(defined()); return data_->generation - 1; }
    void setGeneration(unsigned x) { data_->generation = x + 1; }
    void markDelayed() { data_->delayed = 1; }
    bool delayed() const { return data_->delayed; }
    operator Symbol const &() const { return data_->value; }
    // }}}2
    void setRecursive(bool recursive) { data_->recursive = recursive; }
    void init(AggregateFunction fun, DisjunctiveBounds &&bounds, bool monotone);
    void accumulate(DomainData &data, Location const &loc, SymVec const &tuple, LitVec &cond);
    Interval range() const { return data_->range.range(); }
    AggregateFunction fun() const { return data_->range.fun; }
    DisjunctiveBounds const &bounds() const { return data_->range.bounds; }
    PlainBounds plainBounds() { return data_->range.plainBounds(); }
    bool recursive() const { return data_->recursive; }
    BodyAggregateElements elems() const;
    LiteralId lit() const { return data_->lit; }
    void setLit(LiteralId lit) { data_->lit = lit; }
    bool satisfiable() const { return data_->range.satisfiable(); }
    void setEnqueued(bool enqueued) { data_->enqueued = enqueued; }
    bool enqueued() const { return data_->enqueued; }
    bool initialized() const { return data_->initialized; }
    bool translated() const { return data_->translated; }
    void setTranslated() { data_->translated = true; }

private:
    struct Data {
        Data(Symbol value)
        : value(value)
        , monotone(false)
        , recursive(true)
        , fact(false)
        , enqueued(false)
        , initialized(false)
        , delayed(false)
        , translated(false) { }

        Symbol value;
        BodyAggregateElements_ elems;
        AggregateAtomRange range;
        // This is the literal resulting from the translation of the aggregate.
        // Note that this literal includes the sign of the underlying literal.
        // This is possible with the current implemention,
        // because each aggregate atom in a domain corresponds one-to-one to a literal.
        LiteralId lit;
        // The generation is 0 if undefined > 1 if defined.
        Id_t generation = 0;
        // Only monotone aggregates can be facts in the recursive case.
        uint8_t monotone : 1;
        uint8_t recursive : 1;
        uint8_t fact : 1;
        uint8_t enqueued : 1;
        uint8_t initialized : 1;
        uint8_t delayed : 1;
        uint8_t translated : 1;
    };
    std::unique_ptr<Data> data_;
};

// {{{1 declaration of AssignmentAggregateAtom

class AssignmentAggregateData {
public:
    using Values = std::vector<Symbol>;
    AssignmentAggregateData(Symbol value, AggregateFunction fun)
    : value_(value)
    , fun_(fun) { values_.emplace_back(getNeutral(fun)); }
    AssignmentAggregateData(AssignmentAggregateData &&) = default;
    AssignmentAggregateData(AssignmentAggregateData const &) = default;
    AssignmentAggregateData &operator=(AssignmentAggregateData &&) = default;
    AssignmentAggregateData &operator=(AssignmentAggregateData const &) = default;
    ~AssignmentAggregateData() noexcept = default;
    operator Symbol const &() const { return value_; }
    void accumulate(DomainData &data, Location const &loc, SymVec const &tuple, LitVec &cond);
    BodyAggregateElements const &elems() const { return elems_; }
    void setEnqueued(bool enqueued) { enqueued_ = enqueued; }
    bool enqueued() const { return enqueued_; }
    AggregateFunction fun() const { return fun_; }
    // Note: expensive!!!
    Values values() const;
    Interval range() const;

private:
    Symbol value_;
    BodyAggregateElements elems_;
    Values values_;
    AggregateFunction fun_;
    bool enqueued_ = false;
};

class AssignmentAggregateAtom {
public:
    AssignmentAggregateAtom(AssignmentAggregateAtom &&) = default;
    AssignmentAggregateAtom(AssignmentAggregateAtom const &) = default;
    AssignmentAggregateAtom &operator=(AssignmentAggregateAtom &&) = default;
    AssignmentAggregateAtom &operator=(AssignmentAggregateAtom const &) = default;
    ~AssignmentAggregateAtom() noexcept = default;
    // {{{2 Atom interface
    AssignmentAggregateAtom(Symbol value)
    : value_(value)
    , fact_(false)
    , delayed_(false)
    , translated_(false)
    { }
    bool fact() const { return fact_ && !recursive_; }
    bool defined() const { return generation_ > 0; }
    unsigned generation() const { assert(defined()); return generation_ - 1; }
    void setGeneration(unsigned x) { generation_ = x + 1; }
    void markDelayed() { delayed_ = 1; }
    bool delayed() const { return delayed_; }
    operator Symbol const &() const { return value_; }
    // }}}2
    void setFact(bool fact) { fact_ = fact; }
    void setRecursive(bool recursive) { recursive_ = recursive; }
    void init(AggregateFunction fun, DisjunctiveBounds &&bounds, bool monotone);
    bool recursive() const { return recursive_; }
    LiteralId lit() const { return lit_; }
    void setLit(LiteralId lit) { lit_ = lit; }
    void setData(Id_t offset) { data_ = offset; }
    Id_t data() const { return data_; }
    bool translated() const { return translated_; }
    void setTranslated() { translated_ = true; }

private:
    Symbol value_;
    Symbol bound_;
    LiteralId lit_;
    // The generation is 0 if undefined > 1 if defined.
    Id_t generation_ = 0;
    Id_t data_ = InvalidId;
    uint8_t recursive_ : 1;
    uint8_t fact_ : 1;
    uint8_t delayed_ : 1;
    uint8_t translated_ : 1;
};

// {{{1 declaration of ConjunctionAtom

class ConjunctionElement {
public:
    ConjunctionElement(Symbol value)
    : value_(value) { }
    ConjunctionElement(ConjunctionElement &&) = default;
    ConjunctionElement(ConjunctionElement const &) = default;
    ConjunctionElement &operator=(ConjunctionElement &&) = default;
    ConjunctionElement &operator=(ConjunctionElement const &) = default;
    ~ConjunctionElement() noexcept = default;

    operator Symbol const & () const { return value_; }
    bool needsSemicolon() const;
    bool isSimple(DomainData &data) const;
    void print(PrintPlain out) const;
    void accumulateCond(DomainData &data, LitVec &cond, Id_t &blocked, Id_t &fact);
    void accumulateHead(DomainData &data, LitVec &cond, Id_t &blocked, Id_t &fact);
    Formula const &heads() const { return heads_; }
    Formula const &bodies() const { return bodies_; }

private:
    Symbol value_;
    Formula heads_;
    Formula bodies_;
};

inline PrintPlain &operator<<(PrintPlain &out, ConjunctionElement const &x) {
    x.print(out);
    return out;
}

class ConjunctionAtom {
public:
    using Elements = UniqueVec<ConjunctionElement, std::hash<Symbol>, std::equal_to<Symbol>>;
    ConjunctionAtom(ConjunctionAtom &&) = default;
    ConjunctionAtom(ConjunctionAtom const &) = default;
    ConjunctionAtom &operator=(ConjunctionAtom &&) = default;
    ConjunctionAtom &operator=(ConjunctionAtom const &) = default;
    ~ConjunctionAtom() noexcept = default;
    // {{{2 Atom interface
    ConjunctionAtom(Symbol value)
    : value_(value)
    , condRecursive_(true)
    , headRecursive_(true)
    , delayed_(false)
    , enqueued_(false)
    , translated_(false)
    { }
    bool fact() const { return fact_ == 0 && !condRecursive_; }
    bool defined() const { return generation_ > 0; }
    unsigned generation() const { assert(defined()); return generation_ - 1; }
    void setGeneration(unsigned x) { generation_ = x + 1; }
    void markDelayed() { delayed_ = 1; }
    bool delayed() const { return delayed_; }
    operator Symbol const &() const { return value_; }
    // }}}2
    LiteralId lit() const { return lit_; }
    void setLit(LiteralId lit) { lit_ = lit; }
    bool blocked() const { return blocked_ > 0; }
    Elements const &elems() const { return elems_; }
    void setEnqueued(bool enqueued) { enqueued_ = enqueued; }
    bool enqueued() const { return enqueued_; }
    void accumulateCond(DomainData &data, Symbol elem, LitVec &cond);
    void accumulateHead(DomainData &data, Symbol elem, LitVec &cond);
    void init(bool headRecursive, bool condRecursive);
    bool recursive() const;
    bool nonmonotone() const;
    bool translated() const { return translated_; }
    void setTranslated() { translated_ = true; }

private:
    Elements elems_;
    Symbol value_;
    LiteralId lit_;
    // The generation is 0 if undefined > 1 if defined.
    Id_t generation_ = 0;
    Id_t blocked_ = 0;
    Id_t fact_ = 0;
    uint8_t condRecursive_ : 1;
    uint8_t headRecursive_ : 1;
    uint8_t delayed_ : 1;
    uint8_t enqueued_ : 1;
    uint8_t translated_ : 1;
};

// {{{1 declaration of DisjointAtom

class DisjointElement {
public:
    DisjointElement(CSPGroundAdd &&value, int fixed, ClauseId cond);
    DisjointElement(DisjointElement &&) noexcept = default;
    DisjointElement &operator=(DisjointElement &&) noexcept = default;
    ~DisjointElement() noexcept = default;

    void printPlain(PrintPlain out) const;
    CSPGroundAdd const &value() const { return value_; }
    ClauseId const &cond() const { return cond_; }
    int fixed() const { return fixed_; }
private:
    CSPGroundAdd value_;
    ClauseId cond_;
    int fixed_;
};

using DisjointElemVec = std::vector<DisjointElement>;
using DisjointElemSet = UniqueVec<std::pair<TupleId, DisjointElemVec>, HashFirst<TupleId>, EqualToFirst<TupleId>>;

class DisjointAtom {
public:
    DisjointAtom(DisjointAtom &&) = default;
    DisjointAtom(DisjointAtom const &) = default;
    DisjointAtom &operator=(DisjointAtom &&) noexcept = default;
    DisjointAtom &operator=(DisjointAtom const &) = default;
    ~DisjointAtom() noexcept = default;
    // {{{2 Atom interface
    DisjointAtom(Symbol value)
    : value_(value)
    , generation_(0)
    , delayed_(false)
    , enqueued_(false)
    , recursive_(true)
    , translated_(false) { }
    bool fact() const { return !recursive_ && elems_.size() <= 1; }
    bool defined() const { return generation_ > 0; }
    Id_t generation() const { assert(defined()); return generation_ - 1; }
    void setGeneration(unsigned x) { generation_ = x + 1; }
    void markDelayed() { delayed_ = 1; }
    bool delayed() const { return delayed_; }
    operator Symbol const &() const { return value_; }
    // }}}2
    void init(bool recursive) { recursive_ = recursive; }
    bool recursive() const { return recursive_; }
    void setEnqueued(bool enqueued) { enqueued_ = enqueued; }
    bool enqueued() const { return enqueued_; }
    void accumulate(DomainData &data, SymVec const &tuple, CSPGroundAdd &&value, int fixed, LitVec const &lits);
    bool translated() const { return translated_; }
    void setTranslated() { translated_ = true; }
    LiteralId lit() const { return lit_; }
    void setLit(LiteralId lit) { lit_ = lit; }
    DisjointElemSet const &elems() const { return elems_; }
    bool translate(DomainData &data, Translator &x, MessagePrinter &log);

private:
    Symbol value_;
    DisjointElemSet elems_;
    LiteralId lit_;
    Id_t generation_ = 0;
    uint8_t delayed_ : 1;
    uint8_t enqueued_ : 1;
    uint8_t recursive_ : 1;
    uint8_t translated_ : 1;
};

// {{{1 declaration of DisjunctionAtom

class DisjunctionElement {
public:
    DisjunctionElement(Symbol value)
    : value_(value) { }
    DisjunctionElement(DisjunctionElement &&) = default;
    DisjunctionElement(DisjunctionElement const &) = default;
    DisjunctionElement &operator=(DisjunctionElement &&) = default;
    DisjunctionElement &operator=(DisjunctionElement const &) = default;
    ~DisjunctionElement() noexcept = default;

    operator Symbol const & () const { return value_; }
    void print(PrintPlain out) const;
    void accumulateCond(DomainData &data, LitVec &cond, Id_t &fact);
    void accumulateHead(DomainData &data, LitVec &cond, Id_t &fact);
    bool bodyIsTrue() const;
    bool bodyIsFalse() const;
    bool headIsTrue() const;
    bool headIsFalse() const;
    Formula const &heads() const { return heads_; }
    Formula const &bodies() const { return bodies_; }

private:
    Symbol value_;
    Formula heads_;
    Formula bodies_;
};

inline PrintPlain &operator<<(PrintPlain &out, DisjunctionElement const &x) {
    x.print(out);
    return out;
}

class DisjunctionAtom {
public:
    using Elements = UniqueVec<DisjunctionElement, std::hash<Symbol>, std::equal_to<Symbol>>;
    DisjunctionAtom(DisjunctionAtom &&) = default;
    DisjunctionAtom(DisjunctionAtom const &) = default;
    DisjunctionAtom &operator=(DisjunctionAtom &&) = default;
    DisjunctionAtom &operator=(DisjunctionAtom const &) = default;
    ~DisjunctionAtom() noexcept = default;
    // {{{2 Atom interface
    DisjunctionAtom(Symbol value)
    : value_(value)
    , bodyFact_(false)
    , recursive_(true)
    , delayed_(false)
    , enqueued_(false)
    , translated_(false)
    { }
    // This function could be used to indicate that the head literal.
    // occurs in a rule with an empty body.
    bool fact() const { return bodyFact_; }
    // This function indicates that the disjunction contains a fact
    // and, hence, does not derive anything.
    bool defined() const { return generation_ > 0; }
    unsigned generation() const { assert(defined()); return generation_ - 1; }
    void setGeneration(unsigned x) { generation_ = x + 1; }
    void markDelayed() { delayed_ = 1; }
    bool delayed() const { return delayed_; }
    operator Symbol const &() const { return value_; }
    // }}}2
    void setFact(bool fact) { bodyFact_ = fact; }
    bool headFact() const { return headFact_ > 0 && !recursive_; }
    LiteralId lit() const { return lit_; }
    void setLit(LiteralId lit) { lit_ = lit; }
    Elements const &elems() const { return elems_; }
    void setEnqueued(bool enqueued) { enqueued_ = enqueued; }
    bool enqueued() const { return enqueued_; }
    void accumulateCond(DomainData &data, Symbol elem, LitVec &cond);
    void accumulateHead(DomainData &data, Symbol elem, LitVec &cond);
    void init(bool recursive) { recursive_ = recursive; }
    bool recursive() const { return recursive_; }
    void simplify(bool &headFact);
    bool translated() const { return translated_; }
    void setTranslated() { translated_ = true; }

private:
    Elements elems_;
    Symbol value_;
    LiteralId lit_;
    // The generation is 0 if undefined > 1 if defined.
    Id_t generation_ = 0;
    Id_t headFact_ = 0;
    uint8_t bodyFact_ : 1;
    uint8_t recursive_ : 1;
    uint8_t delayed_ : 1;
    uint8_t enqueued_ : 1;
    uint8_t translated_ : 1;
};

// {{{1 declaration of HeadAggregateAtom

class HeadAggregateAtom {
public:
    HeadAggregateAtom(HeadAggregateAtom &&) = default;
    HeadAggregateAtom(HeadAggregateAtom const &) = default;
    HeadAggregateAtom &operator=(HeadAggregateAtom &&) = default;
    HeadAggregateAtom &operator=(HeadAggregateAtom const &) = default;
    ~HeadAggregateAtom() noexcept = default;
    // {{{2 Atom interface
    HeadAggregateAtom(Symbol value)
    : value_(value)
    , recursive_(true)
    , fact_(false)
    , enqueued_(false)
    , initialized_(false)
    , delayed_(false)
    , translated_(false) { }
    // This function could be used to indicate that the head literal.
    // occurs in a rule with an empty body.
    bool fact() const { return false; }
    bool defined() const { return generation_ > 0; }
    unsigned generation() const { assert(defined()); return generation_ - 1; }
    void setGeneration(unsigned x) { generation_ = x + 1; }
    void markDelayed() { delayed_ = 1; }
    bool delayed() const { return delayed_; }
    operator Symbol const &() const { return value_; }
    // }}}2
    // This function indicates that the bounds of the aggregate are tivially satisfied.
    // (There is not really a need for it at the moment because, unlike with disjunctions,
    // no simplifications can be performed with this information.)
    bool headFact() const { return fact_ && !recursive_; }
    void setRecursive(bool recursive) { recursive_ = recursive; }
    void init(AggregateFunction fun, DisjunctiveBounds &&bounds);
    void accumulate(DomainData &data, Location const &loc, SymVec const &tuple, LiteralId head, LitVec &cond, MessagePrinter &log);
    Interval range() const { return range_.range(); }
    AggregateFunction fun() const { return range_.fun; }
    DisjunctiveBounds const &bounds() const { return range_.bounds; }
    PlainBounds plainBounds() { return range_.plainBounds(); }
    bool recursive() const { return recursive_; }
    HeadAggregateElements const &elems() const { return elems_; }
    bool satisfiable() const { return range_.satisfiable(); }
    void setEnqueued(bool enqueued) { enqueued_ = enqueued; }
    bool enqueued() const { return enqueued_; }
    bool initialized() const { return initialized_; }
    bool translated() const { return translated_; }
    void setTranslated() { translated_ = true; }
    LiteralId lit() const { return lit_; }
    void setLit(LiteralId lit) { lit_ = lit; }
private:
    Symbol value_;
    LiteralId lit_;
    HeadAggregateElements elems_;
    AggregateAtomRange range_;
    Id_t generation_ = 0;
    uint8_t recursive_ : 1;
    uint8_t fact_ : 1;
    uint8_t enqueued_ : 1;
    uint8_t initialized_ : 1;
    uint8_t delayed_ : 1;
    uint8_t translated_ : 1;
};

// }}}1

// {{{1 declaration of PredicateDomain

class PredicateDomain : public AbstractDomain<PredicateAtom> {
public:
    explicit PredicateDomain(Sig sig)
    : sig_(sig) { }

    using AbstractDomain<PredicateAtom>::define;
    // Defines (adds) an atom setting its generation and fact status.
    // Returns a tuple indicating its postion, wheather it was inserted, and wheather it was a fact before.
    std::tuple<Iterator, bool, bool> define(Symbol x, bool fact) {
        auto ret = define(x);
        bool wasfact = !ret.second && ret.first->fact();
        if (fact) { ret.first->setFact(true); }
        return std::forward_as_tuple(ret.first, ret.second, wasfact);
    }

    // This offset divides elements added at the current and previous incremental steps.
    // It is used to only output newly inserted atoms, for projection, and classical negation.
    SizeType incOffset() const {
        return incOffset_;
    }

    void incNext() {
        // It is necessary to hide undefined literals because they should not interfere
        // with future definitions of the same atom.
        // In fact, they could be completely removed from the domain.
        // This cannot be done here but the cleanup function of the output,
        // will remove such atoms.
        for (auto it = begin() + incOffset(), ie = end(); it != ie; ++it) {
            if (!it->defined()) { hide(it); }
        }
        incOffset_ = size();
    }

    // This offset keeps track of atoms already added to the output table.
    // TODO: maybe this can be merged with incNext.
    SizeType showOffset() const {
        return showOffset_;
    }

    void showNext() {
        showOffset_ = size();
    }

    void clear() {
        AbstractDomain<PredicateAtom>::clear();
        incOffset_  = 0;
        showOffset_ = 0;
    }

    Sig const &sig() const {
        return sig_;
    }

    operator Sig const &() const {
        return sig_;
    }

    std::pair<Id_t, Id_t> cleanup(AssignmentLookup assignment, Mapping &map) {
        Id_t facts = 0;
        Id_t deleted = 0;
        Id_t oldOffset = 0;
        Id_t newOffset = 0;
        reset();
        //std::cerr << "cleaning " << *sig_ << std::endl;
        atoms_.erase([&](PredicateAtom &atom) {
            atom.setGeneration(0);
            atom.unmarkDelayed();
            if (!atom.defined()) {
                ++deleted;
                ++oldOffset;
                return true;
            }
            if (atom.hasUid()) {
                auto value = assignment(atom.uid());
                if (!value.first) {
                    switch (value.second) {
                        case Potassco::Value_t::True: {
                            // NOTE: externals cannot become facts here
                            //       because they might get new definitions while grounding
                            //       because there is no distinction between true and weak true
                            //       these definitions might be skipped if a weak true external
                            //       is made a fact here
                            if (!atom.fact()) { ++facts; }
                            atom.setFact(true);
                            break;
                        }
                        case Potassco::Value_t::False: {
                            ++deleted;
                            ++oldOffset;
                            return true;
                        }
                        default: { break; }
                    }
                }
            }
            //std::cerr << "  mapping " << static_cast<Symbol>(atom) << " from " << oldOffset << " to " << newOffset << std::endl;
            map.add(oldOffset, newOffset);
            ++oldOffset;
            ++newOffset;
            return false;
        });
        /*
        std::cerr << "remaining atoms: ";
        for (auto &atom : atoms_) {
            std::cerr << " " << static_cast<Symbol>(atom) << "=" << (atoms_.find(static_cast<Symbol>(atom)) != atoms_.end()) << "/" << atom.generation() << "/" << atom.defined() << "/" << atom.delayed();
        }
        std::cerr << std::endl;
        */
        delayed_.clear();
        generation_ = 1;
        initOffset_ = atoms_.size();
        initDelayedOffset_ = 0;
        incOffset_ = map.bound(incOffset_);
        showOffset_ = map.bound(showOffset_);
        return {facts, deleted};
    }
private:
    Sig sig_;
    SizeType incOffset_ = 0;
    SizeType showOffset_ = 0;
};
using UPredDom = std::unique_ptr<PredicateDomain>;

struct UPredDomHash : std::hash<Sig> {
    using std::hash<Sig>::operator();
    size_t operator()(UPredDom const &dom) const {
        return std::hash<Sig>::operator()(*dom);
    }
};

struct UPredDomEqualTo : private std::equal_to<Sig> {
    bool operator()(UPredDom const &a, Sig const &b) const {
        return std::equal_to<Sig>::operator()(*a, b);
    }
    bool operator()(UPredDom const &a, UPredDom const &b) const {
        return std::equal_to<Sig>::operator()(*a, *b);
    }
};

using PredDomMap = UniqueVec<std::unique_ptr<PredicateDomain>, UPredDomHash, UPredDomEqualTo>;

// {{{1 declaration of TheoryDomain

class TheoryDomain : public AbstractDomain<TheoryAtom> {
public:
private:
};

// {{{1 declaration of BodyAggregateDomain

class BodyAggregateDomain : public AbstractDomain<BodyAggregateAtom> {
public:
private:
};

// {{{1 declaration of AssignmentAggregateDomain

class AssignmentAggregateDomain : public AbstractDomain<AssignmentAggregateAtom> {
private:
    using Data = UniqueVec<AssignmentAggregateData, HashKey<Symbol>, EqualToKey<Symbol>>;
public:
    Id_t data(Symbol value, AggregateFunction fun) {
        auto ret = data_.findPush(value, value, fun);
        return data_.offset(ret.first);
    }
    AssignmentAggregateData &data(Id_t offset) { return data_[offset]; }
    AssignmentAggregateData const &data(Id_t offset) const { return data_[offset]; }
private:
    Data data_;
};

// {{{1 declaration of ConjunctionDomain

class ConjunctionDomain : public AbstractDomain<ConjunctionAtom> {
public:
private:
};
// {{{1 declaration of DisjointDomain

class DisjointDomain : public AbstractDomain<DisjointAtom> {
public:
private:
};
// {{{1 declaration of DisjunctionDomain

class DisjunctionDomain : public AbstractDomain<DisjunctionAtom> {
public:
private:
};
// {{{1 declaration of HeadAggregateDomain

class HeadAggregateDomain : public AbstractDomain<HeadAggregateAtom> {
public:
private:
};

// }}}1

// {{{1 declaration of AuxLiteral

class AuxLiteral : public Literal {
public:
    AuxLiteral(DomainData &data, LiteralId id);
    LiteralId toId() const override;
    LiteralId translate(Translator &x) override;
    void printPlain(PrintPlain out) const override;
    bool isIncomplete() const override;
    Lit_t uid() const override;
    bool isPositive() const override { return id_.sign() == NAF::POS; }
    bool isHeadAtom() const override;
    LiteralId simplify(Mappings &mappings, AssignmentLookup lookup) const override;
    bool isTrue(IsTrueLookup) const override;
    virtual ~AuxLiteral() noexcept;

private:
    DomainData &data_;
    LiteralId id_;
};

// {{{1 declaration of PredicateLiteral

class PredicateLiteral : public Literal {
public:
    PredicateLiteral(DomainData &data, LiteralId id);
    bool isAtomFromPreviousStep() const override;
    bool isHeadAtom() const override;
    void printPlain(PrintPlain out) const override;
    bool isIncomplete() const override;
    LiteralId toId() const override;
    LiteralId translate(Translator &x) override;
    Lit_t uid() const override;
    bool isPositive() const override { return id_.sign() == NAF::POS; }
    LiteralId simplify(Mappings &mappings, AssignmentLookup lookup) const override;
    bool isTrue(IsTrueLookup) const override;
    virtual ~PredicateLiteral() noexcept;

private:
    DomainData &data_;
    LiteralId   id_;
};

// {{{1 declaration of TheoryLiteral

class TheoryLiteral : public Literal {
public:
    TheoryLiteral(DomainData &data, LiteralId id);
    bool isHeadAtom() const override;
    LiteralId toId() const override;
    LiteralId translate(Translator &x) override;
    void printPlain(PrintPlain out) const override;
    bool isIncomplete() const override;
    Lit_t uid() const override;
    std::pair<LiteralId,bool>delayedLit() override;
    virtual ~TheoryLiteral() noexcept;

private:
    DomainData &data_;
    LiteralId id_;
};

// {{{1 declaration of CSPLiteral

class CSPLiteral : public Literal {
public:
    CSPLiteral(DomainData &data, LiteralId id);
    CSPGroundLit const &atom() const;
    bool isBound(Symbol &value, bool negate) const override;
    void updateBound(std::vector<CSPBound> &bounds, bool negate) const override;
    void printPlain(PrintPlain out) const override;
    bool isIncomplete() const override;
    LiteralId toId() const override { return id_; }
    LiteralId translate(Translator &x) override;
    Lit_t uid() const override;
    virtual ~CSPLiteral() noexcept;

private:
    DomainData &data_;
    LiteralId   id_;
};

// {{{1 declaration of BodyAggregateLiteral

class BodyAggregateLiteral : public Literal {
public:
    BodyAggregateLiteral(DomainData &data, LiteralId id);
    LiteralId toId() const override;
    LiteralId translate(Translator &x) override;
    void printPlain(PrintPlain out) const override;
    bool isIncomplete() const override;
    Lit_t uid() const override;
    std::pair<LiteralId,bool>delayedLit() override;
    BodyAggregateDomain &dom() const;
    virtual ~BodyAggregateLiteral() noexcept;

private:
    DomainData &data_;
    LiteralId id_;
};

// {{{1 declaration of AssignmentAggregateLiteral

class AssignmentAggregateLiteral : public Literal {
public:
    AssignmentAggregateLiteral(DomainData &data, LiteralId id);
    LiteralId toId() const override;
    LiteralId translate(Translator &x) override;
    void printPlain(PrintPlain out) const override;
    bool isIncomplete() const override;
    Lit_t uid() const override;
    std::pair<LiteralId,bool>delayedLit() override;
    AssignmentAggregateDomain &dom() const;
    virtual ~AssignmentAggregateLiteral() noexcept;

private:
    DomainData &data_;
    LiteralId id_;
};

// {{{1 declaration of ConjunctionLiteral

class ConjunctionLiteral : public Literal {
public:
    ConjunctionLiteral(DomainData &data, LiteralId id);
    LiteralId toId() const override;
    LiteralId translate(Translator &x) override;
    void printPlain(PrintPlain out) const override;
    bool isIncomplete() const override;
    Lit_t uid() const override;
    std::pair<LiteralId,bool>delayedLit() override;
    bool needsSemicolon() const override;
    ConjunctionDomain &dom() const;
    virtual ~ConjunctionLiteral() noexcept;

private:
    DomainData &data_;
    LiteralId id_;
};

// {{{1 declaration of DisjointLiteral

class DisjointLiteral : public Literal {
public:
    DisjointLiteral(DomainData &data, LiteralId id);
    LiteralId toId() const override;
    LiteralId translate(Translator &x) override;
    void printPlain(PrintPlain out) const override;
    bool isIncomplete() const override;
    Lit_t uid() const override;
    std::pair<LiteralId,bool>delayedLit() override;
    DisjointDomain &dom() const;
    virtual ~DisjointLiteral() noexcept;

private:
    DomainData &data_;
    LiteralId id_;
};

// {{{1 declaration of DisjunctionLiteral

class DisjunctionLiteral : public Literal {
public:
    DisjunctionLiteral(DomainData &data, LiteralId id);
    LiteralId toId() const override;
    LiteralId translate(Translator &x) override;
    void printPlain(PrintPlain out) const override;
    bool isIncomplete() const override;
    Lit_t uid() const override;
    std::pair<LiteralId,bool>delayedLit() override;
    DisjunctionDomain &dom() const;
    bool isBound(Symbol &value, bool negate) const override;
    void updateBound(std::vector<CSPBound> &bound, bool negate) const override;
    virtual ~DisjunctionLiteral() noexcept;

private:
    DomainData &data_;
    LiteralId id_;
};

// {{{1 declaration of HeadAggregateLiteral

class HeadAggregateLiteral : public Literal {
public:
    HeadAggregateLiteral(DomainData &data, LiteralId id);
    LiteralId toId() const override;
    LiteralId translate(Translator &x) override;
    bool isHeadAtom() const override { return true; }
    void printPlain(PrintPlain out) const override;
    bool isIncomplete() const override;
    Lit_t uid() const override;
    std::pair<LiteralId,bool>delayedLit() override;
    HeadAggregateDomain &dom() const;
    virtual ~HeadAggregateLiteral() noexcept;

private:
    DomainData &data_;
    LiteralId id_;
};

// }}}1

// {{{1 declaration of DomainData

class DomainData : private Gringo::TheoryData {
    using Tuples = UniqueVecVec<2, Symbol>;
    using Clauses = UniqueVecVec<2, LiteralId>;
    using Formulas = UniqueVecVec<2, std::pair<Id_t,Id_t>, value_hash<std::pair<Id_t,Id_t>>>;
    using CSPAtoms = UniqueVec<CSPGroundLit, value_hash<CSPGroundLit>>;
public:
    DomainData(Potassco::TheoryData &theory)
    : theory_(theory) { }
    DomainData(DomainData &&) = default;
    DomainData& operator=(DomainData&&) = delete;
    ~DomainData() noexcept = default;

    PredicateDomain &add(Sig const &sig) {
        auto it(predDomains_.find(sig));
        if (it == predDomains_.end()) {
            it = predDomains_.push(gringo_make_unique<PredicateDomain>(sig)).first;
            it->get()->setDomainOffset(predDomains_.offset(it));
        }
        return **it;
    }
    PredDomMap &predDoms() { return predDomains_; }
    PredDomMap const &predDoms() const { return predDomains_; }
    PredicateDomain &predDom(Id_t offset) { return *predDomains_[offset]; }
    PredicateDomain const &predDom(Id_t offset) const { return *predDomains_[offset]; }
    template <class D, typename... Args>
    D &add(Args &&... args) {
        domains_.emplace_back(gringo_make_unique<D>(std::forward<Args>(args)...));
        domains_.back()->setDomainOffset(domains_.size() - 1);
        return static_cast<D&>(*domains_.back());
    }
    template <class D>
    D &getDom(Id_t offset) { return static_cast<D&>(*domains_[offset]); }
    template <class D>
    D const &getDom(Id_t offset) const { return static_cast<D const &>(*domains_[offset]); }
    template <class D>
    auto getAtom(Id_t dom, Id_t offset) -> decltype(std::declval<D>()[offset]) {
        return getDom<D>(dom)[offset];
    }
    template <class D>
    auto getAtom(Id_t dom, Id_t offset) const -> decltype(std::declval<D const>()[offset]) {
        return getDom<D>(dom)[offset];
    }
    template <class D>
    auto getAtom(LiteralId lit) -> decltype(std::declval<D>()[lit.offset()]) {
        return getDom<D>(lit.domain())[lit.offset()];
    }
    template <class D>
    auto getAtom(LiteralId lit) const -> decltype(std::declval<D const>()[lit.offset()]) {
        return getDom<D const>(lit.domain())[lit.offset()];
    }
    Potassco::Atom_t newAtom() { return ++atoms_; }
    LiteralId newAux(NAF naf = NAF::POS) { return {naf, Gringo::Output::AtomType::Aux, newAtom(), 0}; }
    LiteralId newDelayed(NAF naf = NAF::POS) { return {naf, Gringo::Output::AtomType::Aux, newAtom(), 1}; }
    LiteralId getTrueLit() {
        if (!trueLit_.valid()) { trueLit_ = newAux(NAF::NOT); }
        return trueLit_;
    }
    Gringo::Output::TheoryData &theory() { return theory_; }
    Gringo::Output::TheoryData const &theory() const { return theory_; }
    TupleId tuple(SymVec const &cond) {
        return {tuples_.push(cond).first, static_cast<Id_t>(cond.size())};
    }
    IteratorRange<SymVec::const_iterator> tuple(TupleId pos) {
        auto it = tuples_.at(pos.offset, pos.size);
        return {it, it + pos.size};
    }
    std::pair<Id_t,Id_t> clause(LitVec &cond) {
        sort_unique(cond);
        return {clauses_.push(cond).first, cond.size()};
    }
    std::pair<Id_t,Id_t> clause(LitVec &&cond) { return clause(cond); }
    IteratorRange<LitVec::const_iterator> clause(std::pair<Id_t, Id_t> pos) { return clause(pos.first, pos.second); }
    IteratorRange<LitVec::const_iterator> clause(Id_t id, Id_t size) {
        auto it = clauses_.at(id, size);
        return {it, it + size};
    }
    std::pair<Id_t,Id_t> formula(Formula &&lits) {
        sort_unique(lits);
        return {formulas_.push(lits).first, lits.size()};
    }
    IteratorRange<Formula::iterator> formula(Id_t id, Id_t size) {
        auto it = formulas_.at(id, size);
        return {it, it + size};
    }
    CSPGroundLit const &cspAtom(Id_t offset) {
        return cspAtoms_[offset];
    }
    Id_t cspAtom(CSPGroundLit &&lit) {
        auto ret = cspAtoms_.push(std::move(lit));
        return cspAtoms_.offset(ret.first);
    }
    IteratorRange<Formula::iterator> formula(std::pair<Id_t, Id_t> pos) { return formula(pos.first, pos.second); }
    // This should be called before grounding a new step
    // to get rid of unnecessary temporary data.
    void reset() {
        theory_.reset();
        clauses_.clear();
        formulas_.clear();
        domains_.clear();
    }
    bool canSimplify() const {
        return domains_.empty() && clauses_.empty() && formulas_.empty() && theory_.empty();
    }
    Gringo::TheoryData const &theoryInterface() const { return *this; }
private:
    Gringo::TheoryData::TermType termType(Id_t) const override;
    int termNum(Id_t value) const override;
    char const *termName(Id_t value) const override;
    Potassco::IdSpan termArgs(Id_t value) const override;
    Potassco::IdSpan elemTuple(Id_t value) const override;
    Potassco::LitSpan elemCond(Id_t value) const override;
    Potassco::Lit_t elemCondLit(Id_t value) const override;
    Potassco::IdSpan atomElems(Id_t value) const override;
    Potassco::Id_t atomTerm(Id_t value) const override;
    bool atomHasGuard(Id_t value) const override;
    Potassco::Lit_t atomLit(Id_t value) const override;
    std::pair<char const *, Id_t> atomGuard(Id_t value) const override;
    Potassco::Id_t numAtoms() const override;
    std::string termStr(Id_t value) const override;
    std::string elemStr(Id_t value) const override;
    std::string atomStr(Id_t value) const override;

private:
    std::vector<Lit_t> tempLits_;
    Gringo::Output::TheoryData theory_;
    PredDomMap predDomains_;
    UDomVec domains_;
    Potassco::Atom_t atoms_ = 0;
    Clauses clauses_;
    Tuples tuples_;
    Formulas formulas_;
    CSPAtoms cspAtoms_;
    LiteralId trueLit_;
};

template <class M, typename... Args>
auto call(DomainData &data, LiteralId lit, M m, Args&&... args) -> decltype((std::declval<Literal*>()->*m)(std::forward<Args>(args)...)) {
    assert(lit.valid());
    switch (lit.type()) {
        case AtomType::Aux: {
            AuxLiteral x(data, lit);
            return (x.*m)(std::forward<Args>(args)...);
        }
        case AtomType::Predicate: {
            PredicateLiteral x(data, lit);
            return (x.*m)(std::forward<Args>(args)...);
        }
        case AtomType::BodyAggregate: {
            BodyAggregateLiteral x(data, lit);
            return (x.*m)(std::forward<Args>(args)...);
        }
        case AtomType::HeadAggregate: {
            HeadAggregateLiteral x(data, lit);
            return (x.*m)(std::forward<Args>(args)...);
        }
        case AtomType::AssignmentAggregate: {
            AssignmentAggregateLiteral x(data, lit);
            return (x.*m)(std::forward<Args>(args)...);
        }
        case AtomType::Conjunction: {
            ConjunctionLiteral x(data, lit);
            return (x.*m)(std::forward<Args>(args)...);
        }
        case AtomType::LinearConstraint: {
            CSPLiteral x(data, lit);
            return (x.*m)(std::forward<Args>(args)...);
        }
        case AtomType::Disjunction: {
            DisjunctionLiteral x(data, lit);
            return (x.*m)(std::forward<Args>(args)...);
        }
        case AtomType::Disjoint: {
            DisjointLiteral x(data, lit);
            return (x.*m)(std::forward<Args>(args)...);
        }
        case AtomType::Theory: {
            TheoryLiteral x(data, lit);
            return (x.*m)(std::forward<Args>(args)...);
        }
    }
    throw std::logic_error("cannot happen");
}

// }}}1

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_LITERALS_HH

