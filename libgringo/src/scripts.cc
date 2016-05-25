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

bool Scripts::luaExec(Location const &loc, String code) {
    return lua.exec(loc, code);
}
bool Scripts::pyExec(Location const &loc, String code) {
    return py.exec(loc, code);
}
bool Scripts::callable(String name) {
    return (context && context->callable(name)) || py.callable(name) || lua.callable(name);
}
void Scripts::main(Control &ctl) {
    if (py.callable("main")) { return py.main(ctl); }
    if (lua.callable("main")) { return lua.main(ctl); }

}
SymVec Scripts::call(Location const &loc, String name, SymSpan args, MessagePrinter &log) {
    if (context && context->callable(name)) { return context->call(loc, name, args); }
    if (py.callable(name)) { return py.call(loc, name, args, log); }
    if (lua.callable(name)) { return lua.call(loc, name, args, log); }
    GRINGO_REPORT(log, W_OPERATION_UNDEFINED)
        << loc << ": info: operation undefined:\n"
        << "  function '" << name << "' not found\n"
        ;
    return {};
}
Scripts::~Scripts() = default;

} // namespace Gringo
