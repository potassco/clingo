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

namespace Gringo { namespace Input {

// {{{ definition of Aggregate::print

namespace {

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

} // namespace

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
void LitBodyAggregate::print(std::ostream &out) const {
    out << naf;
    _print(out, fun, bounds, elems, _printCond);
}
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
void SimpleBodyLiteral::print(std::ostream &out) const { lit->print(out); }
// head
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
void LitHeadAggregate::print(std::ostream &out) const {
    _print(out, fun, bounds, elems, _printCond);
}
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
void SimpleHeadLiteral::print(std::ostream &out) const { lit->print(out); }

void DisjointAggregate::print(std::ostream &out) const {
    out << naf << "#disjoint{";
    print_comma(out, elems, ";");
    out << "}";
}

// }}}
// {{{ definition of Aggregate::hash

size_t TupleBodyAggregate::hash() const {
    return get_value_hash(typeid(TupleBodyAggregate).hash_code(), size_t(naf), size_t(fun), bounds, elems);
}
size_t LitBodyAggregate::hash() const {
    return get_value_hash(typeid(LitBodyAggregate).hash_code(), size_t(naf), size_t(fun), bounds, elems);
}
size_t Conjunction::hash() const { 
    return get_value_hash(typeid(Conjunction).hash_code(), elems);
}
size_t SimpleBodyLiteral::hash() const { 
    return get_value_hash(typeid(SimpleBodyLiteral).hash_code(), lit);
}
size_t TupleHeadAggregate::hash() const {
    return get_value_hash(typeid(TupleHeadAggregate).hash_code(), size_t(fun), bounds, elems);
}
size_t LitHeadAggregate::hash() const {
    return get_value_hash(typeid(LitHeadAggregate).hash_code(), size_t(fun), bounds, elems);
}
size_t Disjunction::hash() const { 
    return get_value_hash(typeid(Disjunction).hash_code(), elems);
}
size_t SimpleHeadLiteral::hash() const { 
    return get_value_hash(typeid(SimpleHeadLiteral).hash_code(), lit);
}
size_t DisjointAggregate::hash() const { 
    return get_value_hash(typeid(DisjointAggregate).hash_code(), elems);
}

// }}}
// {{{ definition of Aggregate::operator==

bool TupleBodyAggregate::operator==(BodyAggregate const &x) const {
    auto t = dynamic_cast<TupleBodyAggregate const *>(&x);
    return t && naf == t->naf && fun == t->fun && is_value_equal_to(bounds, t->bounds) && is_value_equal_to(elems, t->elems);
}
bool LitBodyAggregate::operator==(BodyAggregate const &x) const {
    auto t = dynamic_cast<LitBodyAggregate const *>(&x);
    return t && naf == t->naf && fun == t->fun && is_value_equal_to(bounds, t->bounds) && is_value_equal_to(elems, t->elems);
}
bool Conjunction::operator==(BodyAggregate const &x) const { 
    auto t = dynamic_cast<Conjunction const *>(&x);
    return t && is_value_equal_to(elems, t->elems);
}
bool SimpleBodyLiteral::operator==(BodyAggregate const &x) const { 
    auto t = dynamic_cast<SimpleBodyLiteral const *>(&x);
    return t && is_value_equal_to(lit, t->lit);
}
bool TupleHeadAggregate::operator==(HeadAggregate const &x) const {
    auto t = dynamic_cast<TupleHeadAggregate const *>(&x);
    return t && fun == t->fun && is_value_equal_to(bounds, t->bounds) && is_value_equal_to(elems, t->elems);
}
bool LitHeadAggregate::operator==(HeadAggregate const &x) const {
    auto t = dynamic_cast<LitHeadAggregate const *>(&x);
    return t && fun == t->fun && is_value_equal_to(bounds, t->bounds) && is_value_equal_to(elems, t->elems);
}
bool Disjunction::operator==(HeadAggregate const &x) const { 
    auto t = dynamic_cast<Disjunction const *>(&x);
    return t && is_value_equal_to(elems, t->elems);
}
bool SimpleHeadLiteral::operator==(HeadAggregate const &x) const { 
    auto t = dynamic_cast<SimpleHeadLiteral const *>(&x);
    return t && is_value_equal_to(lit, t->lit);
}
bool DisjointAggregate::operator==(BodyAggregate const &x) const { 
    auto t = dynamic_cast<DisjointAggregate const *>(&x);
    return t && is_value_equal_to(elems, t->elems);
}

// }}}
// {{{ definition of Aggregate::clone

TupleBodyAggregate *TupleBodyAggregate::clone() const {
    return make_locatable<TupleBodyAggregate>(loc(), naf, removedAssignment, translated, fun, get_clone(bounds), get_clone(elems)).release();
}
LitBodyAggregate *LitBodyAggregate::clone() const {
    return make_locatable<LitBodyAggregate>(loc(), naf, fun, get_clone(bounds), get_clone(elems)).release();
}
Conjunction *Conjunction::clone() const { 
    return make_locatable<Conjunction>(loc(), get_clone(elems)).release();
}
SimpleBodyLiteral *SimpleBodyLiteral::clone() const { 
    return gringo_make_unique<SimpleBodyLiteral>(get_clone(lit)).release();
}
TupleHeadAggregate *TupleHeadAggregate::clone() const {
    return make_locatable<TupleHeadAggregate>(loc(), fun, translated, get_clone(bounds), get_clone(elems)).release();
}
LitHeadAggregate *LitHeadAggregate::clone() const {
    return make_locatable<LitHeadAggregate>(loc(), fun, get_clone(bounds), get_clone(elems)).release();
}
Disjunction *Disjunction::clone() const { 
    return make_locatable<Disjunction>(loc(), get_clone(elems)).release();
}
SimpleHeadLiteral *SimpleHeadLiteral::clone() const { 
    return gringo_make_unique<SimpleHeadLiteral>(get_clone(lit)).release();
}
DisjointAggregate *DisjointAggregate::clone() const { 
    return make_locatable<DisjointAggregate>(loc(), naf, get_clone(elems)).release();
}

// }}}
// {{{ definition of Aggregate::unpool

namespace {

std::function<ULitVec(ULit const &)> _unpool_lit(bool beforeRewrite) {
    return [beforeRewrite](ULit const &x) { return x->unpool(beforeRewrite); };
}
auto _unpool_bound = [](Bound &x) { return x.unpool(); };

} // namespace

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

void SimpleBodyLiteral::unpool(UBodyAggrVec &x, bool beforeRewrite) { 
    for (auto &y : lit->unpool(beforeRewrite)) { x.emplace_back(gringo_make_unique<SimpleBodyLiteral>(std::move(y))); }
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
void SimpleHeadLiteral::unpool(UHeadAggrVec &x, bool beforeRewrite) { 
    for (auto &y : lit->unpool(beforeRewrite)) { x.emplace_back(gringo_make_unique<SimpleHeadLiteral>(std::move(y))); }
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

// }}}
// {{{ definition of Aggregate::collect

void TupleBodyAggregate::collect(VarTermBoundVec &vars) const {
    for (auto &bound : bounds) { bound.bound->collect(vars, bound.rel == Relation::EQ && naf == NAF::POS); }
    for (auto &elem : elems) {
        for (auto &term : std::get<0>(elem)) { term->collect(vars, false); }
        for (auto &lit : std::get<1>(elem)) { lit->collect(vars, false); }
    }
}
void LitBodyAggregate::collect(VarTermBoundVec &vars) const {
    for (auto &bound : bounds) { bound.bound->collect(vars, bound.rel == Relation::EQ && naf == NAF::POS); }
    for (auto &elem : elems) {
        elem.first->collect(vars, false);
        for (auto &lit : elem.second) { lit->collect(vars, false); }
    }
}
void Conjunction::collect(VarTermBoundVec &vars) const {
    for (auto &elem : elems) {
        for (auto &disj : elem.first) { 
            for (auto &lit : disj) { lit->collect(vars, false); }
        }
        for (auto &lit : elem.second) { lit->collect(vars, false); }
    }
}
void SimpleBodyLiteral::collect(VarTermBoundVec &vars) const { lit->collect(vars, true); }

void TupleHeadAggregate::collect(VarTermBoundVec &vars) const {
    for (auto &bound : bounds) { bound.bound->collect(vars, false); }
    for (auto &elem : elems) {
        for (auto &term : std::get<0>(elem)) { term->collect(vars, false); }
        std::get<1>(elem)->collect(vars, false);
        for (auto &lit : std::get<2>(elem)) { lit->collect(vars, false); }
    }
}
void LitHeadAggregate::collect(VarTermBoundVec &vars) const {
    for (auto &bound : bounds) { bound.bound->collect(vars, false); }
    for (auto &elem : elems) {
        elem.first->collect(vars, false);
        for (auto &lit : elem.second) { lit->collect(vars, false); }
    }
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
void SimpleHeadLiteral::collect(VarTermBoundVec &vars) const { lit->collect(vars, true); }
void DisjointAggregate::collect(VarTermBoundVec &vars) const {
    for (auto &elem : elems) {
        for (auto &term : elem.tuple) { term->collect(vars, false); }
        elem.value.collect(vars);
        for (auto &lit : elem.cond) { lit->collect(vars, false); }
    }
}

// }}}
// {{{ definition of Aggregate::rewriteAggregates

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

bool Conjunction::rewriteAggregates(UBodyAggrVec &x) {
    while (elems.size() > 1) {
        ElemVec vec;
        vec.emplace_back(std::move(elems.back()));
        x.emplace_back(make_locatable<Conjunction>(loc(), std::move(vec)));
        elems.pop_back();
    }
    return !elems.empty();
}

bool SimpleBodyLiteral::rewriteAggregates(UBodyAggrVec &) { return true; }

bool DisjointAggregate::rewriteAggregates(UBodyAggrVec &) { return true; }

UHeadAggr TupleHeadAggregate::rewriteAggregates(UBodyAggrVec &) { 
    for (auto &x : elems) {
        if (ULit shifted = std::get<1>(x)->shift(false)) {
            // NOTE: FalseLiteral is a bad name
            std::get<1>(x) = make_locatable<FalseLiteral>(std::get<1>(x)->loc());
            std::get<2>(x).emplace_back(std::move(shifted));
        }
    }
    return nullptr;
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

UHeadAggr Disjunction::rewriteAggregates(UBodyAggrVec &) { 
    for (auto &elem : this->elems) {
        for (auto &head : elem.first) {
            if (ULit shifted = head.first->shift(true)) {
                head.first = make_locatable<FalseLiteral>(head.first->loc());
                head.second.emplace_back(std::move(shifted));
            }
        }
    }
    return nullptr;
}

UHeadAggr SimpleHeadLiteral::rewriteAggregates(UBodyAggrVec &aggr) { 
    ULit shifted(lit->shift(true));
    if (shifted) { 
        aggr.emplace_back(gringo_make_unique<SimpleBodyLiteral>(std::move(shifted)));
        return gringo_make_unique<SimpleHeadLiteral>(make_locatable<FalseLiteral>(lit->loc()));
    }
    return nullptr;
}

// }}}
// {{{ definition of Aggregate::simplify

bool TupleBodyAggregate::simplify(Projections &project, SimplifyState &state, bool) {
    for (auto &bound : bounds) { 
        if (!bound.simplify(state)) { return false; }
    }
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](BodyAggrElemVec::value_type &elem) {
        SimplifyState elemState(state);
        for (auto &term : std::get<0>(elem)) { 
            if (term->simplify(elemState, false, false).update(term).undefined()) { return true; }
        }
        for (auto &lit : std::get<1>(elem)) { 
            // NOTE: projection disabled with singelton=true
            if (!lit->simplify(project, elemState, true, true)) { return true; }
        }
        for (auto &dot : elemState.dots) { std::get<1>(elem).emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { std::get<1>(elem).emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}
bool LitBodyAggregate::simplify(Projections &project, SimplifyState &state, bool) {
    for (auto &bound : bounds) { 
        if (!bound.simplify(state)) { return false; }
    }
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](CondLitVec::value_type &elem) {
        SimplifyState elemState(state);
        if (!std::get<0>(elem)->simplify(project, elemState, true, true)) { return true; }
        for (auto &lit : std::get<1>(elem)) { 
            // NOTE: projection disabled with singelton=true
            if (!lit->simplify(project, elemState, true, true)) { return true; }
        }
        for (auto &dot : elemState.dots) { std::get<1>(elem).emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { std::get<1>(elem).emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}
bool Conjunction::simplify(Projections &project, SimplifyState &state, bool) { 
    for (auto &elem : elems) {
        elem.first.erase(std::remove_if(elem.first.begin(), elem.first.end(), [&](ULitVec &clause) {
            SimplifyState elemState(state);
            for (auto &lit : clause) {
                if (!lit->simplify(project, elemState)) { return true; }
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
            if (!lit->simplify(project, elemState, true, true)) { return true; }
        }
        for (auto &dot : elemState.dots) { elem.second.emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { elem.second.emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}
bool SimpleBodyLiteral::simplify(Projections &project, SimplifyState &state, bool singleton) {
    return lit->simplify(project, state, true, singleton);
}
bool TupleHeadAggregate::simplify(Projections &project, SimplifyState &state) {
    for (auto &bound : bounds) { 
        if (!bound.simplify(state)) { return false; }
    }
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](HeadAggrElemVec::value_type &elem) {
        SimplifyState elemState(state);
        for (auto &term : std::get<0>(elem)) {
            if (term->simplify(elemState, false, false).update(term).undefined()) { return true; }
        }
        if (!std::get<1>(elem)->simplify(project, elemState, false)) {
            return true; 
        }
        for (auto &lit : std::get<2>(elem)) { 
            if (!lit->simplify(project, elemState)) { return true; }
        }
        for (auto &dot : elemState.dots) { std::get<2>(elem).emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { std::get<2>(elem).emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}
bool LitHeadAggregate::simplify(Projections &project, SimplifyState &state) {
    for (auto &bound : bounds) { 
        if (!bound.simplify(state)) { return false; }
    }
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](CondLitVec::value_type &elem) {
        SimplifyState elemState(state);
        if (!std::get<0>(elem)->simplify(project, elemState, false)) { return true; }
        for (auto &lit : std::get<1>(elem)) { 
            if (!lit->simplify(project, elemState)) { return true; }
        }
        for (auto &dot : elemState.dots) { std::get<1>(elem).emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { std::get<1>(elem).emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}
bool Disjunction::simplify(Projections &project, SimplifyState &state) { 
    for (auto &elem : elems) {
        elem.first.erase(std::remove_if(elem.first.begin(), elem.first.end(), [&](Head &head) {
            SimplifyState elemState(state);
            if (!head.first->simplify(project, elemState)) { return true; }
            for (auto &lit : head.second) {
                if (!lit->simplify(project, elemState)) { return true; }
            }
            for (auto &dot : elemState.dots) { head.second.emplace_back(RangeLiteral::make(dot)); }
            for (auto &script : elemState.scripts) { head.second.emplace_back(ScriptLiteral::make(script)); }
            return false;
        }), elem.first.end());
    }
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](ElemVec::value_type &elem) -> bool {
        SimplifyState elemState(state);
        for (auto &lit : elem.second) { 
            if (!lit->simplify(project, elemState)) { return true; }
        }
        for (auto &dot : elemState.dots) { elem.second.emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { elem.second.emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}
bool SimpleHeadLiteral::simplify(Projections &project, SimplifyState &state) {
    return lit->simplify(project, state, false);
}
bool DisjointAggregate::simplify(Projections &project, SimplifyState &state, bool) {
    elems.erase(std::remove_if(elems.begin(), elems.end(), [&](CSPElemVec::value_type &elem) {
        SimplifyState elemState(state);
        for (auto &term : elem.tuple) { 
            if (term->simplify(elemState, false, false).update(term).undefined()) { return true; }
        }
        if (!elem.value.simplify(elemState)) { return true; }
        for (auto &lit : elem.cond) { 
            if (!lit->simplify(project, elemState)) { return true; }
        }
        for (auto &dot : elemState.dots) { elem.cond.emplace_back(RangeLiteral::make(dot)); }
        for (auto &script : elemState.scripts) { elem.cond.emplace_back(ScriptLiteral::make(script)); }
        return false;
    }), elems.end());
    return true;
}
// }}}
// {{{ definition of Aggregate::rewriteArithmetics

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
void SimpleBodyLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &assign, AuxGen &auxGen) {
    lit->rewriteArithmetics(arith, assign, auxGen);
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
void SimpleHeadLiteral::rewriteArithmetics(Term::ArithmeticsMap &, AuxGen &) { 
    // Note: nothing to do
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
// }}}
// {{{ definition of Aggregate::assignLevels

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
void SimpleBodyLiteral::assignLevels(AssignLevel &lvl) { 
    VarTermBoundVec vars;
    lit->collect(vars, false);
    lvl.add(vars);
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
void SimpleHeadLiteral::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    lit->collect(vars, false);
    lvl.add(vars);
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

// }}}
// {{{ definition of Aggregate::isEDB

Value HeadAggregate::isEDB() const     { return Value(); }
Value SimpleHeadLiteral::isEDB() const { return lit->isEDB(); }

// }}}
// {{{ definition of Aggregate::checkLevels

namespace {

// TODO: consider adding a more intelligent parameter to check that does all of this
void _add(ChkLvlVec &levels, VarTermBoundVec &vars) {
    for (auto &x: vars) { 
        auto &lvl(levels[x.first->level]);
        bool bind = x.second && levels.size() == x.first->level + 1;
        if (bind) { lvl.dep.insertEdge(*lvl.current, lvl.var(*x.first)); }
        else      { lvl.dep.insertEdge(lvl.var(*x.first), *lvl.current); }
    }
}

void _add(ChkLvlVec &levels, ULit const &lit, bool bind) {
    VarTermBoundVec vars;
    levels.back().current = &levels.back().dep.insertEnt();
    lit->collect(vars, bind);
    _add(levels, vars);
}

void _add(ChkLvlVec &levels, UTermVec const &terms, CSPAddTerm const *term = nullptr) {
    VarTermBoundVec vars;
    levels.back().current = &levels.back().dep.insertEnt();
    for (auto &x : terms)  { x->collect(vars, false); }
    if (term) { term->collect(vars); }
    _add(levels, vars);
}


void _add(ChkLvlVec &levels, ULitVec const &cond) {
    for (auto &x : cond) { _add(levels, x, true); }
}

template <class T>
bool _aggr(ChkLvlVec &levels, BoundVec const &bounds, T const &f, bool bind) {
    bool ret = true;
    bool assign = false;
    CheckLevel::SC::EntNode *depend = 0;
    for (auto &y : bounds) {
        if (bind && y.rel == Relation::EQ) {
            levels.back().current = &levels.back().dep.insertEnt();
            VarTermBoundVec vars;
            y.bound->collect(vars, true);
            _add(levels, vars);
            ret = f() && ret;
            assign = true;
        }
        else { 
            if (!depend) { depend = &levels.back().dep.insertEnt(); }
            levels.back().current = depend;
            VarTermBoundVec vars;
            y.bound->collect(vars, false);
            _add(levels, vars);
        }
    }
    if (!depend && !assign) { depend = &levels.back().dep.insertEnt(); }
    if (depend) {
        levels.back().current = depend;
        ret = f() && ret;
    }
    return ret;
}

void warnGlobal(VarTermBoundVec &vars, bool warn) {
    if (warn) {
        auto ib = vars.begin(), ie = vars.end();
        ie = std::remove_if(ib, ie, [](VarTermBoundVec::value_type const &a) { return a.first->level > 0; });
        std::sort(ib, ie, [](VarTermBoundVec::value_type const &a, VarTermBoundVec::value_type const &b) { return a.first->name < b.first->name; });
        ie = std::unique(ib, ie, [](VarTermBoundVec::value_type const &a, VarTermBoundVec::value_type const &b) { return a.first->name == b.first->name; });
        for (auto it = ib; it != ie; ++it) {
            GRINGO_REPORT(W_GLOBAL_VARIABLE)
                << it->first->loc() << ": info: global variable in tuple of aggregate element:\n"
                << "  " << it->first->name << "\n"
                ;
        }
    }
}

} // namespace

bool TupleBodyAggregate::check(ChkLvlVec &levels) const {
    auto f = [&]() -> bool {
        bool ret = true;
        VarTermBoundVec vars;
        for (auto &y : elems) {
            levels.emplace_back(loc(), *this);
            _add(levels, y.first);
            _add(levels, y.second);
            ret = levels.back().check() && ret;
            levels.pop_back();
            for (auto &term : y.first) { term->collect(vars, false); }
        }
        warnGlobal(vars, !translated);
        return ret;
    };
    return _aggr(levels, bounds, f, naf == NAF::POS);
}
bool LitBodyAggregate::check(ChkLvlVec &levels) const {
    auto f = [&]() -> bool {
        bool ret = true;
        for (auto &y : elems) {
            levels.emplace_back(loc(), *this);
            _add(levels, y.first, false);
            _add(levels, y.second);
            ret = levels.back().check() && ret;
            levels.pop_back();
        }
        return ret;
    };
    return _aggr(levels, bounds, f, naf == NAF::POS);
}
bool Conjunction::check(ChkLvlVec &levels) const {
    bool ret = true;
    levels.back().current = &levels.back().dep.insertEnt();
    for (auto &elem : elems) {
        levels.emplace_back(loc(), *this);
        _add(levels, elem.second);
        // check safety of condition
        bool check = levels.back().check();
        levels.pop_back();
        if (check) {
            for (auto &disj : elem.first) {
                levels.emplace_back(loc(), *this);
                _add(levels, disj);
                _add(levels, elem.second);
                // check safty of head it can contain pure variables!
                ret = levels.back().check() && ret;
                levels.pop_back();
            }
        }
        else { ret = false; }
    }
    return ret;
}
bool SimpleBodyLiteral::check(ChkLvlVec &levels) const { 
    _add(levels, lit, true);
    return true;
}
bool TupleHeadAggregate::check(ChkLvlVec &levels) const {
    auto f = [&]() -> bool {
        bool ret = true;
        VarTermBoundVec vars;
        for (auto &y : elems) {
            levels.emplace_back(loc(), *this);
            _add(levels, std::get<0>(y));
            _add(levels, std::get<1>(y), false);
            _add(levels, std::get<2>(y));
            ret = levels.back().check() && ret;
            levels.pop_back();
            for (auto &term : std::get<0>(y)) { term->collect(vars, false); }
        }
        warnGlobal(vars, !translated);
        return ret;
    };
    return _aggr(levels, bounds, f, false);
}
bool LitHeadAggregate::check(ChkLvlVec &levels) const {
    auto f = [&]() -> bool {
        bool ret = true;
        for (auto &y : elems) {
            levels.emplace_back(loc(), *this);
            _add(levels, y.first, false);
            _add(levels, y.second);
            ret = levels.back().check() && ret;
            levels.pop_back();
        }
        return ret;
    };
    return _aggr(levels, bounds, f, false);
}
bool Disjunction::check(ChkLvlVec &levels) const {
    bool ret = true;
    levels.back().current = &levels.back().dep.insertEnt();
    for (auto &y : elems) {
        levels.emplace_back(loc(), *this);
        _add(levels, y.second);
        // check condition first 
        // (although by construction it cannot happen that something in expand binds something in condition)
        bool check = levels.back().check();
        levels.pop_back();
        if (check) {
            for (auto &head : y.first) {
                levels.emplace_back(loc(), *this);
                _add(levels, head.first, false);
                _add(levels, head.second);
                _add(levels, y.second);
                ret = levels.back().check() && ret;
                levels.pop_back();
            }
        }
        else { ret = false; }
    }
    return ret;
}
bool SimpleHeadLiteral::check(ChkLvlVec &levels) const {
    levels.back().current = &levels.back().dep.insertEnt();
    _add(levels, lit, false);
    return true;
}
bool DisjointAggregate::check(ChkLvlVec &levels) const {
    auto f = [&]() -> bool {
        bool ret = true;
        for (auto &y : elems) {
            levels.emplace_back(loc(), *this);
            _add(levels, y.tuple, &y.value);
            _add(levels, y.cond);
            ret = levels.back().check() && ret;
            levels.pop_back();
        }
        return ret;
    };
    return _aggr(levels, {}, f, false);
}

// }}}
// {{{ definition of Aggregate::hasPool

bool TupleBodyAggregate::hasPool(bool beforeRewrite) const {
    for (auto &bound : bounds) { if (bound.bound->hasPool()) { return true; } }
    for (auto &elem : elems) {
        for (auto &term : elem.first) { if (term->hasPool()) { return true; } }
        for (auto &lit : elem.second) { if (lit->hasPool(beforeRewrite)) { return true; } }
    }
    return false;
}
bool LitBodyAggregate::hasPool(bool beforeRewrite) const {
    for (auto &bound : bounds) { if (bound.bound->hasPool()) { return true; } }
    for (auto &elem : elems) {
        if (elem.first->hasPool(beforeRewrite)) { return true; }
        for (auto &lit : elem.second) { if (lit->hasPool(beforeRewrite)) { return true; } }
    }
    return false;
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
bool SimpleBodyLiteral::hasPool(bool beforeRewrite) const { 
    return lit->hasPool(beforeRewrite);
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
bool LitHeadAggregate::hasPool(bool beforeRewrite) const {
    for (auto &bound : bounds) { if (bound.bound->hasPool()) { return true; } }
    for (auto &elem : elems) {
        if (elem.first->hasPool(beforeRewrite)) { return true; }
        for (auto &lit : elem.second) { if (lit->hasPool(beforeRewrite)) { return true; } }
    }
    return false;
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
bool SimpleHeadLiteral::hasPool(bool beforeRewrite) const { 
    return lit->hasPool(beforeRewrite);
}
bool DisjointAggregate::hasPool(bool beforeRewrite) const {
    for (auto &elem : elems) {
        for (auto &term : elem.tuple) { if (term->hasPool()) { return true; } }
        if (elem.value.hasPool()) { return true; }
        for (auto &lit : elem.cond) { if (lit->hasPool(beforeRewrite)) { return true; } }
    }
    return false;
}

// }}}
// {{{ definition of Aggregate::replace

void TupleBodyAggregate::replace(Defines &x) {
    for (auto &bound : bounds) { Term::replace(bound.bound, bound.bound->replace(x, true)); }
    for (auto &elem : elems) {
        for (auto &y : elem.first)  { Term::replace(y, y->replace(x, true)); }
        for (auto &y : elem.second) { y->replace(x); }
    }
}
void LitBodyAggregate::replace(Defines &x) {
    for (auto &bound : bounds) { Term::replace(bound.bound, bound.bound->replace(x, true)); }
    for (auto &elem : elems) {
        elem.first->replace(x);
        for (auto &y : elem.second) { y->replace(x); }
    }
}
void Conjunction::replace(Defines &x) { 
    for (auto &elem : elems) {
        for (auto &y : elem.first) { 
            for (auto &z : y) { z->replace(x); }
        }
        for (auto &y : elem.second) { y->replace(x); }
    }
}
void SimpleBodyLiteral::replace(Defines &x) {
    lit->replace(x);
}
void TupleHeadAggregate::replace(Defines &x) {
    for (auto &bound : bounds) { Term::replace(bound.bound, bound.bound->replace(x, true)); }
    for (auto &elem : elems) {
        for (auto &y : std::get<0>(elem)) { Term::replace(y, y->replace(x, true)); }
        std::get<1>(elem)->replace(x);
        for (auto &y : std::get<2>(elem)) { y->replace(x); }
    }
}
void LitHeadAggregate::replace(Defines &x) {
    for (auto &bound : bounds) { Term::replace(bound.bound, bound.bound->replace(x, true)); }
    for (auto &elem : elems) {
        elem.first->replace(x);
        for (auto &y : elem.second) { y->replace(x); }
    }
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
void SimpleHeadLiteral::replace(Defines &x) { 
    lit->replace(x);
}
void DisjointAggregate::replace(Defines &x) { 
    for (auto &elem : elems) {
        for (auto &y : elem.tuple) { y->replace(x, true); }
        elem.value.replace(x);
        for (auto &y : elem.cond) { y->replace(x); }
    }
}

// }}}
// {{{ definition of Aggregate::toGround

CreateBody TupleBodyAggregate::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    if (!isAssignment()) {
        stms.emplace_back(gringo_make_unique<Ground::BodyAggregateComplete>(x.newId(*this), fun, get_clone(bounds)));
        auto &completeRef = static_cast<Ground::BodyAggregateComplete&>(*stms.back());
        CreateStmVec split;
        split.emplace_back([&completeRef, this](Ground::ULitVec &&auxLits) -> Ground::UStm {
            UTermVec tuple;
            tuple.emplace_back(make_locatable<ValTerm>(loc(), Value()));
            // Note: should this become a function?
            UTerm neutral;
            switch (fun) {
                case AggregateFunction::MIN: { neutral = make_locatable<ValTerm>(loc(), Value::createSup());  break; }
                case AggregateFunction::MAX: { neutral = make_locatable<ValTerm>(loc(), Value::createInf());  break; }
                default:                     { neutral = make_locatable<ValTerm>(loc(), Value::createNum(0)); break; }
            }
            for (auto &y : bounds) { auxLits.emplace_back(gringo_make_unique<Ground::RelationLiteral>(y.rel, get_clone(neutral), get_clone(y.bound))); }
            auto ret = gringo_make_unique<Ground::BodyAggregateAccumulate>(completeRef, get_clone(tuple), Ground::ULitVec(), std::move(auxLits));
            completeRef.accuDoms.emplace_back(*ret);
            return std::move(ret);
        });
        for (auto &y : elems) { 
            split.emplace_back([this,&completeRef,&y,&x](Ground::ULitVec &&auxLits) -> Ground::UStm {
                Ground::ULitVec lits;
                for (auto &z : y.second) { lits.emplace_back(z->toGround(x.domains)); }
                auto ret = gringo_make_unique<Ground::BodyAggregateAccumulate>(completeRef, get_clone(y.first), std::move(lits), std::move(auxLits));
                completeRef.accuDoms.emplace_back(*ret);
                return std::move(ret);
            });
        }
        return CreateBody([&completeRef, this](Ground::ULitVec &lits, bool primary) {
            if (primary) { lits.emplace_back(gringo_make_unique<Ground::BodyAggregateLiteral>(completeRef, naf)); }
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
        
        stms.emplace_back(gringo_make_unique<Ground::AssignmentAggregateComplete>(get_clone(repr), get_clone(dataRepr), fun));
        auto &completeRef = static_cast<Ground::AssignmentAggregateComplete&>(*stms.back());
        // NOTE: for assignment aggregates this does not make much sense
        //       the empty aggregate always matches and hence the elements 
        //       should be grounded without auxLits
        CreateStmVec split;
        split.emplace_back([&completeRef, this](Ground::ULitVec &&auxLits) -> Ground::UStm {
            UTermVec tuple;
            tuple.emplace_back(make_locatable<ValTerm>(loc(), Value()));
            auto ret = gringo_make_unique<Ground::AssignmentAggregateAccumulate>(completeRef, get_clone(tuple), Ground::ULitVec(), std::move(auxLits));
            completeRef.accuDoms.emplace_back(*ret);
            return std::move(ret);
        });
        for (auto &y : elems) { 
            split.emplace_back([this,&completeRef,&y,&x](Ground::ULitVec &&auxLits) -> Ground::UStm {
                Ground::ULitVec lits;
                for (auto &z : y.second) { lits.emplace_back(z->toGround(x.domains)); }
                auto ret = gringo_make_unique<Ground::AssignmentAggregateAccumulate>(completeRef, get_clone(y.first), std::move(lits), std::move(auxLits));
                completeRef.accuDoms.emplace_back(*ret);
                return std::move(ret);
            });
        }
        return CreateBody([&completeRef, this](Ground::ULitVec &lits, bool primary) {
            if (primary) { lits.emplace_back(gringo_make_unique<Ground::AssignmentAggregateLiteral>(completeRef)); }
        }, std::move(split));
    }
}
CreateBody LitBodyAggregate::toGround(ToGroundArg &, Ground::UStmVec &) const {
    throw std::logic_error("Aggregate::rewriteAggregates must be called before LitAggregate::toGround");
}
CreateBody Conjunction::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    assert(elems.size() == 1);
    
    UTermVec local;
    std::unordered_set<FWString> seen;
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
    stms.emplace_back(gringo_make_unique<Ground::ConjunctionComplete>(x.newId(*this), std::move(local)));
    auto &completeRef = static_cast<Ground::ConjunctionComplete&>(*stms.back());

    Ground::ULitVec condLits;
    for (auto &y : elems.front().second) { condLits.emplace_back(y->toGround(x.domains)); }
    stms.emplace_back(gringo_make_unique<Ground::ConjunctionAccumulateCond>(completeRef, std::move(condLits)));

    for (auto &y : elems.front().first)  { 
        Ground::ULitVec headLits;    
        for (auto &z : y) {
            headLits.emplace_back(z->toGround(x.domains)); 
        }
        stms.emplace_back(gringo_make_unique<Ground::ConjunctionAccumulateHead>(completeRef, std::move(headLits)));
    }

    CreateStmVec split;
    split.emplace_back([&completeRef](Ground::ULitVec &&auxLits) -> Ground::UStm {
        auto ret = gringo_make_unique<Ground::ConjunctionAccumulateEmpty>(completeRef, std::move(auxLits));
        return std::move(ret);
    });

    return CreateBody([&completeRef](Ground::ULitVec &lits, bool primary) {
        if (primary) { lits.emplace_back(gringo_make_unique<Ground::ConjunctionLiteral>(completeRef)); }
    }, std::move(split));
}
CreateBody SimpleBodyLiteral::toGround(ToGroundArg &x, Ground::UStmVec &) const {
    return {[&](Ground::ULitVec &lits, bool) -> void {
        lits.emplace_back(lit->toGround(x.domains));
    }, CreateStmVec()};
}
CreateBody DisjointAggregate::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    stms.emplace_back(gringo_make_unique<Ground::DisjointComplete>(x.newId(*this)));
    auto &completeRef = static_cast<Ground::DisjointComplete&>(*stms.back());
    CreateStmVec split;
    split.emplace_back([&completeRef, this](Ground::ULitVec &&auxLits) -> Ground::UStm {
        auto ret = gringo_make_unique<Ground::DisjointAccumulate>(completeRef, Ground::ULitVec(), std::move(auxLits));
        completeRef.accuDoms.emplace_back(*ret);
        return std::move(ret);
    });
    for (auto &y : elems) { 
        split.emplace_back([this,&completeRef,&y,&x](Ground::ULitVec &&auxLits) -> Ground::UStm {
            Ground::ULitVec lits;
            for (auto &z : y.cond) { lits.emplace_back(z->toGround(x.domains)); }
            auto ret = gringo_make_unique<Ground::DisjointAccumulate>(completeRef, get_clone(y.tuple), get_clone(y.value), std::move(lits), std::move(auxLits));
            completeRef.accuDoms.emplace_back(*ret);
            return std::move(ret);
        });
    }
    return CreateBody([&completeRef, this](Ground::ULitVec &lits, bool primary) {
        if (primary) { lits.emplace_back(gringo_make_unique<Ground::DisjointLiteral>(completeRef, naf)); }
    }, std::move(split));
}
CreateHead TupleHeadAggregate::toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType) const {
    auto rule = std::make_shared<std::unique_ptr<Ground::HeadAggregateRule>>(nullptr);
    *rule = gringo_make_unique<Ground::HeadAggregateRule>(x.newId(*this), fun, get_clone(bounds), Ground::ULitVec{});
    stms.emplace_back(gringo_make_unique<Ground::HeadAggregateComplete>(**rule));
    auto &completeRef = static_cast<Ground::HeadAggregateComplete&>(*stms.back());
    unsigned elemIndex = 0;
    for (auto &y : elems) {
        Ground::ULitVec lits;
        for (auto &z : std::get<2>(y)) { lits.emplace_back(z->toGround(x.domains)); }
        lits.emplace_back(gringo_make_unique<Ground::HeadAggregateLiteral>(**rule));
        UTerm headRep(std::get<1>(y)->headRepr());
        PredicateDomain *pred = headRep ? &add(x.domains, headRep->getSig()) : nullptr;
        auto ret = gringo_make_unique<Ground::HeadAggregateAccumulate>(**rule, elemIndex, get_clone(std::get<0>(y)), pred, std::move(headRep), std::move(lits));
        completeRef.accuDoms.emplace_back(*ret);
        stms.emplace_back(std::move(ret));
        elemIndex++;
    }
    return CreateHead([rule](Ground::ULitVec &&lits) { rule->get()->lits = std::move(lits); return std::move(*rule); });
}
CreateHead LitHeadAggregate::toGround(ToGroundArg &, Ground::UStmVec &, Ground::RuleType) const {
    throw std::runtime_error("Aggregate::rewriteAggregates must be called before LitAggregate::toGround");
}
CreateHead Disjunction::toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType) const {
    stms.emplace_back(gringo_make_unique<Ground::DisjunctionComplete>(x.newId(*this)));
    auto &complete = static_cast<Ground::DisjunctionComplete&>(*stms.back());

    unsigned elemIndex = 0;
    int headIndex = 0;
    for (auto &y : elems) {
        Ground::ULitVec body;
        for (auto &z : y.second) { body.emplace_back(z->toGround(x.domains)); }

        UTermVec local;
        std::unordered_set<FWString> seen;
        VarTermBoundVec varsBody;
        for (auto &x : y.second) { x->collect(varsBody, false); }
        for (auto &occ : varsBody) {
            if (occ.first->level != 0) {
                seen.emplace(occ.first->name);
            }
        }
        VarTermBoundVec varsHead;
        for (auto &head : y.first) {
            head.first->collect(varsHead, false);
            for (auto &x : head.second) { x->collect(varsHead, false); }
        }
        for (auto &occ : varsHead) {
            if (occ.first->level != 0 && seen.find(occ.first->name) != seen.end()) {
                local.emplace_back(occ.first->clone());
            }
        }
        complete.appendLocal(std::move(local));

        stms.emplace_back(gringo_make_unique<Ground::DisjunctionAccumulateCond>(complete, elemIndex, std::move(body)));
        for (auto &head : y.first) {
            VarTermBoundVec varsHead;
            head.first->collect(varsHead, false);
            for (auto &x : head.second) { x->collect(varsHead, false); }
            for (auto &occ : varsHead) {
                if (occ.first->level != 0 && seen.find(occ.first->name) != seen.end()) {
                    local.emplace_back(occ.first->clone());
                }
            }

            Ground::ULitVec expand;
            for (auto &z : head.second) { expand.emplace_back(z->toGround(x.domains)); }
            UTerm headRepr = head.first->headRepr();
            if (headRepr) {
                PredicateDomain &headDom = add(x.domains, headRepr->getSig());
                complete.appendHead(headDom, std::move(headRepr));
                stms.emplace_back(gringo_make_unique<Ground::DisjunctionAccumulateHead>(complete, elemIndex, headIndex, std::move(expand)));
                ++headIndex;
            }
            else {
                stms.emplace_back(gringo_make_unique<Ground::DisjunctionAccumulateHead>(complete, elemIndex, -1, std::move(expand)));
            }
        }
        ++elemIndex;
    }
    
    return CreateHead([&](Ground::ULitVec &&lits) { return gringo_make_unique<Ground::DisjunctionRule>(complete, std::move(lits)); });
}
CreateHead SimpleHeadLiteral::toGround(ToGroundArg &x, Ground::UStmVec &, Ground::RuleType type) const {
    return
        {[this, &x, type](Ground::ULitVec &&lits) -> Ground::UStm {
            if (UTerm headRepr = lit->headRepr()) {
                FWSignature sig(headRepr->getSig());
                return gringo_make_unique<Ground::Rule>(&add(x.domains, sig), std::move(headRepr), std::move(lits), type);
            }
            else {
                return gringo_make_unique<Ground::Rule>(nullptr, nullptr, std::move(lits), type);
            }
        }};
}

// }}}

// {{{ definition of *BodyAggregate::isAssignment

bool TupleBodyAggregate::isAssignment() const { return !removedAssignment && bounds.size() == 1 && naf == NAF::POS && bounds.front().rel == Relation::EQ && bounds.front().bound->getInvertibility() == Term::INVERTIBLE; }
bool LitBodyAggregate::isAssignment() const   { return false; }
bool Conjunction::isAssignment() const        { return false; }
bool SimpleBodyLiteral::isAssignment() const  { return false; }
bool DisjointAggregate::isAssignment() const  { return false; }

// }}}
// {{{ definition of *BodyAggregate::removeAssignment

void TupleBodyAggregate::removeAssignment() { 
    assert (isAssignment());
    removedAssignment = true;
}
void LitBodyAggregate::removeAssignment()  { }
void Conjunction::removeAssignment()       { }
void SimpleBodyLiteral::removeAssignment() { }
void DisjointAggregate::removeAssignment() { }

// }}}

// {{{ definition of TupleBodyAggregate

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

// }}}
// {{{ definition of LitBodyAggregate

LitBodyAggregate::LitBodyAggregate(NAF naf, AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems)
    : naf(naf)
    , fun(fun)
    , bounds(std::move(bounds))
    , elems(std::move(elems)) { }

LitBodyAggregate::~LitBodyAggregate() { }

// }}}
// {{{ definition of Conjunction

Conjunction::Conjunction(ULit &&head, ULitVec &&cond) {
    elems.emplace_back(ULitVecVec(), std::move(cond));
    elems.back().first.emplace_back();
    elems.back().first.back().emplace_back(std::move(head));
}

Conjunction::Conjunction(ElemVec &&elems) 
: elems(std::move(elems)) { }

Conjunction::~Conjunction() { }

// }}}
// {{{ definition of SimpleBodyLiteral

SimpleBodyLiteral::SimpleBodyLiteral(ULit &&lit)
    : lit(std::move(lit)) { }

Location const &SimpleBodyLiteral::loc() const { return lit->loc(); }
void SimpleBodyLiteral::loc(Location const &loc) { lit->loc(loc); }
SimpleBodyLiteral::~SimpleBodyLiteral() { }

// }}}

// {{{ definition of TupleHeadAggregate

TupleHeadAggregate::TupleHeadAggregate(AggregateFunction fun, BoundVec &&bounds, HeadAggrElemVec &&elems)
    : TupleHeadAggregate(fun, false, std::move(bounds), std::move(elems)) { }

TupleHeadAggregate::TupleHeadAggregate(AggregateFunction fun, bool translated, BoundVec &&bounds, HeadAggrElemVec &&elems)
    : fun(fun)
    , translated(translated)
    , bounds(std::move(bounds))
    , elems(std::move(elems)) { }

TupleHeadAggregate::~TupleHeadAggregate() { }

// }}}
// {{{ definition of LitHeadAggregate

LitHeadAggregate::LitHeadAggregate(AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems)
    : fun(fun)
    , bounds(std::move(bounds))
    , elems(std::move(elems)) { }

LitHeadAggregate::~LitHeadAggregate() { }

// }}}
// {{{ definition of Disjunction

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

// }}}
// {{{ definition of SimpleHeadLiteral

SimpleHeadLiteral::SimpleHeadLiteral(ULit &&lit)
    : lit(std::move(lit)) { }

Location const &SimpleHeadLiteral::loc() const { return lit->loc(); }
void SimpleHeadLiteral::loc(Location const &loc) { lit->loc(loc); }
SimpleHeadLiteral::~SimpleHeadLiteral() { }

// }}}
// {{{ definition of SimpleHeadLiteral

DisjointAggregate::DisjointAggregate(NAF naf, CSPElemVec &&elems)
    : naf(naf)
    , elems(std::move(elems)) { }

DisjointAggregate::~DisjointAggregate() { }

// }}}

// definition of CSPElem {{{

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

// }}}


} } // namespace Input Gringo

