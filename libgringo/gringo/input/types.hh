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

#ifndef _GRINGO_INPUT_TYPES_HH
#define _GRINGO_INPUT_TYPES_HH

#include <gringo/ground/types.hh>

namespace Gringo { namespace Input {

using Gringo::Output::PredicateDomain;
using Gringo::Output::PredDomMap;
using Gringo::Output::DomainData;

struct ToGroundArg;
struct Projections;

struct Literal;
using ULit    = std::unique_ptr<Literal>;
using ULitVec = std::vector<ULit>;

struct Statement;
using UStm = std::unique_ptr<Statement>;
using UStmVec = std::vector<UStm>;

struct BodyAggregate;
using UBodyAggr    = std::unique_ptr<BodyAggregate>;

struct HeadAggregate;
using UHeadAggr    = std::unique_ptr<HeadAggregate>;
using UBodyAggrVec = std::vector<UBodyAggr>;

} } // namespace Input Gringo

#endif // _GRINGO_INPUT_TYPES_HH
