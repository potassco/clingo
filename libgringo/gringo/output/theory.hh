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

#ifndef _GRINGO_OUTPUT_THEORY_HH
#define _GRINGO_OUTPUT_THEORY_HH

#include <gringo/base.hh>
#include <gringo/terms.hh>
#include <gringo/hash_set.hh>
#include <potassco/theory_data.h>
#include <gringo/control.hh>
#include <functional>

namespace Gringo { namespace Output {

class LiteralId;
using LitVec = std::vector<LiteralId>;
using FWStringVec = std::vector<FWString>;
template <class T>
struct GetName {
    using result_type = FWString;
    FWString operator()(T const &x) const {
        return x.name();
    }
};

// {{{1 declaration of TheoryData

class TheoryData {
    using TIdSet = HashSet<Potassco::Id_t>;
    using AtomSet = HashSet<uintptr_t>;
    using ConditionVec = std::vector<LitVec>;
    using PrintLit = std::function<void (std::ostream &out, LiteralId const &)>;
public:
    TheoryData(Potassco::TheoryData &data);
    ~TheoryData() noexcept;
    Potassco::Id_t addTerm(int number);
    Potassco::Id_t addTerm(char const *name);
    Potassco::Id_t addTerm(Potassco::Id_t funcSym, Potassco::IdSpan const& terms);
    Potassco::Id_t addTerm(Potassco::Tuple_t type, Potassco::IdSpan const& terms);
    Potassco::Id_t addTerm(Value value);
    Potassco::Id_t addElem(Potassco::IdSpan const &tuple, LitVec &&cond);
    std::pair<Potassco::TheoryAtom const &, bool> addAtom(std::function<Potassco::Id_t()> newAtom, Potassco::Id_t termId, Potassco::IdSpan const &elems);
    std::pair<Potassco::TheoryAtom const &, bool> addAtom(std::function<Potassco::Id_t()> newAtom, Potassco::Id_t termId, Potassco::IdSpan const &elems, Potassco::Id_t op, Potassco::Id_t rhs);
    void printTerm(std::ostream &out, Potassco::Id_t termId) const;
    void printElem(std::ostream &out, Potassco::Id_t elemId, PrintLit printLit) const;
    bool empty() const;
    LitVec const &getCondition(Potassco::Id_t elemId) const;
    LitVec &getCondition(Potassco::Id_t elemId);
    Potassco::TheoryData const &data() const;
    void setCondition(Potassco::Id_t elementId, Potassco::Id_t newCond);
    bool hasConditions() const;
    void reset();
    Potassco::TheoryAtom const &getAtom(Id_t offset) const { return **(data_.begin() + offset); }

private:
    template <typename ...Args>
    Potassco::Id_t addTerm_(Args ...args);
    template <typename ...Args>
    std::pair<Potassco::TheoryAtom const &, bool> addAtom_(std::function<Potassco::Id_t()> newAtom, Args ...args);
private:
    TIdSet terms_;
    TIdSet elems_;
    AtomSet atoms_;
    Potassco::TheoryData &data_;
    ConditionVec conditions_;
};

// {{{1 declaration of TheoryTerm

class TheoryParser;
class TheoryTerm;
using UTheoryTerm = std::unique_ptr<TheoryTerm>;
using UTheoryTermVec = std::vector<UTheoryTerm>;
class TheoryTerm : public Hashable, public Comparable<TheoryTerm>, public Printable, public Clonable<TheoryTerm> {
public:
    virtual Potassco::Id_t eval(TheoryData &data) const = 0;
    virtual void collect(VarTermBoundVec &vars) = 0;
    virtual void replace(Defines &defs) = 0;
    virtual UTheoryTerm initTheory(TheoryParser &p) = 0;
    virtual ~TheoryTerm() { }
};

// {{{1 declaration of RawTheoryTerm

class RawTheoryTerm : public TheoryTerm {
public:
    using ElemVec = std::vector<std::pair<FWStringVec, UTheoryTerm>>;
public:
    RawTheoryTerm();
    RawTheoryTerm(ElemVec &&elems);
    RawTheoryTerm(RawTheoryTerm &&);
    RawTheoryTerm &operator=(RawTheoryTerm &&);
    void append(FWStringVec &&ops, UTheoryTerm &&term);
    virtual ~RawTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p) override;
    // {{{2 Hashable interface
    size_t hash() const override;
    // {{{2 Comparable interface
    bool operator==(TheoryTerm const &other) const override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Clonable interface
    RawTheoryTerm *clone() const override;
    // }}}2
private:
    ElemVec elems_;
};

class TheoryParser {
    enum TokenType { Op, Id };
    struct Elem {
        Elem(FWString op, bool unary);
        Elem(UTheoryTerm &&term);
        Elem(Elem &&elem);
        ~Elem() noexcept;
        TokenType tokenType;
        union {
            std::pair<FWString,bool> op;
            UTheoryTerm term;
        };
    };
    using Stack = std::vector<Elem>;
public:
    TheoryParser(Location const &loc, TheoryTermDef const &def);
    UTheoryTerm parse(RawTheoryTerm::ElemVec && elems);
    ~TheoryParser() noexcept;
private:
    void reduce();
    bool check(FWString op);
private:
    Location loc_;
    TheoryTermDef const &def_;
    Stack stack_;
};

// {{{1 declaration of UnaryTheoryTerm

class UnaryTheoryTerm : public TheoryTerm {
public:
    UnaryTheoryTerm(FWString op, UTheoryTerm &&arg);
    virtual ~UnaryTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p) override;
    // {{{2 Hashable interface
    size_t hash() const override;
    // {{{2 Comparable interface
    bool operator==(TheoryTerm const &other) const override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Clonable interface
    UnaryTheoryTerm *clone() const override;
    // }}}2
private:
    UTheoryTerm arg_;
    FWString op_;
};

// {{{1 declaration of BinaryTheoryTerm

class BinaryTheoryTerm : public TheoryTerm {
public:
    BinaryTheoryTerm(UTheoryTerm &&left, FWString op, UTheoryTerm &&right);
    virtual ~BinaryTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p) override;
    // {{{2 Hashable interface
    size_t hash() const override;
    // {{{2 Comparable interface
    bool operator==(TheoryTerm const &other) const override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Clonable interface
    BinaryTheoryTerm *clone() const override;
    // }}}2
private:
    UTheoryTerm left_;
    UTheoryTerm right_;
    FWString op_;
};

// {{{1 declaration of TupleTheoryTerm

class TupleTheoryTerm : public TheoryTerm {
public:
    using Type = Potassco::Tuple_t;
public:
    TupleTheoryTerm(Type type, UTheoryTermVec &&args);
    virtual ~TupleTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p) override;
    // {{{2 Hashable interface
    size_t hash() const override;
    // {{{2 Comparable interface
    bool operator==(TheoryTerm const &other) const override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Clonable interface
    TupleTheoryTerm *clone() const override;
    // }}}2
private:
    UTheoryTermVec args_;
    Type type_;
};

// {{{1 declaration of FunctionTheoryTerm

class FunctionTheoryTerm : public TheoryTerm {
public:
    FunctionTheoryTerm(FWString name, UTheoryTermVec &&args);
    virtual ~FunctionTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p) override;
    // {{{2 Hashable interface
    size_t hash() const override;
    // {{{2 Comparable interface
    bool operator==(TheoryTerm const &other) const override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Clonable interface
    FunctionTheoryTerm *clone() const override;
    // }}}2
private:
    UTheoryTermVec args_;
    FWString name_;
};

// }}}1
// {{{1 declaration of TermTheoryTerm

class TermTheoryTerm : public TheoryTerm {
public:
    TermTheoryTerm(UTerm &&term);
    virtual ~TermTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p) override;
    // {{{2 Hashable interface
    size_t hash() const override;
    // {{{2 Comparable interface
    bool operator==(TheoryTerm const &other) const override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Clonable interface
    TermTheoryTerm *clone() const override;
    // }}}2
private:
    UTerm term_;
};

// }}}1

} } // namespace Output Gringo

GRINGO_CALL_HASH(Gringo::Output::TheoryTerm)

#endif // _GRINGO_OUTPUT_THEORY_HH

