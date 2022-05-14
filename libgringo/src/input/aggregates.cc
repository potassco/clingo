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
void printAggr_(std::ostream &out, AggregateFunction fun, T const &x, U const &y, V const &f) {
    auto it = std::begin(x);
    auto ie = std::end(x);
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

void printCond_(std::ostream &out, CondLit const &x) {
    using namespace std::placeholders;
    x.first->print(out);
    out << ":";
    print_comma(out, x.second, ",", [](auto && PH1, auto && PH2) { PH2->print(PH1); });
};

std::function<ULitVec(ULit const &)> _unpool_lit(bool beforeRewrite, bool head) {
    return [beforeRewrite, head](ULit const &x) {
        return x->unpool(beforeRewrite, head);
    };
}

BoundVec _unpool_bound(Bound &x) {
    return x.unpool();
};

void _add(ChkLvlVec &levels, ULit const &lit, bool bind) {
    VarTermBoundVec vars;
    levels.back().current = &levels.back().dep.insertEnt();
    lit->collect(vars, bind);
    addVars(levels, vars);
}

void _add(ChkLvlVec &levels, UTermVec const &terms) {
    VarTermBoundVec vars;
    levels.back().current = &levels.back().dep.insertEnt();
    for (auto const &x : terms)  {
        x->collect(vars, false);
    }
    addVars(levels, vars);
}

void _add(ChkLvlVec &levels, ULitVec const &cond) {
    for (auto const &x : cond) {
        _add(levels, x, true);
    }
}

template <class T>
void _aggr(ChkLvlVec &levels, BoundVec const &bounds, T const &f, bool bind) {
    bool assign = false;
    CheckLevel::SC::EntNode *depend = nullptr;
    for (auto const &y : bounds) {
        if (bind && y.rel == Relation::EQ) {
            levels.back().current = &levels.back().dep.insertEnt();
            VarTermBoundVec vars;
            y.bound->collect(vars, true);
            addVars(levels, vars);
            f();
            assign = true;
        }
        else {
            if (depend == nullptr) {
                depend = &levels.back().dep.insertEnt();
            }
            levels.back().current = depend;
            VarTermBoundVec vars;
            y.bound->collect(vars, false);
            addVars(levels, vars);
        }
    }
    if (depend == nullptr && !assign) {
        depend = &levels.back().dep.insertEnt();
    }
    if (depend) {
        levels.back().current = depend;
        f();
    }
}

void warnGlobal(VarTermBoundVec &vars, bool warn, Logger &log) {
    if (warn) {
        auto ib = vars.begin();
        auto ie = vars.end();
        ie = std::remove_if(ib, ie, [](auto const &a) { return a.first->level > 0; });
        std::sort(ib, ie, [](auto const &a, auto const &b) { return a.first->name < b.first->name; });
        ie = std::unique(ib, ie, [](auto const &a, auto const &b) { return a.first->name == b.first->name; });
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
: naf_(naf)
, removedAssignment_(removedAssignment)
, translated_(translated)
, fun_(fun)
, bounds_(std::move(bounds))
, elems_(std::move(elems)) { }

TupleBodyAggregate::~TupleBodyAggregate() noexcept = default;

void TupleBodyAggregate::print(std::ostream &out) const {
    auto f = [](std::ostream &out, BodyAggrElem const &y) {
        using namespace std::placeholders;
        print_comma(out, y.first, ",", [](auto && PH1, auto && PH2) { PH2->print(PH1); });
        out << ":";
        print_comma(out, y.second, ",", [](auto && PH1, auto && PH2) { PH2->print(PH1); });
    };
    out << naf_;
    printAggr_(out, fun_, bounds_, elems_, f);
}

size_t TupleBodyAggregate::hash() const {
    return get_value_hash(typeid(TupleBodyAggregate).hash_code(), size_t(naf_), size_t(fun_), bounds_, elems_);
}

bool TupleBodyAggregate::operator==(BodyAggregate const &other) const {
    const auto *t = dynamic_cast<TupleBodyAggregate const *>(&other);
    return t != nullptr &&
           naf_ == t->naf_ &&
           fun_ == t->fun_ &&
           is_value_equal_to(bounds_, t->bounds_) &&
           is_value_equal_to(elems_, t->elems_);
}

TupleBodyAggregate *TupleBodyAggregate::clone() const {
    return make_locatable<TupleBodyAggregate>(loc(), naf_, removedAssignment_, translated_, fun_, get_clone(bounds_), get_clone(elems_)).release();
}

bool TupleBodyAggregate::hasPool(bool beforeRewrite) const {
    for (auto const &bound : bounds_) {
        if (bound.bound->hasPool()) {
            return true;
        }
    }
    for (auto const &elem : elems_) {
        for (auto const &term : elem.first) {
            if (term->hasPool()) {
                return true;
            }
        }
        for (auto const &lit : elem.second) {
            if (lit->hasPool(beforeRewrite, false)) {
                return true;
            }
        }
    }
    return false;
}

void TupleBodyAggregate::unpool(UBodyAggrVec &x, bool beforeRewrite) {
    BodyAggrElemVec e;
    for (auto &elem : elems_) {
        auto f = [&](UTermVec &&x) {
            e.emplace_back(std::move(x), get_clone(elem.second));
        };
        Term::unpool(elem.first.begin(), elem.first.end(), Gringo::unpool, f);
    }
    elems_ = std::move(e);
    e.clear();
    for (auto &elem : elems_) {
        if (beforeRewrite) {
            auto f = [&](ULitVec &&y) { e.emplace_back(get_clone(elem.first), std::move(y)); };
            Term::unpool(elem.second.begin(), elem.second.end(), _unpool_lit(beforeRewrite, false), f);
        }
        else {
            Term::unpoolJoin(elem.second, _unpool_lit(beforeRewrite, false));
            e.emplace_back(std::move(elem));
        }
    }
    auto f = [&](BoundVec &&y) { x.emplace_back(make_locatable<TupleBodyAggregate>(loc(), naf_, removedAssignment_, translated_, fun_, std::move(y), get_clone(e))); };
    Term::unpool(bounds_.begin(), bounds_.end(), _unpool_bound, f);
}

bool TupleBodyAggregate::hasUnpoolComparison() const {
    for (auto const &elem : elems_) {
        for (auto const &lit : elem.second) {
            if (lit->hasUnpoolComparison()) {
                return true;
            }
        }
    }
    return false;
}

UBodyAggrVecVec TupleBodyAggregate::unpoolComparison() const {
    BodyAggrElemVec elems;
    for (auto const &elem : elems_) {
        for (auto &cond : unpoolComparison_(elem.second)) {
            elems.emplace_back(get_clone(elem.first), std::move(cond));
        }
    }
    UBodyAggrVecVec ret;
    ret.emplace_back();
    ret.back().emplace_back(make_locatable<TupleBodyAggregate>(loc(),
                                                               naf_,
                                                               removedAssignment_,
                                                               translated_,
                                                               fun_,
                                                               get_clone(bounds_),
                                                               std::move(elems)));
    return ret;
}

void TupleBodyAggregate::collect(VarTermBoundVec &vars) const {
    for (auto const &bound : bounds_) {
        bound.bound->collect(vars, bound.rel == Relation::EQ && naf_ == NAF::POS);
    }
    for (auto const &elem : elems_) {
        for (auto const &term : std::get<0>(elem)) {
            term->collect(vars, false);
        }
        for (auto const &lit : std::get<1>(elem)) {
            lit->collect(vars, false);
        }
    }
}

bool TupleBodyAggregate::rewriteAggregates(UBodyAggrVec &aggr) {
    BoundVec assign;
    auto jt(bounds_.begin());
    for (auto it = jt, ie = bounds_.end(); it != ie; ++it) {
        if (it->rel == Relation::EQ && naf_ == NAF::POS) {
            assign.emplace_back(std::move(*it));
        }
        else {
            if (it != jt) {
                *jt = std::move(*it);
            }
            ++jt;
        }
    }
    bounds_.erase(jt, bounds_.end());
    bool skip = bounds_.empty() && !assign.empty();
    for (auto it = assign.begin(), ie = assign.end() - (skip ? 1 : 0); it != ie; ++it) {
        BoundVec bound;
        bound.emplace_back(std::move(*it));
        aggr.emplace_back(make_locatable<TupleBodyAggregate>(loc(), naf_, removedAssignment_, translated_, fun_, std::move(bound), get_clone(elems_)));
    }
    if (skip) {
        bounds_.emplace_back(std::move(assign.back()));
    }
    if (bounds_.empty() && naf_ == NAF::NOT) {
        aggr.emplace_back(gringo_make_unique<SimpleBodyLiteral>(make_locatable<RelationLiteral>(
            loc(), Relation::NEQ,
            make_locatable<ValTerm>(loc(), Symbol::createNum(0)),
            make_locatable<ValTerm>(loc(), Symbol::createNum(0)))));
    }
    return !bounds_.empty();
}

bool TupleBodyAggregate::simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) {
    static_cast<void>(singleton);
    for (auto &bound : bounds_) {
        if (!bound.simplify(state, log)) {
            return false;
        }
    }
    elems_.erase(std::remove_if(elems_.begin(), elems_.end(), [&](BodyAggrElemVec::value_type &elem) {
        auto elemState = SimplifyState::make_substate(state);
        for (auto &term : std::get<0>(elem)) {
            if (term->simplify(elemState, false, false, log).update(term, false).undefined()) {
                return true;
            }
        }
        for (auto &lit : std::get<1>(elem)) {
            // NOTE: projection disabled with singelton=true
            if (!lit->simplify(log, project, elemState, true, true)) {
                return true;
            }
        }
        for (auto &dot : elemState.dots()) {
            std::get<1>(elem).emplace_back(RangeLiteral::make(dot));
        }
        for (auto &script : elemState.scripts()) {
            std::get<1>(elem).emplace_back(ScriptLiteral::make(script));
        }
        return false;
    }), elems_.end());
    return true;
}

void TupleBodyAggregate::rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::RelationVec &assign, AuxGen &auxGen) {
    static_cast<void>(assign);
    for (auto &bound : bounds_) {
        bound.rewriteArithmetics(arith, auxGen);
    }
    for (auto &elem : elems_) {
        Literal::RelationVec assign;
        arith.emplace_back(gringo_make_unique<Term::LevelMap>());
        for (auto &y : std::get<1>(elem)) {
            y->rewriteArithmetics(arith, assign, auxGen);
        }
        for (auto &y : *arith.back()) {
            std::get<1>(elem).emplace_back(RelationLiteral::make(y));
        }
        for (auto &y : assign) {
            std::get<1>(elem).emplace_back(RelationLiteral::make(y));
        }
        arith.pop_back();
    }
}

void TupleBodyAggregate::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    for (auto &bound : bounds_) {
        bound.bound->collect(vars, false);
    }
    lvl.add(vars);
    for (auto &elem : elems_) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        for (auto &term : std::get<0>(elem)) {
            term->collect(vars, false);
        }
        for (auto &lit : std::get<1>(elem)) {
            lit->collect(vars, false);
        }
        local.add(vars);
    }
}

void TupleBodyAggregate::check(ChkLvlVec &levels, Logger &log) const {
    auto f = [&]() {
        VarTermBoundVec vars;
        for (auto const &y : elems_) {
            levels.emplace_back(loc(), *this);
            _add(levels, y.first);
            _add(levels, y.second);
            levels.back().check(log);
            levels.pop_back();
            for (auto const &term : y.first) {
                term->collect(vars, false);
            }
        }
        warnGlobal(vars, !translated_, log);
    };
    return _aggr(levels, bounds_, f, naf_ == NAF::POS);
}

void TupleBodyAggregate::replace(Defines &x) {
    for (auto &bound : bounds_) {
        Term::replace(bound.bound, bound.bound->replace(x, true));
    }
    for (auto &elem : elems_) {
        for (auto &y : elem.first) {
            Term::replace(y, y->replace(x, true));
        }
        for (auto &y : elem.second) {
            y->replace(x);
        }
    }
}

CreateBody TupleBodyAggregate::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    if (!isAssignment()) {
        stms.emplace_back(gringo_make_unique<Ground::BodyAggregateComplete>(x.domains, x.newId(*this), fun_, get_clone(bounds_)));
        auto &completeRef = static_cast<Ground::BodyAggregateComplete&>(*stms.back()); // NOLINT
        CreateStmVec split;
        split.emplace_back([&completeRef, this](Ground::ULitVec &&lits) -> Ground::UStm {
            UTermVec tuple;
            tuple.emplace_back(make_locatable<ValTerm>(loc(), Symbol()));
            // Note: should this become a function?
            UTerm neutral;
            switch (fun_) {
                case AggregateFunction::MIN: { neutral = make_locatable<ValTerm>(loc(), Symbol::createSup());  break; }
                case AggregateFunction::MAX: { neutral = make_locatable<ValTerm>(loc(), Symbol::createInf());  break; }
                default:                     { neutral = make_locatable<ValTerm>(loc(), Symbol::createNum(0)); break; }
            }
            for (auto const &y : bounds_) {
                lits.emplace_back(gringo_make_unique<Ground::RelationLiteral>(y.rel, get_clone(neutral), get_clone(y.bound)));
            }
            auto ret = gringo_make_unique<Ground::BodyAggregateAccumulate>(completeRef, get_clone(tuple), std::move(lits));
            completeRef.addAccuDom(*ret);
            return std::move(ret);
        });
        for (auto const &y : elems_) {
            split.emplace_back([&completeRef,&y,&x](Ground::ULitVec &&lits) -> Ground::UStm {
                for (auto const &z : y.second) {
                    lits.emplace_back(z->toGround(x.domains, false));
                }
                auto ret = gringo_make_unique<Ground::BodyAggregateAccumulate>(completeRef, get_clone(y.first), std::move(lits));
                completeRef.addAccuDom(*ret);
                return std::move(ret);
            });
        }
        return CreateBody([&completeRef, this](Ground::ULitVec &lits, bool primary, bool auxiliary) {
            if (primary) {
                lits.emplace_back(gringo_make_unique<Ground::BodyAggregateLiteral>(completeRef, naf_, auxiliary));
            }
        }, std::move(split));
    }
    assert(bounds_.size() == 1 && naf_ == NAF::POS);
    VarTermBoundVec vars;
    for (auto const &y : elems_) {
        for (auto const &z : y.first) {
            z->collect(vars, false);
        }
        for (auto const &z : y.second) {
            z->collect(vars, false);
        }
    }
    UTermVec global(x.getGlobal(vars));
    global.emplace_back(get_clone(bounds_.front().bound));
    UTermVec globalSpecial(x.getGlobal(vars));
    UTerm repr(x.newId(std::move(global), loc(), false));
    UTerm dataRepr(x.newId(std::move(globalSpecial), loc()));

    stms.emplace_back(gringo_make_unique<Ground::AssignmentAggregateComplete>(x.domains, get_clone(repr), get_clone(dataRepr), fun_));
    auto &completeRef = static_cast<Ground::AssignmentAggregateComplete&>(*stms.back()); // NOLINT
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
    for (auto const &y : elems_) {
        split.emplace_back([&completeRef,&y,&x](Ground::ULitVec &&lits) -> Ground::UStm {
            for (auto const &z : y.second) {
                lits.emplace_back(z->toGround(x.domains, false));
            }
            auto ret = gringo_make_unique<Ground::AssignmentAggregateAccumulate>(completeRef, get_clone(y.first), std::move(lits));
            completeRef.addAccuDom(*ret);
            return std::move(ret);
        });
    }
    return CreateBody([&completeRef](Ground::ULitVec &lits, bool primary, bool auxiliary) {
        if (primary) {
            lits.emplace_back(gringo_make_unique<Ground::AssignmentAggregateLiteral>(completeRef, auxiliary));
        }
    }, std::move(split));
}

bool TupleBodyAggregate::isAssignment() const {
    return !removedAssignment_ &&
           bounds_.size() == 1 &&
           naf_ == NAF::POS && bounds_.front().rel == Relation::EQ &&
           bounds_.front().bound->getInvertibility() == Term::INVERTIBLE;
}

void TupleBodyAggregate::removeAssignment() {
    assert (isAssignment());
    removedAssignment_ = true;
}

// {{{1 definition of LitBodyAggregate

LitBodyAggregate::LitBodyAggregate(NAF naf, AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems)
: naf_(naf)
, fun_(fun)
, bounds_(std::move(bounds))
, elems_(std::move(elems)) { }

LitBodyAggregate::~LitBodyAggregate() noexcept = default;

void LitBodyAggregate::print(std::ostream &out) const {
    out << naf_;
    printAggr_(out, fun_, bounds_, elems_, printCond_);
}

size_t LitBodyAggregate::hash() const {
    return get_value_hash(typeid(LitBodyAggregate).hash_code(), size_t(naf_), size_t(fun_), bounds_, elems_);
}

bool LitBodyAggregate::operator==(BodyAggregate const &other) const {
    const auto *t = dynamic_cast<LitBodyAggregate const *>(&other);
    return t != nullptr &&
           naf_ == t->naf_ &&
           fun_ == t->fun_ &&
           is_value_equal_to(bounds_, t->bounds_) &&
           is_value_equal_to(elems_, t->elems_);
}

LitBodyAggregate *LitBodyAggregate::clone() const {
    return make_locatable<LitBodyAggregate>(loc(), naf_, fun_, get_clone(bounds_), get_clone(elems_)).release();
}

void LitBodyAggregate::unpool(UBodyAggrVec &x, bool beforeRewrite) {
    CondLitVec e;
    for (auto &elem : elems_) {
        auto f = [&](ULit &&y) { e.emplace_back(std::move(y), get_clone(elem.second)); };
        Term::unpool(elem.first, _unpool_lit(beforeRewrite, true), f);
    }
    elems_ = std::move(e);
    e.clear();
    for (auto &elem : elems_) {
        if (beforeRewrite) {
            auto f = [&](ULitVec &&y) { e.emplace_back(get_clone(elem.first), std::move(y)); };
            Term::unpool(elem.second.begin(), elem.second.end(), _unpool_lit(beforeRewrite, false), f);
        }
        else {
            Term::unpoolJoin(elem.second, _unpool_lit(beforeRewrite, false));
            e.emplace_back(std::move(elem));
        }
    }
    auto f = [&](BoundVec &&y) { x.emplace_back(make_locatable<LitBodyAggregate>(loc(), naf_, fun_, std::move(y), get_clone(e))); };
    Term::unpool(bounds_.begin(), bounds_.end(), _unpool_bound, f);
}

bool LitBodyAggregate::hasUnpoolComparison() const {
    return true;
}

UBodyAggrVecVec LitBodyAggregate::unpoolComparison() const {
    int id = 0;
    BodyAggrElemVec elems;
    for (auto const &elem : elems_) {
        UTermVec tuple;
        elem.first->toTuple(tuple, id);
        ULitVec lits(get_clone(elem.second));
        if (!elem.first->triviallyTrue()) {
            lits.emplace_back(get_clone(elem.first));
        }
        elems.emplace_back(std::move(tuple), std::move(lits));
    }
    UBodyAggr ret(make_locatable<TupleBodyAggregate>(loc(), naf_, false, true, fun_, get_clone(bounds_), std::move(elems)));
    return ret->unpoolComparison();
}

void LitBodyAggregate::collect(VarTermBoundVec &vars) const {
    for (auto const &bound : bounds_) {
        bound.bound->collect(vars, bound.rel == Relation::EQ && naf_ == NAF::POS);
    }
    for (auto const &elem : elems_) {
        elem.first->collect(vars, false);
        for (auto const &lit : elem.second) {
            lit->collect(vars, false);
        }
    }
}

bool LitBodyAggregate::rewriteAggregates(UBodyAggrVec &aggr) {
    static_cast<void>(aggr);
    throw  std::runtime_error("LitHeadAggregate::rewriteAggregate: must not be called");
}

bool LitBodyAggregate::simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) {
    static_cast<void>(singleton);
    for (auto &bound : bounds_) {
        if (!bound.simplify(state, log)) {
            return false;
        }
    }
    elems_.erase(std::remove_if(elems_.begin(), elems_.end(), [&](CondLitVec::value_type &elem) {
        auto elemState = SimplifyState::make_substate(state);
        if (!std::get<0>(elem)->simplify(log, project, elemState, true, true)) {
            return true;
        }
        for (auto &lit : std::get<1>(elem)) {
            // NOTE: projection disabled with singelton=true
            if (!lit->simplify(log, project, elemState, true, true)) {
                return true;
            }
        }
        for (auto &dot : elemState.dots()) {
            std::get<1>(elem).emplace_back(RangeLiteral::make(dot));
        }
        for (auto &script : elemState.scripts()) {
            std::get<1>(elem).emplace_back(ScriptLiteral::make(script));
        }
        return false;
    }), elems_.end());
    return true;
}

void LitBodyAggregate::rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::RelationVec &assign, AuxGen &auxGen) {
    static_cast<void>(assign);
    for (auto &bound : bounds_) {
        bound.rewriteArithmetics(arith, auxGen);
    }
    for (auto &elem : elems_) {
        Literal::RelationVec assign;
        arith.emplace_back(gringo_make_unique<Term::LevelMap>());
        for (auto &y : std::get<1>(elem)) {
            y->rewriteArithmetics(arith, assign, auxGen);
        }
        for (auto &y : *arith.back()) {
            std::get<1>(elem).emplace_back(RelationLiteral::make(y));
        }
        for (auto &y : assign) {
            std::get<1>(elem).emplace_back(RelationLiteral::make(y));
        }
        arith.pop_back();
    }
}

void LitBodyAggregate::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    for (auto &bound : bounds_) {
        bound.bound->collect(vars, false);
    }
    lvl.add(vars);
    for (auto &elem : elems_) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        std::get<0>(elem)->collect(vars, false);
        for (auto &lit : std::get<1>(elem)) {
            lit->collect(vars, false);
        }
        local.add(vars);
    }
}

void LitBodyAggregate::check(ChkLvlVec &levels, Logger &log) const {
    auto f = [&]() {
        for (auto const &y : elems_) {
            levels.emplace_back(loc(), *this);
            _add(levels, y.first, false);
            _add(levels, y.second);
            levels.back().check(log);
            levels.pop_back();
        }
    };
    _aggr(levels, bounds_, f, naf_ == NAF::POS);
}

bool LitBodyAggregate::hasPool(bool beforeRewrite) const {
    for (auto const &bound : bounds_) {
        if (bound.bound->hasPool()) {
            return true;
        }
    }
    for (auto const &elem : elems_) {
        if (elem.first->hasPool(beforeRewrite, false)) {
            return true;
        }
        for (auto const &lit : elem.second) {
            if (lit->hasPool(beforeRewrite, false)) {
                return true;
            }
        }
    }
    return false;
}

void LitBodyAggregate::replace(Defines &x) {
    for (auto &bound : bounds_) {
        Term::replace(bound.bound, bound.bound->replace(x, true));
    }
    for (auto &elem : elems_) {
        elem.first->replace(x);
        for (auto &y : elem.second) {
            y->replace(x);
        }
    }
}

CreateBody LitBodyAggregate::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    static_cast<void>(x);
    static_cast<void>(stms);
    throw std::logic_error("Aggregate::rewriteAggregates must be called before LitAggregate::toGround");
}

bool LitBodyAggregate::isAssignment() const {
    return false;
}

void LitBodyAggregate::removeAssignment()  { }

// {{{1 definition of Conjunction

Conjunction::Conjunction(ULit &&head, ULitVec &&cond) {
    elems_.emplace_back(ULitVecVec(), std::move(cond));
    elems_.back().first.emplace_back();
    elems_.back().first.back().emplace_back(std::move(head));
}

Conjunction::Conjunction(ElemVec &&elems)
: elems_(std::move(elems)) { }

Conjunction::~Conjunction() noexcept = default;

void Conjunction::print(std::ostream &out) const {
    auto f = [](std::ostream &out, Elem const &y) {
        using namespace std::placeholders;
        print_comma(out, y.first, "|", [](std::ostream &out, ULitVec const &z) {
            print_comma(out, z, "&", [](auto && PH1, auto && PH2) { PH2->print(PH1); });
        });
        out << ":";
        print_comma(out, y.second, ",", [](auto && PH1, auto && PH2) { PH2->print(PH1); });
    };
    print_comma(out, elems_, ";", f);
}

size_t Conjunction::hash() const {
    return get_value_hash(typeid(Conjunction).hash_code(), elems_);
}

bool Conjunction::operator==(BodyAggregate const &other) const {
    const auto *t = dynamic_cast<Conjunction const *>(&other);
    return t != nullptr &&
           is_value_equal_to(elems_, t->elems_);
}

Conjunction *Conjunction::clone() const {
    return make_locatable<Conjunction>(loc(), get_clone(elems_)).release();
}

bool Conjunction::hasPool(bool beforeRewrite) const {
    for (auto const &elem : elems_) {
        for (auto const &disj : elem.first) {
            for (auto const &lit : disj) {
                if (lit->hasPool(beforeRewrite, false)) {
                    return true;
                }
            }
        }
        for (auto const &lit : elem.second) {
            if (lit->hasPool(beforeRewrite, false)) {
                return true;
            }
        }
    }
    return false;
}

void Conjunction::unpool(UBodyAggrVec &x, bool beforeRewrite) {
    ElemVec e;
    for (auto &elem : elems_) {
        if (beforeRewrite) {
            ULitVecVec heads;
            for (auto &head : elem.first) {
                auto g = [&](ULitVec &&z) { heads.emplace_back(std::move(z)); };
                Term::unpool(head.begin(), head.end(), _unpool_lit(beforeRewrite, false), g);
            }
            elem.first = std::move(heads);
            auto f = [&](ULitVec &&y) { e.emplace_back(get_clone(elem.first), std::move(y)); };
            Term::unpool(elem.second.begin(), elem.second.end(), _unpool_lit(beforeRewrite, false), f);
        }
        else {
            for (auto &head : elem.first) {
                Term::unpoolJoin(head, _unpool_lit(beforeRewrite, false));
            }
            Term::unpoolJoin(elem.second, _unpool_lit(beforeRewrite, false));
            e.emplace_back(std::move(elem));
        }
    }
    x.emplace_back(make_locatable<Conjunction>(loc(), std::move(e)));
}

bool Conjunction::hasUnpoolComparison() const {
    for (auto const &elem : elems_) {
        for (auto const &disj : elem.first) {
            for (auto const &lit : disj) {
                if (lit->hasUnpoolComparison()) {
                    return true;
                }
            }
        }
        for (auto const &lit : elem.second) {
            if (lit->hasUnpoolComparison()) {
                return true;
            }
        }
    }
    return false;
}

UBodyAggrVecVec Conjunction::unpoolComparison() const {
    // a conjunction has form:
    //   Condiditon -> Clause & ... & Clause
    // clauses have form:
    //   Lit | ... | Lit
    UBodyAggrVecVec ret;
    ret.emplace_back();
    for (auto const &elem : elems_) {
        ULitVecVec heads;
        for (auto &head : elem.first) {
            auto unpooled = unpoolComparison_(head);
            // TODO: is this correct???
            std::move(unpooled.begin(), unpooled.end(), std::back_inserter(heads));
        }
        for (auto &cond : unpoolComparison_(elem.second)) {
            ElemVec elems;
            elems.emplace_back(get_clone(heads), std::move(cond));
            ret.back().emplace_back(make_locatable<Conjunction>(loc(), std::move(elems)));
        }
    }
    return ret;
}

void Conjunction::collect(VarTermBoundVec &vars) const {
    for (auto const &elem : elems_) {
        for (auto const &disj : elem.first) {
            for (auto const &lit : disj) {
                lit->collect(vars, false);
            }
        }
        for (auto const &lit : elem.second) {
            lit->collect(vars, false);
        }
    }
}

bool Conjunction::rewriteAggregates(UBodyAggrVec &aggr) {
    while (elems_.size() > 1) {
        ElemVec vec;
        vec.emplace_back(std::move(elems_.back()));
        aggr.emplace_back(make_locatable<Conjunction>(loc(), std::move(vec)));
        elems_.pop_back();
    }
    return !elems_.empty();
}

bool Conjunction::simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) {
    static_cast<void>(singleton);
    for (auto &elem : elems_) {
        elem.first.erase(std::remove_if(elem.first.begin(), elem.first.end(), [&](ULitVec &clause) {
            auto elemState = SimplifyState::make_substate(state);
            for (auto &lit : clause) {
                if (!lit->simplify(log, project, elemState)) {
                    return true;
                }
            }
            for (auto &dot : elemState.dots()) {
                clause.emplace_back(RangeLiteral::make(dot));
            }
            for (auto &script : elemState.scripts()) {
                clause.emplace_back(ScriptLiteral::make(script));
            }
            return false;
        }), elem.first.end());
    }
    elems_.erase(std::remove_if(elems_.begin(), elems_.end(), [&](ElemVec::value_type &elem) {
        auto elemState = SimplifyState::make_substate(state);
        for (auto &lit : elem.second) {
            // NOTE: projection disabled with singelton=true
            if (!lit->simplify(log, project, elemState, true, true)) {
                return true;
            }
        }
        for (auto &dot : elemState.dots()) {
            elem.second.emplace_back(RangeLiteral::make(dot));
        }
        for (auto &script : elemState.scripts()) {
            elem.second.emplace_back(ScriptLiteral::make(script));
        }
        return false;
    }), elems_.end());
    return true;
}

void Conjunction::rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::RelationVec &assign, AuxGen &auxGen) {
    static_cast<void>(assign);
    for (auto &elem : elems_) {
        for (auto &y : elem.first) {
            Literal::RelationVec assign;
            arith.emplace_back(gringo_make_unique<Term::LevelMap>());
            for (auto &z : y) {
                z->rewriteArithmetics(arith, assign, auxGen);
            }
            for (auto &z : *arith.back()) {
                y.emplace_back(RelationLiteral::make(z));
            }
            for (auto &z : assign) {
                y.emplace_back(RelationLiteral::make(z));
            }
            arith.pop_back();
        }
        Literal::RelationVec assign;
        arith.emplace_back(gringo_make_unique<Term::LevelMap>());
        for (auto &y : elem.second) {
            y->rewriteArithmetics(arith, assign, auxGen);
        }
        for (auto &y : *arith.back()) {
            elem.second.emplace_back(RelationLiteral::make(y));
        }
        for (auto &y : assign) {
            elem.second.emplace_back(RelationLiteral::make(y));
        }
        arith.pop_back();
    }
}

void Conjunction::assignLevels(AssignLevel &lvl) {
    for (auto &elem : elems_) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        for (auto &disj : elem.first) {
            for (auto &lit : disj) {
                lit->collect(vars, false);
            }
        }
        for (auto &lit : elem.second) {
            lit->collect(vars, false);
        }
        local.add(vars);
    }
}

void Conjunction::check(ChkLvlVec &levels, Logger &log) const {
    levels.back().current = &levels.back().dep.insertEnt();
    for (auto const &elem : elems_) {
        levels.emplace_back(loc(), *this);
        _add(levels, elem.second);
        // check safety of condition
        levels.back().check(log);
        levels.pop_back();
        for (auto const &disj : elem.first) {
            levels.emplace_back(loc(), *this);
            _add(levels, disj);
            _add(levels, elem.second);
            // check safty of head it can contain pure variables!
            levels.back().check(log);
            levels.pop_back();
        }
    }
}

void Conjunction::replace(Defines &x) {
    for (auto &elem : elems_) {
        for (auto &y : elem.first) {
            for (auto &z : y) {
                z->replace(x);
            }
        }
        for (auto &y : elem.second) {
            y->replace(x);
        }
    }
}

CreateBody Conjunction::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    assert(elems_.size() == 1);

    UTermVec local;
    std::unordered_set<String> seen;
    VarTermBoundVec varsHead;
    VarTermBoundVec varsBody;
    for (auto const &x : elems_.front().first) {
        for (auto const &y : x) {
            y->collect(varsHead, false);
        }
    }
    for (auto const &x : elems_.front().second) {
        x->collect(varsBody, false);
    }
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
    auto &completeRef = static_cast<Ground::ConjunctionComplete&>(*stms.back()); // NOLINT

    Ground::ULitVec condLits;
    for (auto const &y : elems_.front().second) {
        condLits.emplace_back(y->toGround(x.domains, false));
    }
    stms.emplace_back(gringo_make_unique<Ground::ConjunctionAccumulateCond>(completeRef, std::move(condLits)));

    for (auto const &y : elems_.front().first)  {
        Ground::ULitVec headLits;
        for (auto const &z : y) {
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
        if (primary) {
            lits.emplace_back(gringo_make_unique<Ground::ConjunctionLiteral>(completeRef, auxiliary));
        }
    }, std::move(split));
}

bool Conjunction::isAssignment() const {
    return false;
}

void Conjunction::removeAssignment() { }

// {{{1 definition of SimpleBodyLiteral

SimpleBodyLiteral::SimpleBodyLiteral(ULit &&lit)
: lit_(std::move(lit)) { }

Location const &SimpleBodyLiteral::loc() const {
    return lit_->loc();
}

void SimpleBodyLiteral::loc(Location const &loc) {
    lit_->loc(loc);
}

SimpleBodyLiteral::~SimpleBodyLiteral() noexcept = default;

void SimpleBodyLiteral::print(std::ostream &out) const {
    lit_->print(out);
}

size_t SimpleBodyLiteral::hash() const {
    return get_value_hash(typeid(SimpleBodyLiteral).hash_code(), lit_);
}

bool SimpleBodyLiteral::operator==(BodyAggregate const &other) const {
    const auto *t = dynamic_cast<SimpleBodyLiteral const *>(&other);
    return t != nullptr &&
           is_value_equal_to(lit_, t->lit_);
}

SimpleBodyLiteral *SimpleBodyLiteral::clone() const {
    return gringo_make_unique<SimpleBodyLiteral>(get_clone(lit_)).release();
}

void SimpleBodyLiteral::unpool(UBodyAggrVec &x, bool beforeRewrite) {
    for (auto &y : lit_->unpool(beforeRewrite, false)) {
        x.emplace_back(gringo_make_unique<SimpleBodyLiteral>(std::move(y)));
    }
}

bool SimpleBodyLiteral::hasUnpoolComparison() const {
    return lit_->hasUnpoolComparison();
}

UBodyAggrVecVec SimpleBodyLiteral::unpoolComparison() const {
    UBodyAggrVecVec ret;
    for (auto &lits : lit_->unpoolComparison()) {
        ret.emplace_back();
        for (auto &lit : lits) {
            ret.back().emplace_back(gringo_make_unique<SimpleBodyLiteral>(std::move(lit)));
        }
    }
    return ret;
}

void SimpleBodyLiteral::collect(VarTermBoundVec &vars) const {
    lit_->collect(vars, true);
}

bool SimpleBodyLiteral::rewriteAggregates(UBodyAggrVec &aggr) {
    static_cast<void>(aggr);
    return true;
}

bool SimpleBodyLiteral::simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) {
    return lit_->simplify(log, project, state, true, singleton);
}

void SimpleBodyLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::RelationVec &assign, AuxGen &auxGen) {
    lit_->rewriteArithmetics(arith, assign, auxGen);
}

void SimpleBodyLiteral::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    lit_->collect(vars, false);
    lvl.add(vars);
}

void SimpleBodyLiteral::check(ChkLvlVec &levels, Logger &log) const {
    static_cast<void>(log);
    levels.back().current = &levels.back().dep.insertEnt();
    _add(levels, lit_, true);
}

bool SimpleBodyLiteral::hasPool(bool beforeRewrite) const {
    return lit_->hasPool(beforeRewrite, false);
}

void SimpleBodyLiteral::replace(Defines &x) {
    lit_->replace(x);
}

CreateBody SimpleBodyLiteral::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    static_cast<void>(stms);
    return {[&](Ground::ULitVec &lits, bool /*unused*/, bool auxiliary) -> void {
        lits.emplace_back(lit_->toGround(x.domains, auxiliary));
    }, CreateStmVec()};
}

bool SimpleBodyLiteral::isAssignment() const  {
    return false;
}

void SimpleBodyLiteral::removeAssignment() { }

// }}}1

// {{{1 definition of TupleHeadAggregate

TupleHeadAggregate::TupleHeadAggregate(AggregateFunction fun, BoundVec &&bounds, HeadAggrElemVec &&elems)
: TupleHeadAggregate(fun, false, std::move(bounds), std::move(elems)) { }

TupleHeadAggregate::TupleHeadAggregate(AggregateFunction fun, bool translated, BoundVec &&bounds, HeadAggrElemVec &&elems)
: fun_(fun)
, translated_(translated)
, bounds_(std::move(bounds))
, elems_(std::move(elems)) { }

TupleHeadAggregate::~TupleHeadAggregate() noexcept = default;

void TupleHeadAggregate::print(std::ostream &out) const {
    auto f = [](std::ostream &out, HeadAggrElem const &y) {
        using namespace std::placeholders;
        print_comma(out, std::get<0>(y), ",", [](auto && PH1, auto && PH2) { PH2->print(PH1); });
        out << ":";
        std::get<1>(y)->print(out);
        out << ":";
        print_comma(out, std::get<2>(y), ",", [](auto && PH1, auto && PH2) { PH2->print(PH1); });
    };
    printAggr_(out, fun_, bounds_, elems_, f);
}

size_t TupleHeadAggregate::hash() const {
    return get_value_hash(typeid(TupleHeadAggregate).hash_code(), size_t(fun_), bounds_, elems_);
}

bool TupleHeadAggregate::operator==(HeadAggregate const &other) const {
    const auto *t = dynamic_cast<TupleHeadAggregate const *>(&other);
    return t != nullptr &&
           fun_ == t->fun_ &&
           is_value_equal_to(bounds_, t->bounds_) &&
           is_value_equal_to(elems_, t->elems_);
}

TupleHeadAggregate *TupleHeadAggregate::clone() const {
    return make_locatable<TupleHeadAggregate>(loc(), fun_, translated_, get_clone(bounds_), get_clone(elems_)).release();
}

void TupleHeadAggregate::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    HeadAggrElemVec e;
    for (auto &elem : elems_) {
        auto f = [&](UTermVec &&y) { e.emplace_back(std::move(y), get_clone(std::get<1>(elem)), get_clone(std::get<2>(elem))); };
        Term::unpool(std::get<0>(elem).begin(), std::get<0>(elem).end(), Gringo::unpool, f);
    }
    elems_.clear();
    for (auto &elem : e) {
        auto f = [&](ULit &&y) { elems_.emplace_back(get_clone(std::get<0>(elem)), std::move(y), get_clone(std::get<2>(elem))); };
        Term::unpool(std::get<1>(elem), _unpool_lit(beforeRewrite, false), f);
    }
    e.clear();
    for (auto &elem : elems_) {
        if (beforeRewrite) {
            auto f = [&](ULitVec &&y) { e.emplace_back(get_clone(std::get<0>(elem)), get_clone(std::get<1>(elem)), std::move(y)); };
            Term::unpool(std::get<2>(elem).begin(), std::get<2>(elem).end(), _unpool_lit(beforeRewrite, false), f);
        }
        else {
            Term::unpoolJoin(std::get<2>(elem), _unpool_lit(beforeRewrite, false));
            e.emplace_back(std::move(elem));
        }
    }
    auto f = [&](BoundVec &&y) { x.emplace_back(make_locatable<TupleHeadAggregate>(loc(), fun_, translated_, std::move(y), get_clone(e))); };
    Term::unpool(bounds_.begin(), bounds_.end(), _unpool_bound, f);
}

UHeadAggr TupleHeadAggregate::unpoolComparison(UBodyAggrVec &body) {
    static_cast<void>(body);
    // shift comparisons
    for (auto &x : elems_) {
        if (ULit shifted = std::get<1>(x)->shift(false)) {
            std::get<1>(x) = make_locatable<VoidLiteral>(std::get<1>(x)->loc());
            std::get<2>(x).emplace_back(std::move(shifted));
        }
    }
    // extract elements that need unpooling
    HeadAggrElemVec elems;
    move_if(elems_, elems, [](auto const &elem) {
        for (auto &lit : std::get<2>(elem)) {
            if (lit->hasUnpoolComparison()) {
                return true;
            }
        }
        return false;
    });
    // unpool conditions
    for (auto &elem : elems) {
        for (auto &cond : unpoolComparison_(std::get<2>(elem))) {
            elems_.emplace_back(
                get_clone(std::get<0>(elem)),
                get_clone(std::get<1>(elem)),
                std::move(cond));
        }

    }
    return nullptr;
}

void TupleHeadAggregate::collect(VarTermBoundVec &vars) const {
    for (auto const &bound : bounds_) {
        bound.bound->collect(vars, false);
    }
    for (auto const &elem : elems_) {
        for (auto const &term : std::get<0>(elem)) {
            term->collect(vars, false);
        }
        std::get<1>(elem)->collect(vars, false);
        for (auto const &lit : std::get<2>(elem)) {
            lit->collect(vars, false);
        }
    }
}

namespace {

template <class T>
void zeroLevel(VarTermBoundVec &bound, T const &x) {
    bound.clear();
    x->collect(bound, false);
    for (auto &var : bound) {
        var.first->level = 0;
    }
}

} // namspace

UHeadAggr TupleHeadAggregate::rewriteAggregates(UBodyAggrVec &aggr) {
    // NOTE: if it were possible to add further rules here,
    // then also aggregates with more than one element could be supported
    if (elems_.size() == 1 && bounds_.empty()) {
        VarTermBoundVec bound;
        auto &elem = elems_.front();
        // Note: to handle undefinedness in tuples
        bool weight = fun_ == AggregateFunction::SUM || fun_ == AggregateFunction::SUMP;
        auto &tuple = std::get<0>(elem);
        Location l = tuple.empty() ? loc() : tuple.front()->loc();
        for (auto &term : tuple) {
            // NOTE: there could be special predicates for is_integer and is_defined
            zeroLevel(bound, term);
            UTerm first = get_clone(term);
            if (weight) {
                first = make_locatable<BinOpTerm>(l, BinOp::ADD, std::move(first), make_locatable<ValTerm>(l, Symbol::createNum(0)));
                weight = false;
            }
            aggr.emplace_back(gringo_make_unique<SimpleBodyLiteral>(make_locatable<RelationLiteral>(l, Relation::LEQ, std::move(first), std::move(term))));
        }
        tuple.clear();
        tuple.emplace_back(make_locatable<ValTerm>(l, Symbol::createNum(0)));
        for (auto &lit : std::get<2>(elem)) {
            zeroLevel(bound, lit);
            aggr.emplace_back(gringo_make_unique<SimpleBodyLiteral>(std::move(lit)));
        }
        std::get<2>(elem).clear();
        zeroLevel(bound, std::get<1>(elem));
    }
    return nullptr;
}

bool TupleHeadAggregate::simplify(Projections &project, SimplifyState &state, Logger &log) {
    for (auto &bound : bounds_) {
        if (!bound.simplify(state, log)) {
            return false;
        }
    }
    elems_.erase(std::remove_if(elems_.begin(), elems_.end(), [&](HeadAggrElemVec::value_type &elem) {
        auto elemState = SimplifyState::make_substate(state);
        for (auto &term : std::get<0>(elem)) {
            if (term->simplify(elemState, false, false, log).update(term, false).undefined()) {
                return true;
            }
        }
        if (!std::get<1>(elem)->simplify(log, project, elemState, false)) {
            return true;
        }
        for (auto &lit : std::get<2>(elem)) {
            if (!lit->simplify(log, project, elemState)) {
                return true;
            }
        }
        for (auto &dot : elemState.dots()) {
            std::get<2>(elem).emplace_back(RangeLiteral::make(dot));
        }
        for (auto &script : elemState.scripts()) {
            std::get<2>(elem).emplace_back(ScriptLiteral::make(script));
        }
        return false;
    }), elems_.end());
    return true;
}

void TupleHeadAggregate::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    for (auto &bound : bounds_) {
        bound.rewriteArithmetics(arith, auxGen);
    }
    for (auto &elem : elems_) {
        Literal::RelationVec assign;
        arith.emplace_back(gringo_make_unique<Term::LevelMap>());
        for (auto &y : std::get<2>(elem)) {
            y->rewriteArithmetics(arith, assign, auxGen);
        }
        for (auto &y : *arith.back()) {
            std::get<2>(elem).emplace_back(RelationLiteral::make(y));
        }
        for (auto &y : assign) {
            std::get<2>(elem).emplace_back(RelationLiteral::make(y));
        }
        arith.pop_back();
    }
}

void TupleHeadAggregate::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    for (auto &bound : bounds_) {
        bound.bound->collect(vars, false);
    }
    lvl.add(vars);
    for (auto &elem : elems_) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        for (auto &term : std::get<0>(elem)) {
            term->collect(vars, false);
        }
        std::get<1>(elem)->collect(vars, false);
        for (auto &lit : std::get<2>(elem)) {
            lit->collect(vars, false);
        }
        local.add(vars);
    }
}

void TupleHeadAggregate::check(ChkLvlVec &levels, Logger &log) const {
    auto f = [&]() {
        VarTermBoundVec vars;
        for (auto const &y : elems_) {
            levels.emplace_back(loc(), *this);
            _add(levels, std::get<0>(y));
            _add(levels, std::get<1>(y), false);
            _add(levels, std::get<2>(y));
            levels.back().check(log);
            levels.pop_back();
            for (auto const &term : std::get<0>(y)) {
                term->collect(vars, false);
            }
        }
        warnGlobal(vars, !translated_, log);
    };
    _aggr(levels, bounds_, f, false);
}

bool TupleHeadAggregate::hasPool(bool beforeRewrite) const {
    for (auto const &bound : bounds_) {
        if (bound.bound->hasPool()) {
            return true;
        }
    }
    for (auto const &elem : elems_) {
        for (auto const &term : std::get<0>(elem)) {
            if (term->hasPool()) {
                return true;
            }
        }
        if (std::get<1>(elem)->hasPool(beforeRewrite, false)) {
            return true;
        }
        for (auto const &lit : std::get<2>(elem)) {
            if (lit->hasPool(beforeRewrite, false)) {
                return true;
            }
        }
    }
    return false;
}

void TupleHeadAggregate::replace(Defines &x) {
    for (auto &bound : bounds_) {
        Term::replace(bound.bound, bound.bound->replace(x, true));
    }
    for (auto &elem : elems_) {
        for (auto &y : std::get<0>(elem)) {
            Term::replace(y, y->replace(x, true));
        }
        std::get<1>(elem)->replace(x);
        for (auto &y : std::get<2>(elem)) {
            y->replace(x);
        }
    }
}

CreateHead TupleHeadAggregate::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    bool isSimple = bounds_.empty();
    if (isSimple) {
        for (auto const &elem  : elems_) {
            if (!std::get<2>(elem).empty()) {
                isSimple = false;
                break;
            }
        }
    }
    if (isSimple) {
        DomainData &data = x.domains;
        return CreateHead([&](Ground::ULitVec &&lits) {
            Ground::AbstractRule::HeadVec heads;
            for (auto const &elem : elems_) {
                if (UTerm headRepr = std::get<1>(elem)->headRepr()) {
                    PredicateDomain *headDom = &data.add(headRepr->getSig());
                    heads.emplace_back(std::move(headRepr), headDom);
                }
            }
            return gringo_make_unique<Ground::Rule<false>>(std::move(heads), std::move(lits));
        });
    }

    stms.emplace_back(gringo_make_unique<Ground::HeadAggregateComplete>(x.domains, x.newId(*this), fun_, get_clone(bounds_)));
    auto &completeRef = static_cast<Ground::HeadAggregateComplete&>(*stms.back()); // NOLINT
    for (auto const &y : elems_) {
        Ground::ULitVec lits;
        for (auto const &z : std::get<2>(y)) {
            lits.emplace_back(z->toGround(x.domains, false));
        }
        lits.emplace_back(gringo_make_unique<Ground::HeadAggregateLiteral>(completeRef));
        UTerm headRep(std::get<1>(y)->headRepr());
        PredicateDomain *predDom = headRep ? &x.domains.add(headRep->getSig()) : nullptr;
        auto ret = gringo_make_unique<Ground::HeadAggregateAccumulate>(completeRef, get_clone(std::get<0>(y)), predDom, std::move(headRep), std::move(lits));
        completeRef.addAccuDom(*ret);
        stms.emplace_back(std::move(ret));
    }
    return CreateHead([&completeRef](Ground::ULitVec &&lits) {
        return gringo_make_unique<Ground::HeadAggregateRule>(completeRef, std::move(lits));
    });
}

// {{{1 definition of LitHeadAggregate

LitHeadAggregate::LitHeadAggregate(AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems)
: fun_(fun)
, bounds_(std::move(bounds))
, elems_(std::move(elems)) { }

LitHeadAggregate::~LitHeadAggregate() noexcept = default;

void LitHeadAggregate::print(std::ostream &out) const {
    printAggr_(out, fun_, bounds_, elems_, printCond_);
}

size_t LitHeadAggregate::hash() const {
    return get_value_hash(typeid(LitHeadAggregate).hash_code(), size_t(fun_), bounds_, elems_);
}

bool LitHeadAggregate::operator==(HeadAggregate const &other) const {
    const auto *t = dynamic_cast<LitHeadAggregate const *>(&other);
    return t != nullptr &&
           fun_ == t->fun_ &&
           is_value_equal_to(bounds_, t->bounds_) &&
           is_value_equal_to(elems_, t->elems_);
}

LitHeadAggregate *LitHeadAggregate::clone() const {
    return make_locatable<LitHeadAggregate>(loc(), fun_, get_clone(bounds_), get_clone(elems_)).release();
}

void LitHeadAggregate::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    CondLitVec e;
    for (auto &elem : elems_) {
        auto f = [&](ULit &&y) { e.emplace_back(std::move(y), get_clone(elem.second)); };
        Term::unpool(elem.first, _unpool_lit(beforeRewrite, true), f);
    }
    elems_.clear();
    for (auto &elem : e) {
        if (beforeRewrite) {
            auto f = [&](ULitVec &&y) { elems_.emplace_back(get_clone(elem.first), std::move(y)); };
            Term::unpool(elem.second.begin(), elem.second.end(), _unpool_lit(beforeRewrite, false), f);
        }
        else {
            Term::unpoolJoin(elem.second, _unpool_lit(beforeRewrite, false));
            elems_.emplace_back(std::move(elem));
        }
    }
    e.clear();
    auto f = [&](BoundVec &&y) { x.emplace_back(make_locatable<LitHeadAggregate>(loc(), fun_, std::move(y), get_clone(elems_))); };
    Term::unpool(bounds_.begin(), bounds_.end(), _unpool_bound, f);
}

UHeadAggr LitHeadAggregate::unpoolComparison(UBodyAggrVec &body) {
    static_cast<void>(body);
    int id = 0;
    HeadAggrElemVec elems;
    for (auto &x : elems_) {
        UTermVec tuple;
        x.first->toTuple(tuple, id);
        elems.emplace_back(std::move(tuple), get_clone(x.first), get_clone(x.second));
    }
    UHeadAggr x(make_locatable<TupleHeadAggregate>(loc(), fun_, true, get_clone(bounds_), std::move(elems)));
    Term::replace(x, x->unpoolComparison(body));
    return x;
}

void LitHeadAggregate::collect(VarTermBoundVec &vars) const {
    for (auto const &bound : bounds_) {
        bound.bound->collect(vars, false);
    }
    for (auto const &elem : elems_) {
        elem.first->collect(vars, false);
        for (auto const &lit : elem.second) {
            lit->collect(vars, false);
        }
    }
}

UHeadAggr LitHeadAggregate::rewriteAggregates(UBodyAggrVec &aggr) {
    static_cast<void>(aggr);
    throw  std::runtime_error("LitHeadAggregate::rewriteAggregate: must not be called");
}

bool LitHeadAggregate::simplify(Projections &project, SimplifyState &state, Logger &log) {
    for (auto &bound : bounds_) {
        if (!bound.simplify(state, log)) {
            return false;
        }
    }
    elems_.erase(std::remove_if(elems_.begin(), elems_.end(), [&](CondLitVec::value_type &elem) {
        auto elemState = SimplifyState::make_substate(state);
        if (!std::get<0>(elem)->simplify(log, project, elemState, false)) {
            return true;
        }
        for (auto &lit : std::get<1>(elem)) {
            if (!lit->simplify(log, project, elemState)) {
                return true;
            }
        }
        for (auto &dot : elemState.dots()) {
            std::get<1>(elem).emplace_back(RangeLiteral::make(dot));
        }
        for (auto &script : elemState.scripts()) {
            std::get<1>(elem).emplace_back(ScriptLiteral::make(script));
        }
        return false;
    }), elems_.end());
    return true;
}

void LitHeadAggregate::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    for (auto &bound : bounds_) {
        bound.rewriteArithmetics(arith, auxGen);
    }
    for (auto &elem : elems_) {
        Literal::RelationVec assign;
        arith.emplace_back(gringo_make_unique<Term::LevelMap>());
        for (auto &y : std::get<1>(elem)) {
            y->rewriteArithmetics(arith, assign, auxGen);
        }
        for (auto &y : *arith.back()) {
            std::get<1>(elem).emplace_back(RelationLiteral::make(y));
        }
        for (auto &y : assign) {
            std::get<1>(elem).emplace_back(RelationLiteral::make(y));
        }
        arith.pop_back();
    }
}

void LitHeadAggregate::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    for (auto &bound : bounds_) {
        bound.bound->collect(vars, false);
    }
    lvl.add(vars);
    for (auto &elem : elems_) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        std::get<0>(elem)->collect(vars, false);
        for (auto &lit : std::get<1>(elem)) {
            lit->collect(vars, false);
        }
        local.add(vars);
    }
}

void LitHeadAggregate::check(ChkLvlVec &levels, Logger &log) const {
    auto f = [&]() {
        for (auto const &y : elems_) {
            levels.emplace_back(loc(), *this);
            _add(levels, y.first, false);
            _add(levels, y.second);
            levels.back().check(log);
            levels.pop_back();
        }
    };
    _aggr(levels, bounds_, f, false);
}

bool LitHeadAggregate::hasPool(bool beforeRewrite) const {
    for (auto const &bound : bounds_) {
        if (bound.bound->hasPool()) {
            return true;
        }
    }
    for (auto const &elem : elems_) {
        if (elem.first->hasPool(beforeRewrite, true)) {
            return true;
        }
        for (auto const &lit : elem.second) {
            if (lit->hasPool(beforeRewrite, false)) {
                return true;
            }
        }
    }
    return false;
}

void LitHeadAggregate::replace(Defines &x) {
    for (auto &bound : bounds_) {
        Term::replace(bound.bound, bound.bound->replace(x, true));
    }
    for (auto &elem : elems_) {
        elem.first->replace(x);
        for (auto &y : elem.second) {
            y->replace(x);
        }
    }
}

CreateHead LitHeadAggregate::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    static_cast<void>(x);
    static_cast<void>(stms);
    throw std::logic_error("Aggregate::rewriteAggregates must be called before LitAggregate::toGround");
}

// {{{1 definition of Disjunction

Disjunction::Disjunction(CondLitVec &&elems) {
    for (auto &x : elems) {
        elems_.emplace_back();
        elems_.back().second = std::move(x.second);
        elems_.back().first.emplace_back();
        elems_.back().first.back().first = std::move(x.first);
    }
}

Disjunction::Disjunction(ElemVec &&elems)
: elems_(std::move(elems)) { }

Disjunction::~Disjunction() noexcept = default;

void Disjunction::print(std::ostream &out) const {
    auto f = [](std::ostream &out, Elem const &y) {
        using namespace std::placeholders;
        print_comma(out, y.first, "&", [](std::ostream &out, Head const &z) {
            out << *z.first << ":";
            print_comma(out, z.second, ",", [](auto && PH1, auto && PH2) { PH2->print(PH1); });
        });
        out << ":";
        print_comma(out, y.second, ",", [](auto && PH1, auto && PH2) { PH2->print(PH1); });
    };
    print_comma(out, elems_, ";", f);
}

size_t Disjunction::hash() const {
    return get_value_hash(typeid(Disjunction).hash_code(), elems_);
}

bool Disjunction::operator==(HeadAggregate const &other) const {
    const auto *t = dynamic_cast<Disjunction const *>(&other);
    return t != nullptr &&
           is_value_equal_to(elems_, t->elems_);
}

Disjunction *Disjunction::clone() const {
    return make_locatable<Disjunction>(loc(), get_clone(elems_)).release();
}

void Disjunction::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    ElemVec e;
    for (auto &elem : elems_) {
        if (beforeRewrite) {
            HeadVec heads;
            for (auto &head : elem.first) {
                for (auto &lit : head.first->unpool(beforeRewrite, true)) {
                    Term::unpool(head.second.begin(), head.second.end(), _unpool_lit(beforeRewrite, false), [&](ULitVec &&expand) {
                        heads.emplace_back(get_clone(lit), std::move(expand));
                    });
                }
            }
            elem.first = std::move(heads);
            auto f = [&](ULitVec &&y) { e.emplace_back(get_clone(elem.first), std::move(y)); };
            Term::unpool(elem.second.begin(), elem.second.end(), _unpool_lit(beforeRewrite, false), f);
        }
        else {
            HeadVec heads;
            for (auto &head : elem.first) {
                Term::unpoolJoin(head.second, _unpool_lit(beforeRewrite, false));
                for (auto &lit : head.first->unpool(beforeRewrite, true)) {
                    heads.emplace_back(std::move(lit), get_clone(head.second));
                }
            }
            elem.first = std::move(heads);
            Term::unpoolJoin(elem.second, _unpool_lit(beforeRewrite, false));
            e.emplace_back(std::move(elem));
        }
    }
    x.emplace_back(make_locatable<Disjunction>(loc(), std::move(e)));
}

UHeadAggr Disjunction::unpoolComparison(UBodyAggrVec &body) {
    static_cast<void>(body);
    // shift comparisons
    for (auto &elem : elems_) {
        for (auto &head : elem.first) {
            if (ULit shifted = head.first->shift(true)) {
                head.first = make_locatable<VoidLiteral>(head.first->loc());
                head.second.emplace_back(std::move(shifted));
            }
        }
    }
    // extract elements that need unpooling
    ElemVec elems;
    move_if(elems_, elems, [](auto const &elem) {
        for (auto &head : elem.first) {
            for (auto &lit : head.second) {
                if (lit->hasUnpoolComparison()) {
                    return true;
                }
            }
        }
        for (auto &lit : elem.second) {
            if (lit->hasUnpoolComparison()) {
                return true;
            }
        }
        return false;
    });
    // unpool elements
    for (auto const &elem : elems) {
        // compute cross-product of unpooled head conditions
        HeadVec newHeads;
        for (auto const &head : elem.first) {
            for (auto &cond : unpoolComparison_(head.second)) {
                newHeads.emplace_back(get_clone(head.first), std::move(cond));
            }
        }
        // compute cross-product of the unpooled conditions
        for (auto &cond : unpoolComparison_(elem.second)) {
            elems_.emplace_back(get_clone(newHeads), std::move(cond));
        }
    }
    return nullptr;
}

void Disjunction::collect(VarTermBoundVec &vars) const {
    for (auto const &elem : elems_) {
        for (auto const &head : elem.first) {
            head.first->collect(vars, false);
            for (auto const &lit : head.second) {
                lit->collect(vars, false);
            }
        }
        for (auto const &lit : elem.second) {
            lit->collect(vars, false);
        }
    }
}

UHeadAggr Disjunction::rewriteAggregates(UBodyAggrVec &aggr) {
    for (auto &elem : elems_) {
        for (auto &head : elem.first) {
            if (ULit shifted = head.first->shift(true)) {
                head.first = make_locatable<VoidLiteral>(head.first->loc());
                if (!shifted->triviallyTrue()) {
                    head.second.emplace_back(std::move(shifted));
                }
            }
        }
        if (elem.second.empty() && elem.first.size() == 1) {
            auto &head = elem.first.front();
            VarTermBoundVec vars;
            head.first->collect(vars, false);
            for (auto &var : vars) {
                var.first->level = 0;
            }
            vars.clear();
            for (auto &lit : head.second) {
                lit->collect(vars, false);
                for (auto &var : vars) {
                    var.first->level = 0;
                }
                vars.clear();
                aggr.emplace_back(make_locatable<SimpleBodyLiteral>(loc(), std::move(lit)));
            }
            head.second.clear();
        }
    }
    return nullptr;
}

bool Disjunction::simplify(Projections &project, SimplifyState &state, Logger &log) {
    for (auto &elem : elems_) {
        for (auto &head : elem.first) {
            bool replace = false;
            auto elemState = SimplifyState::make_substate(state);
            if (head.first->simplify(log, project, elemState)) {
                for (auto &lit : head.second) {
                    if (!lit->simplify(log, project, elemState)) {
                        replace = true;
                        break;
                    }
                }
            }
            else {
                replace = true;
            }
            if (replace) {
                auto loc = head.first->loc();
                head.first = Gringo::make_locatable<RelationLiteral>(loc, Relation::EQ, make_locatable<ValTerm>(
                    loc, Symbol::createNum(0)), make_locatable<ValTerm>(loc, Symbol::createNum(0)));
                head.second.clear();
            }
            else {
                for (auto &dot : elemState.dots()) {
                    head.second.emplace_back(RangeLiteral::make(dot));
                }
                for (auto &script : elemState.scripts()) {
                    head.second.emplace_back(ScriptLiteral::make(script));
                }
            }
        }
    }
    elems_.erase(std::remove_if(elems_.begin(), elems_.end(), [&](ElemVec::value_type &elem) -> bool {
        auto elemState = SimplifyState::make_substate(state);
        for (auto &lit : elem.second) {
            if (!lit->simplify(log, project, elemState)) {
                return true;
            }
        }
        for (auto &dot : elemState.dots()) {
            elem.second.emplace_back(RangeLiteral::make(dot));
        }
        for (auto &script : elemState.scripts()) {
            elem.second.emplace_back(ScriptLiteral::make(script));
        }
        return false;
    }), elems_.end());
    return true;
}

void Disjunction::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    for (auto &elem : elems_) {
        for (auto &head : elems_) {
            Literal::RelationVec assign;
            arith.emplace_back(gringo_make_unique<Term::LevelMap>());
            for (auto &y : head.second) {
                y->rewriteArithmetics(arith, assign, auxGen);
            }
            for (auto &y : *arith.back()) {
                head.second.emplace_back(RelationLiteral::make(y));
            }
            for (auto &y : assign) {
                head.second.emplace_back(RelationLiteral::make(y));
            }
            arith.pop_back();
        }
        Literal::RelationVec assign;
        arith.emplace_back(gringo_make_unique<Term::LevelMap>());
        for (auto &y : elem.second) {
            y->rewriteArithmetics(arith, assign, auxGen);
        }
        for (auto &y : *arith.back()) {
            elem.second.emplace_back(RelationLiteral::make(y));
        }
        for (auto &y : assign) {
            elem.second.emplace_back(RelationLiteral::make(y));
        }
        arith.pop_back();
    }
}

void Disjunction::assignLevels(AssignLevel &lvl) {
    for (auto &elem : elems_) {
        AssignLevel &local(lvl.subLevel());
        VarTermBoundVec vars;
        for (auto &head : elem.first) {
            head.first->collect(vars, false);
            for (auto &lit : head.second) {
                lit->collect(vars, false);
            }
        }
        for (auto &lit : elem.second) {
            lit->collect(vars, false);
        }
        local.add(vars);
    }
}

void Disjunction::check(ChkLvlVec &levels, Logger &log) const {
    levels.back().current = &levels.back().dep.insertEnt();
    for (auto const &y : elems_) {
        levels.emplace_back(loc(), *this);
        _add(levels, y.second);
        // check condition first
        // (although by construction it cannot happen that something in expand binds something in condition)
        levels.back().check(log);
        levels.pop_back();
        for (auto const &head : y.first) {
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
    for (auto const &elem : elems_) {
        for (auto const &head : elem.first) {
            if (head.first->hasPool(beforeRewrite, true)) {
                return true;
            }
            for (auto const &lit : head.second) {
                if (lit->hasPool(beforeRewrite, false)) {
                    return true;
                }
            }
        }
        for (auto const &lit : elem.second) {
            if (lit->hasPool(beforeRewrite, false)) {
                return true;
            }
        }
    }
    return false;
}

void Disjunction::replace(Defines &x) {
    for (auto &elem : elems_) {
        for (auto &head : elem.first) {
            head.first->replace(x);
            for (auto &y : elem.second) {
                y->replace(x);
            }
        }
        for (auto &y : elem.second) {
            y->replace(x);
        }
    }
}

CreateHead Disjunction::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    bool isSimple = true;
    for (auto const &y : elems_) {
        if (y.first.size() > 1 || !y.second.empty()) {
            isSimple = false;
            break;
        }
    }
    if (isSimple) {
        DomainData &data = x.domains;
        return CreateHead([&](Ground::ULitVec &&lits) {
            Ground::AbstractRule::HeadVec heads;
            for (auto const &elem : elems_) {
                for (auto const &head : elem.first) {
                    if (UTerm headRepr = head.first->headRepr()) {
                        PredicateDomain *headDom = &data.add(headRepr->getSig());
                        heads.emplace_back(std::move(headRepr), headDom);
                    }
                }
            }
            return gringo_make_unique<Ground::Rule<true>>(std::move(heads), std::move(lits));
        });
    }
    stms.emplace_back(gringo_make_unique<Ground::DisjunctionComplete>(x.domains, x.newId(*this)));
    auto &complete = static_cast<Ground::DisjunctionComplete&>(*stms.back()); // NOLINT
    for (auto const &y : elems_) {
        UTermVec elemVars;
        std::unordered_set<String> seen;
        VarTermBoundVec vars;
        for (auto const &x : y.second) {
            x->collect(vars, false);
        }
        for (auto &occ : vars) {
            if (occ.first->level != 0) {
                seen.emplace(occ.first->name);
            }
        }
        vars.clear();
        for (auto const &head : y.first) {
            head.first->collect(vars, false);
            for (auto const &x : head.second) {
                x->collect(vars, false);
            }
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
            for (auto const &z : y.second) {
                elemCond.emplace_back(z->toGround(x.domains, false));
            }
            Ground::ULitVec headCond;
            headCond.emplace_back(make_locatable<RelationLiteral>(loc(),
                                                                  Relation::NEQ,
                                                                  make_locatable<ValTerm>(loc(), Symbol::createNum(0)),
                                                                  make_locatable<ValTerm>(loc(), Symbol::createNum(0)))->toGround(x.domains, true));
            stms.emplace_back(gringo_make_unique<Ground::DisjunctionAccumulate>(complete,
                                                                                nullptr,
                                                                                nullptr,
                                                                                std::move(headCond),
                                                                                std::move(elemRepr),
                                                                                std::move(elemCond)));
        }
        else {
            for (auto const &head : y.first) {
                Ground::ULitVec elemCond;
                for (auto const &z : y.second) {
                    elemCond.emplace_back(z->toGround(x.domains, false));
                }
                Ground::ULitVec headCond;
                for (auto const &z : head.second) {
                    headCond.emplace_back(z->toGround(x.domains, false));
                }
                UTerm headRepr = head.first->headRepr();
                PredicateDomain *headDom = headRepr ? &x.domains.add(headRepr->getSig()) : nullptr;
                stms.emplace_back(gringo_make_unique<Ground::DisjunctionAccumulate>(complete,
                                                                                    headDom,
                                                                                    std::move(headRepr),
                                                                                    std::move(headCond),
                                                                                    get_clone(elemRepr),
                                                                                    std::move(elemCond)));
            }
        }
    }
    return CreateHead([&](Ground::ULitVec &&lits) {
        return gringo_make_unique<Ground::DisjunctionRule>(complete, std::move(lits));
    });
}

// {{{1 definition of SimpleHeadLiteral

SimpleHeadLiteral::SimpleHeadLiteral(ULit &&lit)
: lit_(std::move(lit)) { }

SimpleHeadLiteral::~SimpleHeadLiteral() noexcept = default;

Location const &SimpleHeadLiteral::loc() const {
    return lit_->loc();
}

void SimpleHeadLiteral::loc(Location const &loc) {
    lit_->loc(loc);
}

void SimpleHeadLiteral::print(std::ostream &out) const {
    lit_->print(out);
}

size_t SimpleHeadLiteral::hash() const {
    return get_value_hash(typeid(SimpleHeadLiteral).hash_code(), lit_);
}

bool SimpleHeadLiteral::operator==(HeadAggregate const &other) const {
    const auto *t = dynamic_cast<SimpleHeadLiteral const *>(&other);
    return t != nullptr &&
           is_value_equal_to(lit_, t->lit_);
}

SimpleHeadLiteral *SimpleHeadLiteral::clone() const {
    return gringo_make_unique<SimpleHeadLiteral>(get_clone(lit_)).release();
}

bool SimpleHeadLiteral::hasPool(bool beforeRewrite) const {
    return lit_->hasPool(beforeRewrite, true);
}

void SimpleHeadLiteral::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    for (auto &y : lit_->unpool(beforeRewrite, true)) {
        x.emplace_back(gringo_make_unique<SimpleHeadLiteral>(std::move(y)));
    }
}

UHeadAggr SimpleHeadLiteral::unpoolComparison(UBodyAggrVec &body) {
    ULit shifted(lit_->shift(true));
    if (shifted) {
        body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(std::move(shifted)));
        return gringo_make_unique<SimpleHeadLiteral>(make_locatable<VoidLiteral>(lit_->loc()));
    }
    return nullptr;
}

void SimpleHeadLiteral::collect(VarTermBoundVec &vars) const {
    lit_->collect(vars, false);
}

UHeadAggr SimpleHeadLiteral::rewriteAggregates(UBodyAggrVec &aggr) {
    static_cast<void>(aggr);
    return nullptr;
}

bool SimpleHeadLiteral::simplify(Projections &project, SimplifyState &state, Logger &log) {
    return lit_->simplify(log, project, state, false);
}

void SimpleHeadLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    // Note: nothing to do
    static_cast<void>(arith);
    static_cast<void>(auxGen);
}

void SimpleHeadLiteral::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    lit_->collect(vars, false);
    lvl.add(vars);
}

Symbol SimpleHeadLiteral::isEDB() const {
    return lit_->isEDB();
}

void SimpleHeadLiteral::check(ChkLvlVec &levels, Logger &log) const {
    static_cast<void>(log);
    _add(levels, lit_, false);
}

void SimpleHeadLiteral::replace(Defines &x) {
    lit_->replace(x);
}

CreateHead SimpleHeadLiteral::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    static_cast<void>(stms);
    return {[this, &x](Ground::ULitVec &&lits) -> Ground::UStm {
        Ground::AbstractRule::HeadVec heads;
        if (UTerm headRepr = lit_->headRepr()) {
            Sig sig(headRepr->getSig());
            heads.emplace_back(std::move(headRepr), &x.domains.add(sig));
        }
        return gringo_make_unique<Ground::Rule<true>>(std::move(heads), std::move(lits));
    }};
}

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

MinimizeHeadLiteral::~MinimizeHeadLiteral() noexcept = default;

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

void MinimizeHeadLiteral::printWithCondition(std::ostream &out, UBodyAggrVec const &condition) const {
    out << ":~";
    auto f = [](std::ostream &out, UBodyAggr const &x) { out << *x; };
    print_comma(out, condition, ";", f);
    out << "." << *this;
}

size_t MinimizeHeadLiteral::hash() const {
    return get_value_hash(typeid(MinimizeHeadLiteral).hash_code(), tuple_);
}

bool MinimizeHeadLiteral::operator==(HeadAggregate const &other) const {
    const auto *t = dynamic_cast<MinimizeHeadLiteral const *>(&other);
    return t != nullptr &&
           is_value_equal_to(tuple_, t->tuple_);
}

MinimizeHeadLiteral *MinimizeHeadLiteral::clone() const {
    return make_locatable<MinimizeHeadLiteral>(loc(), get_clone(tuple_)).release();
}

void MinimizeHeadLiteral::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    assert(beforeRewrite); (void)beforeRewrite;
    auto f = [&](UTermVec &&tuple) { x.emplace_back(make_locatable<MinimizeHeadLiteral>(loc(), std::move(tuple))); };
    Term::unpool(tuple_.begin(), tuple_.end(), Gringo::unpool, f);
}

UHeadAggr MinimizeHeadLiteral::unpoolComparison(UBodyAggrVec &body) {
    static_cast<void>(body);
    return nullptr;
}

void MinimizeHeadLiteral::collect(VarTermBoundVec &vars) const {
    for (auto const &term : tuple_) {
        term->collect(vars, false);
    }
}

UHeadAggr MinimizeHeadLiteral::rewriteAggregates(UBodyAggrVec &aggr) {
    static_cast<void>(aggr);
    return nullptr;
}

bool MinimizeHeadLiteral::simplify(Projections &project, SimplifyState &state, Logger &log) {
    static_cast<void>(project);
    for (auto &term : tuple_) {
        if (term->simplify(state, false, false, log).update(term, false).undefined()) {
            return false;
        }
    }
    return true;
}

void MinimizeHeadLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    // Note: nothing to do
    static_cast<void>(arith);
    static_cast<void>(auxGen);
}

void MinimizeHeadLiteral::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    for (auto &term : tuple_) {
        term->collect(vars, false);
    }
    lvl.add(vars);
}

void MinimizeHeadLiteral::check(ChkLvlVec &levels, Logger &log) const {
    static_cast<void>(log);
    levels.back().current = &levels.back().dep.insertEnt();
    VarTermBoundVec vars;
    collect(vars);
    addVars(levels, vars);
}

bool MinimizeHeadLiteral::hasPool(bool beforeRewrite) const {
    if (beforeRewrite) {
        for (auto const &term : tuple_) {
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

CreateHead MinimizeHeadLiteral::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    static_cast<void>(x);
    static_cast<void>(stms);
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

// {{{1 definition of EdgeHeadAtom

EdgeHeadAtom::EdgeHeadAtom(UTerm &&u, UTerm &&v)
: u_(std::move(u))
, v_(std::move(v))
{ }

EdgeHeadAtom::~EdgeHeadAtom() noexcept = default;

void EdgeHeadAtom::print(std::ostream &out) const {
    out << "#edge(" << *u_ << "," << *v_ << ")";
}

size_t EdgeHeadAtom::hash() const {
    return get_value_hash(typeid(EdgeHeadAtom).hash_code(), u_, v_);
}

bool EdgeHeadAtom::operator==(HeadAggregate const &other) const {
    const auto *t = dynamic_cast<EdgeHeadAtom const *>(&other);
    return t != nullptr &&
           is_value_equal_to(u_, t->u_) &&
           is_value_equal_to(v_, t->v_);;
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
        x.emplace_back(clone());
    }
}

UHeadAggr EdgeHeadAtom::unpoolComparison(UBodyAggrVec &body) {
    static_cast<void>(body);
    return nullptr;
}

void EdgeHeadAtom::collect(VarTermBoundVec &vars) const {
    u_->collect(vars, false);
    v_->collect(vars, false);
}

UHeadAggr EdgeHeadAtom::rewriteAggregates(UBodyAggrVec &aggr) {
    static_cast<void>(aggr);
    return nullptr;
}

bool EdgeHeadAtom::simplify(Projections &project, SimplifyState &state, Logger &log) {
    static_cast<void>(project);
    return !u_->simplify(state, false, false, log).update(u_, false).undefined() &&
           !v_->simplify(state, false, false, log).update(v_, false).undefined();
}

void EdgeHeadAtom::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    // Note: nothing to do
    static_cast<void>(arith);
    static_cast<void>(auxGen);
}

void EdgeHeadAtom::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    collect(vars);
    lvl.add(vars);
}

void EdgeHeadAtom::check(ChkLvlVec &levels, Logger &log) const {
    static_cast<void>(log);
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

CreateHead EdgeHeadAtom::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    static_cast<void>(x);
    static_cast<void>(stms);
    return CreateHead([&](Ground::ULitVec &&lits) {
        return gringo_make_unique<Ground::EdgeStatement>(get_clone(u_), get_clone(v_), std::move(lits));
    });
}

// {{{1 definition of ProjectHeadAtom

ProjectHeadAtom::ProjectHeadAtom(UTerm &&atom)
: atom_(std::move(atom)) { }

ProjectHeadAtom::~ProjectHeadAtom() noexcept = default;

void ProjectHeadAtom::print(std::ostream &out) const {
    out << "#project " << *atom_;
}

size_t ProjectHeadAtom::hash() const {
    return get_value_hash(typeid(ProjectHeadAtom).hash_code(), atom_);
}

bool ProjectHeadAtom::operator==(HeadAggregate const &other) const {
    const auto *t = dynamic_cast<ProjectHeadAtom const *>(&other);
    return t != nullptr &&
           is_value_equal_to(atom_, t->atom_);
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

UHeadAggr ProjectHeadAtom::unpoolComparison(UBodyAggrVec &body) {
    static_cast<void>(body);
    return nullptr;
}

void ProjectHeadAtom::collect(VarTermBoundVec &vars) const {
    atom_->collect(vars, false);
}

UHeadAggr ProjectHeadAtom::rewriteAggregates(UBodyAggrVec &aggr) {
    aggr.emplace_back(gringo_make_unique<SimpleBodyLiteral>(make_locatable<PredicateLiteral>(atom_->loc(), NAF::POS, get_clone(atom_), true)));
    return nullptr;
}

bool ProjectHeadAtom::simplify(Projections &project, SimplifyState &state, Logger &log) {
    static_cast<void>(project);
    return !atom_->simplify(state, false, false, log).update(atom_, false).undefined();
}

void ProjectHeadAtom::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxgen) {
    atom_->rewriteArithmetics(arith, auxgen);
}

void ProjectHeadAtom::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    collect(vars);
    lvl.add(vars);
}

void ProjectHeadAtom::check(ChkLvlVec &levels, Logger &log) const {
    static_cast<void>(log);
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

CreateHead ProjectHeadAtom::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    static_cast<void>(x);
    static_cast<void>(stms);
    return CreateHead([&](Ground::ULitVec &&lits) {
        return gringo_make_unique<Ground::ProjectStatement>(get_clone(atom_), std::move(lits));
    });
}

// {{{1 definition of ExternalHeadAtom

ExternalHeadAtom::ExternalHeadAtom(UTerm &&atom, UTerm &&type)
: atom_(std::move(atom))
, type_(std::move(type))
{ }

ExternalHeadAtom::~ExternalHeadAtom() noexcept = default;

void ExternalHeadAtom::print(std::ostream &out) const {
    out << "#external " << *atom_;
}

void ExternalHeadAtom::printWithCondition(std::ostream &out, UBodyAggrVec const &condition) const {
    out << *this;
    if (!condition.empty()) {
        out << ":";
        auto f = [](std::ostream &out, UBodyAggr const &x) { out << *x; };
        print_comma(out, condition, ";", f);
    }
    out << "." << "[" << *type_ << "]";
}

size_t ExternalHeadAtom::hash() const {
    return get_value_hash(typeid(ExternalHeadAtom).hash_code(), atom_, type_);
}

bool ExternalHeadAtom::operator==(HeadAggregate const &other) const {
    const auto *t = dynamic_cast<ExternalHeadAtom const *>(&other);
    return t != nullptr &&
           is_value_equal_to(atom_, t->atom_) &&
           is_value_equal_to(type_, t->type_);
}

ExternalHeadAtom *ExternalHeadAtom::clone() const {
    return make_locatable<ExternalHeadAtom>(loc(), get_clone(atom_), get_clone(type_)).release();
}

void ExternalHeadAtom::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    if (beforeRewrite) {
        for (auto &atom : Gringo::unpool(atom_)) {
            for (auto &type : Gringo::unpool(type_)) {
                x.emplace_back(make_locatable<ExternalHeadAtom>(loc(), get_clone(atom), std::move(type)));
            }
        }
    }
    else {
        x.emplace_back(clone());
    }
}

UHeadAggr ExternalHeadAtom::unpoolComparison(UBodyAggrVec &body) {
    static_cast<void>(body);
    return nullptr;
}

void ExternalHeadAtom::collect(VarTermBoundVec &vars) const {
    atom_->collect(vars, false);
    type_->collect(vars, false);
}

UHeadAggr ExternalHeadAtom::rewriteAggregates(UBodyAggrVec &aggr) {
    static_cast<void>(aggr);
    return nullptr;
}

bool ExternalHeadAtom::simplify(Projections &project, SimplifyState &state, Logger &log) {
    static_cast<void>(project);
    return !atom_->simplify(state, false, false, log).update(atom_, false).undefined() &&
           !type_->simplify(state, false, false, log).update(type_, false).undefined();
}

void ExternalHeadAtom::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxgen) {
    atom_->rewriteArithmetics(arith, auxgen);
    type_->rewriteArithmetics(arith, auxgen);
}

void ExternalHeadAtom::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    collect(vars);
    lvl.add(vars);
}

void ExternalHeadAtom::check(ChkLvlVec &levels, Logger &log) const {
    static_cast<void>(log);
    levels.back().current = &levels.back().dep.insertEnt();
    VarTermBoundVec vars;
    collect(vars);
    addVars(levels, vars);
}

bool ExternalHeadAtom::hasPool(bool beforeRewrite) const {
    return beforeRewrite && (atom_->hasPool() || type_->hasPool());
}

void ExternalHeadAtom::replace(Defines &x) {
    atom_->replace(x, false);
    type_->replace(x, true);
}

CreateHead ExternalHeadAtom::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    static_cast<void>(stms);
    return {[this, &x](Ground::ULitVec &&lits) -> Ground::UStm {
        Ground::AbstractRule::HeadVec heads;
        Sig sig(atom_->getSig());
        heads.emplace_back(get_clone(atom_), &x.domains.add(sig));
        return gringo_make_unique<Ground::ExternalStatement>(std::move(heads), std::move(lits), get_clone(type_));
    }};
}

// {{{1 definition of HeuristicHeadAtom

HeuristicHeadAtom::HeuristicHeadAtom(UTerm &&atom, UTerm &&bias, UTerm &&priority, UTerm &&mod)
: atom_(std::move(atom))
, value_(std::move(bias))
, priority_(std::move(priority))
, mod_(std::move(mod))
{ }

HeuristicHeadAtom::~HeuristicHeadAtom() noexcept = default;

void HeuristicHeadAtom::print(std::ostream &out) const {
    out << "#heuristic " << *atom_ << "[" << *value_ << "@" << *priority_ << "," << *mod_ << "]";
}

size_t HeuristicHeadAtom::hash() const {
    return get_value_hash(typeid(HeuristicHeadAtom).hash_code(), atom_, value_, priority_, mod_);
}

bool HeuristicHeadAtom::operator==(HeadAggregate const &other) const {
    const auto *t = dynamic_cast<HeuristicHeadAtom const *>(&other);
    return t != nullptr &&
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
                        x.emplace_back(make_locatable<HeuristicHeadAtom>(loc(), get_clone(atom), get_clone(bias), get_clone(priority), get_clone(mod)));
                    }
                }
            }
        }
    }
    else {
        x.emplace_back(clone());
    }
}

UHeadAggr HeuristicHeadAtom::unpoolComparison(UBodyAggrVec &body) {
    static_cast<void>(body);
    return nullptr;
}

void HeuristicHeadAtom::collect(VarTermBoundVec &vars) const {
    atom_->collect(vars, false);
    value_->collect(vars, false);
    priority_->collect(vars, false);
    mod_->collect(vars, false);
}

UHeadAggr HeuristicHeadAtom::rewriteAggregates(UBodyAggrVec &aggr) {
    aggr.emplace_back(gringo_make_unique<SimpleBodyLiteral>(make_locatable<PredicateLiteral>(atom_->loc(), NAF::POS, get_clone(atom_), true)));
    return nullptr;
}

bool HeuristicHeadAtom::simplify(Projections &project, SimplifyState &state, Logger &log) {
    static_cast<void>(project);
    return
        !atom_->simplify(state, false, false, log).update(atom_, false).undefined() &&
        !value_->simplify(state, false, false, log).update(value_, false).undefined() &&
        !priority_->simplify(state, false, false, log).update(priority_, false).undefined() &&
        !mod_->simplify(state, false, false, log).update(mod_, false).undefined();
}

void HeuristicHeadAtom::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxgen) {
    atom_->rewriteArithmetics(arith, auxgen);
}

void HeuristicHeadAtom::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    collect(vars);
    lvl.add(vars);
}

void HeuristicHeadAtom::check(ChkLvlVec &levels, Logger &log) const {
    static_cast<void>(log);
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

CreateHead HeuristicHeadAtom::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    static_cast<void>(x);
    static_cast<void>(stms);
    return CreateHead([&](Ground::ULitVec &&lits) {
        return gringo_make_unique<Ground::HeuristicStatement>(get_clone(atom_), get_clone(value_), get_clone(priority_), get_clone(mod_), std::move(lits));
    });
}

// {{{1 definition of ShowHeadLiteral

ShowHeadLiteral::ShowHeadLiteral(UTerm &&term)
: term_(std::move(term)) { }

ShowHeadLiteral::~ShowHeadLiteral() noexcept = default;

void ShowHeadLiteral::print(std::ostream &out) const {
    out << "#show " << *term_;
}

size_t ShowHeadLiteral::hash() const {
    return get_value_hash(typeid(ShowHeadLiteral).hash_code(), term_);
}

bool ShowHeadLiteral::operator==(HeadAggregate const &other) const {
    const auto *t = dynamic_cast<ShowHeadLiteral const *>(&other);
    return t != nullptr &&
           is_value_equal_to(term_, t->term_);
}

ShowHeadLiteral *ShowHeadLiteral::clone() const {
    return make_locatable<ShowHeadLiteral>(loc(), get_clone(term_)).release();
}

void ShowHeadLiteral::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    if (beforeRewrite) {
        for (auto &term : Gringo::unpool(term_)) {
            x.emplace_back(make_locatable<ShowHeadLiteral>(loc(), std::move(term)));
        }
    }
}

UHeadAggr ShowHeadLiteral::unpoolComparison(UBodyAggrVec &body) {
    static_cast<void>(body);
    return nullptr;
}

void ShowHeadLiteral::collect(VarTermBoundVec &vars) const {
    term_->collect(vars, false);
}

UHeadAggr ShowHeadLiteral::rewriteAggregates(UBodyAggrVec &aggr) {
    static_cast<void>(aggr);
    return nullptr;
}

bool ShowHeadLiteral::simplify(Projections &project, SimplifyState &state, Logger &log) {
    static_cast<void>(project);
    return !term_->simplify(state, false, false, log).update(term_, false).undefined();
}

void ShowHeadLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    // Note: nothing to do
    static_cast<void>(arith);
    static_cast<void>(auxGen);
}

void ShowHeadLiteral::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    collect(vars);
    lvl.add(vars);
}

void ShowHeadLiteral::check(ChkLvlVec &levels, Logger &log) const {
    static_cast<void>(log);
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

CreateHead ShowHeadLiteral::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    static_cast<void>(x);
    static_cast<void>(stms);
    return CreateHead([&](Ground::ULitVec &&lits) {
        return gringo_make_unique<Ground::ShowStatement>(get_clone(term_), std::move(lits));
    });
}

// }}}1

} } // namespace Input Gringo
