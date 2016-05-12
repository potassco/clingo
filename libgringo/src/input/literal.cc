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

#include "gringo/input/literal.hh"

namespace Gringo { namespace Input {

// {{{ definition of Projection

Projection::Projection(UTerm &&projected, UTerm &&project)
    : projected(std::move(projected))
    , project(std::move(project)) { }

Projections::Projections(Projections &&) = default;

Projection::~Projection() = default;

Projection::operator Term const &() const { return *projected; }

// }}}
// {{{ definition of Projections

Projections::Projections() = default;

Projections::~Projections() = default;

Projections::ProjectionMap::iterator Projections::begin() {
    return proj.begin();
}
Projections::ProjectionMap::iterator Projections::end() {
    return proj.end();
}

UTerm Projections::add(Term &term) {
    AuxGen gen;
    auto ret(term.project(true, gen));
    proj.emplace_back(std::move(std::get<1>(ret)), std::move(std::get<2>(ret)));
    return std::move(std::get<0>(ret));
}

// }}}

} } // namespace Input Gringo
