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

#ifndef _GRINGO_OUTPUT_BACKENDS_HH
#define _GRINGO_OUTPUT_BACKENDS_HH

#include <gringo/backend.hh>
#include <potassco/convert.h>
#include <potassco/aspif.h>
#include <potassco/smodels.h>
#include <potassco/theory_data.h>

namespace Gringo { namespace Output {

class SmodelsFormatBackend : public Potassco::SmodelsConvert {
public:
    SmodelsFormatBackend(std::ostream& os)
    : Potassco::SmodelsConvert(out_, true)
    , out_(os, true, 0) { }
private:
    Potassco::SmodelsOutput out_;
};

using IntermediateFormatBackend = Potassco::AspifOutput;

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_BACKENDS_HH

