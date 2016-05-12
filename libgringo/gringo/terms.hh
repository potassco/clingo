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
#include <functional>

namespace Gringo {

// {{{ declaration of CSPMulTerm

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

// }}}
// {{{ declaration of CSPAddTerm

using CSPGroundAdd = std::vector<std::pair<int,Value>>;
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

// }}}
// {{{ declaration of CSPRelTerm

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

// }}}

} // namespace Gringo

GRINGO_CALL_HASH(Gringo::CSPRelTerm)
GRINGO_CALL_HASH(Gringo::CSPMulTerm)
GRINGO_CALL_HASH(Gringo::CSPAddTerm)

#endif // _GRINGO_TERMS_HH
