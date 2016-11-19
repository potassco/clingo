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

#ifndef CLINGO_SCRIPTS_HH
#define CLINGO_SCRIPTS_HH

#include <clingo/control.hh>
#include <gringo/base.hh>

namespace Gringo {

class Script : public Context {
public:
    virtual void main(Control &ctl) = 0;
    virtual char const *version() = 0;
    virtual ~Script() = default;
};
using UScript = std::shared_ptr<Script>;
using UScriptVec = std::vector<std::pair<clingo_ast_script_type, UScript>>;

class Scripts : public Context {
public:
    Scripts() = default;
    bool callable(String name) override;
    SymVec call(Location const &loc, String name, SymSpan args, Logger &log) override;
    void main(Control &ctl);
    void registerScript(clingo_ast_script_type type, UScript script);
    void setContext(Context &ctx) { context_ = &ctx; }
    void resetContext() { context_ = nullptr; }
    void exec(ScriptType type, Location loc, String code) override;
    char const *version(clingo_ast_script_type type);

    ~Scripts();

private:
    Context *context_ = nullptr;
    UScriptVec scripts_;
};

} // namespace Gringo

#endif // CLINGO_SCRIPTS_HH

