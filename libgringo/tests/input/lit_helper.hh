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
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
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
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    UCSPLit lit(make_locatable<CSPLiteral>(loc, rel, std::move(left), std::move(right)));
    csplit(*lit, std::forward<T>(args)...);
    return lit;
}

inline ULit pred(NAF naf, UTerm &&arg) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<PredicateLiteral>(loc, naf, std::move(arg));
}

inline ULit rel(Relation rel, UTerm &&left, UTerm &&right) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<RelationLiteral>(loc, rel, std::move(left), std::move(right));
}

inline ULit range(UTerm &&assign, UTerm &&lower, UTerm &&upper) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<RangeLiteral>(loc, std::move(assign), std::move(lower), std::move(upper));
}

template <class... T>
ULitVec litvec(T&&... args) {
    return init<ULitVec>(std::forward<T>(args)...);
}

// }}}

} } } // namespace Test Input Gringo

#endif // _GRINGO_TEST_LIT_HELPER_HH
