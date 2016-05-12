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

#include "gringo/bug.hh"
#include "gringo/input/statement.hh"
#include "gringo/input/literals.hh"
#include "gringo/input/aggregates.hh"
#include "gringo/ground/statements.hh"
#include "gringo/safetycheck.hh"

#include <numeric>

namespace Gringo { namespace Input {

// {{{ helpers for weak constraints

namespace {

UTermVec wc_to_args(UTerm &&weight, UTerm &&prioriy, UTermVec &&tuple) {
    UTermVec args;
    args.emplace_back(std::move(weight));
    args.emplace_back(std::move(prioriy));
    std::move(tuple.begin(), tuple.end(), std::back_inserter(args));
    return args;
}

UHeadAggr wc_to_head(UTermVec &&args) {
    assert(args.size() > 1);
    Location loc(args.front()->loc() + args.back()->loc());
    return make_locatable<SimpleHeadLiteral>(loc, make_locatable<PredicateLiteral>(loc, NAF::POS, make_locatable<FunctionTerm>(loc, "#wc", std::move(args))));
}

Term const &wc_term_from_head(UHeadAggr const &head) {
    assert(dynamic_cast<SimpleHeadLiteral*>(head.get()));
    assert(dynamic_cast<PredicateLiteral*>(static_cast<SimpleHeadLiteral&>(*head).lit.get()));
    return *static_cast<PredicateLiteral&>(*static_cast<SimpleHeadLiteral&>(*head).lit).repr;
}

UTermVec wc_args_from_term(Term const &term) {
    if (term.getInvertibility() == Term::CONSTANT) {
        bool undefined; // Note: already checked elsewhere
        Value x = term.eval(undefined);
        assert(x.type() == Value::FUNC);
        UTermVec ret;
        for (Value y : x.args()) { ret.emplace_back(make_locatable<ValTerm>(term.loc(), y)); }
        return ret;
    }
    else {
        assert(dynamic_cast<FunctionTerm const*>(&term));
        return get_clone(dynamic_cast<FunctionTerm const &>(term).args);
    }
}

} // namespace

// }}}

// {{{ definition of Statement::Statement

Statement::Statement(UHeadAggr &&head, UBodyAggrVec &&body, StatementType type)
    : head(std::move(head))
    , body(std::move(body))
    , type(type) { }

Statement::Statement(UTerm &&weight, UTerm &&priority, UTermVec &&tuple, UBodyAggrVec &&body)
    : Statement(wc_to_args(std::move(weight), std::move(priority), std::move(tuple)), std::move(body)) {
}

Statement::Statement(UTermVec &&tuple, UBodyAggrVec &&body)
    : Statement(wc_to_head(std::move(tuple)), std::move(body), StatementType::WEAKCONSTRAINT) { }
// }}}
// {{{ definition of Statement::add

void Statement::add(ULit &&lit) {
    Location loc(lit->loc());
    body.emplace_back(make_locatable<SimpleBodyLiteral>(loc, std::move(lit)));
}

// }}}
// {{{ definition of Statement::print

void Statement::print(std::ostream &out) const {
    if (type != StatementType::WEAKCONSTRAINT) {
        if (type == StatementType::EXTERNAL) { out << "#external "; }
        if (head) { out << *head; }
        if (!body.empty()) {
            out << (type == StatementType::EXTERNAL ? ":" : ":-");
            auto f = [](std::ostream &out, UBodyAggr const &x) { out << *x; };
            print_comma(out, body, ";", f);
        }
        out << ".";
    }
    else {
        out << ":~";
        print_comma(out, body, ";", [](std::ostream &out, UBodyAggr const &x) { out << *x; });
        out << ".[";
        Term const &term = wc_term_from_head(head);
        if (term.getInvertibility() == Term::CONSTANT) {
            bool undefined; // Note: don't care
            FWValVec args = term.eval(undefined).args();
            out << args.front() << "@" << *(args.begin() + 1);
            for (auto it = args.begin() + 2, ie = args.end(); it != ie; ++it) { out << "," << *it; }
        }
        else {
            UTermVec const &args = static_cast<FunctionTerm const&>(term).args;
            out << *args.front() << "@" << **(args.begin() + 1);
            for (auto it = args.begin() + 2, ie = args.end(); it != ie; ++it) { out << "," << **it; }
        }
        out << "]";
    }
}

// }}}
// {{{ definition of Statement::isEDB

Value Statement::isEDB() const { return type == StatementType::RULE && body.empty() ? head->isEDB() : Value(); }

// }}}
// {{{ definition of Statement::unpool

UStmVec Statement::unpool(bool beforeRewrite) {
    std::vector<UBodyAggrVec> bodies;
    if (beforeRewrite) {
        Term::unpool(body.begin(), body.end(),
            [] (UBodyAggr &x) -> UBodyAggrVec {
                UBodyAggrVec body;
                x->unpool(body, true);
                return body;
            }, [&bodies](UBodyAggrVec &&x) { bodies.push_back(std::move(x)); });
    }
    else {
        bodies.emplace_back();
        for (auto &y : body) {
            y->unpool(bodies.back(), beforeRewrite);
        }
    }
    UHeadAggrVec heads;
    head->unpool(heads, beforeRewrite);
    UStmVec x;
    for (auto &body : bodies) {
        for (auto &head : heads) {
            x.emplace_back(make_locatable<Statement>(loc(), get_clone(head), get_clone(body), type));
        }
    }
    return x;
}

// }}}
// {{{ definition of Statement::hasPool

bool Statement::hasPool(bool beforeRewrite) const {
    for (auto &x : body) { if (x->hasPool(beforeRewrite)) { return true; } }
    return head->hasPool(beforeRewrite);
}

// }}}
// {{{ definition of Statement::rewrite1

bool Statement::rewrite1(Projections &project) {
    SimplifyState state;
    if (!head->simplify(project, state)) { return false; }
    bool singleton = std::accumulate(body.begin(), body.end(), 0, [](unsigned x, UBodyAggr const &y){ return x + y->projectScore(); }) == 1 && head->isPredicate();
    for (auto &y : body) {
        if (!y->simplify(project, state, singleton)) { return false; }
    }
    for (auto &y : state.dots) { body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(RangeLiteral::make(y))); }
    for (auto &y : state.scripts) { body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(ScriptLiteral::make(y))); }
    return true;
}

// }}}
// {{{ definition of Statement::rewrite2

namespace {

void _rewriteAggregates(UBodyAggrVec &body) {
    UBodyAggrVec aggr;
    auto jt = body.begin();
    for (auto it = jt, ie = body.end(); it != ie; ++it) {
        if ((*it)->rewriteAggregates(aggr)) {
            if (it != jt) { *jt = std::move(*it); }
            ++jt;
        }
    }
    body.erase(jt, body.end());
    std::move(aggr.begin(), aggr.end(), std::back_inserter(body));
}

void _rewriteAssignments(UBodyAggrVec &body) {
    using LitDep = SafetyChecker<VarTerm*, UBodyAggr>;
    using VarMap = std::unordered_map<FWString, LitDep::VarNode*>;
    LitDep dep;
    VarMap map;
    for (auto &y : body) {
        auto &ent = dep.insertEnt(std::move(y));
        VarTermBoundVec vars;
        ent.data->collect(vars);
        for (auto &occ : vars) {
            if (occ.first->level == 0) {
                auto &var(map[occ.first->name]);
                if (!var)  { var = &dep.insertVar(occ.first); }
                if (occ.second) { dep.insertEdge(ent, *var); }
                else            { dep.insertEdge(*var, ent); }
            }
        }
    }
    LitDep::VarVec bound;
    LitDep::EntVec open;
    open.reserve(body.size()); // Note: keeps iterators valid
    body.clear();
    dep.init(open);
    UBodyAggrVec sorted;
    for (auto it = open.begin(); it != open.end(); ) {
        LitDep::EntVec assign;
        for (; it != open.end(); ++it) {
            if (!(*it)->data->isAssignment()) {
                dep.propagate(*it, open, &bound);
                sorted.emplace_back(std::move((*it)->data));
            }
            else { assign.emplace_back(*it); }
        }
        LitDep::EntVec nextAssign;
        while (!assign.empty()) {
            for (auto &x : assign) {
                bool allBound = true;
                for (auto &y : x->provides) {
                    if (!y->bound) {
                        allBound = false;
                        break;
                    }
                }
                if (!allBound) { nextAssign.emplace_back(x); }
                else {
                    x->data->removeAssignment();
                    dep.propagate(x, open, &bound);
                    sorted.emplace_back(std::move(x->data));
                }
            }
            if (!nextAssign.empty()) {
                dep.propagate(nextAssign.back(), open, &bound);
                sorted.emplace_back(std::move(nextAssign.back()->data));
                nextAssign.pop_back();
            }
            assign = std::move(nextAssign);
        }
    }
    for (auto &x : dep.entNodes_) {
        if (x.depends > 0) { sorted.emplace_back(std::move(x.data)); }
    }
    // dep.entNodes_;
    body = std::move(sorted);
}

} // namespace

void Statement::rewrite2() {
    AuxGen auxGen;
    { // rewrite aggregates
        Term::replace(head, head->rewriteAggregates(body));
        _rewriteAggregates(body);
    }
    { // assign levels
        AssignLevel c;
        head->assignLevels(c);
        for (auto &y : body) { y->assignLevels(c); }
        c.assignLevels();
    }
    { // rewrite arithmetics
        Term::ArithmeticsMap arith;
        Literal::AssignVec assign;
        arith.emplace_back();
        head->rewriteArithmetics(arith, auxGen);
        for (auto &y : body) { y->rewriteArithmetics(arith, assign, auxGen); }
        for (auto &y : arith.back()) { body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(RelationLiteral::make(y))); }
        for (auto &y : assign) { body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(RelationLiteral::make(y))); }
        arith.pop_back();
    }
    _rewriteAssignments(body);
}

// }}}
// {{{ definition of Statement::check

bool Statement::check() const {
    bool ret = true;
    ChkLvlVec levels;
    levels.emplace_back(loc(), *this);
    ret = head->check(levels) && ret;
    for (auto &y : body) { ret = y->check(levels) && ret; }
    return levels.back().check() && ret;
}

// }}}
// {{{ definition of Statement::replace

void Statement::replace(Defines &x) {
    head->replace(x);
    for (auto &y : body) { y->replace(x); }
}

// }}}
// {{{ definition of Statement::toGround

namespace {

void toGround(CreateHead &&head, UBodyAggrVec const &body, ToGroundArg &x, Ground::UStmVec &stms) {
    CreateBodyVec createVec;
    for (auto &y : body) { createVec.emplace_back(y->toGround(x, stms)); }
    Ground::ULitVec lits;
    for (auto current = createVec.begin(), end = createVec.end(); current != end; ++current) {
        current->first(lits, true);
        for (auto &z : current->second) {
            Ground::ULitVec splitLits;
            for (auto it = createVec.begin(); it != end; ++it) {
                if (it != current) { it->first(splitLits, it < current); }
            }
            stms.emplace_back(z(std::move(splitLits)));
        }
    }
    stms.emplace_back(head(std::move(lits)));
}

} // namespace

void Statement::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    Ground::RuleType t = Ground::RuleType::NORMAL;
    switch (type) {
        case StatementType::WEAKCONSTRAINT: {
            CreateHead hd{[this](Ground::ULitVec &&lits) -> Ground::UStm { return gringo_make_unique<Ground::WeakConstraint>(wc_args_from_term(wc_term_from_head(head)), std::move(lits)); }};
            Gringo::Input::toGround(std::move(hd), body, x, stms);
            return;
        }
        case StatementType::EXTERNAL:   { t = Ground::RuleType::EXTERNAL;   break; }
        case StatementType::RULE:       { t = Ground::RuleType::NORMAL;     break; }
    }
    Gringo::Input::toGround(head->toGround(x, stms, t), body, x, stms);
}

// }}}
// {{{ definition of Statement::~Statement

Statement::~Statement()                     { }

// }}}

} } // namespace Input Gringo

