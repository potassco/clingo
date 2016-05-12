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

#include <gringo/storage.h>
#include <gringo/domain.h>
#include "clingo/claspoutput.h"
#include "clingo/clingo_app.h"

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
#endif

#define ASSIGNMENT "Assignment"

namespace
{
	struct DomainIter
	{
		DomainIter(Grounder *g, Clasp::Solver *s, ClaspOutput *o)
			: grounder(g)
			, solver(s)
			, output(o)
			, active(false)
			, started(false)
		{ }

		void reset()
		{
			active  = true;
			started = false;
		}

		void begin(lua_State *L, const char *name, uint32_t arity)
		{
			if(!active) { luaL_error(L, "Assignment.begin() may only be called in onModel"); }
			dom     = grounder->domain(grounder->index(name), arity);
			const LparseConverter::SymbolMap &map = output->symbolMap(dom->domId());
			start   = map.begin();
			end     = map.end();
			started = false;
		}

		void pushNext(lua_State *L)
		{
			if(!active) { luaL_error(L, "Assignment.next() may only be called in onModel"); }
			if(start == end) { lua_pushboolean(L, false); }
			else
			{
				if(started) { start++; }
				else        { started = true; }
				lua_pushboolean(L, start != end);
			}
		}

		void pushName(lua_State *L)
		{
			if(!active)  { luaL_error(L, "Assignment.name() may only be called in onModel"); }
			if(!started) { luaL_error(L, "Assignment.name() called without prior call to Assignment.begin()"); }
			lua_pushstring(L, grounder->string(dom->nameId()).c_str());
		}

		void pushArity(lua_State *L)
		{
			if(!active)  { luaL_error(L, "Assignment.arity() may only be called in onModel"); }
			if(!started) { luaL_error(L, "Assignment.arity() called without prior call to Assignment.begin()"); }
			lua_pushinteger(L, dom->arity());
		}

		void checkIter(lua_State *L, const char *func)
		{
			if(!active)      { luaL_error(L, "Assignment.%s() may only be called in on{Assignment,BeginStep,EndStep}", func); }
			if(!started)     { luaL_error(L, "Assignment.%s() called without prior call to Assignment.begin()", func); }
			if(start == end) { luaL_error(L, "Assignment.%s() called after Assignment.next() returned false", func); }

		}

		void pushArgs(lua_State *L)
		{
			checkIter(L, "pushArgs");
			ValRng rng = output->vals(dom, start->first);
			lua_createtable(L, rng.size(), 0);
			int i = 1;
			foreach(const Val &val, rng)
			{
				lua_pushinteger(L, i++);
				grounder->luaPushVal(val);
				lua_rawset(L, -3);
			}
		}

		void pushIsTrue(lua_State *L)
		{
			checkIter(L, "pushIsTrue");
			Clasp::Atom *atom = solver->strategies().symTab->find(start->second);
			lua_pushboolean(L, atom && solver->isTrue(atom->lit));
		}

		void pushIsFalse(lua_State *L)
		{
			checkIter(L, "pushIsFalse");
			Clasp::Atom *atom = solver->strategies().symTab->find(start->second);
			lua_pushboolean(L, atom && solver->isFalse(atom->lit));
		}

		void pushIsUndef(lua_State *L)
		{
			checkIter(L, "pushIsUndef");
			Clasp::Atom *atom = solver->strategies().symTab->find(start->second);
			lua_pushboolean(L, atom && solver->value(atom->lit.var()) == Clasp::value_free);
		}

		void pushLevel(lua_State *L)
		{
			checkIter(L, "pushLevel");
			Clasp::Atom *atom = solver->strategies().symTab->find(start->second);
			lua_pushnumber(L, !atom || solver->value(atom->lit.var()) == Clasp::value_free ? -1 : solver->level(atom->lit.var()));
		}

		typedef LparseConverter::SymbolMap::const_iterator SymIt;

		Grounder      *grounder;
		Clasp::Solver *solver;
		ClaspOutput   *output;
		bool           active;
		Domain        *dom;
		bool           started;
		SymIt          start;
		SymIt          end;
	};

	static DomainIter* checkDomainIter(lua_State *L)
	{
		lua_pushliteral(L, "Assignment.domainIter");
		lua_rawget(L, LUA_REGISTRYINDEX);
		DomainIter *domIter = (DomainIter *)lua_touserdata(L, -1);
		lua_pop(L, 1); // pop storage
		return domIter;
	}

	static int Assignment_begin(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		luaL_checkstring(L, 1);
		luaL_checkinteger(L, 2);
		const char *name = lua_tostring(L, 1);
		int arity  = lua_tointeger(L, 2);
		domIter->begin(L, name, arity);
		return 0;
	}

	static int Assignment_next(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushNext(L);
		return 1;
	}

	static int Assignment_args(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushArgs(L);
		return 1;
	}

	static int Assignment_isTrue(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushIsTrue(L);
		return 1;
	}

	static int Assignment_isFalse(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushIsFalse(L);
		return 1;
	}

	static int Assignment_isUndef(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushIsUndef(L);
		return 1;
	}

	static int Assignment_name(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushName(L);
		return 1;
	}

	static int Assignment_arity(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushArity(L);
		return 1;
	}

	static int Assignment_level(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushLevel(L);
		return 1;
	}

	static const luaL_reg Assignment_methods[] =
	{
		{"begin",   Assignment_begin},
		{"next",    Assignment_next},
		{"args",    Assignment_args},
		{"isTrue",  Assignment_isTrue},
		{"isFalse", Assignment_isFalse},
		{"isUndef", Assignment_isUndef},
		{"args",    Assignment_args},
		{"name",    Assignment_name},
		{"arity",   Assignment_arity},
		{"level",   Assignment_level},
		{0, 0}
	};

	static const luaL_reg Assignment_meta[] =
	{
		{0, 0}
	};

	int Assignment_register (lua_State *L, DomainIter *iter)
	{
		luaL_openlib(L, ASSIGNMENT, Assignment_methods, 0);

		luaL_newmetatable(L, ASSIGNMENT);

		luaL_openlib(L, 0, Assignment_meta, 0);

		lua_pushliteral(L, "__index");
		lua_pushvalue(L, -3);
		lua_rawset(L, -3);

		lua_pushliteral(L, "__metatable");
		lua_pushvalue(L, -3);
		lua_rawset(L, -3);

		lua_pop(L, 2); // pop metatable and meta methods

		lua_pushliteral(L, "Assignment.domainIter");
		lua_pushlightuserdata(L, iter);
		lua_rawset(L, LUA_REGISTRYINDEX);

		return 0;
	}

}

template <Mode M>
class ClingoApp<M>::LuaImpl
{
public:
	LuaImpl(Grounder *g, Clasp::Solver *s, ClaspOutput *o)
		: domIter_(g, s, o)
	{
		lua_State *L = g->luaState();
		if(L)
		{
			onModel_     = onIndex(L, "onModel");
			onBeginStep_ = onIndex(L, "onBeginStep");
			onEndStep_   = onIndex(L, "onEndStep");
			Assignment_register(L, &domIter_);
		}
	}

	int onIndex(lua_State *L, const char *onName)
	{
		lua_getglobal(L, onName);
		int index = lua_gettop(L);
		if(!lua_isfunction(L, index)) { lua_pop(L, 1); return -1; }
		else                          { return index; }
	}

	void onCall(int index)
	{
		if(index != -1)
		{
			lua_State *L = domIter_.grounder->luaState();
			if(L)
			{
				domIter_.reset();
				lua_pushvalue(L, index);
				lua_call(L, 0, 0);
				domIter_.active = false;
			}
		}

	}

	void onModel()     { onCall(onModel_); }
	void onBeginStep() { onCall(onBeginStep_); }
	void onEndStep()   { onCall(onEndStep_); }

	bool locked()
	{
		return onModel_ != -1 || onBeginStep_ != -1 || onEndStep_ != -1;
	}
private:
	DomainIter domIter_;
	int        onModel_;
	int        onBeginStep_;
	int        onEndStep_;
};

#undef ASSIGNMENT
