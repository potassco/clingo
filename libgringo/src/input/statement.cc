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

#include "gringo/bug.hh"
#include "gringo/input/statement.hh"
#include "gringo/input/literals.hh"
#include "gringo/input/aggregates.hh"
#include "gringo/ground/statements.hh"
#include "gringo/safetycheck.hh"

#include <numeric>

namespace Gringo { namespace Input {

// {{{ definition of Statement::Statement

Statement::Statement(UHeadAggr &&head, UBodyAggrVec &&body)
    : head(std::move(head))
    , body(std::move(body)) { }

// }}}

void Statement::initTheory(TheoryDefs &def, Logger &log) {
    head->initTheory(def, !body.empty(), log);
    for (auto &lit : body) {
        lit->initTheory(def, log);
    }
}
// {{{ definition of Statement::add

void Statement::add(ULit &&lit) {
    Location loc(lit->loc());
    body.emplace_back(make_locatable<SimpleBodyLiteral>(loc, std::move(lit)));
}

// }}}
// {{{ definition of Statement::print

void Statement::print(std::ostream &out) const {
    head->printWithCondition(out, body);
}

// }}}
// {{{ definition of Statement::isEDB

Symbol Statement::isEDB() const { return body.empty() ? head->isEDB() : Symbol(); }

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
            x.emplace_back(make_locatable<Statement>(loc(), get_clone(head), get_clone(body)));
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
// {{{ definition of Statement::assignLevels

void Statement::assignLevels(VarTermBoundVec &bound) {
    AssignLevel c;
    head->assignLevels(c);
    for (auto &y : body) { y->assignLevels(c); }
    c.add(bound);
    c.assignLevels();
}

// }}}
// {{{ definition of Statement::simplify

bool Statement::simplify(Projections &project, Logger &log) {
    SimplifyState state;
    if (!head->simplify(project, state, log)) { return false; }
    bool singleton = std::accumulate(body.begin(), body.end(), 0, [](unsigned x, UBodyAggr const &y){ return x + y->projectScore(); }) == 1 && head->isPredicate();
    for (auto &y : body) {
        if (!y->simplify(project, state, singleton, log)) { return false; }
    }
    for (auto &y : state.dots) { body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(RangeLiteral::make(y))); }
    for (auto &y : state.scripts) { body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(ScriptLiteral::make(y))); }
    return true;
}

// }}}
// {{{ definition of Statement::rewrite

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

// TODO: this code has to be adjusted
//       the toGround code then has to take an additional argument
//       to only create aggregate literals up to an index
//       EXAMPLE:
//         h :- B, { c1 }, { c2 }.
//         a1 :- B*, c1.       % accumulation rule
//         a2 :- B*, a1*, c2.  % accumulation rule
//         % TODO: add complete rules...
//         h :- B, a1, a2.
//

void _rewriteAssignments(UBodyAggrVec &body) {
    using LitDep = SafetyChecker<VarTerm*, UBodyAggr>;
    using VarMap = std::unordered_map<String, LitDep::VarNode*>;
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
    for (std::size_t it = 0; it != open.size(); ) {
        LitDep::EntVec assign;
        for (; it != open.size(); ++it) {
            if (!(open[it])->data->isAssignment()) {
                dep.propagate(open[it], open, &bound);
                sorted.emplace_back(std::move((open[it])->data));
            }
            else { assign.emplace_back(open[it]); }
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

void Statement::rewrite() {
    AuxGen auxGen;
    { // rewrite aggregates
        Term::replace(head, head->rewriteAggregates(body));
        _rewriteAggregates(body);
    }
    { // rewrite arithmetics
        Term::ArithmeticsMap arith;
        Literal::AssignVec assign;
        arith.emplace_back(gringo_make_unique<Term::LevelMap>());
        head->rewriteArithmetics(arith, auxGen);
        for (auto &y : body) { y->rewriteArithmetics(arith, assign, auxGen); }
        for (auto &y : *arith.back()) { body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(RelationLiteral::make(y))); }
        for (auto &y : assign) { body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(RelationLiteral::make(y))); }
        arith.pop_back();
    }
    { // rewrite linear inequalities into intervals
      // TODO
      // 1. gather linear inequalities in body
      // 2. compute bounds
      // 3. add intervals
      /*
      IntervalSolver is;
      for (auto &y : body) { y->gatherInequalities(is); }
      for (auto &&rng : is.solve()) {
          body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(RangeLiteral::make(rng)));
      }
      for (auto &y : body) {
          // this creates a new IntervalSolver remembering the bounds
          // furthermore, it marks the variables in those bounds as global
          // and will not provide ranges involving them
          auto subIs = is.nextLevel();
          y->rewriteInequalities(subIs);
      }

       */
    }
    _rewriteAssignments(body);
}

// }}}
// {{{ definition of Statement::check

void Statement::check(Logger &log) const {
    ChkLvlVec levels;
    levels.emplace_back(loc(), *this);
    head->check(levels, log);
    for (auto &y : body) { y->check(levels, log); }
    levels.back().check(log);
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
        current->first(lits, true, false);
        for (auto &z : current->second) {
            Ground::ULitVec splitLits;
            for (auto it = createVec.begin(); it != end; ++it) {
                if (it != current) { it->first(splitLits, it < current, true); }
            }
            stms.emplace_back(z(std::move(splitLits)));
        }
    }
    stms.emplace_back(head(std::move(lits)));
}

} // namespace

void Statement::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    Gringo::Input::toGround(head->toGround(x, stms), body, x, stms);
}

// }}}
// {{{ definition of Statement::~Statement

Statement::~Statement()                     { }

// }}}

} } // namespace Input Gringo

