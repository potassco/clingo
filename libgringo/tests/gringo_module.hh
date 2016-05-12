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

#ifndef _GRINGO_TEST_GRINGO_MODULE_HELPER_HH
#define _GRINGO_TEST_GRINGO_MODULE_HELPER_HH

#include "gringo/control.hh"
#include "gringo/input/groundtermparser.hh"

namespace Gringo { namespace Test {

struct TestGringoModule : Gringo::GringoModule {
    virtual Gringo::Control *newControl(int, char const **) { throw std::logic_error("TestGringoModule::newControl must not be called"); }
    virtual void freeControl(Gringo::Control *)  { throw std::logic_error("TestGringoModule::freeControl must not be called"); }
    virtual Gringo::Value parseValue(std::string const &str) {
        return parser.parse(str);
    }
    Gringo::Input::GroundTermParser parser;
};

inline GringoModule &getTestModule() {
    static TestGringoModule module;
    return module;
}

} }

#endif
