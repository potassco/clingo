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

#include <clingo/scripts.hh>
#include <gringo/logger.hh>

namespace Gringo {

bool Scripts::callable(String name) {
    if (context_ && context_->callable(name)) { return true; }
    for (auto &&script : scripts_) {
        if (script.second->callable(name)) {
            return true;
        }
    }
    return false;
}

void Scripts::main(Control &ctl) {
    for (auto &&script : scripts_) {
        if (script.second->callable("main")) {
            script.second->main(ctl);
            return;
        }
    }
}

SymVec Scripts::call(Location const &loc, String name, SymSpan args, Logger &log) {
    if (context_ && context_->callable(name)) { return context_->call(loc, name, args, log); }
    for (auto &&script : scripts_) {
        if (script.second->callable(name)) {
            return script.second->call(loc, name, args, log);
        }
    }
    GRINGO_REPORT(log, Warnings::OperationUndefined)
        << loc << ": info: operation undefined:\n"
        << "  function '" << name << "' not found\n"
        ;
    return {};
}

void Scripts::registerScript(clingo_ast_script_type type, UScript script) {
    if (script) { scripts_.emplace_back(type, std::move(script)); }
}

void Scripts::exec(ScriptType type, Location loc, String code) {
    bool notfound = true;
    for (auto &&script : scripts_) {
        if (script.first == static_cast<clingo_ast_script_type_t>(type)) {
            script.second->exec(type, loc, code);
            notfound = false;
        }
    }
    if (notfound) {
        std::ostringstream oss;
        oss << loc << ": error: ";
        switch (type) {
            case ScriptType::Python: { oss << "python"; break; }
            case ScriptType::Lua:    { oss << "lua"; break; }
        }
        oss << " support not available\n";
        throw GringoError(oss.str().c_str());
    }
}

char const *Scripts::version(clingo_ast_script_type type) {
    for (auto &&script : scripts_) {
        if (script.first == type) {
            return script.second->version();
        }
    }
    return nullptr;
}

Scripts::~Scripts() = default;

} // namespace Gringo
