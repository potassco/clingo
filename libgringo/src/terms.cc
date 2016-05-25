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
#include "gringo/logger.hh"
#include <cstring>

namespace Gringo {

// {{{1 TheoryOpDef

TheoryOpDef::TheoryOpDef(Location const &loc, String op, unsigned priority, TheoryOperatorType type)
: loc_(loc)
, op_(op)
, priority_(priority)
, type_(type) { }

TheoryOpDef::TheoryOpDef(TheoryOpDef &&) = default;

String TheoryOpDef::op() const {
    return op_;
}

TheoryOpDef::Key TheoryOpDef::key() const {
    return {op_, type_ == TheoryOperatorType::Unary};
}

TheoryOpDef::~TheoryOpDef() noexcept = default;

TheoryOpDef &TheoryOpDef::operator=(TheoryOpDef &&) = default;

Location const &TheoryOpDef::loc() const {
    return loc_;
}

void TheoryOpDef::print(std::ostream &out) const {
    out << op_ << " :" << priority_ << "," << type_;
}

unsigned TheoryOpDef::priority() const {
    return priority_;
}

TheoryOperatorType TheoryOpDef::type() const {
    return type_;
}

// {{{1 TheoryTermDef

TheoryTermDef::TheoryTermDef(Location const &loc, String name)
: loc_(loc)
, name_(name) { }

TheoryTermDef::TheoryTermDef(TheoryTermDef &&) = default;

void TheoryTermDef::addOpDef(TheoryOpDef &&def, MessagePrinter &log) {
    auto it = opDefs_.find(def.key());
    if (it == opDefs_.end()) {
        opDefs_.push(std::move(def));
    }
    else {
        GRINGO_REPORT(log, E_ERROR)
            << def.loc() << ": error: redefinition of theory operator:" << "\n"
            << "  " << def.op() << "\n"
            << it->loc() << ": note: operator first defined here\n";
    }
}

String TheoryTermDef::name() const {
    return name_;
}

Location const &TheoryTermDef::loc() const {
    return loc_;
}

TheoryTermDef::~TheoryTermDef() noexcept = default;

TheoryTermDef &TheoryTermDef::operator=(TheoryTermDef &&) = default;

void TheoryTermDef::print(std::ostream &out) const {
    out << name_ << "{";
    print_comma(out, opDefs_, ",");
    out << "}";
}

std::pair<unsigned, bool> TheoryTermDef::getPrioAndAssoc(String op) const {
    auto ret = opDefs_.find(TheoryOpDef::Key{op, false});
    if (ret != opDefs_.end()) {
        return {ret->priority(), ret->type() == TheoryOperatorType::BinaryLeft};
    }
    else {
        return {0, true};
    }
}

bool TheoryTermDef::hasOp(String op, bool unary) const {
    return opDefs_.find(TheoryOpDef::Key{op, unary}) != opDefs_.end();
}

unsigned TheoryTermDef::getPrio(String op, bool unary) const {
    auto ret = opDefs_.find(TheoryOpDef::Key{op, unary});
    if (ret != opDefs_.end()) {
        return ret->priority();
    }
    else {
        return 0;
    }
}

// {{{1 TheoryAtomDef

TheoryAtomDef::TheoryAtomDef(Location const &loc, String name, unsigned arity, String elemDef, TheoryAtomType type)
: TheoryAtomDef(loc, name, arity, elemDef, type, {}, "") { }

TheoryAtomDef::TheoryAtomDef(Location const &loc, String name, unsigned arity, String elemDef, TheoryAtomType type, StringVec &&ops, String guardDef)
: loc_(loc)
, sig_(name, arity, false)
, elemDef_(elemDef)
, guardDef_(guardDef)
, ops_(std::move(ops))
, type_(type) { }

TheoryAtomDef::TheoryAtomDef(TheoryAtomDef &&) = default;

Sig TheoryAtomDef::sig() const {
    return sig_;
}

bool TheoryAtomDef::hasGuard() const {
    return !ops_.empty();
}

TheoryAtomType TheoryAtomDef::type() const {
    return type_;
}

Location const &TheoryAtomDef::loc() const {
    return loc_;
}

StringVec const &TheoryAtomDef::ops() const {
    return ops_;
}

String TheoryAtomDef::elemDef() const {
    return elemDef_;
}

String TheoryAtomDef::guardDef() const {
    assert(hasGuard());
    return guardDef_;
}

TheoryAtomDef::~TheoryAtomDef() noexcept = default;

TheoryAtomDef &TheoryAtomDef::operator=(TheoryAtomDef &&) = default;

void TheoryAtomDef::print(std::ostream &out) const {
    out << "&" << sig_.name() << "/" << sig_.arity() << ":" << elemDef_;
    if (hasGuard()) {
        out << ",{";
        print_comma(out, ops_, ",");
        out << "}," << guardDef_;
    }
    out << "," << type_;
}

// {{{1 TheoryDef

TheoryDef::TheoryDef(Location const &loc, String name)
: loc_(loc)
, name_(name) { }

TheoryDef::TheoryDef(TheoryDef &&) = default;

String TheoryDef::name() const {
    return name_;
}

Location const &TheoryDef::loc() const {
    return loc_;
}

void TheoryDef::addAtomDef(TheoryAtomDef &&def, MessagePrinter &log) {
    auto it = atomDefs_.find(def.sig());
    if (it == atomDefs_.end()) {
        atomDefs_.push(std::move(def));
    }
    else {
        GRINGO_REPORT(log, E_ERROR)
            << def.loc() << ": error: redefinition of theory atom:" << "\n"
            << "  " << def.sig() << "\n"
            << it->loc() << ": note: atom first defined here\n";
    }
}

void TheoryDef::addTermDef(TheoryTermDef &&def, MessagePrinter &log) {
    auto it = termDefs_.find(def.name());
    if (it == termDefs_.end()) {
        termDefs_.push(std::move(def));
    }
    else {
        GRINGO_REPORT(log, E_ERROR)
            << def.loc() << ": error: redefinition of theory term:" << "\n"
            << "  " << def.name() << "\n"
            << it->loc() << ": note: term first defined term\n";
    }
}

TheoryDef::~TheoryDef() noexcept = default;

TheoryDef &TheoryDef::operator=(TheoryDef &&) = default;

void TheoryDef::print(std::ostream &out) const {
    out << "#theory " << name_ << "{";
    if (!atomDefs_.empty() || !termDefs_.empty()) {
        out << "\n";
    }
    bool sep = false;
    for (auto &def : termDefs_) {
        if (sep) { out << ";\n"; }
        else     { sep = true; }
        out << "  " << def;
    }
    for (auto &def : atomDefs_) {
        if (sep) { out << ";\n"; }
        else     { sep = true; }
        out << "  " << def;
    }
    if (sep) {
        out << "\n";
    }
    out << "}.";
}

TheoryAtomDef const *TheoryDef::getAtomDef(Sig sig) const {
    auto ret = atomDefs_.find(sig);
    return ret != atomDefs_.end() ? &*ret : nullptr;
}

TheoryTermDef const *TheoryDef::getTermDef(String name) const {
    auto ret = termDefs_.find(name);
    return ret != termDefs_.end() ? &*ret : nullptr;
}

TheoryAtomDefs const &TheoryDef::atomDefs() const {
    return atomDefs_;
}

// }}}1

// {{{1 definition of CSPRelTerm

CSPRelTerm::CSPRelTerm(CSPRelTerm &&) = default;
CSPRelTerm &CSPRelTerm::operator=(CSPRelTerm &&) = default;

CSPRelTerm::CSPRelTerm(Relation rel, CSPAddTerm &&term)
    : rel(rel)
    , term(std::move(term)) { }

void CSPRelTerm::collect(VarTermBoundVec &vars) const                            { term.collect(vars); }
void CSPRelTerm::collect(VarTermSet &vars) const                                 { term.collect(vars); }
void CSPRelTerm::replace(Defines &x)                                             { term.replace(x); }
bool CSPRelTerm::operator==(CSPRelTerm const &x) const                           { return rel == x.rel && term == x.term; }
bool CSPRelTerm::simplify(SimplifyState &state, MessagePrinter &log)             { return term.simplify(state, log); }
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

// {{{1 definition of CSPAddTerm

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
bool CSPAddTerm::simplify(SimplifyState &state, MessagePrinter &log) {
    for (auto &y : terms) {
        if (!y.simplify(state, log)) { return false; }
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
void CSPAddTerm::toGround(CSPGroundLit &ground, bool invert, MessagePrinter &log) const {
    bool undefined = false;
    for (auto &x : terms) {
        int coe = x.coe->toNum(undefined, log);
        if (coe != 0) {
            if (x.var) { std::get<1>(ground).emplace_back(invert ? -coe : coe, x.var->eval(undefined, log)); }
            else       { std::get<2>(ground) = eval(invert ? BinOp::ADD : BinOp::SUB, std::get<2>(ground), coe); }
        }
    }
    assert(!undefined);
}

bool CSPAddTerm::checkEval(MessagePrinter &log) const {
    for (auto &x : terms) {
        bool undefined = false;
        x.coe->toNum(undefined, log);
        if (undefined) { return false; }
        if (x.var) {
            x.var->eval(undefined, log);
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

// {{{1 definition of CSPMulTerm

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
bool CSPMulTerm::simplify(SimplifyState &state, MessagePrinter &log) {
    if (var && var->simplify(state, false, false, log).update(var).undefined()) { return false;}
    return !coe->simplify(state, false, false, log).update(coe).undefined();
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

// }}}1

} // namspace Gringo

