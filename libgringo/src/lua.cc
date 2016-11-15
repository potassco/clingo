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

#ifdef WITH_LUA

#include "gringo/lua.hh"
#include "gringo/logger.hh"
#include "gringo/control.hh"
#include "potassco/clingo.h"

#include <lua.hpp>
#include <cstring>

namespace Gringo {

namespace {

// {{{1 auxiliary functions

#define VALUE_CMP(TYPE) \
static int eq##TYPE(lua_State *L) { \
    Symbol *a = static_cast<Symbol*>(luaL_checkudata(L, 1, "clingo."#TYPE)); \
    Symbol *b = static_cast<Symbol*>(luaL_checkudata(L, 2, "clingo."#TYPE)); \
    lua_pushboolean(L, *a == *b); \
    return 1; \
} \
static int lt##TYPE(lua_State *L) { \
    Symbol *a = static_cast<Symbol*>(luaL_checkudata(L, 1, "clingo."#TYPE)); \
    Symbol *b = static_cast<Symbol*>(luaL_checkudata(L, 2, "clingo."#TYPE)); \
    lua_pushboolean(L, *a < *b); \
    return 1; \
} \
static int le##TYPE(lua_State *L) { \
    Symbol *a = static_cast<Symbol*>(luaL_checkudata(L, 1, "clingo."#TYPE)); \
    Symbol *b = static_cast<Symbol*>(luaL_checkudata(L, 2, "clingo."#TYPE)); \
    lua_pushboolean(L, *a <= *b); \
    return 1; \
}

template <typename T>
auto protect(lua_State *L, T f) -> decltype(f()) {
    try                             { return f(); }
    catch (std::exception const &e) { luaL_error(L, e.what()); }
    catch (...)                     { luaL_error(L, "unknown error"); }
    throw std::logic_error("cannot happen");
}
#define PROTECT(E) (protect(L, [&]{ return (E); }))

// translates a clingo api error into a lua error
void handle_c_error(lua_State *L, bool ret) {
    if (!ret) {
        //if (exc && *exc) { std::rethrow_exception(*exc); }
        char const *msg = clingo_error_message();
        if (!msg) { msg = "no message"; }
        luaL_error(L, msg);
    }
}

template <typename... T>
struct Types { };

template<int, typename... T>
struct LastTypes;
template<typename A, typename... T>
struct LastTypes<0, A, T...> { using Type = Types<A, T...>; };
template<int n, typename T, typename... A>
struct LastTypes<n, T, A...> : LastTypes<n-1, A...> { };

template<int n, typename T>
struct LastArgs;
template<int n, typename... Args>
struct LastArgs<n, bool (*)(Args...)> : LastTypes<n, Args...> { };

template <typename T, typename F, typename... Args>
T call_c_(Types<T*>*, lua_State *L, F f, Args ...args) {
    T ret;
    handle_c_error(L, f(args..., &ret));
    return ret;
}

template <typename T, typename U, typename F, typename... Args>
std::pair<T, U> call_c_(Types<T*, U*>*, lua_State *L, F f, Args ...args) {
    T ret1;
    U ret2;
    handle_c_error(L, f(args..., &ret1, &ret2));
    return {ret1, ret2};
}


template <typename F, typename... Args, typename T=typename LastArgs<sizeof...(Args), F>::Type>
static auto call_c(lua_State *L, F f, Args... args) -> decltype(call_c_(static_cast<T*>(nullptr), L, f, args...)) {
    return call_c_(static_cast<T*>(nullptr), L, f, args...);
}

struct Any {
    struct PlaceHolder {
        virtual ~PlaceHolder() { };
    };
    template <class T>
    struct Holder : public PlaceHolder {
        Holder(T const &value) : value(value) { }
        Holder(T&& value) : value(std::forward<T>(value)) { }
        T value;
    };
    Any() : content(nullptr) { }
    Any(Any &&other) : content(nullptr) { std::swap(content, other.content); }
    template<typename T>
    Any(T const &value) : content(new Holder<T>(value)) { }
    template<typename T>
    Any(T &&value) : content(new Holder<typename std::remove_reference<T>::type>(std::forward<T>(value))) { }
    ~Any() { delete content; }

    Any &operator=(Any &&other) {
        std::swap(content, other.content);
        return *this;
    }

    template<typename T>
    T *get() {
        auto x = dynamic_cast<Holder<T>*>(content);
        return x ? &x->value : nullptr;
    }
    template<typename T>
    T const *get() const {
        auto x = dynamic_cast<Holder<T>*>(content);
        return x ? &x->value : nullptr;
    }
    bool empty() const { return !content; }

    PlaceHolder *content = nullptr;
};

struct AnyWrap {
    template <class T, class... Args>
    static T *new_(lua_State *L, Args... args) {
        void *data = lua_newuserdata(L, sizeof(Gringo::Any));
        Gringo::Any *ret = new (data) Any();
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        protect(L, [&] { *ret = Any(T(std::forward<Args>(args)...)) ; });
        return ret->get<T>();
    }
    template <class T>
    static T *get(lua_State *L, int index) {
        auto *self = (Gringo::Any*)luaL_checkudata(L, index, typeName);
        return protect(L, [self]() { return self->get<T>(); });
    }
    static int gc(lua_State *L) {
        Any* del = (Any*)lua_touserdata(L, 1);
        del->~Any();
        return 0;
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo._Any";
};

constexpr char const *AnyWrap::typeName;

luaL_Reg const AnyWrap::meta[] = {
    {"__gc", gc},
    {nullptr, nullptr}
};

clingo_symbol_t luaToVal(lua_State *L, int idx);

#if LUA_VERSION_NUM < 502

int lua_absindex(lua_State *L, int idx) {
    return (idx < 0) ? lua_gettop(L) + idx + 1 : idx;
}

// size_t lua_len(lua_State *L, int index) {
//     return lua_objlen(L, index);
// }

// size_t lua_rawlen(lua_State *L, int index) {
//     return lua_objlen(L, index);
// }

#endif

static int luaPushKwArg(lua_State *L, int index, int pos, char const *name, bool optional) {
    index = lua_absindex(L, index);
    lua_pushinteger(L, pos);
    lua_gettable(L, index);
    if (!lua_isnil(L, -1)) {
        if (name) {
            lua_getfield(L, index, name);
            if (!lua_isnil(L, -1)) {
                lua_pop(L, 2);
                return luaL_error(L, "argument #%d also given by keyword %s", pos, name);
            }
            lua_pop(L, 1);
        }
    }
    else if (name) {
        lua_pop(L, 1);
        lua_getfield(L, index, name);
    }
    if (!optional && lua_isnil(L, -1)) {
        return name ? luaL_error(L, "argument %s (#%d) missing", name, pos) : luaL_error(L, "argument #%d missing", pos);
    }
    return 1;
}

template <class T, class U>
void luaToCpp(lua_State *L, int index, std::pair<T, U> &x);

void luaToCpp(lua_State *L, int index, Potassco::WeightLit_t &x);

template <class T>
void luaToCpp(lua_State *L, int index, std::vector<T> &x);

void luaToCpp(lua_State *L, int index, bool &x) {
    x = lua_toboolean(L, index);
}

template <class T>
void luaToCpp(lua_State *L, int index, T &x, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr) {
    if (lua_type(L, index) != LUA_TNUMBER) {
        luaL_error(L, "number expected");
    }
    x = lua_tonumber(L, index);
}

struct symbol_wrapper {
    clingo_symbol_t symbol;
};

void luaToCpp(lua_State *L, int index, symbol_wrapper &x) {
    x.symbol = luaToVal(L, index);
}

void luaToCpp(lua_State *L, int index, clingo_symbolic_literal_t &x);

template <class T>
void luaToCpp(lua_State *L, int index, std::vector<T> &x) {
    index = lua_absindex(L, index);
    if (lua_type(L, index) != LUA_TTABLE) {
        luaL_error(L, "table expected");
    }
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        auto &y = protect(L, [&x]() -> T& { x.emplace_back(); return x.back(); });
        luaToCpp(L, -1, y);
        lua_pop(L, 1);
    }
}

template <class T, class U>
void luaToCpp(lua_State *L, int index, std::pair<T, U> &x) {
    index = lua_absindex(L, index);
    if (lua_type(L, index) != LUA_TTABLE) {
        luaL_error(L, "table expected");
    }
    lua_pushnil(L);
    if (lua_next(L, index) != 0) {
        luaToCpp(L, -1, x.first);
        lua_pop(L, 1);
    }
    else {
        luaL_error(L, "tuple expected");
    }
    if (lua_next(L, index) != 0) {
        luaToCpp(L, -1, x.second);
        lua_pop(L, 1);
    }
    else {
        luaL_error(L, "tuple expected");
    }
    if (lua_next(L, index) != 0) {
        luaL_error(L, "tuple expected");
    }
}

void luaToCpp(lua_State *L, int index, clingo_symbolic_literal_t &x) {
    std::pair<symbol_wrapper&, bool&> p{reinterpret_cast<symbol_wrapper&>(x.symbol), x.positive};
    luaToCpp(L, index, p);
}

void luaToCpp(lua_State *L, int index, Potassco::WeightLit_t &x) {
    std::pair<Potassco::Lit_t&, Potassco::Weight_t&> y{x.lit, x.weight};
    luaToCpp(L, index, y);
}

// replaces the table at index idx with a pointer holding a vector
std::vector<clingo_symbol_t> *luaToVals(lua_State *L, int idx) {
    idx = lua_absindex(L, idx);
    luaL_checktype(L, idx, LUA_TTABLE);
    std::vector<clingo_symbol_t> *vals = AnyWrap::new_<std::vector<clingo_symbol_t>>(L);
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        clingo_symbol_t sym = luaToVal(L, -1);
        protect(L, [sym, &vals](){ vals->push_back(sym); });
        lua_pop(L, 1);
    }
    lua_replace(L, idx);
    return vals;
}

bool handleError(lua_State *L, Location const &loc, int code, char const *desc, Logger *log) {
    switch (code) {
        case LUA_ERRRUN:
        case LUA_ERRERR:
        case LUA_ERRSYNTAX: {
            std::string s(lua_tostring(L, -1));
            lua_pop(L, 1);
            std::ostringstream msg;
            msg << loc << ": " << (log ? "info" : "error") << ": " << desc << ":\n"
                << (code == LUA_ERRSYNTAX ? "  SyntaxError: " : "  RuntimeError: ")
                << s << "\n"
                ;
            if (!log) { throw GringoError(msg.str().c_str()); }
            else {
                GRINGO_REPORT(*log, clingo_warning_operation_undefined) << msg.str();
                return false;
            }
        }
        case LUA_ERRMEM: {
            std::stringstream msg;
            msg << loc << ": error: lua interpreter ran out of memory" << "\n";
            throw GringoError(msg.str().c_str());
        }
    }
    return true;
}

static int luaTraceback (lua_State *L);

struct LuaClear {
    LuaClear(lua_State *L) : L(L), n(lua_gettop(L)) { }
    ~LuaClear() { lua_settop(L, n); }
    lua_State *L;
    int n;
};

// {{{1 lua C functions

struct LuaCallArgs {
    char const *name;
    clingo_symbol_t const *arguments;
    size_t size;
    clingo_symbol_callback_t *symbol_callback;
    void *data;
};

static int luaTraceback (lua_State *L) {
    if (!lua_isstring(L, 1)) { return 1; }
    lua_getglobal(L, "debug");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 1;
    }
    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return 1;
    }
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);
    lua_getglobal(L, "string");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 1;
    }
    lua_getfield(L, -1, "gsub");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return 1;
    }
    lua_pushvalue(L, -3);
    lua_pushliteral(L, "\t");
    lua_pushliteral(L, "  ");
    lua_call(L, 3, 1);
    return 1;
}

int luaCall(lua_State *L);

void lua_regMeta(lua_State *L, char const *name, luaL_Reg const * funs, lua_CFunction indexfun = nullptr, lua_CFunction newindexfun = nullptr) {
#if LUA_VERSION_NUM < 502
    luaL_newmetatable(L, name);
    luaL_register(L, 0, funs);
#else
    luaL_newmetatable(L, name);
    luaL_setfuncs(L, funs, 0);
#endif
    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -2);
    lua_rawset(L, -3);
    lua_pushliteral(L, "__index");
    if (indexfun) {
        lua_pushcfunction(L, indexfun);
    }
    else {
        lua_pushvalue(L, -2);
    }
    lua_rawset(L, -3);
    if (newindexfun) {
        lua_pushliteral(L, "__newindex");
        lua_pushcfunction(L, newindexfun);
        lua_rawset(L, -3);
    }
}

// {{{1 Object

template <typename T>
struct Object {
    template <typename... Args>
    static int new_(lua_State *L, Args&&... args) {
        new (lua_newuserdata(L, sizeof(T))) T(std::forward<Args>(args)...);
        luaL_getmetatable(L, T::typeName);
        lua_setmetatable(L, -2);
        return 1;
    }

    static void reg(lua_State *L) {
        lua_regMeta(L, T::typeName, T::meta, T::index, T::newIndex);
    }

    static T &get_self(lua_State* L) {
        return *(T*)luaL_checkudata(L, 1, T::typeName);
    }

    T &cmpKey() { return *static_cast<T*>(this); }

    static int eq(lua_State *L) {
        T *a = static_cast<T*>(luaL_checkudata(L, 1, T::typeName));
        T *b = static_cast<T*>(luaL_checkudata(L, 2, T::typeName));
        lua_pushboolean(L, a->cmpKey() == b->cmpKey());
        return 1;
    }
    static int lt(lua_State *L) {
        T *a = static_cast<T*>(luaL_checkudata(L, 1, T::typeName));
        T *b = static_cast<T*>(luaL_checkudata(L, 2, T::typeName));
        lua_pushboolean(L, a->cmpKey() < b->cmpKey());
        return 1;
    }
    static int le(lua_State *L) {
        T *a = static_cast<T*>(luaL_checkudata(L, 1, T::typeName));
        T *b = static_cast<T*>(luaL_checkudata(L, 2, T::typeName));
        lua_pushboolean(L, a->cmpKey() <= b->cmpKey());
        return 1;
    }
    static constexpr luaL_Reg const meta[] = {{nullptr, nullptr}};
    static constexpr lua_CFunction const index = nullptr;
    static constexpr lua_CFunction const newIndex = nullptr;
};

template <typename T>
constexpr luaL_Reg const Object<T>::meta[];
template <typename T>
constexpr lua_CFunction const Object<T>::index;
template <typename T>
constexpr lua_CFunction const Object<T>::newIndex;

// {{{1 wrap SolveResult

struct SolveResult : Object<SolveResult> {
    clingo_solve_result_bitset_t res;
    SolveResult(clingo_solve_result_bitset_t res) : res(res) { }
    static int satisfiable(lua_State *L) {
        auto &&res = get_self(L).res;
        if      (res & clingo_solve_result_satisfiable)   { lua_pushboolean(L, true); }
        else if (res & clingo_solve_result_unsatisfiable) { lua_pushboolean(L, false); }
        else                                              { lua_pushnil(L); }
        return 1;
    }
    static int unsatisfiable(lua_State *L) {
        auto &&res = get_self(L).res;
        if      (res & clingo_solve_result_unsatisfiable) { lua_pushboolean(L, true); }
        else if (res & clingo_solve_result_satisfiable)   { lua_pushboolean(L, false); }
        else                                              { lua_pushnil(L); }
        return 1;
    }
    static int unknown(lua_State *L) {
        auto &&res = get_self(L).res;
        if ((res & clingo_solve_result_unsatisfiable) ||
            (res & clingo_solve_result_satisfiable)) { lua_pushboolean(L, false); }
        else                                         { lua_pushboolean(L, true); }
        return 1;
    }
    static int exhausted(lua_State *L) {
        auto &&res = get_self(L).res;
        lua_pushboolean(L, res & clingo_solve_result_exhausted);
        return 1;
    }
    static int interrupted(lua_State *L) {
        auto &&res = get_self(L).res;
        lua_pushboolean(L, res & clingo_solve_result_interrupted);
        return 1;
    }
    static int index(lua_State *L) {
        char const *name = luaL_checkstring(L, 2);
        if      (strcmp(name, "satisfiable")   == 0) { return satisfiable(L); }
        else if (strcmp(name, "unsatisfiable") == 0) { return unsatisfiable(L); }
        else if (strcmp(name, "unknown")       == 0) { return unknown(L); }
        else if (strcmp(name, "exhausted")     == 0) { return exhausted(L); }
        else if (strcmp(name, "interrupted")   == 0) { return interrupted(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, name);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", name);
        }
    }
    static int toString(lua_State *L) {
        auto &&res = get_self(L).res;
        if      (res & clingo_solve_result_satisfiable)   { lua_pushstring(L, "SAT"); }
        else if (res & clingo_solve_result_unsatisfiable) { lua_pushstring(L, "UNSAT"); }
        else                                              { lua_pushstring(L, "UNKNOWN"); }
        return 1;
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.SolveResult";
};

constexpr char const *SolveResult::typeName;

luaL_Reg const SolveResult::meta[] = {
    {"__tostring", toString},
    {nullptr, nullptr}
};

// {{{1 wrap TheoryTerm

struct TheoryTermType : Object<TheoryTermType> {
    clingo_theory_term_type type;
    TheoryTermType(clingo_theory_term_type type) : type(type) { }
    clingo_theory_term_type cmpKey() { return type; }
    static int addToRegistry(lua_State *L) {
        lua_createtable(L, 0, 6);
        for (auto t : { clingo_theory_term_type_function, clingo_theory_term_type_number, clingo_theory_term_type_symbol, clingo_theory_term_type_tuple, clingo_theory_term_type_list, clingo_theory_term_type_set}) {
            Object::new_(L, t);
            lua_setfield(L, -2, field_(t));
        }
        lua_setfield(L, -2, "TheoryTermType");
        return 0;
    }
    static char const *field_(clingo_theory_term_type_t t) {
        switch (static_cast<clingo_theory_term_type>(t)) {
            case clingo_theory_term_type_function: { return "Function"; }
            case clingo_theory_term_type_number:   { return "Number"; }
            case clingo_theory_term_type_symbol:   { return "Symbol"; }
            case clingo_theory_term_type_tuple:    { return "Tuple"; }
            case clingo_theory_term_type_list:     { return "List"; }
            case clingo_theory_term_type_set:      { return "Set"; }
        }
        return "";
    }
    static int new_(lua_State *L, clingo_theory_term_type_t t) {
        lua_getfield(L, LUA_REGISTRYINDEX, "clingo");
        lua_getfield(L, -1, "TheoryTermType");
        lua_replace(L, -2);
        lua_getfield(L, -1, field_(t));
        lua_replace(L, -2);
        return 1;
    }
    static int toString(lua_State *L) {
        lua_pushstring(L, field_(get_self(L).type));
        return 1;
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.TheoryTermType";
};

constexpr char const *TheoryTermType::typeName;

luaL_Reg const TheoryTermType::meta[] = {
    {"__eq", eq},
    {"__lt", lt},
    {"__le", le},
    {"__tostring", toString},
    { nullptr, nullptr }
};

struct TheoryTerm : Object<TheoryTerm> {
    clingo_theory_atoms_t *atoms;
    clingo_id_t id;
    TheoryTerm(clingo_theory_atoms_t *atoms, clingo_id_t id) : atoms(atoms), id(id) { }
    clingo_id_t cmpKey() { return id; }
    static int name(lua_State *L) {
        auto &self = get_self(L);
        lua_pushstring(L, call_c(L, clingo_theory_atoms_term_name, self.atoms, self.id));
        return 1;
    }
    static int number(lua_State *L) {
        auto &self = get_self(L);
        lua_pushnumber(L, call_c(L, clingo_theory_atoms_term_number, self.atoms, self.id));
        return 1;
    }
    static int args(lua_State *L) {
        auto &self = get_self(L);
        auto ret = call_c(L, clingo_theory_atoms_term_arguments, self.atoms, self.id);
        lua_createtable(L, ret.second, 0);
        int i = 1;
        for (auto it = ret.first, ie = it + ret.second; it != ie; ++it) {
            new_(L, self.atoms, *it);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }
    static int toString(lua_State *L) {
        auto &self = get_self(L);
        size_t size = call_c(L, clingo_theory_atoms_term_to_string_size, self.atoms, self.id);
        char *buf = static_cast<char *>(lua_newuserdata(L, size * sizeof(*buf))); // +1
        handle_c_error(L, clingo_theory_atoms_term_to_string(self.atoms, self.id, buf, size));
        lua_pushstring(L, buf);                                                   // +1
        lua_replace(L, -2);                                                       // -1
        return 1;
    }
    static int type(lua_State *L) {
        auto &self = get_self(L);
        return TheoryTermType::new_(L, call_c(L, clingo_theory_atoms_term_type, self.atoms, self.id));
    }
    static int index(lua_State *L) {
        char const *field = luaL_checkstring(L, 2);
        if (strcmp(field, "type") == 0) { return type(L); }
        else if (strcmp(field, "name") == 0) { return name(L); }
        else if (strcmp(field, "arguments") == 0) { return args(L); }
        else if (strcmp(field, "number") == 0) { return number(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, field);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", name);
        }
    }
    static constexpr char const *typeName = "clingo.TheoryTerm";
    static luaL_Reg const meta[];
};

constexpr char const *TheoryTerm::typeName;
luaL_Reg const TheoryTerm::meta[] = {
    {"__tostring", toString},
    {"__eq", eq},
    {"__lt", lt},
    {"__le", le},
    {nullptr, nullptr}
};

// {{{1 wrap TheoryElement

struct TheoryElement : Object<TheoryElement> {
    TheoryElement(clingo_theory_atoms_t *atoms, clingo_id_t id) : atoms(atoms) , id(id) { }
    clingo_theory_atoms_t *atoms;
    clingo_id_t cmpKey() { return id; }
    clingo_id_t id;

    static int terms(lua_State *L) {
        auto &self = get_self(L);
        auto ret = call_c(L, clingo_theory_atoms_element_tuple, self.atoms, self.id);
        lua_createtable(L, ret.second, 0);
        int i = 1;
        for (auto it = ret.first, ie = it + ret.second; it != ie; ++it) {
            TheoryTerm::new_(L, self.atoms, *it);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }

    static int condition(lua_State *L) {
        auto &self = get_self(L);
        auto ret = call_c(L, clingo_theory_atoms_element_condition, self.atoms, self.id);
        lua_createtable(L, ret.second, 0);
        int i = 1;
        for (auto it = ret.first, ie = it + ret.second; it != ie; ++it) {
            lua_pushnumber(L, *it);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }

    static int conditionId(lua_State *L) {
        auto &self = get_self(L);
        lua_pushnumber(L, call_c(L, clingo_theory_atoms_element_condition_id, self.atoms, self.id));
        return 1;
    }

    static int toString(lua_State *L) {
        auto &self = get_self(L);
        auto size = call_c(L, clingo_theory_atoms_element_to_string_size, self.atoms, self.id);
        char *buf = static_cast<char *>(lua_newuserdata(L, size * sizeof(*buf))); // +1
        handle_c_error(L, clingo_theory_atoms_element_to_string(self.atoms, self.id, buf, size));
        lua_pushstring(L, buf);                                                   // +1
        lua_replace(L, -2);                                                       // -1
        return 1;
    }

    static int index(lua_State *L) {
        char const *field = luaL_checkstring(L, 2);
        if (strcmp(field, "terms") == 0) { return terms(L); }
        else if (strcmp(field, "condition") == 0) { return condition(L); }
        else if (strcmp(field, "condition_id") == 0) { return conditionId(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, field);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", field);
        }
    }

    static constexpr char const *typeName = "clingo.TheoryElement";
    static luaL_Reg const meta[];
};

constexpr char const *TheoryElement::typeName;
luaL_Reg const TheoryElement::meta[] = {
    {"__tostring", toString},
    {"__eq", eq},
    {"__lt", lt},
    {"__le", le},
    {nullptr, nullptr}
};

// {{{1 wrap TheoryAtom

struct TheoryAtom : Object<TheoryAtom> {
    clingo_theory_atoms_t *atoms;
    clingo_id_t id;
    TheoryAtom(clingo_theory_atoms_t *atoms, clingo_id_t id) : atoms(atoms) , id(id) { }
    clingo_id_t cmpKey() { return id; }

    static int elements(lua_State *L) {
        auto &self = get_self(L);
        auto ret = call_c(L, clingo_theory_atoms_atom_elements, self.atoms, self.id);
        lua_createtable(L, ret.second, 0);
        int i = 1;
        for (auto it = ret.first, ie = it + ret.second; it != ie; ++it) {
            TheoryElement::new_(L, self.atoms, *it);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }

    static int term(lua_State *L) {
        auto &self = get_self(L);
        return TheoryTerm::new_(L, self.atoms, call_c(L, clingo_theory_atoms_atom_term, self.atoms, self.id));
    }

    static int literal(lua_State *L) {
        auto &self = get_self(L);
        lua_pushnumber(L, call_c(L, clingo_theory_atoms_atom_literal, self.atoms, self.id));
        return 1;
    }

    static int guard(lua_State *L) {
        auto &self = get_self(L);
        if (!call_c(L, clingo_theory_atoms_atom_has_guard, self.atoms, self.id)) {
            lua_pushnil(L);
            return 1;
        }
        lua_createtable(L, 2, 0);
        auto ret = call_c(L, clingo_theory_atoms_atom_guard, self.atoms, self.id);
        lua_pushstring(L, ret.first);
        lua_rawseti(L, -2, 1);
        TheoryTerm::new_(L, self.atoms, ret.second);
        lua_rawseti(L, -2, 2);
        return 1;
    }

    static int toString(lua_State *L) {
        auto &self = get_self(L);
        auto size = call_c(L, clingo_theory_atoms_atom_to_string_size, self.atoms, self.id);
        char *buf = static_cast<char *>(lua_newuserdata(L, size * sizeof(*buf))); // +1
        handle_c_error(L, clingo_theory_atoms_atom_to_string(self.atoms, self.id, buf, size));
        lua_pushstring(L, buf);                                                   // +1
        lua_replace(L, -2);                                                       // -1
        return 1;
    }

    static int index(lua_State *L) {
        char const *field = luaL_checkstring(L, 2);
        if (strcmp(field, "elements") == 0) { return elements(L); }
        else if (strcmp(field, "term") == 0) { return term(L); }
        else if (strcmp(field, "guard") == 0) { return guard(L); }
        else if (strcmp(field, "literal") == 0) { return literal(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, field);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", field);
        }
    }

    static constexpr char const *typeName = "clingo.TheoryAtom";
    static luaL_Reg const meta[];
};

constexpr char const *TheoryAtom::typeName;
luaL_Reg const TheoryAtom::meta[] = {
    {"__tostring", toString},
    {"__eq", eq},
    {"__lt", lt},
    {"__le", le},
    {nullptr, nullptr}
};

// {{{1 wrap TheoryIter

struct TheoryIter {
    static int iter(lua_State *L, clingo_theory_atoms_t *atoms) {
        lua_pushlightuserdata(L, static_cast<void*>(atoms));
        lua_pushnumber(L, 0);
        lua_pushcclosure(L, iter_, 2);
        return 1;
    }

    static int iter_(lua_State *L) {
        auto atoms = (clingo_theory_atoms_t *)lua_topointer(L, lua_upvalueindex(1));
        clingo_id_t idx = lua_tointeger(L, lua_upvalueindex(2));
        if (idx < call_c(L, clingo_theory_atoms_size, atoms)) {
            lua_pushnumber(L, idx + 1);
            lua_replace(L, lua_upvalueindex(2));
            TheoryAtom::new_(L, atoms, idx);
        }
        else { lua_pushnil(L); }
        return 1;
    }
};

// {{{1 wrap Term

struct SymbolType : Object<SymbolType> {
    clingo_symbol_type_t type;
    SymbolType(clingo_symbol_type_t type) : type(type) { }
    clingo_theory_term_type_t cmpKey() { return type; }
    static int addToRegistry(lua_State *L) {
        lua_createtable(L, 0, 5);
        for (auto t : {clingo_symbol_type_number, clingo_symbol_type_string, clingo_symbol_type_function, clingo_symbol_type_infimum, clingo_symbol_type_supremum}) {
            new_(L, t);
            lua_setfield(L, -2, field_(t));
        }
        lua_setfield(L, -2, "SymbolType");
        return 0;
    }
    static char const *field_(clingo_symbol_type_t type) {
        switch (static_cast<enum clingo_symbol_type>(type)) {
            case clingo_symbol_type_number:   { return "Number"; }
            case clingo_symbol_type_string:   { return "String"; }
            case clingo_symbol_type_function: { return "Function"; }
            case clingo_symbol_type_infimum:  { return "Infimum"; }
            case clingo_symbol_type_supremum: { break; }
        }
        return "Supremum";
    }

    static int toString(lua_State *L) {
        lua_pushstring(L, field_(get_self(L).type));
        return 1;
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.SymbolType";
};

constexpr char const *SymbolType::typeName;

luaL_Reg const SymbolType::meta[] = {
    {"__eq", eq},
    {"__lt", lt},
    {"__le", le},
    {"__tostring", toString},
    { nullptr, nullptr }
};

struct Term : Object<Term> {
    clingo_symbol_t symbol;
    Term(clingo_symbol_t symbol) : symbol(symbol) { }
    static int new_(lua_State *L, clingo_symbol_t sym) {
        auto type = clingo_symbol_type(sym);
        if (type == clingo_symbol_type_supremum) {
            lua_getfield(L, LUA_REGISTRYINDEX, "clingo");
            lua_getfield(L, -1, "Supremum");
            lua_replace(L, -2);
        }
        else if (type == clingo_symbol_type_infimum) {
            lua_getfield(L, LUA_REGISTRYINDEX, "clingo");
            lua_getfield(L, -1, "Infimum");
            lua_replace(L, -2);
        }
        else { Object::new_(L, sym); }
        return 1;
    }
    static int addToRegistry(lua_State *L) {
        clingo_symbol_t sym;
        clingo_symbol_create_supremum(&sym);
        Object::new_(L, sym);
        lua_setfield(L, -2, "Supremum");
        clingo_symbol_create_supremum(&sym);
        Object::new_(L, sym);
        lua_setfield(L, -2, "Infimum");
        return 0;
    }
    static int newFun(lua_State *L) {
        char const *name = luaL_checklstring(L, 1, nullptr);
        bool positive = true;
        if (!lua_isnone(L, 3) && !lua_isnil(L, 3)) {
            positive = lua_toboolean(L, 3);
        }
        if (name[0] == '\0' && !positive) { luaL_argerror(L, 2, "tuples must not have signs"); }
        if (lua_isnoneornil(L, 2)) {
            return new_(L, call_c(L, clingo_symbol_create_id, name, positive));
        }
        else {
            lua_pushvalue(L, 2);
            std::vector<clingo_symbol_t> *args = luaToVals(L, -1);
            new_(L, call_c(L, clingo_symbol_create_function, name, args->data(), args->size(), positive));
            lua_replace(L, -2);
            return 1;
        }
    }
    static int newTuple(lua_State *L) {
        lua_pushstring(L, "");
        lua_insert(L, 1);
        return newFun(L);
    }
    static int newNumber(lua_State *L) {
        clingo_symbol_t sym;
        clingo_symbol_create_number(luaL_checkinteger(L, 1), &sym);
        return Term::new_(L, sym);
    }
    static int newString(lua_State *L) {
        return Term::new_(L, call_c(L, clingo_symbol_create_string, luaL_checkstring(L, 1)));
    }
    bool operator==(Term const &other) {
        return clingo_symbol_is_equal_to(symbol, other.symbol);
    }
    bool operator <(Term const &other) {
        return clingo_symbol_is_less_than(symbol, other.symbol);
    }
    bool operator<=(Term const &other) {
        return !clingo_symbol_is_less_than(other.symbol, symbol);
    }
    static int name(lua_State *L) {
        auto self = get_self(L);
        if (clingo_symbol_type(self.symbol) == clingo_symbol_type_function) {
            lua_pushstring(L, call_c(L, clingo_symbol_name, self.symbol));
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    static int string(lua_State *L) {
        auto self = get_self(L);
        if (clingo_symbol_type(self.symbol) == clingo_symbol_type_string) {
            lua_pushstring(L, call_c(L, clingo_symbol_string, self.symbol));
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    static int number(lua_State *L) {
        auto self = get_self(L);
        if (clingo_symbol_type(self.symbol) == clingo_symbol_type_number) {
            lua_pushnumber(L, call_c(L, clingo_symbol_number, self.symbol));
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    static int negative(lua_State *L) {
        auto self = get_self(L);
        if (clingo_symbol_type(self.symbol) == clingo_symbol_type_function) {
            lua_pushboolean(L, call_c(L, clingo_symbol_is_negative, self.symbol));
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    static int positive(lua_State *L) {
        auto self = get_self(L);
        if (clingo_symbol_type(self.symbol) == clingo_symbol_type_function) {
            lua_pushboolean(L, call_c(L, clingo_symbol_is_positive, self.symbol));
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    static int args(lua_State *L) {
        auto self = get_self(L);
        if (clingo_symbol_type(self.symbol) == clingo_symbol_type_function) {
            auto ret = call_c(L, clingo_symbol_arguments, self.symbol);
            lua_createtable(L, ret.second, 0);
            int i = 1;
            for (auto it = ret.first, ie = it + ret.second; it != ie; ++it) {
                Term::new_(L, *it);
                lua_rawseti(L, -2, i++);
            }
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    static int toString(lua_State *L) {
        auto &self = get_self(L);
        auto size = call_c(L, clingo_symbol_to_string_size, self.symbol);
        char *buf = static_cast<char *>(lua_newuserdata(L, size * sizeof(*buf))); // +1
        handle_c_error(L, clingo_symbol_to_string(self.symbol, buf, size));
        lua_pushstring(L, buf);                                                   // +1
        lua_replace(L, -2);                                                       // -1
        return 1;
    }
    static int type(lua_State *L) {
        auto &self = get_self(L);
        lua_getfield(L, LUA_REGISTRYINDEX, "clingo");
        lua_getfield(L, -1, "SymbolType");
        lua_getfield(L, -1, SymbolType::field_(clingo_symbol_type(self.symbol)));
        return 1;
    }
    static int index(lua_State *L) {
        char const *field = luaL_checkstring(L, 2);
        if      (strcmp(field, "positive") == 0)  { return positive(L); }
        else if (strcmp(field, "negative") == 0)  { return negative(L); }
        else if (strcmp(field, "arguments") == 0) { return args(L); }
        else if (strcmp(field, "name") == 0)      { return name(L); }
        else if (strcmp(field, "string") == 0)    { return string(L); }
        else if (strcmp(field, "number") == 0)    { return number(L); }
        else if (strcmp(field, "type") == 0)      { return type(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, field);
            return 1;
        }
    }
    static constexpr char const *typeName = "clingo.Symbol";
    static luaL_Reg const meta[];
};

constexpr char const *Term::typeName;
luaL_Reg const Term::meta[] = {
    {"__tostring", toString},
    {"__eq", eq},
    {"__lt", lt},
    {"__le", le},
    {nullptr, nullptr}
};

clingo_symbol_t luaToVal(lua_State *L, int idx) {
    int type = lua_type(L, idx);
    switch (type) {
        case LUA_TSTRING: {
            return call_c(L, clingo_symbol_create_string, lua_tostring(L, idx));
        }
        case LUA_TNUMBER: {
            clingo_symbol_t ret;
            clingo_symbol_create_number(lua_tointeger(L, idx), &ret);
            return ret;
        }
        case LUA_TUSERDATA: {
            bool check = false;
            if (lua_getmetatable(L, idx)) {                          // +1
                lua_getfield(L, LUA_REGISTRYINDEX, "clingo.Symbol"); // +1
                check = lua_rawequal(L, -1, -2);
                lua_pop(L, 2);                                       // -2
            }
            if (check) { return static_cast<Term*>(lua_touserdata(L, idx))->symbol; }
        }
        default: { luaL_error(L, "cannot convert to value"); }
    }
    return {};
}

int luaCall(lua_State *L) {
    auto &args = *static_cast<LuaCallArgs*>(lua_touserdata(L, 1));
    bool hasContext = !lua_isnil(L, 2);
    if (hasContext) {
        lua_getfield(L, 2, args.name);
        lua_pushvalue(L, 2);
    }
    else { lua_getglobal(L, args.name); }
    for (auto it = args.arguments, ie = it + args.size; it != ie; ++it) {
        Term::new_(L, *it);
    }
    lua_call(L, args.size + hasContext, 1);
    if (lua_type(L, -1) == LUA_TTABLE) {
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            clingo_symbol_t val = luaToVal(L, -1);
            handle_c_error(L, args.symbol_callback(&val, 1, args.data));
            lua_pop(L, 1);
        }
    }
    else {
        clingo_symbol_t val = luaToVal(L, -1);
        handle_c_error(L, args.symbol_callback(&val, 1, args.data));
    }
    return 0;
}

// {{{1 wrap SolveControl

struct SolveControl : Object<SolveControl> {
    clingo_solve_control_t *ctl;
    SolveControl(clingo_solve_control_t *ctl) : ctl(ctl) { }
    static int getClause(lua_State *L, bool invert) {
        auto self = get_self(L);
        std::vector<clingo_symbolic_literal_t> *lits = AnyWrap::new_<std::vector<clingo_symbolic_literal_t>>(L); // +1
        luaToCpp(L, 2, *lits);
        if (invert) {
            for (auto &lit : *lits) { lit.positive = !lit.positive; }
        }
        handle_c_error(L, clingo_solve_control_add_clause(self.ctl, lits->data(), lits->size()));
        lua_pop(L, 1); // -1
        return 0;
    }
    static int add_clause(lua_State *L) {
        return getClause(L, false);
    }
    static int add_nogood(lua_State *L) {
        return getClause(L, true);
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.SolveControl";
};

constexpr char const *SolveControl::typeName;

luaL_Reg const SolveControl::meta[] = {
    {"add_clause", add_clause},
    {"add_nogood", add_nogood},
    {nullptr, nullptr}
};

// {{{1 wrap Model

struct ModelType : Object<ModelType> {
    using Type = clingo_model_type_t;
    Type type;
    ModelType(Type type) : type(type) { }
    Type cmpKey() { return type; }

    static int addToRegistry(lua_State *L) {
        lua_createtable(L, 0, 6);
        for (auto t : { clingo_model_type_stable_model, clingo_model_type_brave_consequences, clingo_model_type_cautious_consequences }) {
            Object::new_(L, t);
            lua_setfield(L, -2, field_(t));
        }
        lua_setfield(L, -2, "ModelType");
        return 0;
    }
    static char const *field_(Type t) {
        switch (static_cast<enum clingo_model_type>(t)) {
            case clingo_model_type_stable_model:          { return "StableModel"; }
            case clingo_model_type_brave_consequences:    { return "BraveConsequences"; }
            case clingo_model_type_cautious_consequences: { break; }
        }
        return "CautiousConsequences";
    }
    static int new_(lua_State *L, Type t) {
        lua_getfield(L, LUA_REGISTRYINDEX, "clingo");
        lua_getfield(L, -1, "ModelType");
        lua_replace(L, -2);
        lua_getfield(L, -1, field_(t));
        lua_replace(L, -2);
        return 1;
    }
    static int toString(lua_State *L) {
        lua_pushstring(L, field_(get_self(L).type));
        return 1;
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.ModelType";
};

constexpr char const *ModelType::typeName;

luaL_Reg const ModelType::meta[] = {
    {"__eq", eq},
    {"__lt", lt},
    {"__le", le},
    {"__tostring", toString},
    { nullptr, nullptr }
};

struct Model : Object<Model> {
    clingo_model_t *model;
    Model(clingo_model_t *model) : model(model) { }
    static int contains(lua_State *L) {
        auto self = get_self(L);
        clingo_symbol_t sym = luaToVal(L, 2);
        lua_pushboolean(L, call_c(L, clingo_model_contains, self.model, sym));
        return 1;
    }
    static int atoms(lua_State *L) {
        auto self = get_self(L);
        clingo_show_type_bitset_t atomset = 0;
        luaL_checktype(L, 2, LUA_TTABLE);
        lua_getfield(L, 2, "atoms");
        if (lua_toboolean(L, -1)) { atomset |= clingo_show_type_atoms; }
        lua_pop(L, 1);
        lua_getfield(L, 2, "shown");
        if (lua_toboolean(L, -1)) { atomset |= clingo_show_type_shown; }
        lua_pop(L, 1);
        lua_getfield(L, 2, "terms");
        if (lua_toboolean(L, -1)) { atomset |= clingo_show_type_terms; }
        lua_pop(L, 1);
        lua_getfield(L, 2, "csp");
        if (lua_toboolean(L, -1)) { atomset |= clingo_show_type_csp; }
        lua_pop(L, 1);
        lua_getfield(L, 2, "extra");
        if (lua_toboolean(L, -1)) { atomset |= clingo_show_type_extra; }
        lua_pop(L, 1);
        lua_getfield(L, 2, "complement");
        if (lua_toboolean(L, -1)) { atomset |= clingo_show_type_complement; }
        lua_pop(L, 1);
        auto size = call_c(L, clingo_model_symbols_size, self.model, atomset);
        clingo_symbol_t *symbols = static_cast<clingo_symbol_t *>(lua_newuserdata(L, size * sizeof(*symbols))); // +1
        handle_c_error(L, clingo_model_symbols(self.model, atomset, symbols, size));
        lua_createtable(L, size, 0); // +1
        int i = 1;
        for (auto it = symbols, ie = it + size; it != ie; ++it) {
            Term::new_(L, *it);      // +1
            lua_rawseti(L, -2, i++); // -1
        }
        lua_replace(L, -2); // -1
        return 1;
    }
    static int cost(lua_State *L) {
        auto self = get_self(L);
        auto size = call_c(L, clingo_model_cost_size, self.model);
        int64_t *costs = static_cast<int64_t *>(lua_newuserdata(L, size * sizeof(*costs))); // +1
        handle_c_error(L, clingo_model_cost(self.model, costs, size));
        lua_createtable(L, size, 0); // +1
        int i = 1;
        for (auto it = costs, ie = it + size; it != ie; ++it) {
            lua_pushinteger(L, *it); // +1
            lua_rawseti(L, -2, i++); // -1
        }
        lua_replace(L, -2); // -1
        return 1;
    }
    static int thread_id(lua_State *L) {
        auto self = get_self(L);
        auto *ctl = call_c(L, clingo_model_context, self.model);
        lua_pushinteger(L, call_c(L, clingo_solve_control_thread_id, ctl)); // +1
        return 1;
    }
    static int toString(lua_State *L) {
        auto self = get_self(L);
        std::vector<char> *buf = AnyWrap::new_<std::vector<char>>(L); // +1
        auto printSymbol = [buf, L](std::ostream &out, clingo_symbol_t val) {
            auto size = call_c(L, clingo_symbol_to_string_size, val);
            PROTECT(buf->resize(size));
            handle_c_error(L, clingo_symbol_to_string(val, buf->data(), size));
            PROTECT((out << buf->data(), 0));
        };
        auto printAtom = [L, printSymbol](std::ostream &out, clingo_symbol_t val) {
            if (clingo_symbol_type_function == clingo_symbol_type(val)) {
                auto name = call_c(L, clingo_symbol_name, val);
                auto args = call_c(L, clingo_symbol_arguments, val);
                if (args.second == 2 && strcmp(name, "$") == 0) {
                    printSymbol(out, args.first[0]);
                    out << "=";
                    printSymbol(out, args.first[1]);
                }
                else { printSymbol(out, val); }
            }
            else { printSymbol(out, val); }
        };
        std::ostringstream *oss = AnyWrap::new_<std::ostringstream>(L); // +1
        auto size = call_c(L, clingo_model_symbols_size, self.model, clingo_show_type_shown);
        clingo_symbol_t *symbols = static_cast<clingo_symbol_t *>(lua_newuserdata(L, size * sizeof(*symbols))); // +1
        handle_c_error(L, clingo_model_symbols(self.model, clingo_show_type_shown, symbols, size));
        bool comma = false;
        for (auto it = symbols, ie = it + size; it != ie; ++it) {
            if (comma) { *oss << " "; }
            else       { comma = true; }
            printAtom(*oss, *it);
        }
        std::string *str = AnyWrap::new_<std::string>(L); // +1
        lua_pushstring(L, PROTECT((*str = oss->str(), str->c_str()))); // +1
        lua_replace(L, -5); // -1
        lua_pop(L, 3); // -3
        return 1;
    }
    static int context(lua_State *L) {
        auto self = get_self(L);
        return SolveControl::new_(L, call_c(L, clingo_model_context, self.model));
    }
    static int index(lua_State *L) {
        auto self = get_self(L);
        char const *name = luaL_checkstring(L, 2);
        if (strcmp(name, "cost") == 0) {
            return cost(L);
        }
        else if (strcmp(name, "context") == 0) {
            return context(L);
        }
        else if (strcmp(name, "thread_id") == 0) {
            return thread_id(L);
        }
        else if (strcmp(name, "number") == 0) {
            lua_pushnumber(L, call_c(L, clingo_model_number, self.model));
            return 1;
        }
        else if (strcmp(name, "optimality_proven") == 0) {
            lua_pushboolean(L, call_c(L, clingo_model_optimality_proven, self.model));
            return 1;
        }
        else if (strcmp(name, "type") == 0) {
            return ModelType::new_(L, call_c(L, clingo_model_type, self.model));
        }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, name);
            return 1;
        }
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.Model";
};

constexpr char const *Model::typeName;
luaL_Reg const Model::meta[] = {
    {"__tostring", toString},
    {"symbols", atoms},
    {"contains", contains},
    {nullptr, nullptr}
};

// {{{1 wrap Statistics

int newStatistics(lua_State *L, Potassco::AbstractStatistics const *stats, Potassco::AbstractStatistics::Key_t key) {
    switch (protect(L, [stats, key]{ return stats->type(key); })) {
        case Potassco::Statistics_t::Value: {
            lua_pushnumber(L, PROTECT(stats->value(key)));
            return 1;
        }
        case Potassco::Statistics_t::Array: {
            lua_newtable(L);                                         // stack + 1
            for (size_t i = 0, e = PROTECT(stats->size(key)); i != e; ++i) {
                newStatistics(L, stats, PROTECT(stats->at(key, i))); // stack + 2
                lua_rawseti(L, -2, i+1);                             // stack + 1
            }
            return 1;
        }
        case Potassco::Statistics_t::Map: {
            lua_newtable(L);                                             // stack + 1
            for (size_t i = 0, e = PROTECT(stats->size(key)); i != e; ++i) {
                auto name = PROTECT(stats->key(key, i));
                lua_pushstring(L, name);                                 // stack + 2
                newStatistics(L, stats, PROTECT(stats->get(key, name))); // stack + 3
                lua_rawset(L, -3);                                       // stack + 1
            }
            return 1;
        }
        default: {
            return luaL_error(L, "cannot happen");
        }
    }
}

// {{{1 wrap SolveFuture

struct SolveFuture {
    static int get(lua_State *L) {
        Gringo::SolveFuture *& future = *(Gringo::SolveFuture **)luaL_checkudata(L, 1, typeName);
        SolveResult::new_(L, protect(L, [future]() { return (clingo_solve_result_bitset_t)future->get(); }));
        return 1;
    }
    static int wait(lua_State *L) {
        Gringo::SolveFuture *& future = *(Gringo::SolveFuture **)luaL_checkudata(L, 1, typeName);
        if (lua_isnone(L, 2) == 0) {
            double timeout = luaL_checknumber(L, 2);
            lua_pushboolean(L, protect(L, [future, timeout]() { return future->wait(timeout); }));
            return 1;
        }
        else {
            protect(L, [future]() { future->wait(); });
            return 0;
        }
    }
    static int cancel(lua_State *L) {
        Gringo::SolveFuture *& future = *(Gringo::SolveFuture **)luaL_checkudata(L, 1, typeName);
        protect(L, [future]() { future->cancel(); });
        return 0;
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.SolveFuture";
};

constexpr char const *SolveFuture::typeName;
luaL_Reg const SolveFuture::meta[] = {
    {"get",  get},
    {"wait", wait},
    {"cancel", cancel},
    {nullptr, nullptr}
};

// {{{1 wrap SolveIter

struct SolveIter {
    static int close(lua_State *L) {
        Gringo::SolveIter *& iter = *(Gringo::SolveIter **)luaL_checkudata(L, 1, typeName);
        protect(L, [iter]() { iter->close(); });
        return 0;
    }
    static int next(lua_State *L) {
        Gringo::SolveIter *& iter = *(Gringo::SolveIter **)luaL_checkudata(L, lua_upvalueindex(1), typeName);
        Gringo::Model const *m = protect(L, [iter]() { return iter->next(); });
        if (m) {
            *(Gringo::Model const **)lua_newuserdata(L, sizeof(Gringo::Model*)) = m;
            luaL_getmetatable(L, Model::typeName);
            lua_setmetatable(L, -2);
        }
        else   { lua_pushnil(L); }
        return 1;
    }
    static int iter(lua_State *L) {
        luaL_checkudata(L, 1, typeName);
        lua_pushvalue(L,1);
        lua_pushcclosure(L, next, 1);
        return 1;
    }
    static int get(lua_State *L) {
        Gringo::SolveIter *& iter = *(Gringo::SolveIter **)luaL_checkudata(L, 1, typeName);
        SolveResult::new_(L, PROTECT(iter->get()));
        return 1;
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.SolveIter";
};

constexpr char const *SolveIter::typeName;
luaL_Reg const SolveIter::meta[] = {
    {"iter",  iter},
    {"close", close},
    {"get", get},
    {nullptr, nullptr}
};

// {{{1 wrap Configuration

struct Configuration {
    unsigned key;
    int nSubkeys;
    int arrLen;
    int nValues;
    char const* help;
    Gringo::ConfigProxy *proxy;

    static int new_(lua_State *L, unsigned key, Gringo::ConfigProxy &proxy) {
        Configuration &self = *(Configuration*)lua_newuserdata(L, sizeof(Configuration));
        self.proxy = &proxy;
        self.key   = key;
        protect(L, [&self] { self.proxy->getKeyInfo(self.key, &self.nSubkeys, &self.arrLen, &self.help, &self.nValues); });
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        return 1;
    }

    static int keys(lua_State *L) {
        auto &self = *(Configuration *)luaL_checkudata(L, 1, typeName);
        if (self.nSubkeys < 0) { return 0; }
        else {
            lua_createtable(L, self.nSubkeys, 0);
            for (int i = 0; i < self.nSubkeys; ++i) {
                char const *key = protect(L, [&self, i] { return self.proxy->getSubKeyName(self.key, i); });
                lua_pushstring(L, key);
                lua_rawseti(L, -2, i+1);
            }
            return 1;
        }
    }

    static int index(lua_State *L) {
        auto &self = *(Configuration *)luaL_checkudata(L, 1, typeName);
        char const *name = luaL_checkstring(L, 2);
        if (strcmp("keys", name) == 0) { return keys(L); }
        bool desc = strncmp("__desc_", name, 7) == 0;
        if (desc) { name += 7; }
        bool hasSubKey = PROTECT(self.proxy->hasSubKey(self.key, name));
        if (hasSubKey) {
            auto key = PROTECT(self.proxy->getSubKey(self.key, name));
            new_(L, key, *self.proxy);
            auto &sub = *(Configuration *)lua_touserdata(L, -1);
            if (desc) {
                lua_pushstring(L, sub.help);
                return 1;
            }
            else if (sub.nValues < 0) { return 1; }
            else {
                std::string *value = AnyWrap::new_<std::string>(L);
                bool ret = PROTECT(sub.proxy->getKeyValue(sub.key, *value));
                if (ret) {
                    lua_pushstring(L, value->c_str());
                    return 1;
                }
                return 0;
            }
        }
        lua_getmetatable(L, 1);
        lua_getfield(L, -1, name);
        return 1;
    }

    static int newindex(lua_State *L) {
        auto &self = *(Configuration *)luaL_checkudata(L, 1, typeName);
        char const *name = luaL_checkstring(L, 2);
        bool hasSubKey = protect(L, [self, name] { return self.proxy->hasSubKey(self.key, name); });
        if (hasSubKey) {
            auto key = PROTECT(self.proxy->getSubKey(self.key, name));
            const char *value = lua_tostring(L, 3);
            protect(L, [self, key, value]() { self.proxy->setKeyValue(key, value); });
            lua_pushstring(L, value);
            return 1;
        }
        return luaL_error(L, "unknown field: %s", name);
    }

    static int next(lua_State *L) {
        auto &self = *(Configuration *)luaL_checkudata(L, lua_upvalueindex(1), typeName);
        int index = luaL_checkinteger(L, lua_upvalueindex(2));
        lua_pushnumber(L, index + 1);
        lua_replace(L, lua_upvalueindex(2));
        if (index < self.arrLen) {
            unsigned key = protect(L, [&self, index]() { return self.proxy->getArrKey(self.key, index); });
            return new_(L, key, *self.proxy);
        }
        else {
            lua_pushnil(L);
            return 1;
        }
    }

    static int iter(lua_State *L) {
        luaL_checkudata(L, 1, typeName);
        lua_pushvalue(L, 1);
        lua_pushnumber(L, 0);
        lua_pushcclosure(L, next, 2);
        return 1;
    }

    static int len(lua_State *L) {
        auto &self = *(Configuration *)luaL_checkudata(L, 1, typeName);
        lua_pushnumber(L, self.arrLen);
        return 1;
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.Configuration";
};

constexpr char const *Configuration::typeName;
luaL_Reg const Configuration::meta[] = {
    {"__len", len},
    {"iter", iter},
    {nullptr, nullptr}
};

// {{{1 wrap SymbolicAtom

struct SymbolicAtom {
    Gringo::SymbolicAtoms &atoms;
    Gringo::SymbolicAtomIter range;
    static luaL_Reg const meta[];
    SymbolicAtom(Gringo::SymbolicAtoms &atoms, Gringo::SymbolicAtomIter range)
    : atoms(atoms)
    , range(range) { }
    static constexpr const char *typeName = "clingo.SymbolicAtom";
    static int new_(lua_State *L, Gringo::SymbolicAtoms &atoms, Gringo::SymbolicAtomIter range) {
        auto self = (SymbolicAtom*)lua_newuserdata(L, sizeof(SymbolicAtom));
        new (self) SymbolicAtom(atoms, range);
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        return 1;
    }
    static int symbol(lua_State *L) {
        auto self = (SymbolicAtom *)luaL_checkudata(L, 1, typeName);
        Symbol atom = protect(L, [self](){ return self->atoms.atom(self->range); });
        Term::new_(L, atom.rep());
        return 1;
    }
    static int literal(lua_State *L) {
        auto self = (SymbolicAtom *)luaL_checkudata(L, 1, typeName);
        auto lit = protect(L, [self](){ return self->atoms.literal(self->range); });
        lua_pushinteger(L, lit);
        return 1;
    }
    static int index(lua_State *L) {
        char const *name = luaL_checkstring(L, 2);
        if (strcmp(name, "is_fact") == 0) { return is_fact(L); }
        else if (strcmp(name, "is_external") == 0) { return is_external(L); }
        else if (strcmp(name, "symbol") == 0) { return symbol(L); }
        else if (strcmp(name, "literal") == 0) { return literal(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, name);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", name);
        }
    }
    static int is_fact(lua_State *L) {
        auto self = (SymbolicAtom *)luaL_checkudata(L, 1, typeName);
        bool ret = protect(L, [self](){ return self->atoms.fact(self->range); });
        lua_pushboolean(L, ret);
        return 1;
    }
    static int is_external(lua_State *L) {
        auto self = (SymbolicAtom *)luaL_checkudata(L, 1, typeName);
        bool ret = protect(L, [self](){ return self->atoms.external(self->range); });
        lua_pushboolean(L, ret);
        return 1;
    }
};

constexpr const char *SymbolicAtom::typeName;

luaL_Reg const SymbolicAtom::meta[] = {
    {nullptr, nullptr}
};

// {{{1 wrap SymbolicAtoms

struct SymbolicAtoms {
    Gringo::SymbolicAtoms &atoms;
    static luaL_Reg const meta[];

    SymbolicAtoms(Gringo::SymbolicAtoms &atoms) : atoms(atoms) { }

    static constexpr const char *typeName = "clingo.SymbolicAtoms";

    static SymbolicAtoms &get_self(lua_State *L) {
        return *(SymbolicAtoms*)luaL_checkudata(L, 1, typeName);
    }

    static int symbolicAtomIter(lua_State *L) {
        auto current = (SymbolicAtom *)luaL_checkudata(L, lua_upvalueindex(1), SymbolicAtom::typeName);
        if (current->atoms.valid(current->range)) {
            lua_pushvalue(L, lua_upvalueindex(1));                                               // +1
            auto next = protect(L, [current]() { return current->atoms.next(current->range); });
            SymbolicAtom::new_(L, current->atoms, next);                                         // +1
            lua_replace(L, lua_upvalueindex(1));                                                 // -1
        }
        else { lua_pushnil(L); }                                                                 // +1
        return 1;
    }

    static int new_(lua_State *L, Gringo::SymbolicAtoms &atoms) {
        auto self = (SymbolicAtoms*)lua_newuserdata(L, sizeof(SymbolicAtoms));
        new (self) SymbolicAtoms(atoms);
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        return 1;
    }

    static int len(lua_State *L) {
        auto &self = get_self(L);
        int ret = protect(L, [&self]() { return self.atoms.length(); });
        lua_pushinteger(L, ret);
        return 1;
    }

    static int iter(lua_State *L) {
        auto &self = get_self(L);
        auto range = protect(L, [&self]() { return self.atoms.begin(); });
        SymbolicAtom::new_(L, self.atoms, range);                         // +1
        lua_pushcclosure(L, symbolicAtomIter, 1);                         // +0
        return 1;
    }

    static int lookup(lua_State *L) {
        auto &self = get_self(L);
        Gringo::Symbol atom = Symbol{luaToVal(L, 2)};
        auto range = protect(L, [self, atom]() { return self.atoms.lookup(atom); });
        if (self.atoms.valid(range)) { SymbolicAtom::new_(L, self.atoms, range); }
        else                         { lua_pushnil(L); }               // +1
        return 1;
    }

    static int by_signature(lua_State *L) {
        auto &self = get_self(L);
        char const *name = luaL_checkstring(L, 2);
        int arity = luaL_checkinteger(L, 3);
        bool positive = lua_isnone(L, 4) || lua_toboolean(L, 4);
        auto range = protect(L, [&self, name, arity, positive]() { return self.atoms.begin(Sig(name, arity, !positive)); });
        SymbolicAtom::new_(L, self.atoms, range);  // +1
        lua_pushcclosure(L, symbolicAtomIter, 1);  // +0
        return 1;
    }

    static int signatures(lua_State *L) {
        auto &self = get_self(L);
        auto ret = AnyWrap::new_<std::vector<Sig>>(L); // +1
        *ret = protect(L, [&self]() { return self.atoms.signatures(); });
        lua_createtable(L, ret->size(), 0);                    // +1
        int i = 1;
        for (auto &sig : *ret) {
            lua_createtable(L, 3, 0);                          // +1
            lua_pushstring(L, sig.name().c_str());             // +1
            lua_rawseti(L, -2, 1);                             // -1
            lua_pushinteger(L, sig.arity());                   // +1
            lua_rawseti(L, -2, 2);                             // -1
            lua_pushboolean(L, !sig.sign());                   // +1
            lua_rawseti(L, -2, 3);                             // -1
            lua_rawseti(L, -2, i);                             // -1
            ++i;
        }
        lua_replace(L, -2);                                    // -1
        return 1;
    }
    static int index(lua_State *L) {
        char const *name = luaL_checkstring(L, 2);
        if (strcmp(name, "signatures") == 0) { return signatures(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, name);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", name);
        }
    }
};

constexpr const char *SymbolicAtoms::typeName;

luaL_Reg const SymbolicAtoms::meta[] = {
    {"__len", len},
    {"iter", iter},
    {"lookup", lookup},
    {"by_signature", by_signature},
    {nullptr, nullptr}
};

// {{{1 wrap wrap Backend
struct Backend : Object<Backend> {
    Gringo::Control &ctl;
    Gringo::Backend &backend;

    Backend(Gringo::Control &ctl, Gringo::Backend &backend) : ctl(ctl), backend(backend) { }

    static constexpr const char *typeName = "clingo.Backend";
    static luaL_Reg const meta[];

    static int addAtom(lua_State *L) {
        auto &self = get_self(L);
        auto atom = protect(L, [self](){ return self.ctl.addProgramAtom(); });
        lua_pushinteger(L, atom);
        return 1;
    }

    static int addRule(lua_State *L) {
        auto &self = get_self(L);
        auto *head = AnyWrap::new_<Gringo::BackendAtomVec>(L);
        auto *body = AnyWrap::new_<Gringo::BackendLitVec>(L);
        bool choice = false;
        luaL_checktype(L, 2, LUA_TTABLE);
        luaPushKwArg(L, 2, 1, "head", false);
        luaToCpp(L, -1, *head);
        lua_pop(L, 1);
        luaPushKwArg(L, 2, 2, "body", true);
        if (!lua_isnil(L, -1)) { luaToCpp(L, -1, *body); }
        lua_pop(L, 1);
        luaPushKwArg(L, 2, 3, "choice", true);
        luaToCpp(L, -1, choice);
        lua_pop(L, 1);
        protect(L, [self, choice, head, body](){
            Gringo::outputRule(self.backend, choice, *head, *body);
        });
        return 0;
    }

    static int addWeightRule(lua_State *L) {
        auto &self = get_self(L);
        auto *head = AnyWrap::new_<Gringo::BackendAtomVec>(L);
        Weight_t lower;
        auto *body = AnyWrap::new_<Gringo::BackendLitWeightVec>(L);
        bool choice = false;
        luaL_checktype(L, 2, LUA_TTABLE);
        luaPushKwArg(L, 2, 1, "head", false);
        luaToCpp(L, -1, *head);
        lua_pop(L, 1);
        luaPushKwArg(L, 2, 2, "lower", false);
        luaToCpp(L, -1, lower);
        lua_pop(L, 1);
        luaPushKwArg(L, 2, 3, "body", false);
        luaToCpp(L, -1, *body);
        lua_pop(L, 1);
        luaPushKwArg(L, 2, 4, "choice", true);
        luaToCpp(L, -1, choice);
        lua_pop(L, 1);
        protect(L, [self, choice, head, lower, body](){
            Gringo::outputRule(self.backend, choice, *head, lower, *body);
        });
        return 0;
    }
};

luaL_Reg const Backend::meta[] = {
    {"add_atom", addAtom},
    {"add_rule", addRule},
    {"add_weight_rule", addWeightRule},
    {nullptr, nullptr}
};

// {{{1 wrap ControlWrap

struct LuaContext : Gringo::Context {
    LuaContext(lua_State *L, Logger &log, int idx)
    : L(L)
    , log(log)
    , idx(idx) { }

    bool callable(String name) const override {
        if (!L || !idx) { return false; }
        LuaClear lc(L);
        lua_getfield(L, idx, name.c_str());
        return lua_type(L, -1) == LUA_TFUNCTION;
    }

    SymVec call(Location const &loc, String name, SymSpan args) override {
        assert(L);
        LuaClear lc(L);
        std::vector<Symbol> syms;
        LuaCallArgs arg{name.c_str(), reinterpret_cast<clingo_symbol_t const *>(args.first), args.size, [](clingo_symbol_t const *symbols, size_t size, void *data){
            // NOTE: no error handling at the moment but this part will be refactored anyway
            for (auto it = symbols, ie = it + size; it != ie; ++it) {
                static_cast<std::vector<Symbol>*>(data)->emplace_back(Symbol{*it});
            }
            return true;
        }, &syms};
        lua_pushcfunction(L, luaTraceback);
        lua_pushcfunction(L, luaCall);
        lua_pushlightuserdata(L, (void*)&arg);
        lua_pushvalue(L, idx);
        int ret = lua_pcall(L, 2, 0, -4);
        if (!handleError(L, loc, ret, "operation undefined", &log)) { return {}; }
        return syms;
    }

    virtual ~LuaContext() noexcept = default;

    lua_State *L;
    Logger &log;
    int idx;
};

struct ControlWrap {
    static GringoModule *module;
    Control &ctl;
    bool free;
    ControlWrap(Control &ctl, bool free) : ctl(ctl), free(free) { }
    static ControlWrap &get_self(lua_State *L) {
        void *p = nullptr;
        if (lua_istable(L, 1)) {
            lua_rawgeti(L, 1, 1);                   // +1
            p = lua_touserdata(L, -1);
            if (p) {
                if (lua_getmetatable(L, 1)) {       // +1
                    luaL_getmetatable(L, typeName); // +1
                    if (!lua_rawequal(L, -1, -2)) { p = nullptr; }
                    lua_pop(L, 2);                  // -2
                }
                else { p = nullptr; }
            }
            lua_pop(L, 1);                          // -1
        }
        if (!p) {
            const char *msg = lua_pushfstring(L, "%s expected, got %s", typeName, luaL_typename(L, 1));
            luaL_argerror(L, 1, msg);
        }
        return *(ControlWrap*)p;
    }
    static void checkBlocked(lua_State *L, Control &ctl, char const *function) {
        if (protect(L, [&ctl]() { return ctl.blocked(); })) { luaL_error(L, "Control.%s must not be called during solve call", function); }
    }
    static int ground(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        checkBlocked(L, ctl, "ground");
        luaL_checktype(L, 2, LUA_TTABLE);
        LuaContext ctx{ L, ctl.logger(), !lua_isnone(L, 3) && !lua_isnil(L, 3) ? 3 : 0 };
        if (ctx.idx) { luaL_checktype(L, ctx.idx, LUA_TTABLE); }
        Control::GroundVec *vec = AnyWrap::new_<Control::GroundVec>(L);
        lua_pushnil(L);
        while (lua_next(L, 2) != 0) {
            luaL_checktype(L, -1, LUA_TTABLE);
            lua_pushnil(L);
            if (!lua_next(L, -2)) { luaL_error(L, "tuple of name and arguments expected"); }
            char const *name = luaL_checkstring(L, -1);
            lua_pop(L, 1);
            if (!lua_next(L, -2)) { luaL_error(L, "tuple of name and arguments expected"); }
            std::vector<clingo_symbol_t> *args = luaToVals(L, -1);
            protect(L, [name, args, vec](){ vec->emplace_back(name, reinterpret_cast<std::vector<Symbol>&>(*args)); });
            lua_pop(L, 1);
            if (lua_next(L, -2)) { luaL_error(L, "tuple of name and arguments expected"); }
            lua_pop(L, 1);
        }
        protect(L, [&ctl, vec, &ctx]() { ctl.ground(*vec, ctx.idx ? &ctx : nullptr); });
        return 0;
    }
    static int add(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        checkBlocked(L, ctl, "add");
        char const *name = luaL_checkstring(L, 2);
        luaL_checktype(L, 3, LUA_TTABLE);
        char const *prg = luaL_checkstring(L, 4);
        FWStringVec *vals = AnyWrap::new_<FWStringVec>(L);
        lua_pushnil(L);
        while (lua_next(L, 3) != 0) {
            char const *val = luaL_checkstring(L, -1);
            protect(L, [val,&vals](){ vals->push_back(val); });
            lua_pop(L, 1);
        }
        protect(L, [&ctl, name, vals, prg]() { ctl.add(name, *vals, prg); });
        return 0;
    }
    static int load(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        checkBlocked(L, ctl, "load");
        char const *filename = luaL_checkstring(L, 2);
        protect(L, [&ctl, filename]() { ctl.load(filename); });
        return 0;
    }
    static int get_const(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        checkBlocked(L, ctl, "get_const");
        char const *name = luaL_checkstring(L, 2);
        Symbol ret = protect(L, [&ctl, name]() { return ctl.getConst(name); });
        if (ret.type() == Gringo::SymbolType::Special) { lua_pushnil(L); }
        else                                           { Term::new_(L, ret.rep()); }
        return 1;
    }

    static Control::Assumptions *getAssumptions(lua_State *L, int assIdx) {
        Control::Assumptions *ass = AnyWrap::new_<Control::Assumptions>(L);
        if (assIdx) {
            assIdx = lua_absindex(L, assIdx);
            luaL_checktype(L, assIdx, LUA_TTABLE);
            lua_pushnil(L);
            while (lua_next(L, assIdx)) {
                luaL_checktype(L, -1, LUA_TTABLE);
                lua_pushnil(L);
                if (!lua_next(L, -2)) { luaL_error(L, "atom/boolean pair expected"); }
                Symbol atom{luaToVal(L, -1)};
                lua_pop(L, 1);
                if (!lua_next(L, -2)) { luaL_error(L, "atom/boolean pair expected"); }
                bool truth = lua_toboolean(L, -1);
                lua_pop(L, 1);
                if (lua_next(L, -2)) { luaL_error(L, "atom/boolean pair expected"); }
                protect(L, [atom, truth, ass](){ ass->emplace_back(atom, truth); });
                lua_pop(L, 1);
            }
            lua_replace(L, assIdx);
        }
        return ass;
    }

    static int solve(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        checkBlocked(L, ctl, "solve");
        lua_pushstring(L, "statistics");
        lua_pushnil(L);
        lua_rawset(L, 1);
        int mhIndex = !lua_isnone(L, 2) && !lua_isnil(L, 2) ? 2 : 0;
        int assIdx  = !lua_isnone(L, 3) && !lua_isnil(L, 3) ? 3 : 0;
        Gringo::Model const **model = nullptr;
        int mIndex  = 0;
        if (mhIndex) {
            model = (Gringo::Model const **)lua_newuserdata(L, sizeof(Gringo::Model*));
            luaL_getmetatable(L, Model::typeName);
            lua_setmetatable(L, -2);
            mIndex = lua_gettop(L);
        }
        Control::Assumptions *ass = getAssumptions(L, assIdx);
        SolveResult::new_(L, protect(L, [L, &ctl, model, ass, mhIndex, mIndex]() {
            return (clingo_solve_result_bitset_t)ctl.solve(!model ? Control::ModelHandler(nullptr) : [L, model, mhIndex, mIndex](Gringo::Model const &m) -> bool {
                LuaClear lc(L);
                lua_pushcfunction(L, luaTraceback);
                lua_pushvalue(L, mhIndex);
                lua_pushvalue(L, mIndex);
                *model = &m;
                int code = lua_pcall(L, 1, 1, -3);
                Location loc("<on_model>", 1, 1, "<on_model>", 1, 1);
                handleError(L, loc, code, "error in model callback", nullptr);
                return lua_type(L, -1) == LUA_TNIL || lua_toboolean(L, -1);
            }, std::move(*ass));
        }));
        return 1;
    }
    static int cleanup(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        checkBlocked(L, ctl, "cleanup");
        protect(L, [&ctl]() { ctl.cleanupDomains(); });
        return 0;
    }
    static int solve_async(lua_State *L) {
        return luaL_error(L, "asynchronous solving not supported");
    }
    static int solve_iter(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        checkBlocked(L, ctl, "solve_iter");
        lua_pushstring(L, "statistics");
        lua_pushnil(L);
        lua_rawset(L, 1);
        int assIdx  = !lua_isnone(L, 2) && !lua_isnil(L, 2) ? 2 : 0;
        Control::Assumptions *ass = getAssumptions(L, assIdx);
        auto &iter = *(Gringo::SolveIter **)lua_newuserdata(L, sizeof(Gringo::SolveIter*));
        iter = protect(L, [&ctl, ass]() { return ctl.solveIter(std::move(*ass)); });
        luaL_getmetatable(L, SolveIter::typeName);
        lua_setmetatable(L, -2);
        return 1;
    }
    static int assign_external(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        checkBlocked(L, ctl, "assign_external");
        Symbol atom{luaToVal(L, 2)};
        luaL_checkany(L, 3);
        Potassco::Value_t truth;
        if (lua_isnil (L, 3)) { truth = Potassco::Value_t::Free; }
        else {
            luaL_checktype(L, 3, LUA_TBOOLEAN);
            truth = lua_toboolean(L, 3) ? Potassco::Value_t::True : Potassco::Value_t::False;
        }
        protect(L, [&ctl, atom, truth]() { ctl.assignExternal(atom, truth); });
        return 0;
    }
    static int release_external(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        checkBlocked(L, ctl, "release_external");
        Symbol atom{luaToVal(L, 2)};
        protect(L, [&ctl, atom]() { ctl.assignExternal(atom, Potassco::Value_t::Release); });
        return 0;
    }
    static int interrupt(lua_State *L) {
        get_self(L).ctl.interrupt();
        return 0;
    }
    static int newindex(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        char const *name = luaL_checkstring(L, 2);
        if (strcmp(name, "use_enumeration_assumption") == 0) {
            bool enabled = lua_toboolean(L, 3);
            checkBlocked(L, ctl, "use_enumeration_assumption");
            protect(L, [&ctl, enabled]() { ctl.useEnumAssumption(enabled); });
            return 0;
        }
        return luaL_error(L, "unknown field: %s", name);
    }

    static int index(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        char const *name = luaL_checkstring(L, 2);
        if (strcmp(name, "use_enumeration_assumption") == 0) {
            bool enabled = protect(L, [&ctl]() { return ctl.useEnumAssumption(); });
            lua_pushboolean(L, enabled);                // stack +1
            return 1;
        }
        else if (strcmp(name, "statistics") == 0) {
            checkBlocked(L, ctl, "statistics");
            lua_pushstring(L, "statistics");            // stack +1
            lua_rawget(L, 1);                           // stack +0
            if (lua_isnil(L, -1)) {
                auto stats = protect(L, [&ctl](){ return ctl.statistics(); });
                lua_pop(L, 1);                          // stack -1
                newStatistics(L, stats, stats->root()); // stack +0
                lua_pushstring(L, "statistics");        // stack +1
                lua_pushvalue(L, -2);                   // stack +1
                lua_rawset(L, 1);                       // stack -2
            }
            return 1;
        }
        else if (strcmp(name, "configuration") == 0) {
            checkBlocked(L, ctl, "configuration");
            Gringo::ConfigProxy *proxy;
            unsigned key;
            protect(L, [&ctl, &proxy, &key]() -> void {
                proxy = &ctl.getConf();
                key   = proxy->getRootKey();
            });
            return Configuration::new_(L, key, *proxy);
        }
        else if (strcmp(name, "symbolic_atoms") == 0) {
            checkBlocked(L, ctl, "symbolic_atoms");
            auto &proxy = protect(L, [&ctl]() -> Gringo::SymbolicAtoms& { return ctl.getDomain(); });
            return SymbolicAtoms::new_(L, proxy);
        }
        else if (strcmp(name, "theory_atoms") == 0) {
            checkBlocked(L, ctl, "theory_atoms");
            return TheoryIter::iter(L, const_cast<Gringo::TheoryData*>(&ctl.theory()));
        }
        else if (strcmp(name, "backend") == 0) {
            checkBlocked(L, ctl, "backend");
            auto *backend = protect(L, [&ctl](){ return ctl.backend(); });
            if (!backend) { return luaL_error(L, "backend not available"); }
            return Backend::new_(L, ctl, *backend);
        }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, name);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", name);
        }
    }
    static int newControl(lua_State *L) {
        bool hasArg = !lua_isnone(L, 1);
        std::vector<std::string> *args = AnyWrap::new_<std::vector<std::string>>(L);
        if (hasArg) {
            luaL_checktype(L, 1, LUA_TTABLE);
            lua_pushnil(L);
            while (lua_next(L, 1) != 0) {
                char const *arg = luaL_checkstring(L, -1);
                protect(L, [arg, &args](){ args->push_back(arg); });
                lua_pop(L, 1);
            }
        }
        std::vector<char const *> *cargs = AnyWrap::new_<std::vector<char const*>>(L);
        for (auto &arg : *args) {
            protect(L, [&arg, &cargs](){ cargs->push_back(arg.c_str()); });
        }
        return newControl(L, [cargs](void *mem){ new (mem) ControlWrap(*module->newControl(cargs->size(), cargs->data(), nullptr, 20), true); });
    }
    template <class F>
    static int newControl(lua_State *L, F f) {
        lua_newtable(L);                                                           // +1
        auto self = (Gringo::ControlWrap*)lua_newuserdata(L, sizeof(ControlWrap)); // +1
        lua_rawseti(L, -2, 1);                                                     // -1
        protect(L, [self, f]() { f(self); });
        luaL_getmetatable(L, typeName);                                            // +1
        lua_setmetatable(L, -2);                                                   // -1
        return 1;
    }
    static int gc(lua_State *L) {
        auto &self = get_self(L);
        if (self.free) { delete &self.ctl; }
        return 0;
    }
    static int registerPropagator(lua_State *L);
    static int registerObserver(lua_State *L);
    static luaL_Reg meta[];
    static constexpr char const *typeName = "clingo.Control";
};

constexpr char const *ControlWrap::typeName;
GringoModule *ControlWrap::module = nullptr;
luaL_Reg ControlWrap::meta[] = {
    {"ground",  ground},
    {"add", add},
    {"load", load},
    {"solve", solve},
    {"cleanup", cleanup},
    {"solve_async", solve_async},
    {"solve_iter", solve_iter},
    {"get_const", get_const},
    {"assign_external", assign_external},
    {"release_external", release_external},
    {"interrupt", interrupt},
    {"register_propagator", registerPropagator},
    {"register_observer", registerObserver},
    {"__gc", gc},
    {nullptr, nullptr}
};

int luaMain(lua_State *L) {
    auto ctl = (Control*)lua_touserdata(L, 1);
    lua_getglobal(L, "main");
    ControlWrap::newControl(L, [ctl](void *mem) { new (mem) ControlWrap(*ctl, false); });
    lua_call(L, 1, 0);
    return 0;
}

// {{{1 wrap PropagateInit

struct PropagateInit : Object<PropagateInit> {
    lua_State *T;
    Gringo::PropagateInit *init;
    PropagateInit(lua_State *T, Gringo::PropagateInit *init) : T(T), init(init) { }

    static int mapLit(lua_State *L) {
        auto &self = get_self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushnumber(L, protect(L, [self, lit]() { return self.init->mapLit(lit); }));
        return 1;
    }

    static int numThreads(lua_State *L) {
        auto &self = get_self(L);
        lua_pushnumber(L, protect(L, [self]() { return self.init->threads(); }));
        return 1;
    }
    static int addWatch(lua_State *L) {
        auto &self = get_self(L);
        int lit = luaL_checkinteger(L, 2);
        protect(L, [self, lit]() { self.init->addWatch(lit); });
        return 0;
    }

    static int index(lua_State *L) {
        auto &self = get_self(L);
        char const *name = luaL_checkstring(L, 2);
        if (strcmp(name, "theory_atoms")   == 0) { return TheoryIter::iter(L, const_cast<Gringo::TheoryData*>(&self.init->theory())); }
        else if (strcmp(name, "symbolic_atoms") == 0) { return SymbolicAtoms::new_(L, self.init->getDomain()); }
        else if (strcmp(name, "number_of_threads") == 0) { return numThreads(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, name);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", name);
        }
    }

    static int setState(lua_State *L) {
        auto &self = get_self(L);
        int id = luaL_checknumber(L, 2);
        luaL_checkany(L, 3);
        if (id < 1 || id > (int)self.init->threads()) {
            luaL_error(L, "invalid solver thread id %d", id);
        }
        lua_xmove(L, self.T, 1);
        lua_rawseti(self.T, 2, id);
        return 0;
    }

    static constexpr char const *typeName = "clingo.PropagateInit";
    static luaL_Reg const meta[];
};

constexpr char const *PropagateInit::typeName;
luaL_Reg const PropagateInit::meta[] = {
    {"solver_literal", mapLit},
    {"add_watch", addWatch},
    {"set_state", setState},
    {nullptr, nullptr}
};

// {{{1 wrap Assignment

struct Assignment : Object<Assignment> {
    Assignment(Potassco::AbstractAssignment const *ass) : ass(*ass) { }
    Potassco::AbstractAssignment const &ass;

    static int hasConflict(lua_State *L) {
        auto &self = get_self(L);
        lua_pushboolean(L, protect(L, [self]() { return self.ass.hasConflict(); }));
        return 1;
    }

    static int decisionLevel(lua_State *L) {
        auto &self = get_self(L);
        lua_pushinteger(L, protect(L, [self]() { return self.ass.level(); }));
        return 1;
    }

    static int hasLit(lua_State *L) {
        auto &self = get_self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushboolean(L, protect(L, [self,lit]() { return self.ass.hasLit(lit); }));
        return 1;
    }

    static int level(lua_State *L) {
        auto &self = get_self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushinteger(L, protect(L, [self,lit]() { return self.ass.level(lit); }));
        return 1;
    }

    static int decision(lua_State *L) {
        auto &self = get_self(L);
        int level = luaL_checkinteger(L, 2);
        lua_pushinteger(L, protect(L, [self,level]() { return self.ass.decision(level); }));
        return 1;
    }

    static int isFixed(lua_State *L) {
        auto &self = get_self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushboolean(L, protect(L, [self,lit]() { return self.ass.isFixed(lit); }));
        return 1;
    }

    static int isTrue(lua_State *L) {
        auto &self = get_self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushboolean(L, protect(L, [self,lit]() { return self.ass.isTrue(lit); }));
        return 1;
    }

    static int value(lua_State *L) {
        auto &self = get_self(L);
        int lit = luaL_checkinteger(L, 2);
        auto val = protect(L, [self, lit]() { return self.ass.value(lit); });
        if (val == Potassco::Value_t::Free) { lua_pushnil(L); }
        else { lua_pushboolean(L, val == Potassco::Value_t::True); }
        lua_pushboolean(L, val);
        return 1;
    }

    static int isFalse(lua_State *L) {
        auto &self = get_self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushboolean(L, protect(L, [self,lit]() { return self.ass.isFalse(lit); }));
        return 1;
    }

    static int index(lua_State *L) {
        char const *name = luaL_checkstring(L, 2);
        if (strcmp(name, "has_conflict")   == 0) { return hasConflict(L); }
        if (strcmp(name, "decision_level")   == 0) { return decisionLevel(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, name);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", name);
        }
    }

    static constexpr char const *typeName = "clingo.Assignment";
    static luaL_Reg const meta[];
};

constexpr char const *Assignment::typeName;
luaL_Reg const Assignment::meta[] = {
    {"has_lit", hasLit},
    {"value", value},
    {"level", level},
    {"is_fixed", isFixed},
    {"is_true", isTrue},
    {"is_false", isFalse},
    {"decision", decision},
    {nullptr, nullptr}
};

// {{{1 wrap PropagateControl

struct PropagateControl : Object<PropagateControl> {
    PropagateControl(Potassco::AbstractSolver* ctl) : ctl(ctl) { }
    Potassco::AbstractSolver* ctl;

    static int id(lua_State *L) {
        auto &self = get_self(L);
        lua_pushinteger(L, protect(L, [self]() { return self.ctl->id() + 1; }));
        return 1;
    }

    static int assignment(lua_State *L) {
        auto &self = get_self(L);
        Assignment::new_(L, protect(L, [self]() { return &self.ctl->assignment(); }));
        return 1;
    }

    static int addClauseOrNogood(lua_State *L, bool invert) {
        auto &self = get_self(L);
        lua_pushinteger(L, 1);                                       // +1
        lua_gettable(L, 2);                                          // +0
        luaL_checktype(L, -1, LUA_TTABLE);                           // +0
        int lits_index = lua_gettop(L);
        auto lits = AnyWrap::new_<std::vector<Potassco::Lit_t>>(L);  // +1
        lua_pushnil(L);                                              // +1
        while (lua_next(L, -3)) {                                    // -1
            int lit = luaL_checkinteger(L, -1);                      // +0
            protect(L, [lits, lit](){ lits->emplace_back(lit); });
            lua_pop(L, 1);
        }
        unsigned type = 0;
        lua_getfield(L, 2, "tag");                                   // +1
        if (lua_toboolean(L, -1)) {
            type |= Potassco::Clause_t::Volatile;
        }
        lua_pop(L, 1);                                               // -1
        lua_getfield(L, 2, "lock");                                  // +1
        if (lua_toboolean(L, -1)) {
            type |= Potassco::Clause_t::Static;
        }
        lua_pop(L, 1);                                               // -1
        lua_pushboolean(L, protect(L, [self, lits, type, invert]() { // +1
            if (invert) {
                for (auto &lit : *lits) { lit = -lit; }
            }
            return self.ctl->addClause(Potassco::toSpan(*lits), static_cast<Potassco::Clause_t>(type));
        }));
        lua_replace(L, lits_index);
        lua_settop(L, lits_index);
        return 1;
    }

    static int addLiteral(lua_State *L) {
        auto &self = get_self(L);
        lua_pushnumber(L, protect(L, [self](){ return self.ctl->addVariable(); }));
        return 1;
    }

    static int addWatch(lua_State *L) {
        auto &self = get_self(L);
        auto lit = luaL_checkinteger(L, 1);
        protect(L, [self, lit](){ self.ctl->addWatch(static_cast<clingo_literal_t>(lit)); });
        return 0;
    }

    static int removeWatch(lua_State *L) {
        auto &self = get_self(L);
        auto lit = luaL_checkinteger(L, 1);
        protect(L, [self, lit](){ self.ctl->removeWatch(static_cast<clingo_literal_t>(lit)); });
        return 0;
    }

    static int hasWatch(lua_State *L) {
        auto &self = get_self(L);
        auto lit = luaL_checkinteger(L, 1);
        auto ret = protect(L, [self, lit](){ return self.ctl->hasWatch(static_cast<clingo_literal_t>(lit)); });
        lua_pushboolean(L, ret);
        return 1;
    }

    static int addClause(lua_State *L) {
        return addClauseOrNogood(L, false);
    }

    static int addNogood(lua_State *L) {
        return addClauseOrNogood(L, true);
    }

    static int propagate(lua_State *L) {
        auto &self = get_self(L);
        lua_pushboolean(L, protect(L, [self]() { return self.ctl->propagate(); }));
        return 1;
    }

    static int index(lua_State *L) {
        char const *name = luaL_checkstring(L, 2);
        if (strcmp(name, "thread_id")   == 0) { return id(L); }
        if (strcmp(name, "assignment")   == 0) { return assignment(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, name);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", name);
        }
    }

    static constexpr char const *typeName = "clingo.PropagateControl";
    static luaL_Reg const meta[];
};

constexpr char const *PropagateControl::typeName;
luaL_Reg const PropagateControl::meta[] = {
    {"add_literal", addLiteral},
    {"add_watch", addWatch},
    {"has_watch", hasWatch},
    {"remove_watch", removeWatch},
    {"add_clause", addClause},
    {"add_nogood", addNogood},
    {"propagate", propagate},
    {nullptr, nullptr}
};

// }}}
// {{{1 wrap Propagator

class Propagator : public Gringo::Propagator {
public:
    enum Indices : int { PropagatorIndex=1, StateIndex=2, ThreadIndex=3 };
    Propagator(lua_State *L, lua_State *T) : L(L), T(T) { }
    static int init_(lua_State *L) {
        auto *self = (Propagator*)lua_touserdata(L, 1);
        auto *init = (Gringo::PropagateInit*)lua_touserdata(L, 2);
        while (self->threads.size() < static_cast<size_t>(init->threads())) {
            self->threads.emplace_back(lua_newthread(L));
            lua_xmove(L, self->T, 1);
            lua_rawseti(self->T, ThreadIndex, self->threads.size());
        }
        lua_pushvalue(self->T, PropagatorIndex);         // +1
        lua_xmove(self->T, L, 1);                        // +0
        lua_getfield(L, -1, "init");                     // +1
        if (!lua_isnil(L, -1)) {
            lua_insert(L, -2);
            PropagateInit::new_(L, self->T, init);       // +1
            lua_call(L, 2, 0);                           // -3
        }
        else { lua_pop(L, 2); }                          // -2
        return 0;
    }
    void init(Gringo::PropagateInit &init) override {
        threads.reserve(init.threads());
        if (!lua_checkstack(L, 4)) { throw std::runtime_error("lua stack size exceeded"); }
        lua_pushcfunction(L, luaTraceback);
        lua_pushcfunction(L, init_);
        lua_pushlightuserdata(L, this);
        lua_pushlightuserdata(L, &init);
        auto ret = lua_pcall(L, 2, 0, -4);
        if (ret != 0) {
            Location loc("<Propagator::init>", 1, 1, "<Propagator::init>", 1, 1);
            handleError(L, loc, ret, "initializing the propagator failed", nullptr);
        }
    }
    static int getChanges(lua_State *L, Potassco::LitSpan const *changes) {
        lua_newtable(L);
        int m = changes->size;
        for (int i = 0; i < m; ++i) {
            lua_pushinteger(L, *(changes->first + i));
            lua_rawseti(L, -2, i+1);
        }
        return 1;
    }
    static int getState(lua_State *L, lua_State *T, clingo_id_t id) {
        lua_rawgeti(T, StateIndex, id+1);
        lua_xmove(T, L, 1);
        return 1;
    }
    static int propagate_(lua_State *L) {
        auto *self = static_cast<Propagator*>(lua_touserdata(L, 1));
        auto *solver = static_cast<Potassco::AbstractSolver*>(lua_touserdata(L, 2));
        auto *changes = static_cast<Potassco::LitSpan const*>(lua_touserdata(L, 3));
        lua_pushvalue(self->T, PropagatorIndex);         // +1
        lua_xmove(self->T, L, 1);                        // +0
        lua_getfield(L, -1, "propagate");                // +1
        if (!lua_isnil(L, -1)) {
            lua_insert(L, -2);
            PropagateControl::new_(L, solver);           // +1
            getChanges(L, changes);                      // +1
            getState(L, self->T, solver->id());          // +1
            lua_call(L, 4, 0);                           // -5
        }
        else {
            lua_pop(L, 2);                               // -2
        }
        return 0;
    }
    void propagate(Potassco::AbstractSolver &solver, Potassco::LitSpan const &changes) override {
        lua_State *L = threads[solver.id()];
        if (!lua_checkstack(L, 5)) { throw std::runtime_error("lua stack size exceeded"); }
        LuaClear ll(T), lt(L);
        lua_pushcfunction(L, luaTraceback);
        lua_pushcfunction(L, propagate_);
        lua_pushlightuserdata(L, this);
        lua_pushlightuserdata(L, &solver);
        lua_pushlightuserdata(L, &const_cast<Potassco::LitSpan&>(changes));
        auto ret = lua_pcall(L, 3, 0, -5);
        if (ret != 0) {
            Location loc("<Propagator::propagate>", 1, 1, "<Propagator::propagate>", 1, 1);
            handleError(L, loc, ret, "propagate failed", nullptr);
        }
    }
    static int undo_(lua_State *L) {
        auto *self = static_cast<Propagator*>(lua_touserdata(L, 1));
        auto *solver = static_cast<Potassco::AbstractSolver const*>(lua_touserdata(L, 2));
        auto *changes = static_cast<Potassco::LitSpan const*>(lua_touserdata(L, 3));
        lua_pushvalue(self->T, PropagatorIndex);         // +1
        lua_xmove(self->T, L, 1);                        // +0
        lua_getfield(L, -1, "undo");                     // +1
        if (!lua_isnil(L, -1)) {
            lua_insert(L, -2);
            lua_pushnumber(L, solver->id() + 1);         // +1
            Assignment::new_(L, &solver->assignment());  // +1
            getChanges(L, changes);                      // +1
            getState(L, self->T, solver->id());          // +1
            lua_call(L, 5, 0);                           // -6
        }
        else {
            lua_pop(L, 2);                               // -2
        }
        return 0;
    }
    void undo(Potassco::AbstractSolver const &solver, Potassco::LitSpan const &changes) override {
        lua_State *L = threads[solver.id()];
        if (!lua_checkstack(L, 5)) { throw std::runtime_error("lua stack size exceeded"); }
        LuaClear ll(T), lt(L);
        lua_pushcfunction(L, luaTraceback);
        lua_pushcfunction(L, undo_);
        lua_pushlightuserdata(L, this);
        lua_pushlightuserdata(L, &const_cast<Potassco::AbstractSolver&>(solver));
        lua_pushlightuserdata(L, &const_cast<Potassco::LitSpan&>(changes));
        auto ret = lua_pcall(L, 3, 0, -5);
        if (ret != 0) {
            Location loc("<Propagator::undo>", 1, 1, "<Propagator::undo>", 1, 1);
            handleError(L, loc, ret, "undo failed", nullptr);
        }
    }
    static int check_(lua_State *L) {
        auto *self = static_cast<Propagator*>(lua_touserdata(L, 1));
        auto *solver = static_cast<Potassco::AbstractSolver *>(lua_touserdata(L, 2));
        lua_pushvalue(self->T, PropagatorIndex);         // +1
        lua_xmove(self->T, L, 1);                        // +0
        lua_getfield(L, -1, "check");                    // +1
        if (!lua_isnil(L, -1)) {
            lua_insert(L, -2);                           // -1
            PropagateControl::new_(L, solver);           // +1
            getState(L, self->T, solver->id());          // +1
            lua_call(L, 3, 0);                           // -4
        }
        else {
            lua_pop(L, 2);                               // -2
        }
        return 0;
    }
    void check(Potassco::AbstractSolver &solver) override {
        lua_State *L = threads[solver.id()];
        if (!lua_checkstack(L, 4)) { throw std::runtime_error("lua stack size exceeded"); }
        LuaClear ll(T), lt(L);
        lua_pushcfunction(L, luaTraceback);
        lua_pushcfunction(L, check_);
        lua_pushlightuserdata(L, this);
        lua_pushlightuserdata(L, &solver);
        auto ret = lua_pcall(L, 2, 0, -4);
        if (ret != 0) {
            Location loc("<Propagator::check>", 1, 1, "<Propagator::check>", 1, 1);
            handleError(L, loc, ret, "check failed", nullptr);
        }
    }
    virtual ~Propagator() noexcept = default;
private:
    lua_State *L;
    // global data for the executions stacks below
    // (something similar could be achieved using the registry index + luaL_(un)ref)
    lua_State *T;
    // execution threads for progagators (executed in lock step)
    std::vector<lua_State *> threads;
};

int ControlWrap::registerPropagator(lua_State *L) {
    auto &self = get_self(L);
    lua_pushstring(L, "propagators");                 // +1
    lua_rawget(L, 1);                                 // +0
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);                                // -1
        lua_newtable(L);                              // +1
        lua_pushstring(L, "propagators");             // +1
        lua_pushvalue(L, -2);                         // +1
        lua_rawset(L, 1);                             // -2
    }
    auto *T = lua_newthread(L);                       // +1
    luaL_ref(L, -2);                                  // -1
    lua_pop(L, 1);                                    // -1
    lua_pushvalue(L, 2);                              // +1
    lua_xmove(L, T, 1);                               // -1
    lua_newtable(T);
    lua_newtable(T);
    protect(L, [L, T, &self]() { self.ctl.registerPropagator(gringo_make_unique<Propagator>(L, T), true); });
    return 0;
}

// {{{1 wrap GroundProgramObserver

struct TruthValue {
    using Type = Potassco::Value_t::E;

    static int addToRegistry(lua_State *L) {
        lua_createtable(L, 0, 6);
        for (auto t : { Type::True, Type::False, Type::Free, Type::Release }) {
            *(Type*)lua_newuserdata(L, sizeof(Type)) = t;
            luaL_getmetatable(L, typeName);
            lua_setmetatable(L, -2);
            lua_setfield(L, -2, field_(t));
        }
        lua_setfield(L, -2, "TruthValue");
        return 0;
    }
    static char const *field_(Type t) {
        switch (t) {
            case Type::True:    { return "True"; }
            case Type::False:   { return "False"; }
            case Type::Free:    { return "Free"; }
            case Type::Release: { return "Release"; }
            default:            { return ""; }
        }
    }
    static int new_(lua_State *L, Type t) {
        lua_getfield(L, LUA_REGISTRYINDEX, "clingo");
        lua_getfield(L, -1, "TruthValue");
        lua_replace(L, -2);
        lua_getfield(L, -1, field_(t));
        lua_replace(L, -2);
        return 1;
    }
    static int eq(lua_State *L) {
        Type *a = static_cast<Type*>(luaL_checkudata(L, 1, typeName));
        Type *b = static_cast<Type*>(luaL_checkudata(L, 2, typeName));
        lua_pushboolean(L, *a == *b);
        return 1;
    }
    static int lt(lua_State *L) {
        Type *a = static_cast<Type*>(luaL_checkudata(L, 1, typeName));
        Type *b = static_cast<Type*>(luaL_checkudata(L, 2, typeName));
        lua_pushboolean(L, *a < *b);
        return 1;
    }
    static int le(lua_State *L) {
        Type *a = static_cast<Type*>(luaL_checkudata(L, 1, typeName));
        Type *b = static_cast<Type*>(luaL_checkudata(L, 2, typeName));
        lua_pushboolean(L, *a <= *b);
        return 1;
    }
    static int toString(lua_State *L) {
        lua_pushstring(L, field_(*(Type*)luaL_checkudata(L, 1, typeName)));
        return 1;
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.TruthValue";
};

constexpr char const *TruthValue::typeName;

luaL_Reg const TruthValue::meta[] = {
    {"__eq", eq},
    {"__lt", lt},
    {"__le", le},
    {"__tostring", toString},
    { nullptr, nullptr }
};

struct HeuristicType {
    using Type = Potassco::Heuristic_t::E;

    static int addToRegistry(lua_State *L) {
        lua_createtable(L, 0, 6);
        for (auto t : { Type::Level, Type::Sign, Type::Factor, Type::Init, Type::True, Type::False }) {
            *(Type*)lua_newuserdata(L, sizeof(Type)) = t;
            luaL_getmetatable(L, typeName);
            lua_setmetatable(L, -2);
            lua_setfield(L, -2, field_(t));
        }
        lua_setfield(L, -2, "HeuristicType");
        return 0;
    }
    static char const *field_(Type t) {
        switch (t) {
            case Type::Level:  { return "Level"; }
            case Type::Sign:   { return "Sign"; }
            case Type::Factor: { return "Factor"; }
            case Type::Init:   { return "Init"; }
            case Type::True:   { return "True"; }
            case Type::False:  { return "False"; }
            default:           { return ""; }
        }
    }
    static int new_(lua_State *L, Type t) {
        lua_getfield(L, LUA_REGISTRYINDEX, "clingo"); // +1
        lua_getfield(L, -1, "HeuristicType");         // +1
        lua_replace(L, -2);                           // -1
        lua_getfield(L, -1, field_(t));               // +1
        lua_replace(L, -2);                           // -1
        return 1;
    }
    static int eq(lua_State *L) {
        Type *a = static_cast<Type*>(luaL_checkudata(L, 1, typeName));
        Type *b = static_cast<Type*>(luaL_checkudata(L, 2, typeName));
        lua_pushboolean(L, *a == *b);
        return 1;
    }
    static int lt(lua_State *L) {
        Type *a = static_cast<Type*>(luaL_checkudata(L, 1, typeName));
        Type *b = static_cast<Type*>(luaL_checkudata(L, 2, typeName));
        lua_pushboolean(L, *a < *b);
        return 1;
    }
    static int le(lua_State *L) {
        Type *a = static_cast<Type*>(luaL_checkudata(L, 1, typeName));
        Type *b = static_cast<Type*>(luaL_checkudata(L, 2, typeName));
        lua_pushboolean(L, *a <= *b);
        return 1;
    }
    static int toString(lua_State *L) {
        lua_pushstring(L, field_(*(Type*)luaL_checkudata(L, 1, typeName)));
        return 1;
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.HeuristicType";
};

luaL_Reg const HeuristicType::meta[] = {
    {"__eq", eq},
    {"__lt", lt},
    {"__le", le},
    {"__tostring", toString},
    { nullptr, nullptr }
};
constexpr char const *HeuristicType::typeName;

class GroundProgramObserver : public Gringo::Backend {
public:
    GroundProgramObserver(lua_State *L, lua_State *T) : L(L), T(T) { }

    void initProgram(bool incremental) override {
        call("init_program", incremental);
    }
    void beginStep() override {
        call("begin_step");
    }

    void rule(Head_t ht, const AtomSpan& head, const LitSpan& body) override {
        call("rule", ht == Head_t::Choice, head, body);
    }
    void rule(Head_t ht, const AtomSpan& head, Weight_t bound, const WeightLitSpan& body) override {
        call("weight_rule", ht == Head_t::Choice, head, bound, body);
    }
    void minimize(Weight_t prio, const WeightLitSpan& lits) override {
        call("minimize", prio, lits);
    }

    void project(const AtomSpan& atoms) override {
        call("project", atoms);
    }
    void output(Gringo::Symbol sym, Potassco::Atom_t atom) override {
        call("output_atom", sym, atom);
    }
    void output(Gringo::Symbol sym, Potassco::LitSpan const& condition) override {
        call("output_term", sym, condition);
    }
    void output(Gringo::Symbol sym, int value, Potassco::LitSpan const& condition) override {
        call("output_csp", sym, value, condition);
    }
    void external(Atom_t a, Value_t v) override {
        call("external", a, v);
    }
    void assume(const LitSpan& lits) override {
        call("assume", lits);
    }
    void heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& condition) override {
        call("heuristic", a, t, bias, prio, condition);
    }
    void acycEdge(int s, int t, const LitSpan& condition) override {
        call("acyc_edge", s, t, condition);
    }

    void theoryTerm(clingo_id_t termId, int number) override {
        call("theory_term_number", termId, number);
    }
    void theoryTerm(clingo_id_t termId, const StringSpan& name) override {
        std::string s{name.first, name.size};
        call("theory_term_string", termId, s);
    }
    void theoryTerm(clingo_id_t termId, int cId, const IdSpan& args) override {
        call("theory_term_compound", termId, cId, args);
    }
    void theoryElement(clingo_id_t elementId, const IdSpan& terms, const LitSpan& cond) override {
        call("theory_element", elementId, terms, cond);
    }
    void theoryAtom(clingo_id_t atomOrZero, clingo_id_t termId, const IdSpan& elements) override {
        call("theory_atom", atomOrZero, termId, elements);
    }
    void theoryAtom(clingo_id_t atomOrZero, clingo_id_t termId, const IdSpan& elements, clingo_id_t op, clingo_id_t rhs) override {
        call("theory_atom_with_guard", atomOrZero, termId, elements, op, rhs);
    }

    void endStep() override {
        call("end_step");
    }
private:
    static void push(lua_State *L, bool b) {
        lua_pushboolean(L, b);
    }
    static void push(lua_State *L, Symbol b) {
        Term::new_(L, b.rep());
    }
    static void push(lua_State *L, Value_t x) {
        TruthValue::new_(L, x.val_);
    }
    static void push(lua_State *L, Heuristic_t x) {
        HeuristicType::new_(L, x.val_);
    }
    static void push(lua_State *L, Potassco::WeightLit_t lit) {
        lua_newtable(L);
        push(L, lit.lit);
        lua_rawseti(L, -2, 1);
        push(L, lit.weight);
        lua_rawseti(L, -2, 2);
    }
    template <class T>
    static void push(lua_State *L, T n, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr) {
        lua_pushnumber(L, n);
    }
    template <class T>
    static void push(lua_State *L, Potassco::Span<T> span) {
        lua_newtable(L);
        int i = 0;
        for (auto x : span) {
            push(L, x);
            lua_rawseti(L, -2, ++i);
        }
    }
    static void push(lua_State *L, std::string const &s) {
        lua_pushstring(L, s.c_str());
    }

    void push_args() { }
    template <class T, class... U>
    void push_args(T& arg, U&... args) {
        lua_pushlightuserdata(L, &arg);
        push_args(args...);
    }

    template <int>
    static void l_push_args(lua_State *, int) { }
    template <int, class T, class... U>
    static void l_push_args(lua_State *L, int i) {
        T *val = static_cast<T*>(lua_touserdata(L, lua_upvalueindex(i)));
        push(L, *val);
        l_push_args<0, U...>(L, i+1);
    }

    template <int>
    static int count() { return 0; }
    template <int, class U, class... T>
    static int count() { return count<0, T...>() + 1; }

    template <class... T>
    static int l_call(lua_State *L) {
        lua_pushvalue(L, 1);
        lua_pushvalue(L, 2);
        l_push_args<0, T...>(L, 1);
        lua_call(L, (count<0, T...>()+1), 0);
        return 0;
    }
    template <class... Args>
    void call(char const *fun, Args... args) {
        if (!lua_checkstack(L, 3)) { throw std::runtime_error("lua stack size exceeded"); }
        LuaClear t(L);
        // get observer on top of stack L
        lua_pushvalue(T, 1);
        lua_xmove(T, L, 1);                          // +1
        int observer = lua_gettop(L);
        lua_pushcfunction(L, luaTraceback);          // +1
        int handler = lua_gettop(L);
        lua_getfield(L, -2, fun);                    // +1
        if (!lua_isnil(L, -1)) {
            int function = lua_gettop(L);
            int n = count<0, Args...>();
            if (!lua_checkstack(L, std::max(3,n))) { throw std::runtime_error("lua stack size exceeded"); }
            push_args(args...);                      // +n
            lua_pushcclosure(L, l_call<Args...>, n); // +1-n
            lua_pushvalue(L, function);              // +1
            lua_pushvalue(L, observer);              // +1
            auto ret = lua_pcall(L, 2, 0, handler);
            if (ret != 0) {
                std::string f = "<GroundProgramObserver::" + std::string(fun) + ">";
                Location loc(f.c_str(), 1, 1, f.c_str(), 1, 1);
                handleError(L, loc, ret, ("calling " + std::string(fun) + " failed").c_str(), nullptr);
            }
        }
    }

private:
    lua_State *L;
    // state where the observer is stored
    lua_State *T;
};

int ControlWrap::registerObserver(lua_State *L) {
    bool replace = lua_toboolean(L, 3);
    auto &self = get_self(L);
    lua_pushstring(L, "observers");     // +1
    lua_rawget(L, 1);                   // +0
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);                  // -1
        lua_newtable(L);                // +1
        lua_pushstring(L, "observers"); // +1
        lua_pushvalue(L, -2);           // +1
        lua_rawset(L, 1);               // -2
    }
    auto *T = lua_newthread(L);         // +1
    luaL_ref(L, -2);                    // -1
    lua_pop(L, 1);                      // -1
    lua_pushvalue(L, 2);                // +1
    lua_xmove(L, T, 1);                 // -1
    protect(L, [L, T, &self, replace]() { self.ctl.registerObserver(gringo_make_unique<GroundProgramObserver>(L, T), replace); });
    return 0;
}

// {{{1 wrap module functions

int parseTerm(lua_State *L) {
    char const *str = luaL_checkstring(L, 1);
    Symbol val = protect(L, [str]() { return ControlWrap::module->parseValue(str, nullptr, 20); });
    if (val.type() == Gringo::SymbolType::Special) { lua_pushnil(L); }
    else { Term::new_(L, val.rep()); }
    return 1;
}


// {{{1 clingo library

int luaopen_clingo(lua_State* L) {
    static luaL_Reg clingoLib[] = {
        {"Function", Term::newFun},
        {"Tuple", Term::newTuple},
        {"Number", Term::newNumber},
        {"String", Term::newString},
        {"Control", ControlWrap::newControl},
        {"parse_term", parseTerm},
        {nullptr, nullptr}
    };

    Term::reg(L);
    SymbolType::reg(L);
    Model::reg(L);
    SolveControl::reg(L);
    lua_regMeta(L, SolveFuture::typeName,    SolveFuture::meta);
    lua_regMeta(L, SolveIter::typeName,      SolveIter::meta);
    lua_regMeta(L, ControlWrap::typeName,    ControlWrap::meta, ControlWrap::index, ControlWrap::newindex);
    lua_regMeta(L, Configuration::typeName,  Configuration::meta, Configuration::index, Configuration::newindex);
    SolveResult::reg(L);
    lua_regMeta(L, SymbolicAtoms::typeName,  SymbolicAtoms::meta, SymbolicAtoms::index);
    lua_regMeta(L, SymbolicAtom::typeName,   SymbolicAtom::meta, SymbolicAtom::index);
    lua_regMeta(L, AnyWrap::typeName,        AnyWrap::meta);
    TheoryTermType::reg(L);
    lua_regMeta(L, TruthValue::typeName,     TruthValue::meta);
    ModelType::reg(L);
    lua_regMeta(L, HeuristicType::typeName,  HeuristicType::meta);
    TheoryTerm::reg(L);
    TheoryElement::reg(L);
    TheoryAtom::reg(L);
    PropagateInit::reg(L);
    PropagateControl::reg(L);
    Assignment::reg(L);
    Backend::reg(L);

#if LUA_VERSION_NUM < 502
    luaL_register(L, "clingo", clingoLib);
#else
    luaL_newlib(L, clingoLib);
#endif

    lua_pushstring(L, CLINGO_VERSION);
    lua_setfield(L, -2, "__version__");

    SymbolType::addToRegistry(L);
    Term::addToRegistry(L);
    TheoryTermType::addToRegistry(L);
    TruthValue::addToRegistry(L);
    ModelType::addToRegistry(L);
    HeuristicType::addToRegistry(L);

    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "clingo");

    return 1;
}

int luarequire_clingo(lua_State *L) {
    luaL_openlibs(L);
#if LUA_VERSION_NUM < 502
    lua_pushcfunction(L, luaopen_clingo);
    lua_call(L, 0, 1);
#else
    luaL_requiref(L, "clingo", luaopen_clingo, true);
#endif
    return 1;
}

// }}}1

} // namespace

// {{{1 definition of LuaImpl

struct LuaImpl {
    LuaImpl() : L(luaL_newstate()) {
        if (!L) { throw std::runtime_error("could not open lua state"); }
        int n = lua_gettop(L);
        lua_pushcfunction(L, luaTraceback);
        lua_pushcfunction(L, luarequire_clingo);
        int ret = lua_pcall(L, 0, 0, -2);
        Location loc("<LuaImpl>", 1, 1, "<LuaImpl>", 1, 1);
        handleError(L, loc, ret, "running lua script failed", nullptr);
        lua_settop(L, n);
    }
    ~LuaImpl() {
        if (L) { lua_close(L); }
    }
    lua_State *L;
};

// {{{1 definition of Lua

class LuaScript : public Script {
public:
    LuaScript(GringoModule &module) {
        ControlWrap::module = &module;
    }

private:
    void exec(Location const &loc, String code) override {
        if (!impl) { impl = gringo_make_unique<LuaImpl>(); }
        LuaClear lc(impl->L);
        std::stringstream oss;
        oss << loc;
        lua_pushcfunction(impl->L, luaTraceback);
        int ret = luaL_loadbuffer(impl->L, code.c_str(), code.length(), oss.str().c_str());
        handleError(impl->L, loc, ret, "parsing lua script failed", nullptr);
        ret = lua_pcall(impl->L, 0, 0, -2);
        handleError(impl->L, loc, ret, "running lua script failed", nullptr);
    }

    SymVec call(Location const &loc, String name, SymSpan args) override {
        assert(impl);
        LuaClear lc(impl->L);
        std::vector<Symbol> syms;
        LuaCallArgs arg{name.c_str(), reinterpret_cast<clingo_symbol_t const *>(args.first), args.size, [](clingo_symbol_t const *symbols, size_t size, void *data){
            // NOTE: no error handling at the moment but this part will be refactored anyway
            for (auto it = symbols, ie = it + size; it != ie; ++it) {
                static_cast<std::vector<Symbol>*>(data)->emplace_back(Symbol{*it});
            }
            return true;
        }, &syms};
        lua_pushcfunction(impl->L, luaTraceback);
        lua_pushcfunction(impl->L, luaCall);
        lua_pushlightuserdata(impl->L, (void*)&arg);
        lua_pushnil(impl->L);
        int ret = lua_pcall(impl->L, 2, 0, -4);
        if (!handleError(impl->L, loc, ret, "operation undefined", nullptr)) { return {}; }
        return syms;
    }

    bool callable(String name) override {
        if (!impl) { return false; }
        LuaClear lc(impl->L);
        lua_getglobal(impl->L, name.c_str());
        bool ret = lua_type(impl->L, -1) == LUA_TFUNCTION;
        return ret;
    }

    void main(Control &ctl) override {
        assert(impl);
        LuaClear lc(impl->L);
        lua_pushcfunction(impl->L, luaTraceback);
        lua_pushcfunction(impl->L, luaMain);
        lua_pushlightuserdata(impl->L, (void*)&ctl);
        switch (lua_pcall(impl->L, 1, 0, -3)) {
            case LUA_ERRRUN:
            case LUA_ERRERR: {
                Location loc("<lua_main>", 1, 1, "<lua_main>", 1, 1);
                std::ostringstream oss;
                oss << loc << ": " << "error: executing main function failed:\n"
                    << "  RuntimeError: " << lua_tostring(impl->L, -1) << "\n"
                    ;
                lua_pop(impl->L, 1);
                throw GringoError(oss.str().c_str());
            }
            case LUA_ERRMEM: { throw std::runtime_error("lua interpreter ran out of memory"); }
        }
    }
    LuaScript() = default;

private:
    std::unique_ptr<LuaImpl> impl;
};

void luaInitlib(lua_State *L, GringoModule &module) {
    ControlWrap::module = &module;
    luarequire_clingo(L);
}

UScript luaScript(GringoModule &module) {
    return gringo_make_unique<LuaScript>(module);
}

// }}}1

} // namespace Gringo

#else // WITH_LUA

#include "gringo/lua.hh"
#include "gringo/logger.hh"

namespace Gringo {

// {{{1 definition of LuaScript

void luaInitlib(lua_State *, GringoModule &) { }

UScript luaScript(GringoModule &) {
    return nullptr;
}

// }}}1

} // namespace Gringo

#endif // WITH_LUA

