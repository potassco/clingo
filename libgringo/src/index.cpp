// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#include <gringo/index.h>
#include <gringo/lit.h>

MatchIndex::MatchIndex(Lit *lit) : finished_(false), lit_(lit)
{
}

std::pair<bool,bool> MatchIndex::firstMatch(Grounder *grounder, int binder)
{
	(void)binder;
	bool r = lit_->match(grounder);
	if(r) return std::make_pair(true, !finished_);
	else return std::make_pair(false, false);
}

std::pair<bool,bool> MatchIndex::nextMatch(Grounder *grounder, int binder)
{
	(void)grounder;
	(void)binder;
	return std::make_pair(false, false);
}

