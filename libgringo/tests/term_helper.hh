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

#ifndef _GRINGO_TEST_TERM_HELPER_HH
#define _GRINGO_TEST_TERM_HELPER_HH

#include "tests/tests.hh"
#include "gringo/term.hh"

namespace Gringo { namespace Test {

// {{{ definition of functions to create terms

inline std::unique_ptr<ValTerm> val(Symbol v) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<ValTerm>(loc, v);
}

inline std::unique_ptr<VarTerm> var(char const *x, int level = 0) {
    static std::unordered_map<String, Term::SVal> vals;
    auto &ret(vals[x]);
    if (!ret) { ret = std::make_shared<Symbol>(); }
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    auto var(make_locatable<VarTerm>(loc, String(x), ret));
    var->level = level;
    return var;
}

inline std::unique_ptr<LinearTerm> lin(char const *x, int m, int n) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<LinearTerm>(loc, *var(x), m, n);
}

inline std::unique_ptr<DotsTerm> dots(UTerm &&left, UTerm &&right) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<DotsTerm>(loc, std::move(left), std::move(right));
}

inline std::unique_ptr<UnOpTerm> unop(UnOp op, UTerm &&arg) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<UnOpTerm>(loc, op, std::move(arg));
}

inline std::unique_ptr<BinOpTerm> binop(BinOp op, UTerm &&left, UTerm &&right) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<BinOpTerm>(loc, op, std::move(left), std::move(right));
}

template <class... T>
std::unique_ptr<FunctionTerm> fun(char const *name, T&&... args) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<FunctionTerm>(loc, String(name), Test::init<UTermVec>(std::forward<T>(args)...));
}

template <class... T>
std::unique_ptr<LuaTerm> lua(char const *name, T&&... args) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<LuaTerm>(loc, String(name), Test::init<UTermVec>(std::forward<T>(args)...));
}

template <class... T>
std::unique_ptr<PoolTerm> pool(T&&... args) {
    Location loc(String("dummy"), 1, 1, String("dummy"), 1, 1);
    return make_locatable<PoolTerm>(loc, Test::init<UTermVec>(std::forward<T>(args)...));
}

template <class... T>
UTermVec termvec(T&&... args) {
    return Test::init<UTermVec>(std::forward<T>(args)...);
}

inline Symbol NUM(int num) { return Symbol::createNum(num); }
inline Symbol SUP() { return Symbol::createSup(); }
inline Symbol INF() { return Symbol::createInf(); }
inline Symbol STR(String name) { return Symbol::createStr(name); }
inline Symbol ID(String name, bool sign = false) { return Symbol::createId(name, sign); }
inline Symbol FUN(String name, SymVec args, bool sign = false) { return Symbol::createFun(name, Potassco::toSpan(args), sign); }

// }}}

} } // namespace Gringo Test

#endif // _GRINGO_TEST_TERM_HELPER_HH

