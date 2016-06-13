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
#include "gringo/version.hh"
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

Symbol luaToVal(lua_State *L, int idx) {
    int type = lua_type(L, idx);
    switch(type) {
        case LUA_TSTRING: {
            char const *name = lua_tostring(L, idx);
            return protect(L, [name]() { return Symbol::createStr(name); });
        }
        case LUA_TNUMBER: {
            int num = lua_tointeger(L, idx);
            return Symbol::createNum(num);
        }
        case LUA_TUSERDATA: {
            bool check = false;
            if (lua_getmetatable(L, idx)) {                        // +1
                lua_getfield(L, LUA_REGISTRYINDEX, "clingo.Term"); // +1
                check = lua_rawequal(L, -1, -2);
                lua_pop(L, 2);                                     // -2
            }
            if (check) { return *(Symbol*)lua_touserdata(L, idx); }
        }
        default: { luaL_error(L, "cannot convert to value"); }
    }
    return {};
}

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

void luaToCpp(lua_State *L, int index, Potassco::WeightLit_t &x) {
    std::pair<Potassco::Lit_t&, Potassco::Weight_t&> y{x.lit, x.weight};
    luaToCpp(L, index, y);
}

SymVec *luaToVals(lua_State *L, int idx) {
    idx = lua_absindex(L, idx);
    luaL_checktype(L, idx, LUA_TTABLE);
    SymVec *vals = AnyWrap::new_<SymVec>(L);
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        Symbol val = luaToVal(L, -1);
        protect(L, [val,&vals](){ vals->push_back(val); });
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

// {{{1 lua C functions

using LuaCallArgs = std::tuple<char const *, SymSpan, SymVec>;

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

// {{{1 wrap SolveResult

struct SolveResult {
    static int new_(lua_State *L, Gringo::SolveResult res) {
        *(Gringo::SolveResult*)lua_newuserdata(L, sizeof(Gringo::SolveResult)) = res;
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        return 1;
    }
    static int satisfiable(lua_State *L) {
        Gringo::SolveResult &res = *(Gringo::SolveResult*)luaL_checkudata(L, 1, typeName);
        switch (res.satisfiable()) {
            case Gringo::SolveResult::Satisfiable:   { lua_pushboolean(L, true); return 1; }
            case Gringo::SolveResult::Unsatisfiable: { lua_pushboolean(L, false); return 1; }
            case Gringo::SolveResult::Unknown:       { lua_pushnil(L); return 1; }
        }
        return luaL_error(L, "must not happen");
    }
    static int unsatisfiable(lua_State *L) {
        Gringo::SolveResult &res = *(Gringo::SolveResult*)luaL_checkudata(L, 1, typeName);
        switch (res.satisfiable()) {
            case Gringo::SolveResult::Satisfiable:   { lua_pushboolean(L, false); return 1; }
            case Gringo::SolveResult::Unsatisfiable: { lua_pushboolean(L, true); return 1; }
            case Gringo::SolveResult::Unknown:       { lua_pushnil(L); return 1; }
        }
        return luaL_error(L, "must not happen");
    }
    static int unknown(lua_State *L) {
        Gringo::SolveResult &res = *(Gringo::SolveResult*)luaL_checkudata(L, 1, typeName);
        switch (res.satisfiable()) {
            case Gringo::SolveResult::Satisfiable:   { lua_pushboolean(L, false); return 1; }
            case Gringo::SolveResult::Unsatisfiable: { lua_pushboolean(L, false); return 1; }
            case Gringo::SolveResult::Unknown:       { lua_pushboolean(L, true); return 1; }
        }
        return luaL_error(L, "must not happen");
    }
    static int exhausted(lua_State *L) {
        Gringo::SolveResult &res = *(Gringo::SolveResult*)luaL_checkudata(L, 1, typeName);
        lua_pushboolean(L, res.exhausted());
        return 1;
    }
    static int interrupted(lua_State *L) {
        Gringo::SolveResult &res = *(Gringo::SolveResult*)luaL_checkudata(L, 1, typeName);
        lua_pushboolean(L, res.interrupted());
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
        Gringo::SolveResult &res = *(Gringo::SolveResult*)luaL_checkudata(L, 1, typeName);
        switch (res.satisfiable()) {
            case Gringo::SolveResult::Satisfiable:   { lua_pushstring(L, "SAT"); return 1; }
            case Gringo::SolveResult::Unsatisfiable: { lua_pushstring(L, "UNSAT"); return 1; }
            case Gringo::SolveResult::Unknown:       { lua_pushstring(L, "UNKNOWN"); return 1; }
        }
        return luaL_error(L, "must not happen");
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.SolveResult";
};

constexpr char const *SolveResult::typeName;

luaL_Reg const SolveResult::meta[] = {
    {"__tostring", toString},
    {nullptr, nullptr}
};

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

    static T *self(lua_State* L) {
        return (T*)luaL_checkudata(L, 1, T::typeName);
    }

    static int eq(lua_State *L) {
        T *a = static_cast<T*>(luaL_checkudata(L, 1, T::typeName));
        T *b = static_cast<T*>(luaL_checkudata(L, 2, T::typeName));
        lua_pushboolean(L, *a == *b);
        return 1;
    }
    static int lt(lua_State *L) {
        T *a = static_cast<T*>(luaL_checkudata(L, 1, T::typeName));
        T *b = static_cast<T*>(luaL_checkudata(L, 2, T::typeName));
        lua_pushboolean(L, *a < *b);
        return 1;
    }
    static int le(lua_State *L) {
        T *a = static_cast<T*>(luaL_checkudata(L, 1, T::typeName));
        T *b = static_cast<T*>(luaL_checkudata(L, 2, T::typeName));
        lua_pushboolean(L, *a <= *b);
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

// {{{1 wrap TheoryTerm

struct TheoryTermType {
    using Type = Gringo::TheoryData::TermType;
    static int addToRegistry(lua_State *L) {
        lua_createtable(L, 0, 6);
        for (auto t : { Type::Function, Type::Number, Type::Symbol, Type::Tuple, Type::List, Type::Set }) {
            *(Type*)lua_newuserdata(L, sizeof(Type)) = t;
            luaL_getmetatable(L, typeName);
            lua_setmetatable(L, -2);
            lua_setfield(L, -2, field_(t));
        }
        lua_setfield(L, -2, "TheoryTermType");
        return 0;
    }
    static char const *field_(Type t) {
        switch (t) {
            case Type::Function: { return "Function"; }
            case Type::Number:   { return "Number"; }
            case Type::Symbol:   { return "Symbol"; }
            case Type::Tuple:    { return "Tuple"; }
            case Type::List:     { return "List"; }
            case Type::Set:      { return "Set"; }
        }
        return "";
    }
    static int new_(lua_State *L, Type t) {
        lua_getfield(L, LUA_REGISTRYINDEX, "clingo");
        lua_getfield(L, -1, "TheoryTermType");
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

struct TheoryTerm {
    using Data = std::pair<Gringo::TheoryData const *, Id_t>;
    static int new_(lua_State *L, Gringo::TheoryData const *data, Id_t value) {
        new (lua_newuserdata(L, sizeof(Data))) Data{data, value};
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        return 1;
    }
    static int name(lua_State *L) {
        auto self = (Data*)luaL_checkudata(L, 1, typeName);
        lua_pushstring(L, protect(L, [self](){ return self->first->termName(self->second); }));
        return 1;
    }
    static int number(lua_State *L) {
        auto self = (Data*)luaL_checkudata(L, 1, typeName);
        lua_pushnumber(L, protect(L, [self](){ return self->first->termNum(self->second); }));
        return 1;
    }
    static int args(lua_State *L) {
        auto self = (Data*)luaL_checkudata(L, 1, typeName);
        auto args = self->first->termArgs(self->second);
        lua_createtable(L, args.size, 0);
        int i = 1;
        for (auto &x : args) {
            new_(L, self->first, x);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }
    static int toString(lua_State *L) {
        auto self = (Data*)luaL_checkudata(L, 1, typeName);
        auto *str = AnyWrap::new_<std::string>(L);
        lua_pushstring(L, protect(L, [self, str]() { return (*str = self->first->termStr(self->second)).c_str(); }));
        return 1;
    }
    static int type(lua_State *L) {
        auto self = (Data*)luaL_checkudata(L, 1, typeName);
        return TheoryTermType::new_(L, protect(L, [self]() { return self->first->termType(self->second); }));
    }
    static int eq(lua_State *L) {
        auto a = static_cast<Data*>(luaL_checkudata(L, 1, typeName));
        auto b = static_cast<Data*>(luaL_checkudata(L, 2, typeName));
        lua_pushboolean(L, a->second == b->second);
        return 1;
    }
    static int lt(lua_State *L) {
        auto a = static_cast<Data*>(luaL_checkudata(L, 1, typeName));
        auto b = static_cast<Data*>(luaL_checkudata(L, 2, typeName));
        lua_pushboolean(L, a->second < b->second);
        return 1;
    }
    static int le(lua_State *L) {
        auto a = static_cast<Data*>(luaL_checkudata(L, 1, typeName));
        auto b = static_cast<Data*>(luaL_checkudata(L, 2, typeName));
        lua_pushboolean(L, a->second <= b->second);
        return 1;
    }
    static int index(lua_State *L) {
        char const *field = luaL_checkstring(L, 2);
        if (strcmp(field, "type") == 0) { return type(L); }
        else if (strcmp(field, "name") == 0) { return name(L); }
        else if (strcmp(field, "args") == 0) { return args(L); }
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
    TheoryElement(Gringo::TheoryData const *data, Id_t idx) : data(data) , idx(idx) { }
    Gringo::TheoryData const *data;
    Id_t idx;

    static int terms(lua_State *L) {
        auto self = Object::self(L);
        auto args = protect(L, [self]() { return self->data->elemTuple(self->idx); });
        lua_createtable(L, args.size, 0);
        int i = 1;
        for (auto &x : args) {
            TheoryTerm::new_(L, self->data, x);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }

    static int condition(lua_State *L) {
        auto self = Object::self(L);
        auto args = protect(L, [self]() { return self->data->elemCond(self->idx); });
        lua_createtable(L, args.size, 0);
        int i = 1;
        for (auto &x : args) {
            lua_pushnumber(L, x);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }

    static int conditionLiteral(lua_State *L) {
        auto self = Object::self(L);
        lua_pushnumber(L, protect(L, [self]() { return self->data->elemCondLit(self->idx); }));
        return 1;
    }

    static int toString(lua_State *L) {
        auto self = Object::self(L);
        std::string *rep = AnyWrap::new_<std::string>(L);
        lua_pushstring(L, protect(L, [self, rep]() { return (*rep = self->data->elemStr(self->idx)).c_str(); }));
        return 1;
    }

    static int index(lua_State *L) {
        char const *field = luaL_checkstring(L, 2);
        if (strcmp(field, "terms") == 0) { return terms(L); }
        else if (strcmp(field, "condition") == 0) { return condition(L); }
        else if (strcmp(field, "condition_literal") == 0) { return conditionLiteral(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, field);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", field);
        }
    }

    static constexpr char const *typeName = "clingo.TheoryElement";
    static luaL_Reg const meta[];
};
bool operator< (TheoryElement const &a, TheoryElement const &b) { return a.idx <  b.idx; }
bool operator<=(TheoryElement const &a, TheoryElement const &b) { return a.idx <= b.idx; }
bool operator==(TheoryElement const &a, TheoryElement const &b) { return a.idx == b.idx; }

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
    TheoryAtom(Gringo::TheoryData const *data, Id_t idx) : data(data) , idx(idx) { }
    Gringo::TheoryData const *data;
    Id_t idx;

    static int elements(lua_State *L) {
        auto self = Object::self(L);
        auto args = protect(L, [self]() { return self->data->atomElems(self->idx); });
        lua_createtable(L, args.size, 0);
        int i = 1;
        for (auto &x : args) {
            TheoryElement::new_(L, self->data, x);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }

    static int term(lua_State *L) {
        auto self = Object::self(L);
        return TheoryTerm::new_(L, self->data, protect(L, [self]() { return self->data->atomTerm(self->idx); }));
    }

    static int literal(lua_State *L) {
        auto self = Object::self(L);
        lua_pushnumber(L, protect(L, [self]() { return self->data->atomLit(self->idx); }));
        return 1;
    }

    static int guard(lua_State *L) {
        auto self = Object::self(L);
        if (!protect(L, [self](){ return self->data->atomHasGuard(self->idx); })) {
            lua_pushnil(L);
            return 1;
        }
        lua_createtable(L, 2, 0);
        auto guard = protect(L, [self](){ return self->data->atomGuard(self->idx); });
        lua_pushstring(L, guard.first);
        lua_rawseti(L, -2, 1);
        TheoryTerm::new_(L, self->data, guard.second);
        lua_rawseti(L, -2, 2);
        return 1;
    }

    static int toString(lua_State *L) {
        auto self = Object::self(L);
        std::string *rep = AnyWrap::new_<std::string>(L);
        lua_pushstring(L, protect(L, [self, rep]() { return (*rep = self->data->atomStr(self->idx)).c_str(); }));
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

bool operator< (TheoryAtom const &a, TheoryAtom const &b) { return a.idx <  b.idx; }
bool operator<=(TheoryAtom const &a, TheoryAtom const &b) { return a.idx <= b.idx; }
bool operator==(TheoryAtom const &a, TheoryAtom const &b) { return a.idx == b.idx; }

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
    static int iter(lua_State *L, Gringo::TheoryData const *data) {
        lua_pushlightuserdata(L, (void*)data);
        lua_pushnumber(L, 0);
        lua_pushcclosure(L, iter_, 2);
        return 1;
    }

    static int iter_(lua_State *L) {
        auto data = (Gringo::TheoryData const *)lua_topointer(L, lua_upvalueindex(1));
        Id_t idx = lua_tointeger(L, lua_upvalueindex(2));
        if (idx < data->numAtoms()) {
            lua_pushnumber(L, idx + 1);
            lua_replace(L, lua_upvalueindex(2));
            TheoryAtom::new_(L, data, idx);
        }
        else { lua_pushnil(L); }
        return 1;
    }
};

// {{{1 wrap Term

struct TermType {
    enum Type { Number, String, Function, Inf, Sup };
    static int addToRegistry(lua_State *L) {
        lua_createtable(L, 0, 5);
        *(Type*)lua_newuserdata(L, sizeof(Type)) = Number;
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, "Number");
        *(Type*)lua_newuserdata(L, sizeof(Type)) = String;
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, "String");
        *(Type*)lua_newuserdata(L, sizeof(Type)) = Function;
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, "Function");
        *(Type*)lua_newuserdata(L, sizeof(Type)) = Inf;
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, "Inf");
        *(Type*)lua_newuserdata(L, sizeof(Type)) = Sup;
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, "Sup");
        lua_setfield(L, -2, "TermType");
        return 0;
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
        switch (*(Type*)luaL_checkudata(L, 1, typeName)) {
            case Number:   { lua_pushstring(L, "Number"); break; }
            case String:   { lua_pushstring(L, "String"); break; }
            case Function: { lua_pushstring(L, "Function"); break; }
            case Inf:      { lua_pushstring(L, "Inf"); break; }
            case Sup:      { lua_pushstring(L, "Sup"); break; }
        }
        return 1;
    }
    static luaL_Reg const meta[];
    static constexpr char const *typeName = "clingo.TermType";
};

constexpr char const *TermType::typeName;

luaL_Reg const TermType::meta[] = {
    {"__eq", eq},
    {"__lt", lt},
    {"__le", le},
    {"__tostring", toString},
    { nullptr, nullptr }
};

struct Term {
    static int new_(lua_State *L, Symbol v) {
        if (v.type() == SymbolType::Sup) {
            lua_getfield(L, LUA_REGISTRYINDEX, "clingo");
            lua_getfield(L, -1, "Sup");
            lua_replace(L, -2);
        }
        else if (v.type() == SymbolType::Inf) {
            lua_getfield(L, LUA_REGISTRYINDEX, "clingo");
            lua_getfield(L, -1, "Inf");
            lua_replace(L, -2);
        }
        else {
            *(Symbol*)lua_newuserdata(L, sizeof(Symbol)) = v;
            luaL_getmetatable(L, typeName);
            lua_setmetatable(L, -2);
        }
        return 1;
    }
    static int addToRegistry(lua_State *L) {
        *(Symbol*)lua_newuserdata(L, sizeof(Symbol)) = Symbol::createSup();
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, "Sup");
        *(Symbol*)lua_newuserdata(L, sizeof(Symbol)) = Symbol::createInf();
        luaL_getmetatable(L, typeName);
        lua_setmetatable(L, -2);
        lua_setfield(L, -2, "Inf");
        return 0;
    }
    static int newFun(lua_State *L) {
        char const *name = luaL_checklstring(L, 1, nullptr);
        bool sign = false;
        if (!lua_isnone(L, 3) && !lua_isnil(L, 3)) {
            sign = lua_toboolean(L, 3);
        }
        if (name[0] == '\0' && sign) { luaL_argerror(L, 2, "tuples must not have signs"); }
        if (lua_isnoneornil(L, 2)) {
            return new_(L, protect(L, [name, sign](){ return Symbol::createId(name, sign); }));
        }
        else {
            SymVec *vals = luaToVals(L, 2);
            return new_(L, protect(L, [name, sign, vals](){ return vals->empty() && name[0] != '\0' ? Symbol::createId(name, sign) : Symbol::createFun(name, Potassco::toSpan(*vals), sign); }));
        }
    }
    static int newTuple(lua_State *L) {
        lua_pushstring(L, "");
        lua_insert(L, 1);
        return newFun(L);
    }
    static int newNumber(lua_State *L) {
        return Term::new_(L, Symbol::createNum(luaL_checkinteger(L, 1)));
    }
    static int newString(lua_State *L) {
        return Term::new_(L, Symbol::createStr(luaL_checkstring(L, 1)));
    }
    VALUE_CMP(Term)
    static int name(lua_State *L) {
        Symbol val = *(Symbol*)luaL_checkudata(L, 1, typeName);
        if (val.type() == SymbolType::Fun) {
            lua_pushstring(L, protect(L, [val]() { return val.name().c_str(); }));
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    static int string(lua_State *L) {
        Symbol val = *(Symbol*)luaL_checkudata(L, 1, typeName);
        if (val.type() == SymbolType::Str) {
            lua_pushstring(L, protect(L, [val]() { return val.string().c_str(); }));
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    static int number(lua_State *L) {
        Symbol val = *(Symbol*)luaL_checkudata(L, 1, typeName);
        if (val.type() == SymbolType::Num) {
            lua_pushnumber(L, protect(L, [val]() { return val.num(); }));
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    static int sign(lua_State *L) {
        Symbol val = *(Symbol*)luaL_checkudata(L, 1, typeName);
        if (val.type() == SymbolType::Fun) {
            lua_pushboolean(L, protect(L, [val]() { return val.sign(); }));
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    static int args(lua_State *L) {
        Symbol val = *(Symbol*)luaL_checkudata(L, 1, typeName);
        if (val.type() == SymbolType::Fun) {
            lua_createtable(L, val.args().size, 0);
            int i = 1;
            for (auto &x : val.args()) {
                Term::new_(L, x);
                lua_rawseti(L, -2, i++);
            }
        }
        else {
            lua_pushnil(L);
        }
        return 1;
    }
    static int toString(lua_State *L) {
        std::string *rep = AnyWrap::new_<std::string>(L);
        Symbol val = *(Symbol*)luaL_checkudata(L, 1, typeName);
        lua_pushstring(L, protect(L, [val, rep]() {
            std::ostringstream oss;
            oss << val;
            *rep = oss.str();
            return rep->c_str();
        }));
        return 1;
    }
    static int type(lua_State *L) {
        Symbol val = *(Symbol*)luaL_checkudata(L, 1, typeName);
        lua_getfield(L, LUA_REGISTRYINDEX, "clingo");
        lua_getfield(L, -1, "TermType");
        char const *field = "";
        switch (val.type()) {
            case SymbolType::Str:     { field = "String"; break; }
            case SymbolType::Num:     { field = "Number"; break; }
            case SymbolType::Inf:     { field = "Inf"; break; }
            case SymbolType::Sup:     { field = "Sup"; break; }
            case SymbolType::Fun:     { field = "Function"; break; }
            case SymbolType::Special: { luaL_error(L, "must not happen"); }
        }
        lua_getfield(L, -1, field);
        return 1;
    }
    static int index(lua_State *L) {
        char const *field = luaL_checkstring(L, 2);
        if (strcmp(field, "sign") == 0) { return sign(L); }
        else if (strcmp(field, "args") == 0) { return args(L); }
        else if (strcmp(field, "name") == 0) { return name(L); }
        else if (strcmp(field, "string") == 0) { return string(L); }
        else if (strcmp(field, "number") == 0) { return number(L); }
        else if (strcmp(field, "type") == 0) { return type(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, field);
            return 1;
        }
    }
    static constexpr char const *typeName = "clingo.Term";
    static luaL_Reg const meta[];
};

constexpr char const *Term::typeName;
luaL_Reg const Term::meta[] = {
    {"__tostring", toString},
    {"__eq", eqTerm},
    {"__lt", ltTerm},
    {"__le", leTerm},
    {nullptr, nullptr}
};

int luaCall(lua_State *L) {
    auto &args = *(LuaCallArgs*)lua_touserdata(L, 1);
    bool hasContext = !lua_isnil(L, 2);
    if (hasContext) {
        lua_getfield(L, 2, std::get<0>(args));
        lua_pushvalue(L, 2);
    }
    else { lua_getglobal(L, std::get<0>(args)); }
    for (auto &x : std::get<1>(args)) { Term::new_(L, x); }
    lua_call(L, std::get<1>(args).size + hasContext, 1);
    if (lua_type(L, -1) == LUA_TTABLE) {
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            Symbol val = luaToVal(L, -1);
            protect(L, [val, &args]() { std::get<2>(args).emplace_back(val); });
            lua_pop(L, 1);
        }
    }
    else {
        Symbol val = luaToVal(L, -1);
        protect(L, [val, &args]() { std::get<2>(args).emplace_back(val); });
    }
    return 0;
}

// {{{1 wrap SolveControl

struct SolveControl {
    static int getClause(lua_State *L, bool invert) {
        Model const *& model =  *(Model const **)luaL_checkudata(L, 1, typeName);
        Gringo::Model::LitVec *lits = AnyWrap::new_<Gringo::Model::LitVec>(L);
        luaL_checktype(L, 2, LUA_TTABLE);
        lua_pushnil(L);
        while (lua_next(L, 2)) {
            luaL_checktype(L, -1, LUA_TTABLE);
            lua_pushnil(L);
            if (!lua_next(L, -2)) { luaL_error(L, "atom/boolean pair expected"); }
            Symbol atom = luaToVal(L, -1);
            lua_pop(L, 1);
            if (!lua_next(L, -2)) { luaL_error(L, "atom/boolean pair expected"); }
            bool truth = lua_toboolean(L, -1);
            lua_pop(L, 1);
            if (lua_next(L, -2)) { luaL_error(L, "atom/boolean pair expected"); }
            protect(L, [invert, atom, truth, lits](){ lits->emplace_back(atom, truth ^ invert); });
            lua_pop(L, 1);
        }
        protect(L, [model, lits](){ model->addClause(*lits); });
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

struct Model {
    static int contains(lua_State *L) {
        Gringo::Model const *& model =  *(Gringo::Model const **)luaL_checkudata(L, 1, typeName);
        Symbol val = luaToVal(L, 2);
        lua_pushboolean(L, protect(L, [val, model]() { return model->contains(val); }));
        return 1;
    }
    static int atoms(lua_State *L) {
        Gringo::Model const *& model = *(Gringo::Model const **)luaL_checkudata(L, 1, typeName);
        unsigned atomset = 0;
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
        lua_getfield(L, 2, "comp");
        if (lua_toboolean(L, -1)) { atomset |= clingo_show_type_comp; }
        lua_pop(L, 1);
        SymSpan atoms = protect(L, [&model, atomset]() { return model->atoms(atomset); });
        lua_createtable(L, atoms.size, 0);
        int i = 1;
        for (auto x : atoms) {
            Term::new_(L, x);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }
    static int optimization(lua_State *L) {
        Gringo::Model const *& model = *(Gringo::Model const **)luaL_checkudata(L, 1, typeName);
        Int64Vec *values = AnyWrap::new_<Int64Vec>(L);
        protect(L, [&model, values]() { *values = model->optimization(); });
        lua_createtable(L, values->size(), 0);
        int i = 1;
        for (auto x : *values) {
            lua_pushinteger(L, x);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }
    static int toString(lua_State *L) {
        Gringo::Model const *& model =  *(Gringo::Model const **)luaL_checkudata(L, 1, typeName);
        std::string *rep = AnyWrap::new_<std::string>(L);
        lua_pushstring(L, protect(L, [model, rep]() {
            auto printAtom = [](std::ostream &out, Symbol val) {
                auto sig = val.sig();
                if (val.type() == SymbolType::Fun && sig.name() == "$" && sig.arity() == 2 && !sig.sign()) {
                    auto args = val.args().first;
                    out << args[0] << "=" << args[1];
                }
                else { out << val; }
            };
            std::ostringstream oss;
            print_comma(oss, model->atoms(clingo_show_type_shown), " ", printAtom);
            *rep = oss.str();
            return rep->c_str();
        }));
        return 1;
    }
    static int context(lua_State *L) {
        auto &model = *(Gringo::Model const **)luaL_checkudata(L, 1, typeName);
        *(Gringo::Model const **)lua_newuserdata(L, sizeof(Gringo::Model*)) = model;
        luaL_getmetatable(L, SolveControl::typeName);
        lua_setmetatable(L, -2);
        return 1;
    }
    static int index(lua_State *L) {
        char const *name = luaL_checkstring(L, 2);
        if (strcmp(name, "optimization") == 0) {
            return optimization(L);
        }
        else if (strcmp(name, "context") == 0) {
            return context(L);
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
    {"atoms", atoms},
    {"contains", contains},
    {nullptr, nullptr}
};

// {{{1 wrap Statistics

int newStatistics(lua_State *L, Statistics const *stats) {
    char const *prefix = lua_tostring(L, -1); // stack + 1
    auto ret           = protect(L, [stats, prefix]{ return stats->getStat(prefix); });
    switch (ret.error()) {
        case Statistics::error_none: {
            lua_pop(L, 1);
            lua_pushnumber(L, (double)ret);
            return 1;
        }
        case Statistics::error_not_available: {
            return luaL_error(L, "error_not_available: %s", prefix);
        }
        case Statistics::error_unknown_quantity: {
            return luaL_error(L, "error_unknown_quantity: %s", prefix);
        }
        case Statistics::error_ambiguous_quantity: {
            char const *keys = protect(L, [stats, prefix]() { return stats->getKeys(prefix); });
            if (!keys) { luaL_error(L, "error zero keys string: %s", prefix); }
            lua_newtable(L); // stack + 2
            for (char const *it = keys, *sep = *prefix ? "." : ""; *it; it+= strlen(it) + 1) {
                if (strcmp(it, "__len") == 0) {
                    int len = (int)(double)protect(L, [stats, prefix]{ return stats->getStat((std::string(prefix) + ".__len").c_str()); });
                    for (int i = 1; i <= len; ++i) {
                        lua_pushvalue(L, -2);
                        lua_pushliteral(L, ".");
                        lua_pushinteger(L, i-1);
                        lua_concat(L, 3);        // stack + 3
                        newStatistics(L, stats); // stack + 3
                        lua_rawseti(L, -2, i);   // stack + 2
                    }
                    break;
                }
                else {
                    it += (*it == '.');
                    lua_pushlstring(L, it, strlen(it)); // stack + 3
                    lua_pushvalue(L, -3);
                    lua_pushstring(L, sep);
                    lua_pushstring(L, it);
                    lua_concat(L, 3);        // stack + 4
                    newStatistics(L, stats); // stack + 4
                    lua_rawset(L, -3);       // stack + 2
                }
            }
            lua_replace(L, -2);
            return 1;
        }

    }
    assert(false);
    return 1;
}

// {{{1 wrap SolveFuture

struct SolveFuture {
    static int get(lua_State *L) {
        Gringo::SolveFuture *& future = *(Gringo::SolveFuture **)luaL_checkudata(L, 1, typeName);
        SolveResult::new_(L, protect(L, [future]() { return future->get(); }));
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
        lua_pushnumber(L, protect(L, [iter]() { return (int)iter->get(); }));
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
        unsigned key;
        bool hasSubKey = protect(L, [self, name, &key] { return self.proxy->hasSubKey(self.key, name, &key); });
        if (hasSubKey) {
            new_(L, key, *self.proxy);
            auto &sub = *(Configuration *)lua_touserdata(L, -1);
            if (desc) {
                lua_pushstring(L, sub.help);
                return 1;
            }
            else if (sub.nValues < 0) { return 1; }
            else {
                std::string *value = AnyWrap::new_<std::string>(L);
                bool ret = protect(L, [&sub, value]() { return sub.proxy->getKeyValue(sub.key, *value); });
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
        unsigned key;
        bool hasSubKey = protect(L, [self, name, &key] { return self.proxy->hasSubKey(self.key, name, &key); });
        if (hasSubKey) {
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
        Term::new_(L, atom);
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
        Gringo::Symbol atom = luaToVal(L, 2);
        auto range = protect(L, [self, atom]() { return self.atoms.lookup(atom); });
        if (self.atoms.valid(range)) { SymbolicAtom::new_(L, self.atoms, range); }
        else                         { lua_pushnil(L); }               // +1
        return 1;
    }

    static int by_signature(lua_State *L) {
        auto &self = get_self(L);
        char const *name = luaL_checkstring(L, 2);
        int arity = luaL_checkinteger(L, 3);
        auto range = protect(L, [&self, name, arity]() { return self.atoms.begin(Sig(name, arity, false)); });
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
            lua_createtable(L, 2, 0);                          // +1
            lua_pushstring(L, sig.name().c_str());             // +1
            lua_rawseti(L, -2, 1);                             // -1
            lua_pushinteger(L, sig.arity());                   // +1
            lua_rawseti(L, -2, 2);                             // -1
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
        auto self = Object<Backend>::self(L);
        auto atom = protect(L, [self](){ return self->ctl.addProgramAtom(); });
        lua_pushinteger(L, atom);
        return 1;
    }

    static int addRule(lua_State *L) {
        auto *self = Object<Backend>::self(L);
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
            Gringo::outputRule(self->backend, choice, *head, *body);
        });
        return 0;
    }

    static int addWeightRule(lua_State *L) {
        auto *self = Object<Backend>::self(L);
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
            Gringo::outputRule(self->backend, choice, *head, lower, *body);
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

struct LuaClear {
    LuaClear(lua_State *L) : L(L), n(lua_gettop(L)) { }
    ~LuaClear() { lua_settop(L, n); }
    lua_State *L;
    int n;
};

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
        LuaCallArgs arg(name.c_str(), args, {});
        lua_pushcfunction(L, luaTraceback);
        lua_pushcfunction(L, luaCall);
        lua_pushlightuserdata(L, (void*)&arg);
        lua_pushvalue(L, idx);
        int ret = lua_pcall(L, 2, 0, -4);
        if (!handleError(L, loc, ret, "operation undefined", &log)) { return {}; }
        return std::move(std::get<2>(arg));
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
            SymVec *args = luaToVals(L, -1);
            protect(L, [name, args, vec](){ vec->emplace_back(name, *args); });
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
        if (ret.type() == SymbolType::Special) { lua_pushnil(L); }
        else                                   { Term::new_(L, ret); }
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
                Symbol atom = luaToVal(L, -1);
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
        lua_pushstring(L, "stats");
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
            return (int)ctl.solve(!model ? Control::ModelHandler(nullptr) : [L, model, mhIndex, mIndex](Gringo::Model const &m) -> bool {
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
        lua_pushstring(L, "stats");
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
        Symbol atom = luaToVal(L, 2);
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
        Symbol atom = luaToVal(L, 2);
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
        if (strcmp(name, "use_enum_assumption") == 0) {
            bool enabled = lua_toboolean(L, 3);
            checkBlocked(L, ctl, "use_enum_assumption");
            protect(L, [&ctl, enabled]() { ctl.useEnumAssumption(enabled); });
            return 0;
        }
        return luaL_error(L, "unknown field: %s", name);
    }

    static int index(lua_State *L) {
        auto &ctl = get_self(L).ctl;
        char const *name = luaL_checkstring(L, 2);
        if (strcmp(name, "use_enum_assumption") == 0) {
            bool enabled = protect(L, [&ctl]() { return ctl.useEnumAssumption(); });
            lua_pushboolean(L, enabled);     // stack +1
            return 1;
        }
        else if (strcmp(name, "stats") == 0) {
            checkBlocked(L, ctl, "stats");
            lua_pushstring(L, "stats");      // stack +1
            lua_rawget(L, 1);                // stack +0
            if (lua_isnil(L, -1)) {
                auto stats = protect(L, [&ctl](){ return ctl.getStats(); });
                lua_pop(L, 1);               // stack -1
                lua_pushliteral(L, "");      // stack +1
                newStatistics(L, stats);     // stack +0
                lua_pushstring(L, "stats");  // stack +1
                lua_pushvalue(L, -2);        // stack +1
                lua_rawset(L, 1);            // stack -2
            }
            return 1;
        }
        else if (strcmp(name, "conf") == 0) {
            checkBlocked(L, ctl, "conf");
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
            return TheoryIter::iter(L, &ctl.theory());
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
        protect(L, [&cargs](){ cargs->push_back("clingo"); });
        for (auto &arg : *args) {
            protect(L, [&arg, &cargs](){ cargs->push_back(arg.c_str()); });
        }
        protect(L, [&cargs](){ cargs->push_back(nullptr); });
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
        auto self = Object::self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushnumber(L, protect(L, [self, lit]() { return self->init->mapLit(lit); }));
        return 1;
    }

    static int numThreads(lua_State *L) {
        auto self = Object::self(L);
        lua_pushnumber(L, protect(L, [self]() { return self->init->threads(); }));
        return 1;
    }
    static int addWatch(lua_State *L) {
        auto self = Object::self(L);
        int lit = luaL_checkinteger(L, 2);
        protect(L, [self, lit]() { self->init->addWatch(lit); });
        return 0;
    }

    static int index(lua_State *L) {
        char const *name = luaL_checkstring(L, 2);
        if (strcmp(name, "theory_atoms")   == 0) { return TheoryIter::iter(L, &self(L)->init->theory()); }
        else if (strcmp(name, "symbolic_atoms")   == 0) { return SymbolicAtoms::new_(L, self(L)->init->getDomain()); }
        else if (strcmp(name, "threads")   == 0) { return numThreads(L); }
        else {
            lua_getmetatable(L, 1);
            lua_getfield(L, -1, name);
            return !lua_isnil(L, -1) ? 1 : luaL_error(L, "unknown field: %s", name);
        }
    }

    static int setState(lua_State *L) {
        auto self = Object::self(L);
        int id = luaL_checknumber(L, 2);
        luaL_checkany(L, 3);
        if (id < 1 || id > (int)self->init->threads()) {
            luaL_error(L, "invalid solver thread id %d", id);
        }
        lua_xmove(L, self->T, 1);
        lua_rawseti(self->T, 2, id);
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
        auto self = Object::self(L);
        lua_pushboolean(L, protect(L, [self]() { return self->ass.hasConflict(); }));
        return 1;
    }

    static int decisionLevel(lua_State *L) {
        auto self = Object::self(L);
        lua_pushinteger(L, protect(L, [self]() { return self->ass.level(); }));
        return 1;
    }

    static int hasLit(lua_State *L) {
        auto self = Object::self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushboolean(L, protect(L, [self,lit]() { return self->ass.hasLit(lit); }));
        return 1;
    }

    static int level(lua_State *L) {
        auto self = Object::self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushinteger(L, protect(L, [self,lit]() { return self->ass.level(lit); }));
        return 1;
    }

    static int decision(lua_State *L) {
        auto self = Object::self(L);
        int level = luaL_checkinteger(L, 2);
        lua_pushinteger(L, protect(L, [self,level]() { return self->ass.decision(level); }));
        return 1;
    }

    static int isFixed(lua_State *L) {
        auto self = Object::self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushboolean(L, protect(L, [self,lit]() { return self->ass.isFixed(lit); }));
        return 1;
    }

    static int isTrue(lua_State *L) {
        auto self = Object::self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushboolean(L, protect(L, [self,lit]() { return self->ass.isTrue(lit); }));
        return 1;
    }

    static int value(lua_State *L) {
        auto self = Object::self(L);
        int lit = luaL_checkinteger(L, 2);
        auto val = protect(L, [self, lit]() { return self->ass.value(lit); });
        if (val == Potassco::Value_t::Free) { lua_pushnil(L); }
        else { lua_pushboolean(L, val == Potassco::Value_t::True); }
        lua_pushboolean(L, val);
        return 1;
    }

    static int isFalse(lua_State *L) {
        auto self = Object::self(L);
        int lit = luaL_checkinteger(L, 2);
        lua_pushboolean(L, protect(L, [self,lit]() { return self->ass.isFalse(lit); }));
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
        auto self = Object::self(L);
        lua_pushinteger(L, protect(L, [self]() { return self->ctl->id() + 1; }));
        return 1;
    }

    static int assignment(lua_State *L) {
        auto self = Object::self(L);
        Assignment::new_(L, protect(L, [self]() { return &self->ctl->assignment(); }));
        return 1;
    }

    static int addClauseOrNogood(lua_State *L, bool invert) {
        auto self = Object::self(L);
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
            return self->ctl->addClause(Potassco::toSpan(*lits), static_cast<Potassco::Clause_t>(type));
        }));
        lua_replace(L, lits_index);
        lua_settop(L, lits_index);
        return 1;
    }

    static int addClause(lua_State *L) {
        return addClauseOrNogood(L, false);
    }

    static int addNogood(lua_State *L) {
        return addClauseOrNogood(L, true);
    }

    static int propagate(lua_State *L) {
        auto self = Object::self(L);
        lua_pushboolean(L, protect(L, [self]() { return self->ctl->propagate(); }));
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
    static int getState(lua_State *L, lua_State *T, Potassco::Id_t id) {
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
            lua_pushnumber(L, solver->id());             // +1
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

// {{{1 wrap module functions

int parseTerm(lua_State *L) {
    char const *str = luaL_checkstring(L, 1);
    Symbol val = protect(L, [str]() { return ControlWrap::module->parseValue(str, nullptr, 20); });
    if (val.type() == SymbolType::Special) { lua_pushnil(L); }
    else { Term::new_(L, val); }
    return 1;
}


// {{{1 clingo library

int luaopen_clingo(lua_State* L) {
    static luaL_Reg clingoLib[] = {
        {"fun", Term::newFun},
        {"tuple", Term::newTuple},
        {"number", Term::newNumber},
        {"str", Term::newString},
        {"Control", ControlWrap::newControl},
        {"parse_term", parseTerm},
        {nullptr, nullptr}
    };

    lua_regMeta(L, Term::typeName,           Term::meta, Term::index);
    lua_regMeta(L, TermType::typeName,       TermType::meta);
    lua_regMeta(L, Model::typeName,          Model::meta, Model::index);
    lua_regMeta(L, SolveControl::typeName,   SolveControl::meta);
    lua_regMeta(L, SolveFuture::typeName,    SolveFuture::meta);
    lua_regMeta(L, SolveIter::typeName,      SolveIter::meta);
    lua_regMeta(L, ControlWrap::typeName,    ControlWrap::meta, ControlWrap::index, ControlWrap::newindex);
    lua_regMeta(L, Configuration::typeName,  Configuration::meta, Configuration::index, Configuration::newindex);
    lua_regMeta(L, SolveResult::typeName,    SolveResult::meta, SolveResult::index);
    lua_regMeta(L, SymbolicAtoms::typeName,  SymbolicAtoms::meta, SymbolicAtoms::index);
    lua_regMeta(L, SymbolicAtom::typeName,   SymbolicAtom::meta, SymbolicAtom::index);
    lua_regMeta(L, AnyWrap::typeName,        AnyWrap::meta);
    lua_regMeta(L, TheoryTermType::typeName, TheoryTermType::meta);
    lua_regMeta(L, TheoryTerm::typeName,     TheoryTerm::meta, TheoryTerm::index);
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

    lua_pushstring(L, GRINGO_VERSION);
    lua_setfield(L, -2, "__version__");

    TermType::addToRegistry(L);
    Term::addToRegistry(L);
    TheoryTermType::addToRegistry(L);

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

Lua::Lua(GringoModule &module) {
    ControlWrap::module = &module;
}

bool Lua::exec(Location const &loc, String code) {
    if (!impl) { impl = gringo_make_unique<LuaImpl>(); }
    LuaClear lc(impl->L);
    std::stringstream oss;
    oss << loc;
    lua_pushcfunction(impl->L, luaTraceback);
    int ret = luaL_loadbuffer(impl->L, code.c_str(), code.length(), oss.str().c_str());
    handleError(impl->L, loc, ret, "parsing lua script failed", nullptr);
    ret = lua_pcall(impl->L, 0, 0, -2);
    handleError(impl->L, loc, ret, "running lua script failed", nullptr);
    return true;
}

SymVec Lua::call(Location const &loc, String name, SymSpan args, Logger &log) {
    assert(impl);
    LuaClear lc(impl->L);
    LuaCallArgs arg(name.c_str(), args, {});
    lua_pushcfunction(impl->L, luaTraceback);
    lua_pushcfunction(impl->L, luaCall);
    lua_pushlightuserdata(impl->L, (void*)&arg);
    lua_pushnil(impl->L);
    int ret = lua_pcall(impl->L, 2, 0, -4);
    if (!handleError(impl->L, loc, ret, "operation undefined", &log)) { return {}; }
    return std::move(std::get<2>(arg));
}

bool Lua::callable(String name) {
    if (!impl) { return false; }
    LuaClear lc(impl->L);
    lua_getglobal(impl->L, name.c_str());
    bool ret = lua_type(impl->L, -1) == LUA_TFUNCTION;
    return ret;
}

void Lua::main(Control &ctl) {
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
void Lua::initlib(lua_State *L, GringoModule &module) {
    ControlWrap::module = &module;
    luarequire_clingo(L);
}
Lua::~Lua() = default;

// }}}1

} // namespace Gringo

#else // WITH_LUA

#include "gringo/lua.hh"
#include "gringo/logger.hh"

namespace Gringo {

// {{{1 definition of LuaImpl

struct LuaImpl { };

// {{{1 definition of Lua

Lua::Lua(Gringo::GringoModule &) { }
bool Lua::exec(Location const &loc, String) {
    std::ostringstream oss;
    oss << loc << ": error: clingo has been build without lua support\n";
    throw GringoError(oss.str().c_str());
}
bool Lua::callable(String) {
    return false;
}
SymVec Lua::call(Location const &, String, SymSpan, Logger &) {
    return {};
}
void Lua::main(Control &) { }
void Lua::initlib(lua_State *, Gringo::GringoModule &) {
    throw std::runtime_error("clingo lib has been build without lua support");
}
Lua::~Lua() = default;

// }}}1

} // namespace Gringo

#endif // WITH_LUA

