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
#include "gringo/input/aggregates.hh"
#include "gringo/input/literals.hh"
#include "gringo/ground/statements.hh"
#include "gringo/ground/literals.hh"
#include "gringo/logger.hh"
#ifdef _MSC_VER
#pragma warning (disable : 4503) // decorated name length exceeded
#endif
namespace Gringo { namespace Input {

namespace {

// {{{ definition of auxiliary functions

template <class T, class U, class V>
void _print(std::ostream &out, AggregateFunction fun, T const &x, U const &y, V const &f) {
    auto it = std::begin(x), ie = std::end(x);
    if (it != ie) {
        it->bound->print(out);
        out << inv(it->rel);
        ++it;
    }
    out << fun << "{";
    print_comma(out, y, ";", f);
    out << "}";
    for (; it != ie; ++it) {
        out << it->rel;
        it->bound->print(out);
    }
}

auto _printCond = [](std::ostream &out, CondLit const &x) {
    using namespace std::placeholders;
    x.first->print(out);
    out << ":";
    print_comma(out, x.second, ",", std::bind(&Literal::print, _2, _1));
};

std::function<ULitVec(ULit const &)> _unpool_lit(bool beforeRewrite) {
    return [beforeRewrite](ULit const &x) { return x->unpool(beforeRewrite); };
}
auto _unpool_bound = [](Bound &x) { return x.unpool(); };

void _add(ChkLvlVec &levels, ULit const &lit, bool bind) {
    VarTermBoundVec vars;
    levels.back().current = &levels.back().dep.insertEnt();
    lit->collect(vars, bind);
    addVars(levels, vars);
}

void _add(ChkLvlVec &levels, UTermVec const &terms, CSPAddTerm const *term = nullptr) {
    VarTermBoundVec vars;
    levels.back().current = &levels.back().dep.insertEnt();
    for (auto &x : terms)  { x->collect(vars, false); }
    if (term) { term->collect(vars); }
    addVars(levels, vars);
}

void _add(ChkLvlVec &levels, ULitVec const &cond) {
    for (auto &x : cond) { _add(levels, x, true); }
}

template <class T>
void _aggr(ChkLvlVec &levels, BoundVec const &bounds, T const &f, bool bind) {
    bool assign = false;
    CheckLevel::SC::EntNode *depend = 0;
    for (auto &y : bounds) {
        if (bind && y.rel == Relation::EQ) {
            levels.back().current = &levels.back().dep.insertEnt();
            VarTermBoundVec vars;
            y.bound->collect(vars, true);
            addVars(levels, vars);
            f();
            assign = true;
        }
        else {
            if (!depend) { depend = &levels.back().dep.insertEnt(); }
            levels.back().current = depend;
            VarTermBoundVec vars;
            y.bound->collect(vars, false);
            addVars(levels, vars);
        }
    }
    if (!depend && !assign) { depend = &levels.back().dep.insertEnt(); }
    if (depend) {
        levels.back().current = depend;
        f();
    }
}

void warnGlobal(VarTermBoundVec &vars, bool warn, Logger &log) {
    if (warn) {
        auto ib = vars.begin(), ie = vars.end();
        ie = std::remove_if(ib, ie, [](VarTermBoundVec::value_type const &a) { return a.first->level > 0; });
        std::sort(ib, ie, [](VarTermBoundVec::value_type const &a, VarTermBoundVec::value_type const &b) { return a.first->name < b.first->name; });
        ie = std::unique(ib, ie, [](VarTermBoundVec::value_type const &a, VarTermBoundVec::value_type const &b) { return a.first->name == b.first->name; });
        for (auto it = ib; it != ie; ++it) {
            GRINGO_REPORT(log, Warnings::GlobalVariable)
                << it->first->loc() << ": info: global variable in tuple of aggregate element:\n"
                << "  " << it->first->name << "\n"
                ;
        }
    }
}

// }}}1

} // namespace

// {{{1 definition of TupleBodyAggregate

TupleBodyAggregate::TupleBodyAggregate(NAF naf, AggregateFunction fun, BoundVec &&bounds, BodyAggrElemVec &&elems)
    : TupleBodyAggregate(naf, false, false, fun, std::move(bounds), std::move(elems)) { }

TupleBodyAggregate::TupleBodyAggregate(NAF naf, bool removedAssignment, bool translated, AggregateFunction fun, BoundVec &&bounds, BodyAggrElemVec &&elems)
    : naf(naf)
    , removedAssignment(removedAssignment)
    , translated(translated)
    , fun(fun)
    , bounds(std::move(bounds))
    , elems(std::move(elems)) { }

TupleBodyAggregate::~TupleBodyAggregate() { }

void TupleBodyAggregate::print(std::ostream &out) const {
    auto f = [](std::ostream &out, BodyAggrElem const &y) {
        using namespace std::placeholders;
        print_comma(out, y.first, ",", std::bind(&Term::print, _2, _1));
        out << ":";
        print_comma(out, y.second, ",", std::bind(&Literal::print, _2, _1));
    };
    out << naf;
    _print(out, fun, bounds, elems, f);
}

size_t TupleBodyAggregate::hash() const {
    return get_value_hash(typeid(TupleBodyAggregate).hash_code(), size_t(naf), size_t(fun), bounds, elems);
}

bool TupleBodyAggregate::operator==(BodyAggregate const &x) const {
    auto t = dynamic_cast<TupleBodyAggregate const *>(&x);
    return t && naf == t->naf && fun == t->fun && is_value_equal_to(bounds, t->bounds) && is_value_equal_to(elems, t->elems);
}

TupleBodyAggregate *TupleBodyAggregate::clone() const {
    return make_locatable<TupleBodyAggregate>(loc(), naf, removedAssignment, translated, fun, get_clone(bounds), get_clone(elems)).release();
}

void TupleBodyAggregate::unpool(UBodyAggrVec &x, bool beforeRewrite) {
    BodyAggrElemVec e;
    for (auto &elem : elems) {
        auto f = [&](UTermVec &&x) {
            e.emplace_back(std::move(x), get_clone(elem.second));
        };
        Term::unpool(elem.first.begin(), elem.first.end(), Gringo::unpool, f);
    }
    elems = std::move(e);
    e.clear();
    for (auto &elem : elems) {
        if (beforeRewrite) {
            auto f = [&](ULitVec &&y) { e.emplace_back(get_clone(elem.first), std::move(y)); };
            Term::unpool(elem.second.begin(), elem.second.end(), _unpool_lit(beforeRewrite), f);
        }
        else {
            Term::unpoolJoin(elem.second, _unpool_lit(beforeRewrite));
            e.emplace_back(std::move(elem));
        }
    }
    auto f = [&](BoundVec &&y) { x.emplace_back(make_locatable<TupleBodyAggregate>(loc(), naf, removedAssignment, translated, fun, std::move(y), get_clone(e))); };
    Term::unpool(bounds.begin(), bounds.end(), _unpool_bound, f);
}

void TupleBodyAggregate::collect(VarTermBoundVec &vars) const {
    for (auto &bound : bounds) { bound.bound->collect(vars, bound.rel == Relation::EQ && naf == NAF::POS); }
    for (auto &elem : elems) {
        for (auto &term : std::get<0>(elem)) { term->collect(vars, false); }
        for (auto &lit : std::get<1>(elem)) { lit->collect(vars, false); }
    }
}

bool TupleBodyAggregate::rewriteAggregates(UBodyAggrVec &aggr) {
    BoundVec assign;
    auto jt(bounds.begin());
    for (auto it = jt, ie = bounds.end(); it != ie; ++it) {
        if (it->rel == Relation::EQ && naf == NAF::POS) { assign.emplace_back(std::move(*it)); }
        else {
            if (it != jt) { *jt = std::move(*it); }
            ++jt;
        }
    }
    bounds.erase(jt, bounds.end());
    bool skip = bounds.empty() && !assign.empty();
    for (auto it = assign.begin(), ie = assign.end() - skip; it != ie; ++it) {
        BoundVec bound;
        bound.emplace_back(std::move(*it));
        aggr.emplace_back(make_locatable<TupleBodyAggregate>(loc(), naf, removedAssignment, translated, fun, std::move(bound), get_clone(elems)));
    }
    if (skip) { bounds.emplace_back(std::move(assign.back())); }
    return !bounds.empty();
}

bool TupleBodyAggregate::simplify(Projections &project, SimplifyState &state, bool, Logger &log) {
    for (auto &bound : bounds) {
        if (!bound.simplify(state, log)) { return false; }
    }
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](BodyAggrElemVec::value_type &elem) {
        SimplifyState elemState(state);
        for (auto &term : std::get<0>(elem)) {
            if (term->simplify(elemState, false, false, log).update(term).undefined()) { return true; }
        }
        for (auto &lit : std::get<1>(elem)) {
            // NOTE: projection disabled with singelton=true
            if (!lit->simplify(log, project, elemState, true, true)) { return true; }
        }
        for (auto &dot : elemState.dots) { std::get<1>(elem).emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { std::get<1>(elem).emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}

void TupleBodyAggregate::rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &, AuxGen &auxGen) {
    for (auto &bound : bounds) { bound.rewriteArithmetics(arith, auxGen); }
    for (auto &elem : elems) {
        Literal::AssignVec assign;
        arith.emplace_back();
        for (auto &y : std::get<1>(elem)) { y->rewriteArithmetics(arith, assign, auxGen); }
        for (auto &y : arith.back()) { std::get<1>(elem).emplace_back(RelationLiteral::make(y)); }
        for (auto &y : assign) { std::get<1>(elem).emplace_back(RelationLiteral::make(y)); }
        arith.pop_back();
    }
}

void TupleBodyAggregate::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    for (auto &bound : bounds) { bound.bound->collect(vars, false); }
    lvl.add(vars);
    for (auto &elem : elems) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        for (auto &term : std::get<0>(elem)) { term->collect(vars, false); }
        for (auto &lit : std::get<1>(elem)) { lit->collect(vars, false); }
        local.add(vars);
    }
}

void TupleBodyAggregate::check(ChkLvlVec &levels, Logger &log) const {
    auto f = [&]() {
        VarTermBoundVec vars;
        for (auto &y : elems) {
            levels.emplace_back(loc(), *this);
            _add(levels, y.first);
            _add(levels, y.second);
            levels.back().check(log);
            levels.pop_back();
            for (auto &term : y.first) { term->collect(vars, false); }
        }
        warnGlobal(vars, !translated, log);
    };
    return _aggr(levels, bounds, f, naf == NAF::POS);
}

bool TupleBodyAggregate::hasPool(bool beforeRewrite) const {
    for (auto &bound : bounds) { if (bound.bound->hasPool()) { return true; } }
    for (auto &elem : elems) {
        for (auto &term : elem.first) { if (term->hasPool()) { return true; } }
        for (auto &lit : elem.second) { if (lit->hasPool(beforeRewrite)) { return true; } }
    }
    return false;
}

void TupleBodyAggregate::replace(Defines &x) {
    for (auto &bound : bounds) { Term::replace(bound.bound, bound.bound->replace(x, true)); }
    for (auto &elem : elems) {
        for (auto &y : elem.first)  { Term::replace(y, y->replace(x, true)); }
        for (auto &y : elem.second) { y->replace(x); }
    }
}

CreateBody TupleBodyAggregate::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    if (!isAssignment()) {
        stms.emplace_back(gringo_make_unique<Ground::BodyAggregateComplete>(x.domains, x.newId(*this), fun, get_clone(bounds)));
        auto &completeRef = static_cast<Ground::BodyAggregateComplete&>(*stms.back());
        CreateStmVec split;
        split.emplace_back([&completeRef, this](Ground::ULitVec &&lits) -> Ground::UStm {
            UTermVec tuple;
            tuple.emplace_back(make_locatable<ValTerm>(loc(), Symbol()));
            // Note: should this become a function?
            UTerm neutral;
            switch (fun) {
                case AggregateFunction::MIN: { neutral = make_locatable<ValTerm>(loc(), Symbol::createSup());  break; }
                case AggregateFunction::MAX: { neutral = make_locatable<ValTerm>(loc(), Symbol::createInf());  break; }
                default:                     { neutral = make_locatable<ValTerm>(loc(), Symbol::createNum(0)); break; }
            }
            for (auto &y : bounds) { lits.emplace_back(gringo_make_unique<Ground::RelationLiteral>(y.rel, get_clone(neutral), get_clone(y.bound))); }
            auto ret = gringo_make_unique<Ground::BodyAggregateAccumulate>(completeRef, get_clone(tuple), std::move(lits));
            completeRef.addAccuDom(*ret);
            return std::move(ret);
        });
        for (auto &y : elems) {
            split.emplace_back([this,&completeRef,&y,&x](Ground::ULitVec &&lits) -> Ground::UStm {
                for (auto &z : y.second) { lits.emplace_back(z->toGround(x.domains, false)); }
                auto ret = gringo_make_unique<Ground::BodyAggregateAccumulate>(completeRef, get_clone(y.first), std::move(lits));
                completeRef.addAccuDom(*ret);
                return std::move(ret);
            });
        }
        return CreateBody([&completeRef, this](Ground::ULitVec &lits, bool primary, bool auxiliary) {
            if (primary) { lits.emplace_back(gringo_make_unique<Ground::BodyAggregateLiteral>(completeRef, naf, auxiliary)); }
        }, std::move(split));
    }
    else {
        assert(bounds.size() == 1 && naf == NAF::POS);
        VarTermBoundVec vars;
        for (auto &y : elems) {
            for (auto &z : y.first)  { z->collect(vars, false); }
            for (auto &z : y.second) { z->collect(vars, false); }
        }
        UTermVec global(x.getGlobal(vars));
        global.emplace_back(get_clone(bounds.front().bound));
        UTermVec globalSpecial(x.getGlobal(vars));
        UTerm repr(x.newId(std::move(global), loc(), false));
        UTerm dataRepr(x.newId(std::move(globalSpecial), loc()));

        stms.emplace_back(gringo_make_unique<Ground::AssignmentAggregateComplete>(x.domains, get_clone(repr), get_clone(dataRepr), fun));
        auto &completeRef = static_cast<Ground::AssignmentAggregateComplete&>(*stms.back());
        // NOTE: for assignment aggregates this does not make much sense
        //       the empty aggregate always matches and hence the elements
        //       should be grounded without auxLits
        CreateStmVec split;
        split.emplace_back([&completeRef, this](Ground::ULitVec &&lits) -> Ground::UStm {
            UTermVec tuple;
            tuple.emplace_back(make_locatable<ValTerm>(loc(), Symbol()));
            auto ret = gringo_make_unique<Ground::AssignmentAggregateAccumulate>(completeRef, get_clone(tuple), std::move(lits));
            completeRef.addAccuDom(*ret);
            return std::move(ret);
        });
        for (auto &y : elems) {
            split.emplace_back([this,&completeRef,&y,&x](Ground::ULitVec &&lits) -> Ground::UStm {
                for (auto &z : y.second) { lits.emplace_back(z->toGround(x.domains, false)); }
                auto ret = gringo_make_unique<Ground::AssignmentAggregateAccumulate>(completeRef, get_clone(y.first), std::move(lits));
                completeRef.addAccuDom(*ret);
                return std::move(ret);
            });
        }
        return CreateBody([&completeRef, this](Ground::ULitVec &lits, bool primary, bool auxiliary) {
            if (primary) { lits.emplace_back(gringo_make_unique<Ground::AssignmentAggregateLiteral>(completeRef, auxiliary)); }
        }, std::move(split));
    }
}

bool TupleBodyAggregate::isAssignment() const { return !removedAssignment && bounds.size() == 1 && naf == NAF::POS && bounds.front().rel == Relation::EQ && bounds.front().bound->getInvertibility() == Term::INVERTIBLE; }

void TupleBodyAggregate::removeAssignment() {
    assert (isAssignment());
    removedAssignment = true;
}

// {{{1 definition of LitBodyAggregate

LitBodyAggregate::LitBodyAggregate(NAF naf, AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems)
    : naf(naf)
    , fun(fun)
    , bounds(std::move(bounds))
    , elems(std::move(elems)) { }

LitBodyAggregate::~LitBodyAggregate() { }

void LitBodyAggregate::print(std::ostream &out) const {
    out << naf;
    _print(out, fun, bounds, elems, _printCond);
}

size_t LitBodyAggregate::hash() const {
    return get_value_hash(typeid(LitBodyAggregate).hash_code(), size_t(naf), size_t(fun), bounds, elems);
}

bool LitBodyAggregate::operator==(BodyAggregate const &x) const {
    auto t = dynamic_cast<LitBodyAggregate const *>(&x);
    return t && naf == t->naf && fun == t->fun && is_value_equal_to(bounds, t->bounds) && is_value_equal_to(elems, t->elems);
}

LitBodyAggregate *LitBodyAggregate::clone() const {
    return make_locatable<LitBodyAggregate>(loc(), naf, fun, get_clone(bounds), get_clone(elems)).release();
}

void LitBodyAggregate::unpool(UBodyAggrVec &x, bool beforeRewrite) {
    CondLitVec e;
    for (auto &elem : elems) {
        auto f = [&](ULit &&y) { e.emplace_back(std::move(y), get_clone(elem.second)); };
        Term::unpool(elem.first, _unpool_lit(beforeRewrite), f);
    }
    elems = std::move(e);
    e.clear();
    for (auto &elem : elems) {
        if (beforeRewrite) {
            auto f = [&](ULitVec &&y) { e.emplace_back(get_clone(elem.first), std::move(y)); };
            Term::unpool(elem.second.begin(), elem.second.end(), _unpool_lit(beforeRewrite), f);
        }
        else {
            Term::unpoolJoin(elem.second, _unpool_lit(beforeRewrite));
            e.emplace_back(std::move(elem));
        }
    }
    auto f = [&](BoundVec &&y) { x.emplace_back(make_locatable<LitBodyAggregate>(loc(), naf, fun, std::move(y), get_clone(e))); };
    Term::unpool(bounds.begin(), bounds.end(), _unpool_bound, f);
}

void LitBodyAggregate::collect(VarTermBoundVec &vars) const {
    for (auto &bound : bounds) { bound.bound->collect(vars, bound.rel == Relation::EQ && naf == NAF::POS); }
    for (auto &elem : elems) {
        elem.first->collect(vars, false);
        for (auto &lit : elem.second) { lit->collect(vars, false); }
    }
}

bool LitBodyAggregate::rewriteAggregates(UBodyAggrVec &aggr) {
    int id = 0;
    BodyAggrElemVec elems;
    for (auto &x : this->elems) {
        UTermVec tuple;
        x.first->toTuple(tuple, id);
        ULitVec lits(std::move(x.second));
        lits.emplace_back(std::move(x.first));
        elems.emplace_back(std::move(tuple), std::move(lits));
    }
    UBodyAggr x(make_locatable<TupleBodyAggregate>(loc(), naf, false, true, fun, std::move(bounds), std::move(elems)));
    if (x->rewriteAggregates(aggr)) { aggr.emplace_back(std::move(x)); }
    return false;
}

bool LitBodyAggregate::simplify(Projections &project, SimplifyState &state, bool, Logger &log) {
    for (auto &bound : bounds) {
        if (!bound.simplify(state, log)) { return false; }
    }
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](CondLitVec::value_type &elem) {
        SimplifyState elemState(state);
        if (!std::get<0>(elem)->simplify(log, project, elemState, true, true)) { return true; }
        for (auto &lit : std::get<1>(elem)) {
            // NOTE: projection disabled with singelton=true
            if (!lit->simplify(log, project, elemState, true, true)) { return true; }
        }
        for (auto &dot : elemState.dots) { std::get<1>(elem).emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { std::get<1>(elem).emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}

void LitBodyAggregate::rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &, AuxGen &auxGen) {
    for (auto &bound : bounds) { bound.rewriteArithmetics(arith, auxGen); }
    for (auto &elem : elems) {
        Literal::AssignVec assign;
        arith.emplace_back();
        for (auto &y : std::get<1>(elem)) { y->rewriteArithmetics(arith, assign, auxGen); }
        for (auto &y : arith.back()) { std::get<1>(elem).emplace_back(RelationLiteral::make(y)); }
        for (auto &y : assign) { std::get<1>(elem).emplace_back(RelationLiteral::make(y)); }
        arith.pop_back();
    }
}

void LitBodyAggregate::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    for (auto &bound : bounds) { bound.bound->collect(vars, false); }
    lvl.add(vars);
    for (auto &elem : elems) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        std::get<0>(elem)->collect(vars, false);
        for (auto &lit : std::get<1>(elem)) { lit->collect(vars, false); }
        local.add(vars);
    }
}

void LitBodyAggregate::check(ChkLvlVec &levels, Logger &log) const {
    auto f = [&]() {
        for (auto &y : elems) {
            levels.emplace_back(loc(), *this);
            _add(levels, y.first, false);
            _add(levels, y.second);
            levels.back().check(log);
            levels.pop_back();
        }
    };
    _aggr(levels, bounds, f, naf == NAF::POS);
}

bool LitBodyAggregate::hasPool(bool beforeRewrite) const {
    for (auto &bound : bounds) { if (bound.bound->hasPool()) { return true; } }
    for (auto &elem : elems) {
        if (elem.first->hasPool(beforeRewrite)) { return true; }
        for (auto &lit : elem.second) { if (lit->hasPool(beforeRewrite)) { return true; } }
    }
    return false;
}

void LitBodyAggregate::replace(Defines &x) {
    for (auto &bound : bounds) { Term::replace(bound.bound, bound.bound->replace(x, true)); }
    for (auto &elem : elems) {
        elem.first->replace(x);
        for (auto &y : elem.second) { y->replace(x); }
    }
}

CreateBody LitBodyAggregate::toGround(ToGroundArg &, Ground::UStmVec &) const {
    throw std::logic_error("Aggregate::rewriteAggregates must be called before LitAggregate::toGround");
}

bool LitBodyAggregate::isAssignment() const   { return false; }

void LitBodyAggregate::removeAssignment()  { }

// {{{1 definition of Conjunction

Conjunction::Conjunction(ULit &&head, ULitVec &&cond) {
    elems.emplace_back(ULitVecVec(), std::move(cond));
    elems.back().first.emplace_back();
    elems.back().first.back().emplace_back(std::move(head));
}

Conjunction::Conjunction(ElemVec &&elems)
: elems(std::move(elems)) { }

Conjunction::~Conjunction() { }

void Conjunction::print(std::ostream &out) const {
    auto f = [](std::ostream &out, Elem const &y) {
        using namespace std::placeholders;
        print_comma(out, y.first, "|", [](std::ostream &out, ULitVec const &z) {
            print_comma(out, z, "&", std::bind(&Literal::print, _2, _1));
        });
        out << ":";
        print_comma(out, y.second, ",", std::bind(&Literal::print, _2, _1));
    };
    print_comma(out, elems, ";", f);
}

size_t Conjunction::hash() const {
    return get_value_hash(typeid(Conjunction).hash_code(), elems);
}

bool Conjunction::operator==(BodyAggregate const &x) const {
    auto t = dynamic_cast<Conjunction const *>(&x);
    return t && is_value_equal_to(elems, t->elems);
}

Conjunction *Conjunction::clone() const {
    return make_locatable<Conjunction>(loc(), get_clone(elems)).release();
}

void Conjunction::unpool(UBodyAggrVec &x, bool beforeRewrite) {
    ElemVec e;
    for (auto &elem : elems) {
        if (beforeRewrite) {
            ULitVecVec heads;
            for (auto &head : elem.first) {
                auto g = [&](ULitVec &&z) { heads.emplace_back(std::move(z)); };
                Term::unpool(head.begin(), head.end(), _unpool_lit(beforeRewrite), g);
            }
            elem.first = std::move(heads);
            auto f = [&](ULitVec &&y) { e.emplace_back(get_clone(elem.first), std::move(y)); };
            Term::unpool(elem.second.begin(), elem.second.end(), _unpool_lit(beforeRewrite), f);
        }
        else {
            for (auto &head : elem.first) {
                Term::unpoolJoin(head, _unpool_lit(beforeRewrite));
            }
            Term::unpoolJoin(elem.second, _unpool_lit(beforeRewrite));
            e.emplace_back(std::move(elem));
        }
    }
    x.emplace_back(make_locatable<Conjunction>(loc(), std::move(e)));
}

void Conjunction::collect(VarTermBoundVec &vars) const {
    for (auto &elem : elems) {
        for (auto &disj : elem.first) {
            for (auto &lit : disj) { lit->collect(vars, false); }
        }
        for (auto &lit : elem.second) { lit->collect(vars, false); }
    }
}

bool Conjunction::rewriteAggregates(UBodyAggrVec &x) {
    while (elems.size() > 1) {
        ElemVec vec;
        vec.emplace_back(std::move(elems.back()));
        x.emplace_back(make_locatable<Conjunction>(loc(), std::move(vec)));
        elems.pop_back();
    }
    return !elems.empty();
}

bool Conjunction::simplify(Projections &project, SimplifyState &state, bool, Logger &log) {
    for (auto &elem : elems) {
        elem.first.erase(std::remove_if(elem.first.begin(), elem.first.end(), [&](ULitVec &clause) {
            SimplifyState elemState(state);
            for (auto &lit : clause) {
                if (!lit->simplify(log, project, elemState)) { return true; }
            }
            for (auto &dot : elemState.dots) { clause.emplace_back(RangeLiteral::make(dot)); }
            for (auto &script : elemState.scripts) { clause.emplace_back(ScriptLiteral::make(script)); }
            return false;
        }), elem.first.end());
    }
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](ElemVec::value_type &elem) {
        SimplifyState elemState(state);
        for (auto &lit : elem.second) {
            // NOTE: projection disabled with singelton=true
            if (!lit->simplify(log, project, elemState, true, true)) { return true; }
        }
        for (auto &dot : elemState.dots) { elem.second.emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { elem.second.emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}

void Conjunction::rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &, AuxGen &auxGen) {
    for (auto &elem : elems) {
        for (auto &y : elem.first) {
            Literal::AssignVec assign;
            arith.emplace_back();
            for (auto &z : y) {
                z->rewriteArithmetics(arith, assign, auxGen);
            }
            for (auto &z : arith.back()) { y.emplace_back(RelationLiteral::make(z)); }
            for (auto &z : assign) { y.emplace_back(RelationLiteral::make(z)); }
            arith.pop_back();
        }
        Literal::AssignVec assign;
        arith.emplace_back();
        for (auto &y : elem.second) { y->rewriteArithmetics(arith, assign, auxGen); }
        for (auto &y : arith.back()) { elem.second.emplace_back(RelationLiteral::make(y)); }
        for (auto &y : assign) { elem.second.emplace_back(RelationLiteral::make(y)); }
        arith.pop_back();
    }
}

void Conjunction::assignLevels(AssignLevel &lvl) {
    for (auto &elem : elems) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        for (auto &disj : elem.first) {
            for (auto &lit : disj) {
                lit->collect(vars, false);
            }
        }
        for (auto &lit : elem.second) { lit->collect(vars, false); }
        local.add(vars);
    }
}

void Conjunction::check(ChkLvlVec &levels, Logger &log) const {
    levels.back().current = &levels.back().dep.insertEnt();
    for (auto &elem : elems) {
        levels.emplace_back(loc(), *this);
        _add(levels, elem.second);
        // check safety of condition
        levels.back().check(log);
        levels.pop_back();
        for (auto &disj : elem.first) {
            levels.emplace_back(loc(), *this);
            _add(levels, disj);
            _add(levels, elem.second);
            // check safty of head it can contain pure variables!
            levels.back().check(log);
            levels.pop_back();
        }
    }
}

bool Conjunction::hasPool(bool beforeRewrite) const {
    for (auto &elem : elems) {
        for (auto &disj : elem.first) {
            for (auto &lit : disj) {
                if (lit->hasPool(beforeRewrite)) { return true; }
            }
        }
        for (auto &lit : elem.second) { if (lit->hasPool(beforeRewrite)) { return true; } }
    }
    return false;
}

void Conjunction::replace(Defines &x) {
    for (auto &elem : elems) {
        for (auto &y : elem.first) {
            for (auto &z : y) { z->replace(x); }
        }
        for (auto &y : elem.second) { y->replace(x); }
    }
}

CreateBody Conjunction::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    assert(elems.size() == 1);

    UTermVec local;
    std::unordered_set<String> seen;
    VarTermBoundVec varsHead;
    VarTermBoundVec varsBody;
    for (auto &x : elems.front().first) {
        for (auto &y : x) { y->collect(varsHead, false); }
    }
    for (auto &x : elems.front().second) { x->collect(varsBody, false); }
    for (auto &occ : varsBody) {
        if (occ.first->level != 0) {
            seen.emplace(occ.first->name);
        }
    }
    for (auto &occ : varsHead) {
        if (occ.first->level != 0 && seen.find(occ.first->name) != seen.end()) {
            local.emplace_back(occ.first->clone());
        }
    }
    stms.emplace_back(gringo_make_unique<Ground::ConjunctionComplete>(x.domains, x.newId(*this), std::move(local)));
    auto &completeRef = static_cast<Ground::ConjunctionComplete&>(*stms.back());

    Ground::ULitVec condLits;
    for (auto &y : elems.front().second) { condLits.emplace_back(y->toGround(x.domains, false)); }
    stms.emplace_back(gringo_make_unique<Ground::ConjunctionAccumulateCond>(completeRef, std::move(condLits)));

    for (auto &y : elems.front().first)  {
        Ground::ULitVec headLits;
        for (auto &z : y) {
            headLits.emplace_back(z->toGround(x.domains, false));
        }
        stms.emplace_back(gringo_make_unique<Ground::ConjunctionAccumulateHead>(completeRef, std::move(headLits)));
    }

    CreateStmVec split;
    split.emplace_back([&completeRef](Ground::ULitVec &&lits) -> Ground::UStm {
        auto ret = gringo_make_unique<Ground::ConjunctionAccumulateEmpty>(completeRef, std::move(lits));
        return std::move(ret);
    });

    return CreateBody([&completeRef](Ground::ULitVec &lits, bool primary, bool auxiliary) {
        if (primary) { lits.emplace_back(gringo_make_unique<Ground::ConjunctionLiteral>(completeRef, auxiliary)); }
    }, std::move(split));
}

bool Conjunction::isAssignment() const { return false; }

void Conjunction::removeAssignment() { }

// {{{1 definition of SimpleBodyLiteral

SimpleBodyLiteral::SimpleBodyLiteral(ULit &&lit)
    : lit(std::move(lit)) { }

Location const &SimpleBodyLiteral::loc() const { return lit->loc(); }

void SimpleBodyLiteral::loc(Location const &loc) { lit->loc(loc); }

SimpleBodyLiteral::~SimpleBodyLiteral() { }

void SimpleBodyLiteral::print(std::ostream &out) const { lit->print(out); }

size_t SimpleBodyLiteral::hash() const {
    return get_value_hash(typeid(SimpleBodyLiteral).hash_code(), lit);
}

bool SimpleBodyLiteral::operator==(BodyAggregate const &x) const {
    auto t = dynamic_cast<SimpleBodyLiteral const *>(&x);
    return t && is_value_equal_to(lit, t->lit);
}

SimpleBodyLiteral *SimpleBodyLiteral::clone() const {
    return gringo_make_unique<SimpleBodyLiteral>(get_clone(lit)).release();
}

void SimpleBodyLiteral::unpool(UBodyAggrVec &x, bool beforeRewrite) {
    for (auto &y : lit->unpool(beforeRewrite)) { x.emplace_back(gringo_make_unique<SimpleBodyLiteral>(std::move(y))); }
}

void SimpleBodyLiteral::collect(VarTermBoundVec &vars) const { lit->collect(vars, true); }

bool SimpleBodyLiteral::rewriteAggregates(UBodyAggrVec &) { return true; }

bool SimpleBodyLiteral::simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) {
    return lit->simplify(log, project, state, true, singleton);
}

void SimpleBodyLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &assign, AuxGen &auxGen) {
    lit->rewriteArithmetics(arith, assign, auxGen);
}

void SimpleBodyLiteral::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    lit->collect(vars, false);
    lvl.add(vars);
}

void SimpleBodyLiteral::check(ChkLvlVec &levels, Logger &) const {
    levels.back().current = &levels.back().dep.insertEnt();
    _add(levels, lit, true);
}

bool SimpleBodyLiteral::hasPool(bool beforeRewrite) const {
    return lit->hasPool(beforeRewrite);
}

void SimpleBodyLiteral::replace(Defines &x) {
    lit->replace(x);
}

CreateBody SimpleBodyLiteral::toGround(ToGroundArg &x, Ground::UStmVec &) const {
    return {[&](Ground::ULitVec &lits, bool, bool auxiliary) -> void {
        lits.emplace_back(lit->toGround(x.domains, auxiliary));
    }, CreateStmVec()};
}

bool SimpleBodyLiteral::isAssignment() const  { return false; }

void SimpleBodyLiteral::removeAssignment() { }

// }}}1

// {{{1 definition of HeadAggregate

Symbol HeadAggregate::isEDB() const     { return Symbol(); }

// {{{1 definition of TupleHeadAggregate

TupleHeadAggregate::TupleHeadAggregate(AggregateFunction fun, BoundVec &&bounds, HeadAggrElemVec &&elems)
    : TupleHeadAggregate(fun, false, std::move(bounds), std::move(elems)) { }

TupleHeadAggregate::TupleHeadAggregate(AggregateFunction fun, bool translated, BoundVec &&bounds, HeadAggrElemVec &&elems)
    : fun(fun)
    , translated(translated)
    , bounds(std::move(bounds))
    , elems(std::move(elems)) { }

TupleHeadAggregate::~TupleHeadAggregate() { }

void TupleHeadAggregate::print(std::ostream &out) const {
    auto f = [](std::ostream &out, HeadAggrElem const &y) {
        using namespace std::placeholders;
        print_comma(out, std::get<0>(y), ",", std::bind(&Term::print, _2, _1));
        out << ":";
        std::get<1>(y)->print(out);
        out << ":";
        print_comma(out, std::get<2>(y), ",", std::bind(&Literal::print, _2, _1));
    };
    _print(out, fun, bounds, elems, f);
}

size_t TupleHeadAggregate::hash() const {
    return get_value_hash(typeid(TupleHeadAggregate).hash_code(), size_t(fun), bounds, elems);
}

bool TupleHeadAggregate::operator==(HeadAggregate const &x) const {
    auto t = dynamic_cast<TupleHeadAggregate const *>(&x);
    return t && fun == t->fun && is_value_equal_to(bounds, t->bounds) && is_value_equal_to(elems, t->elems);
}

TupleHeadAggregate *TupleHeadAggregate::clone() const {
    return make_locatable<TupleHeadAggregate>(loc(), fun, translated, get_clone(bounds), get_clone(elems)).release();
}

void TupleHeadAggregate::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    HeadAggrElemVec e;
    for (auto &elem : elems) {
        auto f = [&](UTermVec &&y) { e.emplace_back(std::move(y), get_clone(std::get<1>(elem)), get_clone(std::get<2>(elem))); };
        Term::unpool(std::get<0>(elem).begin(), std::get<0>(elem).end(), Gringo::unpool, f);
    }
    elems.clear();
    for (auto &elem : e) {
        auto f = [&](ULit &&y) { elems.emplace_back(get_clone(std::get<0>(elem)), std::move(y), get_clone(std::get<2>(elem))); };
        Term::unpool(std::get<1>(elem), _unpool_lit(beforeRewrite), f);
    }
    e.clear();
    for (auto &elem : elems) {
        if (beforeRewrite) {
            auto f = [&](ULitVec &&y) { e.emplace_back(get_clone(std::get<0>(elem)), get_clone(std::get<1>(elem)), std::move(y)); };
            Term::unpool(std::get<2>(elem).begin(), std::get<2>(elem).end(), _unpool_lit(beforeRewrite), f);
        }
        else {
            Term::unpoolJoin(std::get<2>(elem), _unpool_lit(beforeRewrite));
            e.emplace_back(std::move(elem));
        }
    }
    auto f = [&](BoundVec &&y) { x.emplace_back(make_locatable<TupleHeadAggregate>(loc(), fun, translated, std::move(y), get_clone(e))); };
    Term::unpool(bounds.begin(), bounds.end(), _unpool_bound, f);
}

void TupleHeadAggregate::collect(VarTermBoundVec &vars) const {
    for (auto &bound : bounds) { bound.bound->collect(vars, false); }
    for (auto &elem : elems) {
        for (auto &term : std::get<0>(elem)) { term->collect(vars, false); }
        std::get<1>(elem)->collect(vars, false);
        for (auto &lit : std::get<2>(elem)) { lit->collect(vars, false); }
    }
}

UHeadAggr TupleHeadAggregate::rewriteAggregates(UBodyAggrVec &body) {
    for (auto &x : elems) {
        if (ULit shifted = std::get<1>(x)->shift(false)) {
            // NOTE: FalseLiteral is a bad name
            std::get<1>(x) = make_locatable<FalseLiteral>(std::get<1>(x)->loc());
            std::get<2>(x).emplace_back(std::move(shifted));
        }
    }
    if (elems.size() == 1 && bounds.empty()) {
        auto &elem = elems.front();
        // Note: to handle undefinedness in tuples
        bool weight = fun == AggregateFunction::SUM || fun == AggregateFunction::SUMP;
        auto &tuple = std::get<0>(elem);
        Location l = tuple.empty() ? loc() : tuple.front()->loc();
        for (auto &term : tuple) {
            // NOTE: there could be special predicates for is_integer and is_defined
            UTerm first = get_clone(term);
            if (weight) {
                first = make_locatable<BinOpTerm>(l, BinOp::ADD, std::move(first), make_locatable<ValTerm>(l, Symbol::createNum(0)));
                weight = false;
            }
            body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(make_locatable<RelationLiteral>(l, Relation::LEQ, std::move(first), std::move(term))));
        }
        tuple.clear();
        tuple.emplace_back(make_locatable<ValTerm>(l, Symbol::createNum(0)));
        for (auto &lit : std::get<2>(elem)) {
            body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(std::move(lit)));
        }
        std::get<2>(elem).clear();
    }
    return nullptr;
}

bool TupleHeadAggregate::simplify(Projections &project, SimplifyState &state, Logger &log) {
    for (auto &bound : bounds) {
        if (!bound.simplify(state, log)) { return false; }
    }
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](HeadAggrElemVec::value_type &elem) {
        SimplifyState elemState(state);
        for (auto &term : std::get<0>(elem)) {
            if (term->simplify(elemState, false, false, log).update(term).undefined()) { return true; }
        }
        if (!std::get<1>(elem)->simplify(log, project, elemState, false)) {
            return true;
        }
        for (auto &lit : std::get<2>(elem)) {
            if (!lit->simplify(log, project, elemState)) { return true; }
        }
        for (auto &dot : elemState.dots) { std::get<2>(elem).emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { std::get<2>(elem).emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}

void TupleHeadAggregate::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    for (auto &bound : bounds) { bound.rewriteArithmetics(arith, auxGen); }
    for (auto &elem : elems) {
        Literal::AssignVec assign;
        arith.emplace_back();
        for (auto &y : std::get<2>(elem)) { y->rewriteArithmetics(arith, assign, auxGen); }
        for (auto &y : arith.back()) { std::get<2>(elem).emplace_back(RelationLiteral::make(y)); }
        for (auto &y : assign) { std::get<2>(elem).emplace_back(RelationLiteral::make(y)); }
        arith.pop_back();
    }
}

void TupleHeadAggregate::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    for (auto &bound : bounds) { bound.bound->collect(vars, false); }
    lvl.add(vars);
    for (auto &elem : elems) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        for (auto &term : std::get<0>(elem)) { term->collect(vars, false); }
        std::get<1>(elem)->collect(vars, false);
        for (auto &lit : std::get<2>(elem)) { lit->collect(vars, false); }
        local.add(vars);
    }
}

void TupleHeadAggregate::check(ChkLvlVec &levels, Logger &log) const {
    auto f = [&]() {
        VarTermBoundVec vars;
        for (auto &y : elems) {
            levels.emplace_back(loc(), *this);
            _add(levels, std::get<0>(y));
            _add(levels, std::get<1>(y), false);
            _add(levels, std::get<2>(y));
            levels.back().check(log);
            levels.pop_back();
            for (auto &term : std::get<0>(y)) { term->collect(vars, false); }
        }
        warnGlobal(vars, !translated, log);
    };
    _aggr(levels, bounds, f, false);
}

bool TupleHeadAggregate::hasPool(bool beforeRewrite) const {
    for (auto &bound : bounds) { if (bound.bound->hasPool()) { return true; } }
    for (auto &elem : elems) {
        for (auto &term : std::get<0>(elem)) { if (term->hasPool()) { return true; } }
        if (std::get<1>(elem)->hasPool(beforeRewrite)) { return true; }
        for (auto &lit : std::get<2>(elem)) { if (lit->hasPool(beforeRewrite)) { return true; } }
    }
    return false;
}

void TupleHeadAggregate::replace(Defines &x) {
    for (auto &bound : bounds) { Term::replace(bound.bound, bound.bound->replace(x, true)); }
    for (auto &elem : elems) {
        for (auto &y : std::get<0>(elem)) { Term::replace(y, y->replace(x, true)); }
        std::get<1>(elem)->replace(x);
        for (auto &y : std::get<2>(elem)) { y->replace(x); }
    }
}

CreateHead TupleHeadAggregate::toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType) const {
    bool isSimple = bounds.empty();
    if (isSimple) {
        for (auto &elem  : elems) {
            if (!std::get<2>(elem).empty()) { isSimple = false; break; }
        }
    }
    if (isSimple) {
        DomainData &data = x.domains;
        return CreateHead([&](Ground::ULitVec &&lits) {
            Ground::Rule::HeadVec heads;
            for (auto &elem : elems) {
                if (UTerm headRepr = std::get<1>(elem)->headRepr()) {
                    PredicateDomain *headDom = &data.add(headRepr->getSig());
                    heads.emplace_back(std::move(headRepr), headDom);
                }
            }
            return gringo_make_unique<Ground::Rule>(std::move(heads), std::move(lits), Ground::RuleType::Choice);
        });
    }
    else {
        stms.emplace_back(gringo_make_unique<Ground::HeadAggregateComplete>(x.domains, x.newId(*this), fun, get_clone(bounds)));
        auto &completeRef = static_cast<Ground::HeadAggregateComplete&>(*stms.back());
        for (auto &y : elems) {
            Ground::ULitVec lits;
            for (auto &z : std::get<2>(y)) { lits.emplace_back(z->toGround(x.domains, false)); }
            lits.emplace_back(gringo_make_unique<Ground::HeadAggregateLiteral>(completeRef));
            UTerm headRep(std::get<1>(y)->headRepr());
            PredicateDomain *predDom = headRep ? &x.domains.add(headRep->getSig()) : nullptr;
            auto ret = gringo_make_unique<Ground::HeadAggregateAccumulate>(completeRef, get_clone(std::get<0>(y)), predDom, std::move(headRep), std::move(lits));
            completeRef.addAccuDom(*ret);
            stms.emplace_back(std::move(ret));
        }
        return CreateHead([&completeRef](Ground::ULitVec &&lits) { return gringo_make_unique<Ground::HeadAggregateRule>(completeRef, std::move(lits)); });
    }
}

void TupleHeadAggregate::getNeg(std::function<void (Sig)> f) const {
    for (auto &x : elems) { std::get<1>(x)->getNeg(f); }
}

// {{{1 definition of LitHeadAggregate

LitHeadAggregate::LitHeadAggregate(AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems)
    : fun(fun)
    , bounds(std::move(bounds))
    , elems(std::move(elems)) { }

LitHeadAggregate::~LitHeadAggregate() { }

void LitHeadAggregate::print(std::ostream &out) const {
    _print(out, fun, bounds, elems, _printCond);
}

size_t LitHeadAggregate::hash() const {
    return get_value_hash(typeid(LitHeadAggregate).hash_code(), size_t(fun), bounds, elems);
}

bool LitHeadAggregate::operator==(HeadAggregate const &x) const {
    auto t = dynamic_cast<LitHeadAggregate const *>(&x);
    return t && fun == t->fun && is_value_equal_to(bounds, t->bounds) && is_value_equal_to(elems, t->elems);
}

LitHeadAggregate *LitHeadAggregate::clone() const {
    return make_locatable<LitHeadAggregate>(loc(), fun, get_clone(bounds), get_clone(elems)).release();
}

void LitHeadAggregate::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    CondLitVec e;
    for (auto &elem : elems) {
        auto f = [&](ULit &&y) { e.emplace_back(std::move(y), get_clone(elem.second)); };
        Term::unpool(elem.first, _unpool_lit(beforeRewrite), f);
    }
    elems.clear();
    for (auto &elem : e) {
        if (beforeRewrite) {
            auto f = [&](ULitVec &&y) { elems.emplace_back(get_clone(elem.first), std::move(y)); };
            Term::unpool(elem.second.begin(), elem.second.end(), _unpool_lit(beforeRewrite), f);
        }
        else {
            Term::unpoolJoin(elem.second, _unpool_lit(beforeRewrite));
            elems.emplace_back(std::move(elem));
        }
    }
    e.clear();
    auto f = [&](BoundVec &&y) { x.emplace_back(make_locatable<LitHeadAggregate>(loc(), fun, std::move(y), get_clone(elems))); };
    Term::unpool(bounds.begin(), bounds.end(), _unpool_bound, f);
}

void LitHeadAggregate::collect(VarTermBoundVec &vars) const {
    for (auto &bound : bounds) { bound.bound->collect(vars, false); }
    for (auto &elem : elems) {
        elem.first->collect(vars, false);
        for (auto &lit : elem.second) { lit->collect(vars, false); }
    }
}

UHeadAggr LitHeadAggregate::rewriteAggregates(UBodyAggrVec &aggr) {
    int id = 0;
    HeadAggrElemVec elems;
    for (auto &x : this->elems) {
        UTermVec tuple;
        x.first->toTuple(tuple, id);
        elems.emplace_back(std::move(tuple), std::move(x.first), std::move(x.second));
    }
    UHeadAggr x(make_locatable<TupleHeadAggregate>(loc(), fun, true, std::move(bounds), std::move(elems)));
    Term::replace(x, x->rewriteAggregates(aggr));
    return x;
}

bool LitHeadAggregate::simplify(Projections &project, SimplifyState &state, Logger &log) {
    for (auto &bound : bounds) {
        if (!bound.simplify(state, log)) { return false; }
    }
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](CondLitVec::value_type &elem) {
        SimplifyState elemState(state);
        if (!std::get<0>(elem)->simplify(log, project, elemState, false)) { return true; }
        for (auto &lit : std::get<1>(elem)) {
            if (!lit->simplify(log, project, elemState)) { return true; }
        }
        for (auto &dot : elemState.dots) { std::get<1>(elem).emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { std::get<1>(elem).emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}

void LitHeadAggregate::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    for (auto &bound : bounds) { bound.rewriteArithmetics(arith, auxGen); }
    for (auto &elem : elems) {
        Literal::AssignVec assign;
        arith.emplace_back();
        for (auto &y : std::get<1>(elem)) { y->rewriteArithmetics(arith, assign, auxGen); }
        for (auto &y : arith.back()) { std::get<1>(elem).emplace_back(RelationLiteral::make(y)); }
        for (auto &y : assign) { std::get<1>(elem).emplace_back(RelationLiteral::make(y)); }
        arith.pop_back();
    }
}

void LitHeadAggregate::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    for (auto &bound : bounds) { bound.bound->collect(vars, false); }
    lvl.add(vars);
    for (auto &elem : elems) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        std::get<0>(elem)->collect(vars, false);
        for (auto &lit : std::get<1>(elem)) { lit->collect(vars, false); }
        local.add(vars);
    }
}

void LitHeadAggregate::check(ChkLvlVec &levels, Logger &log) const {
    auto f = [&]() {
        for (auto &y : elems) {
            levels.emplace_back(loc(), *this);
            _add(levels, y.first, false);
            _add(levels, y.second);
            levels.back().check(log);
            levels.pop_back();
        }
    };
    _aggr(levels, bounds, f, false);
}

bool LitHeadAggregate::hasPool(bool beforeRewrite) const {
    for (auto &bound : bounds) { if (bound.bound->hasPool()) { return true; } }
    for (auto &elem : elems) {
        if (elem.first->hasPool(beforeRewrite)) { return true; }
        for (auto &lit : elem.second) { if (lit->hasPool(beforeRewrite)) { return true; } }
    }
    return false;
}

void LitHeadAggregate::replace(Defines &x) {
    for (auto &bound : bounds) { Term::replace(bound.bound, bound.bound->replace(x, true)); }
    for (auto &elem : elems) {
        elem.first->replace(x);
        for (auto &y : elem.second) { y->replace(x); }
    }
}

void LitHeadAggregate::getNeg(std::function<void (Sig)> f) const {
    for (auto &x : elems) { x.first->getNeg(f); }
}

CreateHead LitHeadAggregate::toGround(ToGroundArg &, Ground::UStmVec &, Ground::RuleType) const {
    throw std::logic_error("Aggregate::rewriteAggregates must be called before LitAggregate::toGround");
}

// {{{1 definition of Disjunction

Disjunction::Disjunction(CondLitVec &&elems) {
    for (auto &x : elems) {
        this->elems.emplace_back();
        this->elems.back().second = std::move(x.second);
        this->elems.back().first.emplace_back();
        this->elems.back().first.back().first = std::move(x.first);
    }
}

Disjunction::Disjunction(ElemVec &&elems)
: elems(std::move(elems)) { }

Disjunction::~Disjunction() { }

void Disjunction::print(std::ostream &out) const {
    auto f = [](std::ostream &out, Elem const &y) {
        using namespace std::placeholders;
        print_comma(out, y.first, "&", [](std::ostream &out, Head const &z) {
            out << *z.first << ":";
            print_comma(out, z.second, ",", std::bind(&Literal::print, _2, _1));
        });
        out << ":";
        print_comma(out, y.second, ",", std::bind(&Literal::print, _2, _1));
    };
    print_comma(out, elems, ";", f);
}

size_t Disjunction::hash() const {
    return get_value_hash(typeid(Disjunction).hash_code(), elems);
}

bool Disjunction::operator==(HeadAggregate const &x) const {
    auto t = dynamic_cast<Disjunction const *>(&x);
    return t && is_value_equal_to(elems, t->elems);
}

Disjunction *Disjunction::clone() const {
    return make_locatable<Disjunction>(loc(), get_clone(elems)).release();
}

void Disjunction::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    ElemVec e;
    for (auto &elem : elems) {
        if (beforeRewrite) {
            HeadVec heads;
            for (auto &head : elem.first) {
                for (auto &lit : head.first->unpool(beforeRewrite)) {
                    Term::unpool(head.second.begin(), head.second.end(), _unpool_lit(beforeRewrite), [&](ULitVec &&expand) {
                        heads.emplace_back(get_clone(lit), std::move(expand));
                    });
                }
            }
            elem.first = std::move(heads);
            auto f = [&](ULitVec &&y) { e.emplace_back(get_clone(elem.first), std::move(y)); };
            Term::unpool(elem.second.begin(), elem.second.end(), _unpool_lit(beforeRewrite), f);
        }
        else {
            HeadVec heads;
            for (auto &head : elem.first) {
                Term::unpoolJoin(head.second, _unpool_lit(beforeRewrite));
                for (auto &lit : head.first->unpool(beforeRewrite)) {
                    heads.emplace_back(std::move(lit), get_clone(head.second));
                }
            }
            elem.first = std::move(heads);
            Term::unpoolJoin(elem.second, _unpool_lit(beforeRewrite));
            e.emplace_back(std::move(elem));
        }
    }
    x.emplace_back(make_locatable<Disjunction>(loc(), std::move(e)));
}

void Disjunction::collect(VarTermBoundVec &vars) const {
    for (auto &elem : elems) {
        for (auto &head : elem.first) {
            head.first->collect(vars, false);
            for (auto &lit : head.second) { lit->collect(vars, false); }
        }
        for (auto &lit : elem.second) { lit->collect(vars, false); }
    }
}

UHeadAggr Disjunction::rewriteAggregates(UBodyAggrVec &body) {
    for (auto &elem : this->elems) {
        for (auto &head : elem.first) {
            if (ULit shifted = head.first->shift(true)) {
                head.first = make_locatable<FalseLiteral>(head.first->loc());
                head.second.emplace_back(std::move(shifted));
            }
        }
        if (elem.second.empty() && elem.first.size() == 1) {
            auto &head = elem.first.front();
            for (auto &lit : head.second) { body.emplace_back(make_locatable<SimpleBodyLiteral>(loc(), std::move(lit))); }
            head.second.clear();
        }
    }
    return nullptr;
}

bool Disjunction::simplify(Projections &project, SimplifyState &state, Logger &log) {
    for (auto &elem : elems) {
        elem.first.erase(std::remove_if(elem.first.begin(), elem.first.end(), [&](Head &head) {
            SimplifyState elemState(state);
            if (!head.first->simplify(log, project, elemState)) { return true; }
            for (auto &lit : head.second) {
                if (!lit->simplify(log, project, elemState)) { return true; }
            }
            for (auto &dot : elemState.dots) { head.second.emplace_back(RangeLiteral::make(dot)); }
            for (auto &script : elemState.scripts) { head.second.emplace_back(ScriptLiteral::make(script)); }
            return false;
        }), elem.first.end());
    }
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](ElemVec::value_type &elem) -> bool {
        SimplifyState elemState(state);
        for (auto &lit : elem.second) {
            if (!lit->simplify(log, project, elemState)) { return true; }
        }
        for (auto &dot : elemState.dots) { elem.second.emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { elem.second.emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}

void Disjunction::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    for (auto &elem : elems) {
        for (auto &head : elems) {
            Literal::AssignVec assign;
            arith.emplace_back();
            for (auto &y : head.second) { y->rewriteArithmetics(arith, assign, auxGen); }
            for (auto &y : arith.back()) { head.second.emplace_back(RelationLiteral::make(y)); }
            for (auto &y : assign) { head.second.emplace_back(RelationLiteral::make(y)); }
            arith.pop_back();
        }
        Literal::AssignVec assign;
        arith.emplace_back();
        for (auto &y : elem.second) { y->rewriteArithmetics(arith, assign, auxGen); }
        for (auto &y : arith.back()) { elem.second.emplace_back(RelationLiteral::make(y)); }
        for (auto &y : assign) { elem.second.emplace_back(RelationLiteral::make(y)); }
        arith.pop_back();
    }
}

void Disjunction::assignLevels(AssignLevel &lvl) {
    for (auto &elem : elems) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        for (auto &head : elem.first) {
            head.first->collect(vars, false);
            for (auto &lit : head.second) { lit->collect(vars, false); }
        }
        for (auto &lit : elem.second) { lit->collect(vars, false); }
        local.add(vars);
    }
}

void Disjunction::check(ChkLvlVec &levels, Logger &log) const {
    levels.back().current = &levels.back().dep.insertEnt();
    for (auto &y : elems) {
        levels.emplace_back(loc(), *this);
        _add(levels, y.second);
        // check condition first
        // (although by construction it cannot happen that something in expand binds something in condition)
        levels.back().check(log);
        levels.pop_back();
        for (auto &head : y.first) {
            levels.emplace_back(loc(), *this);
            _add(levels, head.first, false);
            _add(levels, head.second);
            _add(levels, y.second);
            levels.back().check(log);
            levels.pop_back();
        }
    }
}

bool Disjunction::hasPool(bool beforeRewrite) const {
    for (auto &elem : elems) {
        for (auto &head : elem.first) {
            if (head.first->hasPool(beforeRewrite)) { return true; }
            for (auto &lit : head.second) { if (lit->hasPool(beforeRewrite)) { return true; } }
        }
        for (auto &lit : elem.second) { if (lit->hasPool(beforeRewrite)) { return true; } }
    }
    return false;
}

void Disjunction::replace(Defines &x) {
    for (auto &elem : elems) {
        for (auto &head : elem.first) {
            head.first->replace(x);
            for (auto &y : elem.second) { y->replace(x); }
        }
        for (auto &y : elem.second) { y->replace(x); }
    }
}

void Disjunction::getNeg(std::function<void (Sig)> f) const {
    for (auto &x : elems) {
        for (auto &y : x.first) { y.first->getNeg(f); }
    }
}

CreateHead Disjunction::toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType) const {
    bool isSimple = true;
    for (auto &y : elems) {
        if (y.first.size() > 1 || !y.second.empty()) { isSimple = false; break; }
    }
    if (isSimple) {
        DomainData &data = x.domains;
        return CreateHead([&](Ground::ULitVec &&lits) {
            Ground::Rule::HeadVec heads;
            for (auto &elem : elems) {
                for (auto &head : elem.first) {
                    if (UTerm headRepr = head.first->headRepr()) {
                        PredicateDomain *headDom = &data.add(headRepr->getSig());
                        heads.emplace_back(std::move(headRepr), headDom);
                    }
                }
            }
            return gringo_make_unique<Ground::Rule>(std::move(heads), std::move(lits), Ground::RuleType::Disjunctive);
        });
    }
    else {
        stms.emplace_back(gringo_make_unique<Ground::DisjunctionComplete>(x.domains, x.newId(*this)));
        auto &complete = static_cast<Ground::DisjunctionComplete&>(*stms.back());
        for (auto &y : elems) {
            UTermVec elemVars;
            std::unordered_set<String> seen;
            VarTermBoundVec vars;
            for (auto &x : y.second) { x->collect(vars, false); }
            for (auto &occ : vars) {
                if (occ.first->level != 0) { seen.emplace(occ.first->name); }
            }
            vars.clear();
            for (auto &head : y.first) {
                head.first->collect(vars, false);
                for (auto &x : head.second) { x->collect(vars, false); }
            }
            for (auto &occ : vars) {
                if (occ.first->level != 0 && seen.find(occ.first->name) != seen.end()) {
                    elemVars.emplace_back(occ.first->clone());
                    seen.emplace(occ.first->name);
                }
            }
            UTerm elemRepr = x.newId(std::move(elemVars), loc(), true);
            if (y.first.empty()) {
                Ground::ULitVec elemCond;
                for (auto &z : y.second) { elemCond.emplace_back(z->toGround(x.domains, false)); }
                Ground::ULitVec headCond;
                headCond.emplace_back(make_locatable<RelationLiteral>(loc(), Relation::NEQ, make_locatable<ValTerm>(loc(), Symbol::createNum(0)), make_locatable<ValTerm>(loc(), Symbol::createNum(0)))->toGround(x.domains, true));
                stms.emplace_back(gringo_make_unique<Ground::DisjunctionAccumulate>(complete, nullptr, nullptr, std::move(headCond), std::move(elemRepr), std::move(elemCond)));
            }
            else {
                for (auto &head : y.first) {
                    Ground::ULitVec elemCond;
                    for (auto &z : y.second) { elemCond.emplace_back(z->toGround(x.domains, false)); }
                    Ground::ULitVec headCond;
                    for (auto &z : head.second) { headCond.emplace_back(z->toGround(x.domains, false)); }
                    UTerm headRepr = head.first->headRepr();
                    PredicateDomain *headDom = headRepr ? &x.domains.add(headRepr->getSig()) : nullptr;
                    stms.emplace_back(gringo_make_unique<Ground::DisjunctionAccumulate>(complete, headDom, std::move(headRepr), std::move(headCond), get_clone(elemRepr), std::move(elemCond)));
                }
            }
        }
        return CreateHead([&](Ground::ULitVec &&lits) { return gringo_make_unique<Ground::DisjunctionRule>(complete, std::move(lits)); });
    }
}

// {{{1 definition of SimpleHeadLiteral

SimpleHeadLiteral::SimpleHeadLiteral(ULit &&lit)
    : lit(std::move(lit)) { }

Location const &SimpleHeadLiteral::loc() const { return lit->loc(); }

void SimpleHeadLiteral::loc(Location const &loc) { lit->loc(loc); }

SimpleHeadLiteral::~SimpleHeadLiteral() { }

void SimpleHeadLiteral::print(std::ostream &out) const { lit->print(out); }

size_t SimpleHeadLiteral::hash() const {
    return get_value_hash(typeid(SimpleHeadLiteral).hash_code(), lit);
}

bool SimpleHeadLiteral::operator==(HeadAggregate const &x) const {
    auto t = dynamic_cast<SimpleHeadLiteral const *>(&x);
    return t && is_value_equal_to(lit, t->lit);
}

SimpleHeadLiteral *SimpleHeadLiteral::clone() const {
    return gringo_make_unique<SimpleHeadLiteral>(get_clone(lit)).release();
}

void SimpleHeadLiteral::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    for (auto &y : lit->unpool(beforeRewrite)) { x.emplace_back(gringo_make_unique<SimpleHeadLiteral>(std::move(y))); }
}

void SimpleHeadLiteral::collect(VarTermBoundVec &vars) const { lit->collect(vars, true); }

UHeadAggr SimpleHeadLiteral::rewriteAggregates(UBodyAggrVec &aggr) {
    ULit shifted(lit->shift(true));
    if (shifted) {
        aggr.emplace_back(gringo_make_unique<SimpleBodyLiteral>(std::move(shifted)));
        return gringo_make_unique<SimpleHeadLiteral>(make_locatable<FalseLiteral>(lit->loc()));
    }
    return nullptr;
}

bool SimpleHeadLiteral::simplify(Projections &project, SimplifyState &state, Logger &log) {
    return lit->simplify(log, project, state, false);
}

void SimpleHeadLiteral::rewriteArithmetics(Term::ArithmeticsMap &, AuxGen &) {
    // Note: nothing to do
}

void SimpleHeadLiteral::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    lit->collect(vars, false);
    lvl.add(vars);
}

Symbol SimpleHeadLiteral::isEDB() const { return lit->isEDB(); }

void SimpleHeadLiteral::check(ChkLvlVec &levels, Logger &) const {
    _add(levels, lit, false);
}

bool SimpleHeadLiteral::hasPool(bool beforeRewrite) const {
    return lit->hasPool(beforeRewrite);
}

void SimpleHeadLiteral::replace(Defines &x) {
    lit->replace(x);
}

void SimpleHeadLiteral::getNeg(std::function<void (Sig)> f) const {
    lit->getNeg(f);
}

CreateHead SimpleHeadLiteral::toGround(ToGroundArg &x, Ground::UStmVec &, Ground::RuleType type) const {
    return
        {[this, &x, type](Ground::ULitVec &&lits) -> Ground::UStm {
            Ground::Rule::HeadVec heads;
            if (UTerm headRepr = lit->headRepr()) {
                Sig sig(headRepr->getSig());
                heads.emplace_back(std::move(headRepr), &x.domains.add(sig));
            }
            return gringo_make_unique<Ground::Rule>(std::move(heads), std::move(lits), type);
        }};
}

// {{{1 definition of DisjointAggregate

CSPElem::CSPElem(Location const &loc, UTermVec &&tuple, CSPAddTerm &&value, ULitVec &&cond)
    : loc(loc)
    , tuple(std::move(tuple))
    , value(std::move(value))
    , cond(std::move(cond)) { }
CSPElem::CSPElem(CSPElem &&) = default;
CSPElem &CSPElem::operator=(CSPElem &&) = default;
CSPElem::~CSPElem() { }
void CSPElem::print(std::ostream &out) const {
    using namespace std::placeholders;
    print_comma(out, tuple, ",", std::bind(&Term::print, _2, _1));
    out << ":" << value;
    if (!cond.empty()) {
        out << ":";
        print_comma(out, cond, ",", std::bind(&Literal::print, _2, _1));
    }
}
size_t CSPElem::hash() const {
    return get_value_hash(tuple, value, cond);
}
CSPElem CSPElem::clone() const {
    return {loc, get_clone(tuple), get_clone(value), get_clone(cond)};
}
bool CSPElem::operator==(CSPElem const &x) const {
    return is_value_equal_to(tuple, x.tuple) && is_value_equal_to(value, x.value) && is_value_equal_to(cond, x.cond);
}

DisjointAggregate::DisjointAggregate(NAF naf, CSPElemVec &&elems)
    : naf(naf)
    , elems(std::move(elems)) { }

DisjointAggregate::~DisjointAggregate() { }

void DisjointAggregate::print(std::ostream &out) const {
    out << naf << "#disjoint{";
    print_comma(out, elems, ";");
    out << "}";
}

size_t DisjointAggregate::hash() const {
    return get_value_hash(typeid(DisjointAggregate).hash_code(), elems);
}

bool DisjointAggregate::operator==(BodyAggregate const &x) const {
    auto t = dynamic_cast<DisjointAggregate const *>(&x);
    return t && is_value_equal_to(elems, t->elems);
}

DisjointAggregate *DisjointAggregate::clone() const {
    return make_locatable<DisjointAggregate>(loc(), naf, get_clone(elems)).release();
}

void DisjointAggregate::unpool(UBodyAggrVec &x, bool beforeRewrite) {
    CSPElemVec e;
    for (auto &elem : elems) {
        auto f = [&](UTermVec &&y) { e.emplace_back(elem.loc, std::move(y), get_clone(elem.value), get_clone(elem.cond)); };
        Term::unpool(elem.tuple.begin(), elem.tuple.end(), Gringo::unpool, f);
    }
    elems.clear();
    for (auto &elem : e) {
        for (auto &y : elem.value.unpool()) {
            elems.emplace_back(elem.loc, get_clone(elem.tuple), std::move(y), get_clone(elem.cond));
        }
    }
    e.clear();
    for (auto &elem : elems) {
        if (beforeRewrite) {
            auto f = [&](ULitVec &&cond) { e.emplace_back(elem.loc, get_clone(elem.tuple), get_clone(elem.value), std::move(cond)); };
            Term::unpool(elem.cond.begin(), elem.cond.end(), _unpool_lit(beforeRewrite), f);
        }
        else {
            Term::unpoolJoin(elem.cond, _unpool_lit(beforeRewrite));
            e.emplace_back(std::move(elem));
        }
    }

    x.emplace_back(make_locatable<DisjointAggregate>(loc(), naf, std::move(e)));
}

void DisjointAggregate::collect(VarTermBoundVec &vars) const {
    for (auto &elem : elems) {
        for (auto &term : elem.tuple) { term->collect(vars, false); }
        elem.value.collect(vars);
        for (auto &lit : elem.cond) { lit->collect(vars, false); }
    }
}

bool DisjointAggregate::rewriteAggregates(UBodyAggrVec &) { return true; }

bool DisjointAggregate::simplify(Projections &project, SimplifyState &state, bool, Logger &log) {
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](CSPElemVec::value_type &elem) {
        SimplifyState elemState(state);
        for (auto &term : elem.tuple) {
            if (term->simplify(elemState, false, false, log).update(term).undefined()) { return true; }
        }
        if (!elem.value.simplify(elemState, log)) { return true; }
        for (auto &lit : elem.cond) {
            if (!lit->simplify(log, project, elemState)) { return true; }
        }
        for (auto &dot : elemState.dots) { elem.cond.emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { elem.cond.emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}

void DisjointAggregate::rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &, AuxGen &auxGen) {
    for (auto &elem : elems) {
        Literal::AssignVec assign;
        arith.emplace_back();
        for (auto &y : elem.cond) { y->rewriteArithmetics(arith, assign, auxGen); }
        for (auto &y : arith.back()) { elem.cond.emplace_back(RelationLiteral::make(y)); }
        for (auto &y : assign) { elem.cond.emplace_back(RelationLiteral::make(y)); }
        arith.pop_back();
    }
}

void DisjointAggregate::assignLevels(AssignLevel &lvl) {
    for (auto &elem : elems) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        for (auto &term : elem.tuple) { term->collect(vars, false); }
        elem.value.collect(vars);
        for (auto &lit : elem.cond) { lit->collect(vars, false); }
        local.add(vars);
    }
}

void DisjointAggregate::check(ChkLvlVec &levels, Logger &log) const {
    auto f = [&]() {
        for (auto &y : elems) {
            levels.emplace_back(loc(), *this);
            _add(levels, y.tuple, &y.value);
            _add(levels, y.cond);
            levels.back().check(log);
            levels.pop_back();
        }
    };
    _aggr(levels, {}, f, false);
}

bool DisjointAggregate::hasPool(bool beforeRewrite) const {
    for (auto &elem : elems) {
        for (auto &term : elem.tuple) { if (term->hasPool()) { return true; } }
        if (elem.value.hasPool()) { return true; }
        for (auto &lit : elem.cond) { if (lit->hasPool(beforeRewrite)) { return true; } }
    }
    return false;
}

void DisjointAggregate::replace(Defines &x) {
    for (auto &elem : elems) {
        for (auto &y : elem.tuple) { y->replace(x, true); }
        elem.value.replace(x);
        for (auto &y : elem.cond) { y->replace(x); }
    }
}

CreateBody DisjointAggregate::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    stms.emplace_back(gringo_make_unique<Ground::DisjointComplete>(x.domains, x.newId(*this)));
    auto &completeRef = static_cast<Ground::DisjointComplete&>(*stms.back());
    CreateStmVec split;
    split.emplace_back([&completeRef, this](Ground::ULitVec &&lits) -> Ground::UStm {
        auto ret = gringo_make_unique<Ground::DisjointAccumulate>(completeRef, std::move(lits));
        completeRef.addAccuDom(*ret);
        return std::move(ret);
    });
    for (auto &y : elems) {
        split.emplace_back([this,&completeRef,&y,&x](Ground::ULitVec &&lits) -> Ground::UStm {
            for (auto &z : y.cond) { lits.emplace_back(z->toGround(x.domains, false)); }
            auto ret = gringo_make_unique<Ground::DisjointAccumulate>(completeRef, get_clone(y.tuple), get_clone(y.value), std::move(lits));
            completeRef.addAccuDom(*ret);
            return std::move(ret);
        });
    }
    return CreateBody([&completeRef, this](Ground::ULitVec &lits, bool primary, bool auxiliary) {
        if (primary) { lits.emplace_back(gringo_make_unique<Ground::DisjointLiteral>(completeRef, naf, auxiliary)); }
    }, std::move(split));
}

bool DisjointAggregate::isAssignment() const  { return false; }

void DisjointAggregate::removeAssignment() { }

// {{{1 definition of MinimizeHeadLiteral

MinimizeHeadLiteral::MinimizeHeadLiteral(UTerm &&weight, UTerm &&priority, UTermVec &&tuple)
: tuple_(std::move(tuple)) {
    tuple_.emplace_back(std::move(weight));
    tuple_.emplace_back(std::move(priority));
    std::rotate(tuple_.begin(), tuple_.end() - 2, tuple_.end());
}

MinimizeHeadLiteral::MinimizeHeadLiteral(UTermVec &&tuple)
: tuple_(std::move(tuple)) {
    assert(tuple_.size() >= 2);
}

MinimizeHeadLiteral::~MinimizeHeadLiteral() { }

void MinimizeHeadLiteral::print(std::ostream &out) const {
    out << "[";
    weight().print(out);
    out << "@";
    priority().print(out);
    for (auto it = tuple_.begin() + 2, ie = tuple_.end(); it != ie; ++it) {
        out << ",";
        (*it)->print(out);
    }
    out << "]";
}

size_t MinimizeHeadLiteral::hash() const {
    return get_value_hash(typeid(MinimizeHeadLiteral).hash_code(), tuple_);
}

bool MinimizeHeadLiteral::operator==(HeadAggregate const &x) const {
    auto t = dynamic_cast<MinimizeHeadLiteral const *>(&x);
    return t && is_value_equal_to(tuple_, t->tuple_);
}

MinimizeHeadLiteral *MinimizeHeadLiteral::clone() const {
    return make_locatable<MinimizeHeadLiteral>(loc(), get_clone(tuple_)).release();
}

void MinimizeHeadLiteral::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    assert(beforeRewrite); (void)beforeRewrite;
    auto f = [&](UTermVec &&tuple) { x.emplace_back(make_locatable<MinimizeHeadLiteral>(loc(), std::move(tuple))); };
    Term::unpool(tuple_.begin(), tuple_.end(), Gringo::unpool, f);
}

void MinimizeHeadLiteral::collect(VarTermBoundVec &vars) const {
    for (auto &term : tuple_) {
        term->collect(vars, false);
    }
}

UHeadAggr MinimizeHeadLiteral::rewriteAggregates(UBodyAggrVec &) {
    return nullptr;
}

bool MinimizeHeadLiteral::simplify(Projections &, SimplifyState &state, Logger &log) {
    for (auto &term : tuple_) {
        if (term->simplify(state, false, false, log).update(term).undefined()) {
            return false;
        }
    }
    return true;
}

void MinimizeHeadLiteral::rewriteArithmetics(Term::ArithmeticsMap &, AuxGen &) {
    // Note: nothing to do
}

void MinimizeHeadLiteral::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    for (auto &term : tuple_) {
        term->collect(vars, false);
    }
    lvl.add(vars);
}

void MinimizeHeadLiteral::check(ChkLvlVec &levels, Logger &) const {
    levels.back().current = &levels.back().dep.insertEnt();
    VarTermBoundVec vars;
    collect(vars);
    addVars(levels, vars);
}

bool MinimizeHeadLiteral::hasPool(bool beforeRewrite) const {
    if (beforeRewrite) {
        for (auto &term : tuple_) {
            if (term->hasPool()) {
                return true;
            }
        }
    }
    return false;
}

void MinimizeHeadLiteral::replace(Defines &x) {
    for (auto &term : tuple_) {
        Term::replace(term, term->replace(x, true));
    }
}

CreateHead MinimizeHeadLiteral::toGround(ToGroundArg &, Ground::UStmVec &, Ground::RuleType) const {
    return CreateHead([&](Ground::ULitVec &&lits) {
        return gringo_make_unique<Ground::WeakConstraint>(get_clone(tuple_), std::move(lits));
    });
}

Term &MinimizeHeadLiteral::weight() const {
    return *tuple_[0];
}

Term &MinimizeHeadLiteral::priority() const {
    return *tuple_[1];
}

void MinimizeHeadLiteral::getNeg(std::function<void (Sig)>) const { }

// {{{1 definition of EdgeHeadAtom

EdgeHeadAtom::EdgeHeadAtom(UTerm &&u, UTerm &&v)
: u_(std::move(u))
, v_(std::move(v))
{ }

EdgeHeadAtom::~EdgeHeadAtom() { }

void EdgeHeadAtom::print(std::ostream &out) const {
    out << "#edge(" << *u_ << "," << *v_ << ")";
}

size_t EdgeHeadAtom::hash() const {
    return get_value_hash(typeid(EdgeHeadAtom).hash_code(), u_, v_);
}

bool EdgeHeadAtom::operator==(HeadAggregate const &x) const {
    auto t = dynamic_cast<EdgeHeadAtom const *>(&x);
    return t && is_value_equal_to(u_, t->u_) && is_value_equal_to(v_, t->v_);;
}

EdgeHeadAtom *EdgeHeadAtom::clone() const {
    return make_locatable<EdgeHeadAtom>(loc(), get_clone(u_), get_clone(v_)).release();
}

void EdgeHeadAtom::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    if (beforeRewrite) {
        for (auto &u : Gringo::unpool(u_)) {
            for (auto &v : Gringo::unpool(v_)) {
                x.emplace_back(make_locatable<EdgeHeadAtom>(loc(), get_clone(u), std::move(v)));
            }
        }
    }
    else {
        x.emplace_back(this->clone());
    }
}

void EdgeHeadAtom::collect(VarTermBoundVec &vars) const {
    u_->collect(vars, false);
    v_->collect(vars, false);
}

UHeadAggr EdgeHeadAtom::rewriteAggregates(UBodyAggrVec &) {
    return nullptr;
}

bool EdgeHeadAtom::simplify(Projections &, SimplifyState &state, Logger &log) {
    return !u_->simplify(state, false, false, log).update(u_).undefined() &&
           !v_->simplify(state, false, false, log).update(v_).undefined();
}

void EdgeHeadAtom::rewriteArithmetics(Term::ArithmeticsMap &, AuxGen &) {
    // Note: nothing to do
}

void EdgeHeadAtom::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    collect(vars);
    lvl.add(vars);
}

void EdgeHeadAtom::check(ChkLvlVec &levels, Logger &) const {
    levels.back().current = &levels.back().dep.insertEnt();
    VarTermBoundVec vars;
    collect(vars);
    addVars(levels, vars);
}

bool EdgeHeadAtom::hasPool(bool beforeRewrite) const {
    return beforeRewrite && (u_->hasPool() || v_->hasPool());
}

void EdgeHeadAtom::replace(Defines &x) {
    Term::replace(u_, u_->replace(x, true));
    Term::replace(v_, v_->replace(x, true));
}

CreateHead EdgeHeadAtom::toGround(ToGroundArg &, Ground::UStmVec &, Ground::RuleType) const {
    return CreateHead([&](Ground::ULitVec &&lits) {
        return gringo_make_unique<Ground::EdgeStatement>(get_clone(u_), get_clone(v_), std::move(lits));
    });
}

void EdgeHeadAtom::getNeg(std::function<void (Sig)>) const { }

// {{{1 definition of ProjectHeadAtom

ProjectHeadAtom::ProjectHeadAtom(UTerm &&atom)
: atom_(std::move(atom))
{ }

ProjectHeadAtom::~ProjectHeadAtom() { }

void ProjectHeadAtom::print(std::ostream &out) const {
    out << "#project " << *atom_;
}

size_t ProjectHeadAtom::hash() const {
    return get_value_hash(typeid(ProjectHeadAtom).hash_code(), atom_);
}

bool ProjectHeadAtom::operator==(HeadAggregate const &x) const {
    auto t = dynamic_cast<ProjectHeadAtom const *>(&x);
    return t && is_value_equal_to(atom_, t->atom_);
}

ProjectHeadAtom *ProjectHeadAtom::clone() const {
    return make_locatable<ProjectHeadAtom>(loc(), get_clone(atom_)).release();
}

void ProjectHeadAtom::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    if (beforeRewrite) {
        for (auto &atom : Gringo::unpool(atom_)) {
            x.emplace_back(make_locatable<ProjectHeadAtom>(loc(), std::move(atom)));
        }
    }
    else {
        x.emplace_back(clone());
    }
}

void ProjectHeadAtom::collect(VarTermBoundVec &vars) const {
    atom_->collect(vars, false);
}

UHeadAggr ProjectHeadAtom::rewriteAggregates(UBodyAggrVec &body) {
    body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(make_locatable<PredicateLiteral>(atom_->loc(), NAF::POS, get_clone(atom_), true)));
    return nullptr;
}

bool ProjectHeadAtom::simplify(Projections &, SimplifyState &state, Logger &log) {
    return !atom_->simplify(state, false, false, log).update(atom_).undefined();
}

void ProjectHeadAtom::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxgen) {
    atom_->rewriteArithmetics(arith, auxgen);
}

void ProjectHeadAtom::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    collect(vars);
    lvl.add(vars);
}

void ProjectHeadAtom::check(ChkLvlVec &levels, Logger &) const {
    levels.back().current = &levels.back().dep.insertEnt();
    VarTermBoundVec vars;
    collect(vars);
    addVars(levels, vars);
}

bool ProjectHeadAtom::hasPool(bool beforeRewrite) const {
    return beforeRewrite && atom_->hasPool();
}

void ProjectHeadAtom::replace(Defines &x) {
    atom_->replace(x, false);
}

CreateHead ProjectHeadAtom::toGround(ToGroundArg &, Ground::UStmVec &, Ground::RuleType) const {
    return CreateHead([&](Ground::ULitVec &&lits) {
        return gringo_make_unique<Ground::ProjectStatement>(get_clone(atom_), std::move(lits));
    });
}

void ProjectHeadAtom::getNeg(std::function<void (Sig)>) const { }

// {{{1 definition of HeuristicHeadAtom

HeuristicHeadAtom::HeuristicHeadAtom(UTerm &&atom, UTerm &&bias, UTerm &&priority, UTerm &&mod)
: atom_(std::move(atom))
, value_(std::move(bias))
, priority_(std::move(priority))
, mod_(std::move(mod))
{ }

HeuristicHeadAtom::~HeuristicHeadAtom() { }

void HeuristicHeadAtom::print(std::ostream &out) const {
    out << "#heuristic " << *atom_ << "[" << *value_ << "@" << *priority_ << "," << *mod_ << "]";
}

size_t HeuristicHeadAtom::hash() const {
    return get_value_hash(typeid(HeuristicHeadAtom).hash_code(), atom_, value_, priority_, mod_);
}

bool HeuristicHeadAtom::operator==(HeadAggregate const &x) const {
    auto t = dynamic_cast<HeuristicHeadAtom const *>(&x);
    return t &&
           is_value_equal_to(atom_, t->atom_) &&
           is_value_equal_to(value_, t->value_) &&
           is_value_equal_to(priority_, t->priority_) &&
           is_value_equal_to(mod_, t->mod_);
}

HeuristicHeadAtom *HeuristicHeadAtom::clone() const {
    return make_locatable<HeuristicHeadAtom>(loc(), get_clone(atom_), get_clone(value_), get_clone(priority_), get_clone(mod_)).release();
}

void HeuristicHeadAtom::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    if (beforeRewrite) {
        for (auto &atom : Gringo::unpool(atom_)) {
            for (auto &bias : Gringo::unpool(value_)) {
                for (auto &priority : Gringo::unpool(priority_)) {
                    for (auto &mod : Gringo::unpool(mod_)) {
                        x.emplace_back(make_locatable<HeuristicHeadAtom>(loc(), get_clone(atom), get_clone(bias), std::move(priority), get_clone(mod)));
                    }
                }
            }
        }
    }
    else {
        x.emplace_back(clone());
    }
}

void HeuristicHeadAtom::collect(VarTermBoundVec &vars) const {
    atom_->collect(vars, false);
    value_->collect(vars, false);
    priority_->collect(vars, false);
    mod_->collect(vars, false);
}

UHeadAggr HeuristicHeadAtom::rewriteAggregates(UBodyAggrVec &body) {
    body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(make_locatable<PredicateLiteral>(atom_->loc(), NAF::POS, get_clone(atom_), true)));
    return nullptr;
}

bool HeuristicHeadAtom::simplify(Projections &, SimplifyState &state, Logger &log) {
    return
        !atom_->simplify(state, false, false, log).update(atom_).undefined() &&
        !value_->simplify(state, false, false, log).update(value_).undefined() &&
        !priority_->simplify(state, false, false, log).update(priority_).undefined() &&
        !mod_->simplify(state, false, false, log).update(mod_).undefined();
}

void HeuristicHeadAtom::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxgen) {
    atom_->rewriteArithmetics(arith, auxgen);
}

void HeuristicHeadAtom::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    collect(vars);
    lvl.add(vars);
}

void HeuristicHeadAtom::check(ChkLvlVec &levels, Logger &) const {
    levels.back().current = &levels.back().dep.insertEnt();
    VarTermBoundVec vars;
    collect(vars);
    addVars(levels, vars);
}

bool HeuristicHeadAtom::hasPool(bool beforeRewrite) const {
    return beforeRewrite && (atom_->hasPool() || value_->hasPool() || priority_->hasPool());
}

void HeuristicHeadAtom::replace(Defines &x) {
    Term::replace(atom_, atom_->replace(x, false));
    Term::replace(value_, value_->replace(x, true));
    Term::replace(priority_, priority_->replace(x, true));
    Term::replace(mod_, mod_->replace(x, true));
}

CreateHead HeuristicHeadAtom::toGround(ToGroundArg &, Ground::UStmVec &, Ground::RuleType) const {
    return CreateHead([&](Ground::ULitVec &&lits) {
        return gringo_make_unique<Ground::HeuristicStatement>(get_clone(atom_), get_clone(value_), get_clone(priority_), get_clone(mod_), std::move(lits));
    });
}

void HeuristicHeadAtom::getNeg(std::function<void (Sig)>) const { }

// {{{1 definition of ShowHeadLiteral

ShowHeadLiteral::ShowHeadLiteral(UTerm &&term, bool csp)
: term_(std::move(term))
, csp_(csp)
{ }

ShowHeadLiteral::~ShowHeadLiteral() { }

void ShowHeadLiteral::print(std::ostream &out) const {
    out << "#show " << (csp_ ? "$" : "") << *term_;
}

size_t ShowHeadLiteral::hash() const {
    return get_value_hash(typeid(ShowHeadLiteral).hash_code(), term_, csp_);
}

bool ShowHeadLiteral::operator==(HeadAggregate const &x) const {
    auto t = dynamic_cast<ShowHeadLiteral const *>(&x);
    return t && is_value_equal_to(term_, t->term_) && is_value_equal_to(csp_, t->csp_);;
}

ShowHeadLiteral *ShowHeadLiteral::clone() const {
    return make_locatable<ShowHeadLiteral>(loc(), get_clone(term_), csp_).release();
}

void ShowHeadLiteral::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    if (beforeRewrite) {
        for (auto &term : Gringo::unpool(term_)) {
            x.emplace_back(make_locatable<ShowHeadLiteral>(loc(), std::move(term), csp_));
        }
    }
}

void ShowHeadLiteral::collect(VarTermBoundVec &vars) const {
    term_->collect(vars, false);
}

UHeadAggr ShowHeadLiteral::rewriteAggregates(UBodyAggrVec &) {
    return nullptr;
}

bool ShowHeadLiteral::simplify(Projections &, SimplifyState &state, Logger &log) {
    return !term_->simplify(state, false, false, log).update(term_).undefined();
}

void ShowHeadLiteral::rewriteArithmetics(Term::ArithmeticsMap &, AuxGen &) {
    // Note: nothing to do
}

void ShowHeadLiteral::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    collect(vars);
    lvl.add(vars);
}

void ShowHeadLiteral::check(ChkLvlVec &levels, Logger &) const {
    levels.back().current = &levels.back().dep.insertEnt();
    VarTermBoundVec vars;
    collect(vars);
    addVars(levels, vars);
}

bool ShowHeadLiteral::hasPool(bool beforeRewrite) const {
    return beforeRewrite && term_->hasPool();
}

void ShowHeadLiteral::replace(Defines &x) {
    Term::replace(term_, term_->replace(x, true));
}

CreateHead ShowHeadLiteral::toGround(ToGroundArg &, Ground::UStmVec &, Ground::RuleType) const {
    return CreateHead([&](Ground::ULitVec &&lits) {
        return gringo_make_unique<Ground::ShowStatement>(get_clone(term_), csp_, std::move(lits));
    });
}

void ShowHeadLiteral::getNeg(std::function<void (Sig)>) const { }

// }}}1

} } // namespace Input Gringo

