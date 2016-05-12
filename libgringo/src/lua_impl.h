// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#include <gringo/grounder.h>
#include <gringo/luaterm.h>
#include <gringo/lualit.h>
#include <gringo/func.h>

#if WITH_LUA == 1
extern "C"
{
#	include <lua.h>
#	include <lualib.h>
#	include <lauxlib.h>
}
#else
#	include <lua/lua.h>
#	include <lua/lualib.h>
#	include <lua/lauxlib.h>
#	ifdef WITH_LUASQL
#		include <luasql/luasql.h>
#	endif
#endif

namespace
{
	static const char *VAL = "Val";

	static const char *typeVal(lua_State *L, Val *val)
	{
		(void)L;
		switch(val->type)
		{
			case Val::INF:    { return "INF"; }
			case Val::ID:     { return "ID"; }
			case Val::STRING: { return "STRING"; }
			case Val::NUM:    { return "NUM"; }
			case Val::FUNC:   { return "FUNC"; }
			case Val::SUP:    { return "SUP"; }
			default:          { return "UNKNOWN"; }
		}
	}

	static Val *checkVal (lua_State *L, int index)
	{
		Val *val;
		luaL_checktype(L, index, LUA_TUSERDATA);
		val = (Val *)luaL_checkudata(L, index, VAL);
		if (val == NULL) { luaL_argerror(L, index, VAL); }
		return val;
	}

	static Storage* checkStorage(lua_State *L)
	{
		lua_pushliteral(L, "Val.storage");
		lua_rawget(L, LUA_REGISTRYINDEX);
		Storage *storage = (Storage *)lua_touserdata(L, -1);
		lua_pop(L, 1); // pop storage
		return storage;
	}

	static void pushVal (lua_State *L, const Val &val)
	{
		switch(val.type)
		{
			case Val::STRING:
			{
				Storage *storage = checkStorage(L);
				lua_pushstring(L, storage->string(val.index).c_str());
				break;
			}
			case Val::NUM:
			{
				lua_pushinteger(L, val.num);
				break;
			}
			default:
			{
				*(Val*)lua_newuserdata(L, sizeof(Val)) = val;
				luaL_getmetatable(L, VAL);
				lua_setmetatable(L, -2);
				break;
			}
		}
	}

	static Val toVal(lua_State *L, int index)
	{
		int type = lua_type(L, index);
		switch(type)
		{
			case LUA_TSTRING:
			{
				const char *id   = lua_tostring(L, index);
				Storage *storage = checkStorage(L);
				return Val::create(Val::STRING, storage->index(std::string(id)));
			}
			case LUA_TNUMBER:
			{
				int num = lua_tointeger(L, index);
				return Val::create(Val::NUM, num);
			}
			case LUA_TBOOLEAN:
			{
				int num = lua_toboolean(L, index);
				return Val::create(Val::NUM, num);
			}
			case LUA_TUSERDATA:
			{
				return *checkVal(L, index);
			}
			default:
			{
				std::stringstream ss;
				ss << "string, number, or Val expected, got ";
				ss << lua_typename(L, type);
				luaL_argerror(L, index, ss.str().c_str());
				return Val::create();
			}
		}
	}

	static int typeVal(lua_State *L, int index)
	{
		int type = lua_type(L, index);
		switch(type)
		{
			case LUA_TNUMBER: { return Val::NUM; }
			case LUA_TSTRING: { return Val::STRING; }
			default:
			{
				Val *val  = checkVal(L, index);
				return val->type;
			}
		}
	}

	static int Val_new (lua_State *L)
	{
		int type  = luaL_checkinteger(L, 1);
		switch(type)
		{
			case Val::STRING:
			{
				luaL_checkstring(L, 2);
				lua_pushvalue(L, 2);
				break;
			}
			case Val::NUM:
			{
				luaL_checkinteger(L, 2);
				lua_pushinteger(L, lua_tointeger(L, 2));
				break;
			}
			case Val::SUP:
			case Val::INF:
			{
				pushVal(L, Val::create(type, 0));
				break;
			}
			case Val::ID:
			{
				Storage *storage = checkStorage(L);
				const char *name;
				name = luaL_checkstring(L, 2);
				uint32_t index = storage->index(std::string(name));
				pushVal(L, Val::create(type, index));
				break;
			}
			case Val::FUNC:
			{
				Storage *storage = checkStorage(L);
				const char *name;
				int table;
				if(lua_type(L, 2) == LUA_TTABLE)
				{
					name = "";
					luaL_checktype(L, 2, LUA_TTABLE);
					table = 2;
				}
				else
				{
					name = luaL_checkstring(L, 2);
					luaL_checktype(L, 3, LUA_TTABLE);
					table = 3;
				}
				ValVec vals;
				lua_pushnil(L);
				while (lua_next(L, table) != 0)
				{
					vals.push_back(toVal(L, -1));
					lua_pop(L, 1); // pop val
				}
				uint32_t index = storage->index(Func(storage, storage->index(std::string(name)), vals));
				pushVal(L, Val::create(type, index));
				break;
			}
			default:
			{
				luaL_argerror(L, 2, "unknown type");
				break;
			}
		}
		return 1;
	}

	static int Val_type (lua_State *L)
	{
		lua_pushinteger(L, typeVal(L, 1));
		return 1;
	}

	static int Val_name (lua_State *L)
	{
		Val *val  = checkVal(L, 1);
		if(val->type != Val::FUNC && val->type != Val::ID)
		{
			std::stringstream ss;
			ss << "Val.FUNC or Val.ID expected, but got Val.";
			ss << typeVal(L, val);
			luaL_argerror(L, 1, ss.str().c_str());
		}
		Storage *storage = checkStorage(L);
		if(val->type == Val::FUNC)
		{
			const Func &f = storage->func(val->index);
			lua_pushstring(L, storage->string(f.name()).c_str());
		}
		else
		{
			lua_pushstring(L, storage->string(val->index).c_str());
		}
		return 1;
	}

	static int Val_args (lua_State *L)
	{
		Val *val  = checkVal(L, 1);
		if(val->type != Val::FUNC)
		{
			std::stringstream ss;
			ss << "Val.FUNC expected, but got ";
			ss << typeVal(L, val);
			luaL_argerror(L, 1, ss.str().c_str());
		}
		const Func &f = checkStorage(L)->func(val->index);
		lua_createtable(L, f.args().size(), 0);
		int i = 1;
		foreach(const Val &val, f.args())
		{
			lua_pushinteger(L, i++);
			pushVal(L, val);
			lua_rawset(L, -3);
		}
		return 1;
	}

	static int Val_cmp (lua_State *L)
	{
		int t1 = typeVal(L, 1);
		int t2 = typeVal(L, 2);
		if(t1 != t2) { lua_pushinteger(L, t1 < t2 ? -1 : 1); }
		else
		{
			switch(t1)
			{
				case Val::STRING:
				{
					lua_pushinteger(L, strcmp(lua_tostring(L, 1), lua_tostring(L, 2)));
					break;
				}
				case Val::NUM:
				{
					lua_pushinteger(L, lua_tointeger(L, 1) - lua_tointeger(L, 2));
					break;
				}
				case Val::ID:
				case Val::FUNC:
				{
					Storage *storage = checkStorage(L);
					lua_pushinteger(L, checkVal(L, 1)->compare(*checkVal(L, 2), storage));
					break;
				}
				default:
				{
					lua_pushinteger(L, 0);
					break;
				}
			}
		}
		return 1;
	}

	static const luaL_reg Val_methods[] =
	{
		{"new",  Val_new},
		{"cmp",  Val_cmp},
		{"type", Val_type},
		{"name", Val_name},
		{"args", Val_args},
		{0, 0}
	};

	static int Val_tostring (lua_State *L)
	{
		Val *val = checkVal(L, 1);
		Storage *storage = checkStorage(L);
		std::stringstream ss;
		val->print(storage, ss);
		lua_pushstring(L, ss.str().c_str());
		return 1;
	}

	static const luaL_reg Val_meta[] =
	{
		{"__tostring", Val_tostring},
		{0, 0}
	};

	int Val_register (lua_State *L, Storage *storage)
	{
		luaL_openlib(L, VAL, Val_methods, 0);

		lua_pushliteral(L, "INF");
		lua_pushinteger(L, Val::INF);
		lua_rawset(L, -3);

		lua_pushliteral(L, "ID");
		lua_pushinteger(L, Val::ID);
		lua_rawset(L, -3);

		lua_pushliteral(L, "STRING");
		lua_pushinteger(L, Val::STRING);
		lua_rawset(L, -3);

		lua_pushliteral(L, "NUM");
		lua_pushinteger(L, Val::NUM);
		lua_rawset(L, -3);

		lua_pushliteral(L, "FUNC");
		lua_pushinteger(L, Val::FUNC);
		lua_rawset(L, -3);

		lua_pushliteral(L, "SUP");
		lua_pushinteger(L, Val::SUP);
		lua_rawset(L, -3);

		luaL_newmetatable(L, VAL);

		luaL_openlib(L, 0, Val_meta, 0);

		lua_pushliteral(L, "__index");
		lua_pushvalue(L, -3);
		lua_rawset(L, -3);

		lua_pushliteral(L, "__metatable");
		lua_pushvalue(L, -3);
		lua_rawset(L, -3);

		lua_pop(L, 2); // pop metatable and meta methods

		lua_pushliteral(L, "Val.storage");
		lua_pushlightuserdata(L, storage);
		lua_rawset(L, LUA_REGISTRYINDEX);

		return 0;
	}

	class LuaTop
	{
	public:
		LuaTop(lua_State *state, int size = 0)
			: state_(state)
			, size_(lua_gettop(state) + size)
		{ }
		~LuaTop() { lua_settop(state_, size_); }

	private:
		lua_State *state_;
		int        size_;
	};

}

class Grounder::LuaImpl
{
public:
	LuaImpl(Grounder *g);
	void call(Loc const &loc, char const *name, const ValVec &args, ValVec &vals);
	void exec(const Loc &loc, const std::string &lua);
	lua_State *state() { return luaState_; }
	void pushVal(const Val &val) { ::pushVal(luaState_, val); }
	~LuaImpl();
private:
	static int error(lua_State *L);

private:
    Grounder                *grounder_;
    lua_State               *luaState_;
};

Grounder::LuaImpl::LuaImpl(Grounder *g)
	: grounder_(g)
	, luaState_(lua_open())
{
	if(!luaState_) { throw std::runtime_error("lua: could not initialize lua"); }
	luaL_openlibs(luaState_);
#ifdef WITH_LUASQL
	luasqlL_openlibs(luaState_);
#endif
	lua_atpanic(luaState_, &error);
	Val_register(luaState_, grounder_);

}

void Grounder::LuaImpl::call(Loc const &loc, char const *name, const ValVec &args, ValVec &vals)
{
	LuaTop top(luaState_); (void)top;
	lua_getglobal(luaState_, name);
	if(!lua_isfunction(luaState_, lua_gettop(luaState_)))
	{
		std::ostringstream oss;
		oss << "lua function not found:\n";
		oss << "\t" << StrLoc(grounder_, loc) << ": " << name;
		throw std::runtime_error(oss.str().c_str());
	}
	foreach(const Val &val, args) { ::pushVal(luaState_, val); }
	lua_call(luaState_, args.size(), 1);
	if(lua_type(luaState_, -1) == LUA_TTABLE)
	{
		int tableIndex = lua_gettop(luaState_);
		lua_pushnil(luaState_);
		while (lua_next(luaState_, tableIndex) != 0)
		{
			vals.push_back(toVal(luaState_, -1));
			lua_pop(luaState_, 1); // pop val
		}
	}
	else { vals.push_back(toVal(luaState_, -1)); }
}

void Grounder::LuaImpl::exec(const Loc &loc, const std::string &lua)
{
	std::stringstream oss;
	oss << StrLoc(grounder_, loc);
	if(luaL_loadbuffer(luaState_, lua.c_str(), lua.size(), oss.str().c_str())) { error(luaState_); }
	lua_call(luaState_, 0, 0);
}

int Grounder::LuaImpl::error(lua_State *L)
{
	throw std::runtime_error(lua_tostring(L, -1));
}

Grounder::LuaImpl::~LuaImpl()
{
	lua_getglobal(luaState_, "cleanup");
	if(lua_isfunction(luaState_, -1) && lua_pcall(luaState_, 0, 0, 0))
	{
		std::cerr << "warning:" << lua_tostring(luaState_, -1) << std::endl;
	}
	lua_close(luaState_);
}
