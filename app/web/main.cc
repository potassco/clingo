// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Benjamin Kaufmann

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

#ifdef WITH_PYTHON
#   include <Python.h>
#   include <python.hh>
#endif
#ifdef WITH_LUA
#   include <lua.hpp>
#endif
#include "clingo/clingo_app.hh"
#include <iterator>

class ExitException : public std::exception {
public:
    ExitException(int status) : status_(status) {
        std::ostringstream oss;
        oss << "exited with status: " << status_;
        msg_ = oss.str();
    }
    int status() const { return status_; }
    char const *what() const noexcept { return msg_.c_str(); }
    ~ExitException() = default;
private:
    std::string msg_;
    int status_;
};

clingo_application_t g_app = {
#ifdef WITH_PYTHON
    "with Python " PY_VERSION
#else
    "without Python"
#endif
    ,
#ifdef WITH_LUA
    "with " LUA_RELEASE
#else
    "without Lua"
#endif
    ,
    [](clingo_control_t *ctl) {
#ifdef WITH_PYTHON
        Gringo::registerPython(ctl, clingo_control_new);
#endif
        // TOOD: register lua too!
    }
};

struct WebApp : ClingoApp {
    WebApp() : ClingoApp(g_app) { }
    void exit(int status) const {
        throw ExitException(status);
    }
};

extern "C" int run(char const *program, char const *options) {
    try {
        std::streambuf* orig = std::cin.rdbuf();
        auto exit(Gringo::onExit([orig]{ std::cin.rdbuf(orig); }));
        std::istringstream input(program);
        std::cin.rdbuf(input.rdbuf());
        std::vector<std::vector<char>> opts;
        opts.emplace_back(std::initializer_list<char>{'c','l','i','n','g','o','\0'});
        std::istringstream iss(options);
        for (std::istream_iterator<std::string> it(iss), ie; it != ie; ++it) {
            opts.emplace_back(it->c_str(), it->c_str() + it->size() + 1);
        }
        std::vector<char*> args;
        for (auto &opt : opts) {
            args.emplace_back(opt.data());
        }
        WebApp app;
        args.emplace_back(nullptr);
        return app.main(args.size()-2, args.data());
    }
    catch (ExitException const &e) {
        return e.status();
    }
}

