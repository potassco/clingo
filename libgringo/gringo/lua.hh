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

#ifndef _GRINGO_LUA_HH
#define _GRINGO_LUA_HH

#include <ostream>
#include <gringo/locatable.hh>
#include <gringo/control.hh>

struct lua_State;

namespace Gringo {

struct Control;
struct LuaImpl;
struct Lua {
    Lua(GringoModule &module);
    bool exec(Location const &loc, FWString name);
    ValVec call(Any const &context, Location const &loc, FWString name, ValVec const &args);
    bool callable(Any const &context, FWString name);
    void main(Control &ctl);
    static void initlib(lua_State *L, GringoModule &module);
    ~Lua();

    std::unique_ptr<LuaImpl> impl;
};

} // namespace Gringo

#endif // _GRINGO_LUA_HH


