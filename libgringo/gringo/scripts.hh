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

#ifndef _GRINGO_SCRIPTS_HH
#define _GRINGO_SCRIPTS_HH

#include <gringo/control.hh>

namespace Gringo {

template <class T>
class ScopeExit {
public:
    ScopeExit(T &&exit) : exit_(std::forward<T>(exit)) { }
    ~ScopeExit() { exit_(); }
private:
    T exit_;
};

template <typename T>
ScopeExit<T> onExit(T &&exit) {
    return ScopeExit<T>(std::forward<T>(exit));
}

class Scripts {
public:
    Scripts() = default;
    bool callable(String name);
    SymVec call(Location const &loc, String name, SymSpan args, Logger &log);
    void main(Control &ctl);
    void registerScript(clingo_ast_script_type type, UScript script);
    void setContext(Context &ctx) { context_ = &ctx; }
    void resetContext() { context_ = nullptr; }
    void exec(clingo_ast_script_type type, Location loc, String code);
    ~Scripts();

private:
    Context *context_ = nullptr;
    UScriptVec scripts_;
};

} // namespace Gringo

#endif // _GRINGO_SCRIPTS_HH

