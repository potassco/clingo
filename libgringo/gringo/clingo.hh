// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) Roland Kaminski

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

#ifndef GRINGO_CLINGO_HH
#define GRINGO_CLINGO_HH

#include <stdexcept>
#include <clingo.h>

namespace Gringo {

struct ClingoError : std::exception {
    ClingoError(clingo_error_t err) : err(err) { }
    virtual ~ClingoError() noexcept = default;
    virtual const char* what() const noexcept {
        return clingo_error_str(err);
    }
    clingo_error_t const err;
};

void clingo_expect(bool expr) {
    if (!expr) { throw std::runtime_error("unexpected"); }
}

}

#define GRINGO_CLINGO_TRY try {
#define GRINGO_CLINGO_CATCH } \
catch(Gringo::ClingoError &e) { return e.err; } \
catch(std::bad_alloc &e)      { return clingo_error_bad_alloc; } \
catch(std::runtime_error &e)  { return clingo_error_runtime; } \
catch(std::logic_error &e)    { return clingo_error_logic; } \
catch(...)                    { return clingo_error_unknown; } \
return clingo_error_success

#endif // GRINGO_CLINGO_HH
