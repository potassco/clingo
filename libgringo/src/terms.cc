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

#include "gringo/terms.hh"

namespace Gringo {

// {{{ definition of CSPRelTerm

CSPRelTerm::CSPRelTerm(CSPRelTerm &&) = default;
CSPRelTerm &CSPRelTerm::operator=(CSPRelTerm &&) = default;

CSPRelTerm::CSPRelTerm(Relation rel, CSPAddTerm &&term) 
    : rel(rel)
    , term(std::move(term)) { }

void CSPRelTerm::collect(VarTermBoundVec &vars) const                            { term.collect(vars); }
void CSPRelTerm::collect(VarTermSet &vars) const                                 { term.collect(vars); }
void CSPRelTerm::replace(Defines &x)                                             { term.replace(x); }
bool CSPRelTerm::operator==(CSPRelTerm const &x) const                           { return rel == x.rel && term == x.term; }
bool CSPRelTerm::simplify(SimplifyState &state)                                  { return term.simplify(state); }
void CSPRelTerm::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) { term.rewriteArithmetics(arith, auxGen); }
size_t CSPRelTerm::hash() const                                                  { return get_value_hash(size_t(rel), term); }
bool CSPRelTerm::hasPool() const                                                 { return term.hasPool(); }
std::vector<CSPRelTerm> CSPRelTerm::unpool() const {
    std::vector<CSPRelTerm> ret;
    for (auto &x : term.unpool()) { ret.emplace_back(rel, std::move(x)); }
    return ret;
}
CSPRelTerm::~CSPRelTerm() { }
std::ostream &operator<<(std::ostream &out, CSPRelTerm const &x) {
    out << "$" << x.rel << x.term;
    return out;
}
CSPRelTerm clone<CSPRelTerm>::operator()(CSPRelTerm const &x) const {
    return CSPRelTerm(x.rel, get_clone(x.term));
}

// }}}
// {{{ definition of CSPAddTerm

CSPAddTerm::CSPAddTerm(CSPAddTerm &&) = default;
CSPAddTerm &CSPAddTerm::operator=(CSPAddTerm &&) = default;

CSPAddTerm::CSPAddTerm(CSPMulTerm &&x) { terms.emplace_back(std::move(x)); }
CSPAddTerm::CSPAddTerm(Terms &&terms) : terms(std::move(terms)) { }

void CSPAddTerm::append(CSPMulTerm &&x) { terms.emplace_back(std::move(x)); }

void CSPAddTerm::collect(VarTermBoundVec &vars) const { 
    for (auto &x : terms) { x.collect(vars); }
}
void CSPAddTerm::collect(VarTermSet &vars) const {
    for (auto &x : terms) { x.collect(vars); }
}
void CSPAddTerm::replace(Defines &x) {
    for (auto &y : terms) { y.replace(x); }
}
bool CSPAddTerm::operator==(CSPAddTerm const &x) const { return is_value_equal_to(terms, x.terms); }
bool CSPAddTerm::simplify(SimplifyState &state) {
    for (auto &y : terms) { 
        if (!y.simplify(state)) { return false; }
    }
    return true;
}
void CSPAddTerm::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) { 
    for (auto &x : terms) { x.rewriteArithmetics(arith, auxGen); }
}
size_t CSPAddTerm::hash() const { return get_value_hash(terms); }
bool CSPAddTerm::hasPool() const { 
    for (auto &y : terms) { 
        if (y.hasPool()) { return true; }
    }
    return false;
}
std::vector<CSPAddTerm> CSPAddTerm::unpool() const {
    std::vector<CSPAddTerm> x;
    auto f = [&](Terms &&args) { x.emplace_back(std::move(args)); };
    Term::unpool(terms.begin(), terms.end(), std::bind(&CSPMulTerm::unpool, std::placeholders::_1), f);
    return x;
}
void CSPAddTerm::toGround(CSPGroundLit &ground, bool invert) const {
    bool undefined = false;
    for (auto &x : terms) {
        int coe = x.coe->toNum(undefined);
        if (coe != 0) {
            if (x.var) { std::get<1>(ground).emplace_back(invert ? -coe : coe, x.var->eval(undefined)); }
            else       { std::get<2>(ground) = eval(invert ? BinOp::ADD : BinOp::SUB, std::get<2>(ground), coe); }
        }
    }
    assert(!undefined);
}

bool CSPAddTerm::checkEval() const {
    for (auto &x : terms) {
        bool undefined = false;
        x.coe->toNum(undefined);
        if (undefined) { return false; }
        if (x.var) {
            x.var->eval(undefined);
            if (undefined) { return false; }
        }
    }
    return true;
}

CSPAddTerm::~CSPAddTerm() { }
std::ostream &operator<<(std::ostream &out, CSPAddTerm const &x) {
    print_comma(out, x.terms, "$+");
    return out;
}
CSPAddTerm clone<CSPAddTerm>::operator()(CSPAddTerm const &x) const {
    return CSPAddTerm(get_clone(x.terms));
}

// }}}
// {{{ definition of CSPMulTerm

CSPMulTerm::CSPMulTerm(CSPMulTerm &&) = default;
CSPMulTerm &CSPMulTerm::operator=(CSPMulTerm &&) = default;

CSPMulTerm::CSPMulTerm(UTerm &&var, UTerm &&coe)
    : var(std::move(var))
    , coe(std::move(coe)) { }

void CSPMulTerm::collect(VarTermBoundVec &vars) const { 
    if (var) { var->collect(vars, false); }
    coe->collect(vars, false);
}
void CSPMulTerm::collect(VarTermSet &vars) const {
    if (var) { var->collect(vars); }
    coe->collect(vars);
}
void CSPMulTerm::replace(Defines &x) {
    if (var) { Term::replace(var, var->replace(x, true)); }
    Term::replace(coe, coe->replace(x, true));
}
bool CSPMulTerm::operator==(CSPMulTerm const &x) const { 
    if (var && x.var) { return is_value_equal_to(var, x.var) && is_value_equal_to(coe, x.coe); }
    else { return !var && !x.var && is_value_equal_to(coe, x.coe); }
}
bool CSPMulTerm::simplify(SimplifyState &state) {
    if (var && var->simplify(state, false, false).update(var).undefined()) { return false;}
    return !coe->simplify(state, false, false).update(coe).undefined();
}
void CSPMulTerm::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) { 
    if (var) { Term::replace(var, var->rewriteArithmetics(arith, auxGen)); }
    Term::replace(coe, coe->rewriteArithmetics(arith, auxGen));
}
size_t CSPMulTerm::hash() const { 
    if (var) { return get_value_hash(var, coe); }
    else     { return get_value_hash(coe); }
}
bool CSPMulTerm::hasPool() const { 
    return (var && var->hasPool()) || coe->hasPool();
}
std::vector<CSPMulTerm> CSPMulTerm::unpool() const {
    std::vector<CSPMulTerm> value;
    if (var) {
        auto f = [&](UTerm &&a, UTerm &&b) { value.emplace_back(std::move(a), std::move(b)); };
        Term::unpool(var, coe, Gringo::unpool, Gringo::unpool, f);
    }
    else {
        for (auto &x : Gringo::unpool(coe)) { value.emplace_back(nullptr, std::move(x)); }
    }
    return value;
}
CSPMulTerm::~CSPMulTerm() { }
std::ostream &operator<<(std::ostream &out, CSPMulTerm const &x) {
    out << *x.coe;
    if (x.var) { out << "$*$" << *x.var; };
    return out;
}
CSPMulTerm clone<CSPMulTerm>::operator()(CSPMulTerm const &x) const {
    return CSPMulTerm(x.var ? get_clone(x.var) : nullptr, get_clone(x.coe));
}

// }}}

} // namespace Gringo
