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
#include <gringo/unique_list.hh>
#include <gringo/output/literal.hh>

namespace Gringo { namespace Output {

// {{{ declaration of AuxLiteral

struct AuxAtom {
    AuxAtom(unsigned name);
    int lparseUid(LparseOutputter &out);
    unsigned name;
    unsigned uid = 0;
};
using SAuxAtom    = std::shared_ptr<AuxAtom>;
using SAuxAtomVec = std::vector<SAuxAtom>;

std::ostream &operator<<(std::ostream &out, AuxAtom const &x);

struct AuxLiteral : Literal {
    AuxLiteral(SAuxAtom atom, NAF naf);
    virtual SAuxAtom isAuxAtom() const;
    virtual AuxLiteral *clone() const;
    virtual ULit toLparse(LparseTranslator &x);
    virtual void printPlain(std::ostream &out) const;
    virtual bool isIncomplete() const;
    virtual int lparseUid(LparseOutputter &out) const;
    virtual size_t hash() const;
    virtual bool operator==(Literal const &x) const;
    virtual ULit negateLit(LparseTranslator &x) const;
    virtual bool invertible() const;
    virtual void invert();
    virtual bool isPositive() const { return naf == NAF::POS; }
    virtual ~AuxLiteral();
    
    SAuxAtom atom;
    NAF      naf;
};

// }}}
// {{{ declaration of BooleanLiteral

struct BooleanLiteral : Literal {
    BooleanLiteral(bool value);
    virtual BooleanLiteral *clone() const;
    virtual ULit toLparse(LparseTranslator &x);
    virtual void printPlain(std::ostream &out) const;
    virtual bool isIncomplete() const;
    virtual int lparseUid(LparseOutputter &out) const;
    virtual size_t hash() const;
    virtual bool operator==(Literal const &x) const;
    virtual bool isPositive() const { return false; }
    virtual ~BooleanLiteral();

    bool value;
};

// }}}
// {{{ declaration of PredicateLiteral

struct PredicateLiteral : Literal {
    PredicateLiteral();
    PredicateLiteral(NAF naf, PredicateDomain::element_type &repr);
    virtual PredicateDomain::element_type *isAtom() const;
    virtual ULit negateLit(LparseTranslator &x) const;
    virtual void printPlain(std::ostream &out) const;
    virtual bool isIncomplete() const;
    virtual PredicateLiteral *clone() const;
    virtual ULit toLparse(LparseTranslator &x);
    virtual int lparseUid(LparseOutputter &out) const;
    virtual size_t hash() const;
    virtual bool operator==(Literal const &x) const;
    virtual bool invertible() const;
    virtual void invert();
    virtual bool isPositive() const { return naf == NAF::POS; }
    virtual std::pair<bool, TruthValue> getTruth(AssignmentLookup);
    virtual ~PredicateLiteral();

    NAF                            naf  = NAF::POS;
    PredicateDomain::element_type *repr = nullptr;
};

// }}}
// {{{ declaration of BodyAggregateState

int clamp(int64_t x);
bool neutral(ValVec const &tuple, AggregateFunction fun, Location const &loc);
int toInt(IntervalSet<Value>::LBound const &x);
int toInt(IntervalSet<Value>::RBound const &x);
Value getWeight(AggregateFunction fun, FWValVec const &x);

using BdAggrElemSet = unique_list<std::pair<FWValVec, std::vector<ULitVec>>, extract_first<FWValVec>>;

struct BodyAggregateState {
    using Bounds       = IntervalSet<Value>;
    using element_type = std::pair<Value const, BodyAggregateState>;

    bool fact(bool recursive) const;
    unsigned generation() const;
    void generation(unsigned x);
    bool isFalse();
    static element_type &ignore();
    void accumulate(ValVec const &tuple, AggregateFunction fun, bool fact, bool remove);
    void init(AggregateFunction fun);
    bool defined() const;
    Bounds::Interval range(AggregateFunction fun) const;
    ~BodyAggregateState();

    Bounds         bounds;
    BdAggrElemSet  elems;
    union {
        int64_t    intMin;
        Value::POD valMin;
    };
    union {
        int64_t    intMax;
        Value::POD valMax;
    };
    unsigned _generation = 0;
    bool     _defined    = false; // aggregate is false if _defined == false (might be uninitialized in this state)
    bool     _positive   = false; // wheather the aggregate is monotone
    bool     _fact       = false; // wheather the current (possibly) incomplete aggregate is a fact
    bool     enqueued    = false; // wheather this state is in the todo list
    bool     initialized = false; // wheather this aggregate state has been initialized
};

// }}}
// {{{ declaration of BodyAggregate

struct BodyAggregate : Literal {
    using Interval = IntervalSet<Value>::Interval;
    using BoundsVec = std::vector<std::pair<Relation, Value>>;
    using ConjunctiveBounds = std::vector<std::pair<Interval, Interval>>;
    using ULitValVec = std::vector<std::pair<ULit,Value>>;

    BodyAggregate();
    ULitValVec toLparseElems(LparseTranslator &x, bool equivalence);
    virtual void printPlain(std::ostream &out) const;
    virtual bool isIncomplete() const;
    virtual BodyAggregate *clone() const;
    virtual size_t hash() const;
    virtual bool operator==(Literal const &) const;
    virtual ULit toLparse(LparseTranslator &x);
    virtual int lparseUid(LparseOutputter &out) const;
    virtual ~BodyAggregate();

    BoundsVec                         bounds;
    NAF                               naf        = NAF::POS;
    AggregateFunction                 fun        = AggregateFunction::COUNT;
    bool                              incomplete = false;
    BodyAggregateState::element_type *repr       = nullptr;
};

// }}}
// {{{ declaration of AssignmentAggregateState

struct AssignmentAggregateState {
    using ValSet = std::set<Value>;
    struct Data {
        BdAggrElemSet elems;
        bool          enqueued = false;
        bool          fact     = false;
    };
    using element_type = std::pair<Value const, AssignmentAggregateState>;

    AssignmentAggregateState(Data *data = nullptr, unsigned generation = 0);
    bool fact(bool recursive) const;
    unsigned generation() const;
    void generation(unsigned x);
    bool isFalse();
    static element_type &ignore();
    bool defined() const;

    Data      *data; // can be a reference!!!
    unsigned  _generation;
};

// }}}
// {{{ declaration of AssignmentAggregate

struct AssignmentAggregate : Literal {
    AssignmentAggregate();
    virtual void printPlain(std::ostream &out) const;
    virtual bool isIncomplete() const;
    virtual AssignmentAggregate *clone() const;
    virtual size_t hash() const;
    virtual bool operator==(Literal const &) const;
    virtual ULit toLparse(LparseTranslator &x);
    virtual int lparseUid(LparseOutputter &out) const;
    virtual ~AssignmentAggregate();

    AggregateFunction                       fun        = AggregateFunction::COUNT;
    bool                                    incomplete = false;
    AssignmentAggregateState::element_type *repr       = nullptr;
};

// }}}
// {{{ declaration of ConjunctionElem

struct ConjunctionElem {
    using ULitVecList = unique_list<ULitVec, identity<ULitVec>, value_hash<ULitVec>, value_equal_to<ULitVec>>;

    ConjunctionElem(Value repr);
    operator Value const & () const;
    bool needsSemicolon() const;
    bool isSimple() const;
    void print(std::ostream &out) const;

    Value   repr;
    ULitVecList heads;
    ULitVecList bodies;
};

inline std::ostream &operator<<(std::ostream &out, ConjunctionElem const &x) {
    x.print(out);
    return out;
}

// }}}
// {{{ declaration of ConjunctionState

struct ConjunctionState {
    using element_type = std::pair<Value const, ConjunctionState>;
    using Elem         = ConjunctionElem;
    using ElemSet      = unique_list<Elem, identity<Value>>;
    using BlockSet     = std::unordered_set<Value>;

    bool fact(bool recursive) const { return _fact && !recursive; }
    unsigned generation() const { return _generation - 2; }
    void generation(unsigned x) { _generation = x + 2; }
    bool isFalse();
    static element_type &ignore();
    bool defined() const  { return _generation > 1; }
    bool enqueued() const { return _generation == 1; }
    void enqueue() {
        assert(_generation == 0);
        _generation = 1;
    }
    void dequeue() {
        assert(_generation == 1);
        _generation = 0;
    }
    ElemSet  elems;
    ULit     bdLit;
    bool     _fact       = true;
    bool     incomplete  = false;
    unsigned _generation = 0; // 0 = undefined, 1 enqueued, > 0 defined
    unsigned numBlocked  = 0;
};

// }}}
// {{{ declaration of Conjunction

struct Conjunction : Literal {
    virtual void printPlain(std::ostream &out) const;
    virtual bool isIncomplete() const;
    virtual Conjunction *clone() const;
    virtual size_t hash() const;
    virtual bool operator==(Literal const &) const;
    virtual ULit toLparse(LparseTranslator &x);
    virtual int lparseUid(LparseOutputter &out) const;
    virtual bool needsSemicolon() const;
    virtual ~Conjunction();

    ConjunctionState::element_type *repr       = nullptr;
};

// }}}
// {{{ declaration of DisjointState

struct DisjointElem {
    DisjointElem(CSPGroundAdd &&value, int fixed, ULitVec &&lits);
    DisjointElem(DisjointElem &&);
    DisjointElem clone() const;
    ~DisjointElem();

    CSPGroundAdd value;
    int fixed;
    ULitVec lits;
};

using DisjointElemVec = std::vector<DisjointElem>;
using DisjointElemSet = unique_list<std::pair<FWValVec, DisjointElemVec>, extract_first<FWValVec>>;

struct DisjointState {
    using element_type = std::pair<Value const, DisjointState>;

    bool fact(bool recursive) const;
    unsigned generation() const;
    void generation(unsigned x);
    bool isFalse();
    static element_type &ignore();
    bool defined() const;
    ~DisjointState();

    DisjointElemSet elems;
    bool     enqueued = false;
    unsigned _generation = 0;
};

// }}}
// {{{ declaration of DisjointLiteral

struct DisjointLiteral : Literal {
    DisjointLiteral(NAF naf);
    virtual void printPlain(std::ostream &out) const;
    virtual bool isIncomplete() const;
    virtual DisjointLiteral *clone() const;
    virtual size_t hash() const;
    virtual bool operator==(Literal const &) const;
    virtual ULit toLparse(LparseTranslator &x);
    virtual int lparseUid(LparseOutputter &out) const;
    virtual ~DisjointLiteral();

    NAF const                    naf;
    bool                         incomplete = false;
    DisjointState::element_type *repr       = nullptr;
};

// }}}
// {{{ declaration of CSPLiteral

struct CSPLiteral : Literal {
    CSPLiteral();
    void reset(CSPGroundLit &&ground);
    static void printElem(std::ostream &out, ConjunctionState::Elem const &x);
    virtual void printPlain(std::ostream &out) const;
    virtual bool isIncomplete() const;
    virtual CSPLiteral *clone() const;
    virtual size_t hash() const;
    virtual bool operator==(Literal const &) const;
    virtual ULit toLparse(LparseTranslator &x);
    virtual int lparseUid(LparseOutputter &out) const;
    virtual bool isBound(Value &value, bool negate) const;
    virtual void updateBound(CSPBound &bounds, bool negate) const;
    virtual bool invertible() const;
    virtual void invert();
    virtual ~CSPLiteral();
    CSPGroundLit ground;
};

// }}}

} } // namespace Output Gringo

GRINGO_CALL_CLONE(Gringo::Output::DisjointElem)

#endif // _GRINGO_OUTPUT_LITERALS_HH

