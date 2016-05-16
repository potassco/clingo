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

#ifndef _GRINGO_TERMS_HH
#define _GRINGO_TERMS_HH

#include <gringo/base.hh>
#include <gringo/term.hh>
#include <gringo/hash_set.hh>
#include <potassco/theory_data.h>
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
    void addOpDef(TheoryOpDef &&def);
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
    void addAtomDef(TheoryAtomDef &&def);
    void addTermDef(TheoryTermDef &&def);
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
    bool simplify(SimplifyState &state);
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
    bool simplify(SimplifyState &state);
    void collect(VarTermBoundVec &vars) const;
    void collect(VarTermSet &vars) const;
    void replace(Defines &x);
    bool operator==(CSPAddTerm const &x) const;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen);
    std::vector<CSPAddTerm> unpool() const;
    size_t hash() const;
    bool hasPool() const;
    void toGround(CSPGroundLit &ground, bool invert) const;
    bool checkEval() const;
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
    bool simplify(SimplifyState &state);
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
