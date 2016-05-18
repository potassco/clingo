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

#ifndef _GRINGO_TEST_AGGREGATE_HELPER_HH
#define _GRINGO_TEST_AGGREGATE_HELPER_HH

#include "tests/tests.hh"
#include "gringo/input/aggregates.hh"

namespace Gringo { namespace Input { namespace Test {

using namespace Gringo::Test;

// {{{ definition of functions to create aggregates

template <class... T>
BoundVec boundvec(T&&... args) {
    return init<2, BoundVec>(std::forward<T>(args)...);
}

template <class... T>
CondLitVec condlitvec(T&&... args) {
    return init<2,CondLitVec>(std::forward<T>(args)...);
}

template <class... T>
BodyAggrElemVec bdelemvec(T&&... args) {
    return init<2,BodyAggrElemVec>(std::forward<T>(args)...);
}

template <class... T>
HeadAggrElemVec hdelemvec(T&&... args) {
    return init<3,HeadAggrElemVec>(std::forward<T>(args)...);
}

inline UBodyAggr bdaggr(NAF naf, AggregateFunction fun, BoundVec &&bounds, BodyAggrElemVec &&elems) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<TupleBodyAggregate>(loc, naf, fun, std::move(bounds), std::move(elems));
}

inline UBodyAggr bdaggr(NAF naf, AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<LitBodyAggregate>(loc, naf, fun, std::move(bounds), std::move(elems));
}

inline UBodyAggr bdaggr(ULit &&lit, ULitVec &&cond) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<Conjunction>(loc, std::move(lit), std::move(cond));
}

inline UHeadAggr hdaggr(AggregateFunction fun, BoundVec &&bounds, HeadAggrElemVec &&elems) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<TupleHeadAggregate>(loc, fun, std::move(bounds), std::move(elems));
}

inline UHeadAggr hdaggr(AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<LitHeadAggregate>(loc, fun, std::move(bounds), std::move(elems));
}

inline UHeadAggr hdaggr(CondLitVec &&elems) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<Disjunction>(loc, std::move(elems));
}

// }}}

} } } // namespace Test Input Gringo

#endif // _GRINGO_TEST_AGGREGATE_HELPER_HH


