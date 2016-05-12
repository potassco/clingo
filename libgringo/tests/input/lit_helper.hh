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

#ifndef _GRINGO_TEST_LIT_HELPER_HH
#define _GRINGO_TEST_LIT_HELPER_HH

#include "tests/tests.hh"
#include "gringo/input/literals.hh"

namespace Gringo { namespace Input { namespace Test {

using namespace Gringo::Test;

// {{{ definition of functions to create literals

inline CSPMulTerm cspmul(UTerm &&coe, UTerm &&var = nullptr) {
    return CSPMulTerm(std::move(var), std::move(coe));
}

inline void cspadd(CSPAddTerm &) { }

template <class... T>
inline void cspadd(CSPAddTerm &add, CSPMulTerm &&mul, T&&... args) {
    add.append(std::move(mul));
    cspadd(add, std::forward<T>(args)...);
}

template <class... T>
inline CSPAddTerm cspadd(CSPMulTerm &&mul, T&&... args) {
    Location loc(FWString("dummy"), 1, 1, FWString("dummy"), 1, 1);
    CSPAddTerm add(std::move(mul));
    cspadd(add, std::forward<T>(args)...);
    return add;
}

inline void csplit(CSPLiteral &) { }

template <class... T>
inline void csplit(CSPLiteral &lit, Relation rel, CSPAddTerm &&add, T&&... args) {
    lit.append(rel, std::move(add));
    csplit(lit, std::forward<T>(args)...);
}

template <class... T>
inline UCSPLit csplit(CSPAddTerm &&left, Relation rel, CSPAddTerm &&right, T&&... args) {
    Location loc(FWString("dummy"), 1, 1, FWString("dummy"), 1, 1);
    UCSPLit lit(make_locatable<CSPLiteral>(loc, rel, std::move(left), std::move(right)));
    csplit(*lit, std::forward<T>(args)...);
    return lit;
}

inline ULit pred(NAF naf, UTerm &&arg) {
    Location loc(FWString("dummy"), 1, 1, FWString("dummy"), 1, 1);
    return make_locatable<PredicateLiteral>(loc, naf, std::move(arg));
}

inline ULit rel(Relation rel, UTerm &&left, UTerm &&right) {
    Location loc(FWString("dummy"), 1, 1, FWString("dummy"), 1, 1);
    return make_locatable<RelationLiteral>(loc, rel, std::move(left), std::move(right));
}

inline ULit range(UTerm &&assign, UTerm &&lower, UTerm &&upper) {
    Location loc(FWString("dummy"), 1, 1, FWString("dummy"), 1, 1);
    return make_locatable<RangeLiteral>(loc, std::move(assign), std::move(lower), std::move(upper));
}

template <class... T>
ULitVec litvec(T&&... args) {
    return init<ULitVec>(std::forward<T>(args)...);
}

// }}}

} } } // namespace Test Input Gringo

#endif // _GRINGO_TEST_LIT_HELPER_HH
