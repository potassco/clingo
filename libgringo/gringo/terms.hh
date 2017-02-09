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

#ifndef _GRINGO_TERMS_HH
#define _GRINGO_TERMS_HH

#include <gringo/base.hh>
#include <gringo/term.hh>
#include <gringo/hash_set.hh>
#include <potassco/theory_data.h>
#include <gringo/logger.hh>
#include <functional>

namespace Gringo {

using StringVec = std::vector<String>;
template <class T>
struct GetName {
    using result_type = String;
    String operator()(T const &x) const {
        return x.name();
    }
};

// {{{1 TheoryOpDef

class TheoryOpDef {
public:
    using Key = std::pair<String, bool>;
    struct GetKey {
        Key operator()(TheoryOpDef const &x) const {
            return x.key();
        }
    };
public:
    TheoryOpDef(Location const &loc, String op, unsigned priority, TheoryOperatorType type);
    TheoryOpDef(TheoryOpDef &&);
    ~TheoryOpDef() noexcept;
    TheoryOpDef &operator=(TheoryOpDef &&);
    String op() const;
    Key key() const;
    Location const &loc() const;
    void print(std::ostream &out) const;
    unsigned priority() const;
    TheoryOperatorType type() const;
private:
    Location loc_;
    String op_;
    unsigned priority_;
    TheoryOperatorType type_;
};
using TheoryOpDefs = UniqueVec<TheoryOpDef, HashKey<TheoryOpDef::Key, TheoryOpDef::GetKey, value_hash<TheoryOpDef::Key>>, EqualToKey<TheoryOpDef::Key, TheoryOpDef::GetKey>>;
inline std::ostream &operator<<(std::ostream &out, TheoryOpDef const &def) {
    def.print(out);
    return out;
}

// {{{1 TheoryTermDef

class TheoryTermDef {
public:
    TheoryTermDef(Location const &loc, String name);
    TheoryTermDef(TheoryTermDef &&);
    ~TheoryTermDef() noexcept;
    TheoryTermDef &operator=(TheoryTermDef &&);
    void addOpDef(TheoryOpDef &&def, Logger &log);
    String name() const;
    Location const &loc() const;
    void print(std::ostream &out) const;
    // returns (priority, flag) where flag is true if the binary operator is left associative
    std::pair<unsigned, bool> getPrioAndAssoc(String op) const;
    unsigned getPrio(String op, bool unary) const;
    bool hasOp(String op, bool unary) const;
private:
    Location loc_;
    String name_;
    TheoryOpDefs opDefs_;
};
using TheoryTermDefs = UniqueVec<TheoryTermDef, HashKey<String, GetName<TheoryTermDef>>, EqualToKey<String, GetName<TheoryTermDef>>>;
inline std::ostream &operator<<(std::ostream &out, TheoryTermDef const &def) {
    def.print(out);
    return out;
}

// {{{1 TheoryAtomDef

class TheoryAtomDef {
public:
    using Key = Sig;
    struct GetKey {
        Key operator()(TheoryAtomDef const &x) const {
            return x.sig();
        }
    };
public:
    TheoryAtomDef(Location const &loc, String name, unsigned arity, String elemDef, TheoryAtomType type);
    TheoryAtomDef(Location const &loc, String name, unsigned arity, String elemDef, TheoryAtomType type, StringVec &&ops, String guardDef);
    TheoryAtomDef(TheoryAtomDef &&);
    ~TheoryAtomDef() noexcept;
    TheoryAtomDef &operator=(TheoryAtomDef &&);
    Sig sig() const;
    bool hasGuard() const;
    TheoryAtomType type() const;
    StringVec const &ops() const;
    Location const &loc() const;
    String elemDef() const;
    String guardDef() const;
    void print(std::ostream &out) const;
private:
    Location loc_;
    Sig sig_;
    String elemDef_;
    String guardDef_;
    StringVec ops_;
    TheoryAtomType type_;
};
using TheoryAtomDefs = UniqueVec<TheoryAtomDef, HashKey<TheoryAtomDef::Key, TheoryAtomDef::GetKey, value_hash<TheoryAtomDef::Key>>, EqualToKey<TheoryAtomDef::Key, TheoryAtomDef::GetKey>>;
inline std::ostream &operator<<(std::ostream &out, TheoryAtomDef const &def) {
    def.print(out);
    return out;
}

// {{{1 TheoryDef

class TheoryDef {
public:
    TheoryDef(Location const &loc, String name);
    TheoryDef(TheoryDef &&);
    ~TheoryDef() noexcept;
    TheoryDef &operator=(TheoryDef &&);
    String name() const;
    void addAtomDef(TheoryAtomDef &&def, Logger &log);
    void addTermDef(TheoryTermDef &&def, Logger &log);
    TheoryAtomDef const *getAtomDef(Sig name) const;
    TheoryTermDef const *getTermDef(String name) const;
    TheoryAtomDefs const &atomDefs() const;
    Location const &loc() const;
    void print(std::ostream &out) const;
private:
    Location loc_;
    TheoryTermDefs termDefs_;
    TheoryAtomDefs atomDefs_;
    String name_;
};
using TheoryDefs = UniqueVec<TheoryDef, HashKey<String, GetName<TheoryDef>>, EqualToKey<String, GetName<TheoryDef>>>;
inline std::ostream &operator<<(std::ostream &out, TheoryDef const &def) {
    def.print(out);
    return out;
}
// }}}1

// {{{1 declaration of CSPMulTerm

struct CSPMulTerm {
    CSPMulTerm(UTerm &&var, UTerm &&coe);
    CSPMulTerm(CSPMulTerm &&x);
    CSPMulTerm &operator=(CSPMulTerm &&x);
    void collect(VarTermBoundVec &vars) const;
    void collect(VarTermSet &vars) const;
    void replace(Defines &x);
    bool operator==(CSPMulTerm const &x) const;
    bool simplify(SimplifyState &state, Logger &log);
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen);
    size_t hash() const;
    bool hasPool() const;
    std::vector<CSPMulTerm> unpool() const;
    ~CSPMulTerm();

    UTerm var;
    UTerm coe;
};

std::ostream &operator<<(std::ostream &out, CSPMulTerm const &x);

template <>
struct clone<CSPMulTerm> {
    CSPMulTerm operator()(CSPMulTerm const &x) const;
};

// {{{1 declaration of CSPAddTerm

using CSPGroundAdd = std::vector<std::pair<int,Symbol>>;
using CSPGroundLit = std::tuple<Relation, CSPGroundAdd, int>;

struct CSPAddTerm {
    using Terms = std::vector<CSPMulTerm>;

    CSPAddTerm(CSPMulTerm &&x);
    CSPAddTerm(CSPAddTerm &&x);
    CSPAddTerm(Terms &&terms);
    CSPAddTerm &operator=(CSPAddTerm &&x);
    void append(CSPMulTerm &&x);
    bool simplify(SimplifyState &state, Logger &log);
    void collect(VarTermBoundVec &vars) const;
    void collect(VarTermSet &vars) const;
    void replace(Defines &x);
    bool operator==(CSPAddTerm const &x) const;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen);
    std::vector<CSPAddTerm> unpool() const;
    size_t hash() const;
    bool hasPool() const;
    void toGround(CSPGroundLit &ground, bool invert, Logger &log) const;
    bool checkEval(Logger &log) const;
    ~CSPAddTerm();

    Terms terms;
};

std::ostream &operator<<(std::ostream &out, CSPAddTerm const &x);

template <>
struct clone<CSPAddTerm> {
    CSPAddTerm operator()(CSPAddTerm const &x) const;
};

// {{{1 declaration of CSPRelTerm

struct CSPRelTerm {
    CSPRelTerm(CSPRelTerm &&x);
    CSPRelTerm(Relation rel, CSPAddTerm &&x);
    CSPRelTerm &operator=(CSPRelTerm &&x);
    void collect(VarTermBoundVec &vars) const;
    void collect(VarTermSet &vars) const;
    void replace(Defines &x);
    bool operator==(CSPRelTerm const &x) const;
    bool simplify(SimplifyState &state, Logger &log);
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen);
    size_t hash() const;
    bool hasPool() const;
    std::vector<CSPRelTerm> unpool() const;
    ~CSPRelTerm();

    Relation rel;
    CSPAddTerm term;
};

std::ostream &operator<<(std::ostream &out, CSPRelTerm const &x);

template <>
struct clone<CSPRelTerm> {
    CSPRelTerm operator()(CSPRelTerm const &x) const;
};

// }}}1

} // namespace Gringo

GRINGO_CALL_HASH(Gringo::CSPRelTerm)
GRINGO_CALL_HASH(Gringo::CSPMulTerm)
GRINGO_CALL_HASH(Gringo::CSPAddTerm)

#endif // _GRINGO_TERMS_HH
