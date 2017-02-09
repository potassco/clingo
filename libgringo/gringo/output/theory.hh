// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#ifndef _GRINGO_OUTPUT_THEORY_HH
#define _GRINGO_OUTPUT_THEORY_HH

#include <gringo/base.hh>
#include <gringo/terms.hh>
#include <gringo/types.hh>
#include <gringo/hash_set.hh>
#include <potassco/theory_data.h>
#include <functional>

namespace Gringo { namespace Output {

class LiteralId;
using LitVec = std::vector<LiteralId>;
using Gringo::StringVec;
using Gringo::GetName;

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
    Potassco::Id_t addTermFun(Potassco::Id_t funcSym, Potassco::IdSpan const& terms);
    Potassco::Id_t addTermTup(Potassco::Tuple_t type, Potassco::IdSpan const& terms);
    Potassco::Id_t addTerm(Symbol value);
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
    void reset(bool resetData);
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
    virtual Potassco::Id_t eval(TheoryData &data, Logger &log) const = 0;
    virtual void collect(VarTermBoundVec &vars) = 0;
    virtual void replace(Defines &defs) = 0;
    virtual UTheoryTerm initTheory(TheoryParser &p, Logger &log) = 0;
    virtual ~TheoryTerm() { }
};

// {{{1 declaration of RawTheoryTerm

class RawTheoryTerm : public TheoryTerm {
public:
    using ElemVec = std::vector<std::pair<StringVec, UTheoryTerm>>;
public:
    RawTheoryTerm();
    RawTheoryTerm(ElemVec &&elems);
    RawTheoryTerm(RawTheoryTerm &&);
    RawTheoryTerm &operator=(RawTheoryTerm &&);
    void append(StringVec &&ops, UTheoryTerm &&term);
    virtual ~RawTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data, Logger &log) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p, Logger &log) override;
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
        Elem(String op, bool unary);
        Elem(UTheoryTerm &&term);
        Elem(Elem &&elem);
        ~Elem() noexcept;
        TokenType tokenType;
        union {
            std::pair<String,bool> op;
            UTheoryTerm term;
        };
    };
    using Stack = std::vector<Elem>;
public:
    TheoryParser(Location const &loc, TheoryTermDef const &def);
    UTheoryTerm parse(RawTheoryTerm::ElemVec && elems, Logger &log);
    ~TheoryParser() noexcept;
private:
    void reduce();
    bool check(String op);
private:
    Location loc_;
    TheoryTermDef const &def_;
    Stack stack_;
};

// {{{1 declaration of UnaryTheoryTerm

class UnaryTheoryTerm : public TheoryTerm {
public:
    UnaryTheoryTerm(String op, UTheoryTerm &&arg);
    virtual ~UnaryTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data, Logger &log) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p, Logger &log) override;
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
    String op_;
};

// {{{1 declaration of BinaryTheoryTerm

class BinaryTheoryTerm : public TheoryTerm {
public:
    BinaryTheoryTerm(UTheoryTerm &&left, String op, UTheoryTerm &&right);
    virtual ~BinaryTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data, Logger &log) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p, Logger &log) override;
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
    String op_;
};

// {{{1 declaration of TupleTheoryTerm

class TupleTheoryTerm : public TheoryTerm {
public:
    using Type = Potassco::Tuple_t;
public:
    TupleTheoryTerm(Type type, UTheoryTermVec &&args);
    virtual ~TupleTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data, Logger &log) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p, Logger &log) override;
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
    FunctionTheoryTerm(String name, UTheoryTermVec &&args);
    virtual ~FunctionTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data, Logger &log) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p, Logger &log) override;
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
    String name_;
};

// }}}1
// {{{1 declaration of TermTheoryTerm

class TermTheoryTerm : public TheoryTerm {
public:
    TermTheoryTerm(UTerm &&term);
    virtual ~TermTheoryTerm() noexcept;
    // {{{2 TheoryTerm interface
    Potassco::Id_t eval(TheoryData &data, Logger &log) const override;
    void collect(VarTermBoundVec &vars) override;
    void replace(Defines &defs) override;
    UTheoryTerm initTheory(TheoryParser &p, Logger &log) override;
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

