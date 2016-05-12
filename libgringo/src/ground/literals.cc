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

#include "gringo/ground/literals.hh"
#include "gringo/ground/binders.hh"
#include "gringo/logger.hh"
#include "gringo/scripts.hh"
#include <cmath>
#include <cstring>

namespace Gringo { namespace Ground {

namespace {

// {{{ declaration of RangeBinder

struct RangeBinder : Binder {
    RangeBinder(UTerm &&assign, RangeLiteralShared &range) 
        : assign(std::move(assign))
        , range(range) { }
    virtual IndexUpdater *getUpdater() { return nullptr; }
    virtual void match() {
        bool undefined = false;
        Value l{range.first->eval(undefined)}, r{range.second->eval(undefined)};
        if (!undefined && l.type() == Value::NUM && r.type() == Value::NUM) {
            current = l.num();
            end     = r.num();
        }
        else {
            if (!undefined) {
                GRINGO_REPORT(W_OPERATION_UNDEFINED) 
                    << (range.first->loc() + range.second->loc()) << ": info: interval undefined:\n"
                    << "  " << *range.first << ".." << *range.second << "\n";
            }
            current = 1;
            end     = 0;
        }
    }
    virtual bool next() {
        // Note: if assign does not match it is not a variable and will not match at all
        //       if assign is just a number, then this is handled in the corresponding Binder
        return current <= end && assign->match(Value::createNum(current++));
    }
    virtual void start()  { }
    virtual void finish() { }
    virtual void print(std::ostream &out) const { out << *assign << "=" << *range.first << ".." << *range.second; }
    virtual ~RangeBinder() { }

    UTerm               assign;
    RangeLiteralShared &range;
    int                 current = 0;
    int                 end     = 0;
    
};

// }}}
// {{{ declaration of RangeMatcher

struct RangeMatcher : Binder {
    RangeMatcher(Term &assign, RangeLiteralShared &range) 
        : assign(assign)
        , range(range) { }
    virtual IndexUpdater *getUpdater() { return nullptr; }
    virtual void match() {
        bool undefined = false;
        Value l{range.first->eval(undefined)}, r{range.second->eval(undefined)}, a{assign.eval(undefined)};
        if (!undefined && l.type() == Value::NUM && r.type() == Value::NUM) {
            firstMatch = a.type() == Value::NUM && l.num() <= a.num() && a.num() <= r.num();
        }
        else { 
            if (!undefined) {
                GRINGO_REPORT(W_OPERATION_UNDEFINED) 
                    << (range.first->loc() + range.second->loc()) << ": info: interval undefined:\n"
                    << "  " << *range.first << ".." << *range.second << "\n";
            }
            firstMatch = false;
        }
    }
    virtual bool next() {
        bool m = firstMatch;
        firstMatch = false;
        return m;
    }
    virtual void start()  { }
    virtual void finish() { }
    virtual void print(std::ostream &out) const { out << assign << "=" << *range.first << ".." << *range.second; }
    Term               &assign;
    RangeLiteralShared &range;
    bool                firstMatch = false;
};

// }}}

// {{{ declaration of ScriptBinder

struct ScriptBinder : Binder {
    ScriptBinder(Scripts &scripts, UTerm &&assign, ScriptLiteralShared &shared) 
        : scripts(scripts)
        , assign(std::move(assign))
        , shared(shared) { }
    virtual IndexUpdater *getUpdater() { return nullptr; }
    virtual void match() {
        ValVec args;
        bool undefined = false;
        for (auto &x : std::get<1>(shared)) { args.emplace_back(x->eval(undefined)); }
        if (!undefined) {
            matches = scripts.call(assign->loc(), *std::get<0>(shared), args);
        }
        else { matches = {}; }
        current = matches.begin();
    }
    virtual bool next() {
        while (current != matches.end()) {
            if (assign->match(*current++)) { return true; }
        }
        return false;
    }
    virtual void start()  { }
    virtual void finish() { }
    virtual void print(std::ostream &out) const { 
        out << *assign << "=" << *std::get<0>(shared) << "(";
        print_comma(out, std::get<1>(shared), ",", [](std::ostream &out, UTerm const &term) { out << *term; });
        out << ")";
    }
    virtual ~ScriptBinder() { }

    Scripts             &scripts;
    UTerm                assign;
    ScriptLiteralShared &shared;
    ValVec               matches;
    ValVec::iterator     current;
};

// }}}

// {{{ declaration of RelationMatcher

struct RelationMatcher : Binder {
    RelationMatcher(RelationShared &shared)
        : shared(shared) { }
    virtual IndexUpdater *getUpdater() { return nullptr; }
    virtual void match() {
        bool undefined = false;
        Value l(std::get<1>(shared)->eval(undefined));
        Value r(std::get<2>(shared)->eval(undefined));
        if (!undefined) {
            switch (std::get<0>(shared)) {
                case Relation::GT:  { firstMatch = l >  r; break; }
                case Relation::LT:  { firstMatch = l <  r; break; }
                case Relation::GEQ: { firstMatch = l >= r; break; }
                case Relation::LEQ: { firstMatch = l <= r; break; }
                case Relation::NEQ: { firstMatch = l != r; break; }
                case Relation::EQ:  { firstMatch = l == r; break; }
            }
        }
        else { firstMatch = false; }
    }
    virtual bool next() { 
        bool ret = firstMatch;
        firstMatch = false;
        return ret;
    }
    virtual void start()  { }
    virtual void finish() { }
    virtual void print(std::ostream &out) const { out << *std::get<1>(shared) << std::get<0>(shared) << *std::get<2>(shared); }
    virtual ~RelationMatcher() { }
    
    RelationShared &shared;
    bool firstMatch = false;
};

// }}}
// {{{ declaration of AssignBinder

struct AssignBinder : Binder {
    AssignBinder(UTerm &&lhs, Term &rhs)
        : lhs(std::move(lhs))
        , rhs(rhs) { }
    virtual IndexUpdater *getUpdater() { return nullptr; }
    virtual void match() { 
        bool undefined = false;
        Value valRhs = rhs.eval(undefined);
        if (!undefined) {
            firstMatch = lhs->match(valRhs);
        }
        else { firstMatch = false; }
    }
    virtual bool next() {
        bool ret = firstMatch;
        firstMatch = false;
        return ret;
    }
    virtual void start()  { }
    virtual void finish() { }
    virtual void print(std::ostream &out) const { out << *lhs << "=" << rhs; }
    UTerm lhs;
    Term &rhs;
    bool firstMatch = false;
};


// }}}

 // {{{ declaration of CSPLiteralMatcher

struct CSPLiteralMatcher : Binder {
    CSPLiteralMatcher(CSPLiteralShared &terms)
        : terms(terms) { }
    virtual IndexUpdater *getUpdater() { return nullptr; }
    virtual void match() { 
        firstMatch = std::get<1>(terms).checkEval() && std::get<2>(terms).checkEval();
    }
    virtual bool next() { 
        bool ret = firstMatch;
        firstMatch = false;
        return ret;
    }
    virtual void start()  { }
    virtual void finish() { }
    virtual void print(std::ostream &out) const { out << std::get<1>(terms) << std::get<0>(terms) << std::get<2>(terms); }
    virtual ~CSPLiteralMatcher() { }
    
    CSPLiteralShared &terms;
    bool firstMatch = false;
};

// }}}

} // namespace

// {{{ definition of PredicateLiteral::BodyOccurrence

UGTerm PredicateLiteral::getRepr() const          { return repr->gterm(); }
bool PredicateLiteral::isPositive() const         { return gLit.naf == NAF::POS; }
bool PredicateLiteral::isNegative() const         { return gLit.naf != NAF::POS; }
void PredicateLiteral::setType(OccurrenceType x)  { type = x; }
OccurrenceType PredicateLiteral::getType() const  { return type; }
BodyOcc::DefinedBy &PredicateLiteral::definedBy() { return defs; }
void PredicateLiteral::checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const { 
    if (defs.empty() && done.find(repr->loc()) == done.end() && edb.find(repr->getSig()) == edb.end()) {
        // accumulate warnings in array of printables ..
        done.insert(repr->loc());
        undef.emplace_back(repr->loc(), this);
    }
}

ExternalBodyOcc::ExternalBodyOcc()               { }
UGTerm ExternalBodyOcc::getRepr() const          { return gringo_make_unique<GValTerm>(Value::createId("#external")); }
bool ExternalBodyOcc::isPositive() const         { return false; }
bool ExternalBodyOcc::isNegative() const         { return false; }
void ExternalBodyOcc::setType(OccurrenceType)    { }
OccurrenceType ExternalBodyOcc::getType() const  { return OccurrenceType::STRATIFIED; }
BodyOcc::DefinedBy &ExternalBodyOcc::definedBy() { return defs; }
void ExternalBodyOcc::checkDefined(LocSet &, SigSet const &, UndefVec &) const { }
ExternalBodyOcc::~ExternalBodyOcc()              { }

// }}}

// {{{ definition of *Literal::*Literal

RangeLiteral::RangeLiteral(UTerm &&assign, UTerm &&lower, UTerm &&upper)
    : assign(std::move(assign))
    , range(std::move(lower), std::move(upper)) { }

ScriptLiteral::ScriptLiteral(UTerm &&assign, FWString name, UTermVec &&args)
    : assign(std::move(assign))
    , shared(name, std::move(args)) { }

RelationLiteral::RelationLiteral(Relation rel, UTerm &&left, UTerm &&right)
    : shared(rel, std::move(left), std::move(right)) { }

PredicateLiteral::PredicateLiteral(PredicateDomain &domain, NAF naf, UTerm &&repr)
    : repr(std::move(repr))
    , domain(domain) { gLit.naf = naf; }

ProjectionLiteral::ProjectionLiteral(PredicateDomain &domain, UTerm &&repr, bool initialized)
    : PredicateLiteral(domain, NAF::POS, std::move(repr)), initialized_(initialized) { }

CSPLiteral::CSPLiteral(Relation rel, CSPAddTerm &&left, CSPAddTerm &&right)
    : terms(rel, std::move(left), std::move(right)) { }

// }}}
// {{{ definition of *Literal::print

void RangeLiteral::print(std::ostream &out) const     { out << *assign << "=" << *range.first << ".." << *range.second; }
void ScriptLiteral::print(std::ostream &out) const    {
    out << *assign << "=" << *std::get<0>(shared) << "(";
    print_comma(out, std::get<1>(shared), ",", [](std::ostream &out, UTerm const &term) { out << *term; });
    out << ")";
}
void RelationLiteral::print(std::ostream &out) const  { out << *std::get<1>(shared) << std::get<0>(shared) << *std::get<2>(shared); }
void PredicateLiteral::print(std::ostream &out) const { 
    out << gLit.naf << *repr;
    switch (type) {
        case OccurrenceType::POSITIVELY_STRATIFIED: { break; }
        case OccurrenceType::STRATIFIED:            { out << "!"; break; }
        case OccurrenceType::UNSTRATIFIED:          { out << "?"; break; }
    }
}
void CSPLiteral::print(std::ostream &out) const  { out << std::get<1>(terms) << std::get<0>(terms) << std::get<2>(terms); }

// }}}
// {{{ definition of *Literal::isRecursive

bool RangeLiteral::isRecursive() const     { return false; }
bool ScriptLiteral::isRecursive() const    { return false; }
bool RelationLiteral::isRecursive() const  { return false; }
bool PredicateLiteral::isRecursive() const { return type == OccurrenceType::UNSTRATIFIED; }
bool CSPLiteral::isRecursive() const       { return false; }

// }}}
// {{{ definition of *Literal::occurrence

BodyOcc *RangeLiteral::occurrence()     { return nullptr; }
BodyOcc *ScriptLiteral::occurrence()    { return nullptr; }
BodyOcc *RelationLiteral::occurrence()  { return nullptr; }
BodyOcc *PredicateLiteral::occurrence() { return this; }
BodyOcc *CSPLiteral::occurrence()       { return &ext; }

// }}}
// {{{ definition of *Literal::collect

void RangeLiteral::collect(VarTermBoundVec &vars) const {
    assign->collect(vars, true);
    range.first->collect(vars, false);
    range.second->collect(vars, false);
}
void ScriptLiteral::collect(VarTermBoundVec &vars) const {
    assign->collect(vars, true);
    for (auto &x : std::get<1>(shared)) { x->collect(vars, false); }
}
void RelationLiteral::collect(VarTermBoundVec &vars) const { 
    std::get<1>(shared)->collect(vars, std::get<0>(shared) == Relation::EQ);
    std::get<2>(shared)->collect(vars, false);
}
void PredicateLiteral::collect(VarTermBoundVec &vars) const { repr->collect(vars, gLit.naf == NAF::POS); }
void CSPLiteral::collect(VarTermBoundVec &vars) const { 
    std::get<1>(terms).collect(vars);
    std::get<2>(terms).collect(vars);
}

// }}}
// {{{ definition of Literal::collectImportant

void Literal::collectImportant(Term::VarSet &vars) {
    auto occ(occurrence());
    if (occ && occ->getType() != OccurrenceType::POSITIVELY_STRATIFIED) {
        VarTermBoundVec x;
        collect(x);
        for (auto &occ : x) { vars.emplace(occ.first->name); }
    }
}

void CSPLiteral::collectImportant(Term::VarSet &vars) {
    VarTermBoundVec x;
    collect(x);
    for (auto &occ : x) { vars.emplace(occ.first->name); }
}

// }}}
// {{{ definition of *Literal::index

UIdx RangeLiteral::index(Scripts &, BinderType, Term::VarSet &bound) {
    if (assign->bind(bound)) { return gringo_make_unique<RangeBinder>(get_clone(assign), range); }
    else                     { return gringo_make_unique<RangeMatcher>(*assign, range); }
}
UIdx ScriptLiteral::index(Scripts &scripts, BinderType, Term::VarSet &bound) {
    UTerm clone(assign->clone());
    clone->bind(bound);
    return gringo_make_unique<ScriptBinder>(scripts, std::move(clone), shared);
}
UIdx RelationLiteral::index(Scripts &, BinderType, Term::VarSet &bound) {
    if (std::get<0>(shared) == Relation::EQ) {
        UTerm clone(std::get<1>(shared)->clone());
        VarTermVec occBound;
        if (clone->bind(bound)) { return gringo_make_unique<AssignBinder>(std::move(clone), *std::get<2>(shared)); }
    }
    return gringo_make_unique<RelationMatcher>(shared);
}
UIdx PredicateLiteral::index(Scripts &, BinderType type, Term::VarSet &bound) {
    return make_binder(domain, gLit.naf, *repr, gLit.repr, type, isRecursive(), bound, 0);
}
UIdx ProjectionLiteral::index(Scripts &, BinderType type, Term::VarSet &bound) {
    assert(bound.empty());
    assert(type == BinderType::ALL || type == BinderType::NEW);
    return make_binder(domain, gLit.naf, *repr, gLit.repr, type, isRecursive(), bound, initialized_ ? domain.exports.incOffset : 0);
}
UIdx CSPLiteral::index(Scripts &, BinderType, Term::VarSet &) {
    return gringo_make_unique<CSPLiteralMatcher>(terms);
}

// }}}
// {{{ definition of *Literal::score

Literal::Score RangeLiteral::score(Term::VarSet const &) { 
    if (range.first->getInvertibility() == Term::CONSTANT && range.second->getInvertibility() == Term::CONSTANT) {
        bool undefined = false;
        Value l(range.first->eval(undefined));
        Value r(range.second->eval(undefined));
        return (l.type() == Value::NUM && r.type() == Value::NUM) ? r.num() - l.num() : -1;
    }
    return 0;
}
Literal::Score ScriptLiteral::score(Term::VarSet const &) { 
    return 0;
}
Literal::Score RelationLiteral::score(Term::VarSet const &) { 
    return -1;
}
Literal::Score PredicateLiteral::score(Term::VarSet const &bound) {
    return gLit.naf == NAF::POS ? estimate(domain.exports.size(), *repr, bound) : 0;
}
Literal::Score CSPLiteral::score(Term::VarSet const &) { 
    return std::numeric_limits<Literal::Score>::infinity();
}

// }}}
// {{{ definition of *Literal::toOutput

std::pair<Output::Literal*,bool> RangeLiteral::toOutput()     { return {nullptr, true}; }
std::pair<Output::Literal*,bool> ScriptLiteral::toOutput()    { return {nullptr, true}; }
std::pair<Output::Literal*,bool> RelationLiteral::toOutput()  { return {nullptr, true}; }
std::pair<Output::Literal*,bool> PredicateLiteral::toOutput() { 
    // TODO: the name isFalse is really bad...
    // TODO: make the inc_ literals a decicated class
    if (gLit.repr->second.isFalse() || std::strncmp("#inc_", gLit.repr->first.name()->c_str(), 5) == 0) {
        return {nullptr,true};
    }
    switch (gLit.naf) {
        case NAF::POS:
        case NAF::NOTNOT: { return {&gLit, gLit.repr->second.fact(isRecursive())}; }
        case NAF::NOT:    { return {&gLit, gLit.repr->second.isFalse()}; }
    }
    assert(false);
    return {nullptr,true};
}
std::pair<Output::Literal*,bool> CSPLiteral::toOutput() {
    CSPGroundLit add;
    std::get<0>(add) = std::get<0>(terms);
    std::get<1>(terms).toGround(add, false);
    std::get<2>(terms).toGround(add, true);
    repr.reset(std::move(add));
    return {&repr, false};
}

// }}}
// {{{ definition of *Literal::~*Literal

RangeLiteral::~RangeLiteral() { }
ScriptLiteral::~ScriptLiteral() { }
RelationLiteral::~RelationLiteral() { }
PredicateLiteral::~PredicateLiteral() { }
ProjectionLiteral::~ProjectionLiteral() { }
CSPLiteral::~CSPLiteral() { }

// }}}

} } // namespace Ground Gringo

