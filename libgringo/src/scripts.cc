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

#include <gringo/scripts.hh>
#include <gringo/logger.hh>

namespace Gringo {

Scripts::Scripts(GringoModule &module) 
    : py(module)
    , lua(module) { }

bool Scripts::luaExec(Location const &loc, FWString code) {
    return lua.exec(loc, code);
}
bool Scripts::pyExec(Location const &loc, FWString code) {
    return py.exec(loc, code);
}
bool Scripts::callable(FWString name) {
    return py.callable(context, name) || lua.callable(context, name);
}
void Scripts::main(Control &ctl) {
    if (py.callable(Any(), "main")) { return py.main(ctl); }
    if (lua.callable(Any(), "main")) { return lua.main(ctl); }
    
}
ValVec Scripts::call(Location const &loc, FWString name, ValVec const &args) {
    if (py.callable(context, name)) { return py.call(context, loc, name, args); }
    if (lua.callable(context, name)) { return lua.call(context, loc, name, args); }
    GRINGO_REPORT(W_OPERATION_UNDEFINED)
        << loc << ": info: operation undefined:\n"
        << "  function '" << *name << "' not found\n"
        ;
    return {};
}
Scripts::~Scripts() = default;

} // namespace Gringo
