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

#include <gringo/output.h>

Output::Output(std::ostream *out) :
	out_(out),
	s_(0),
	show_(true)
{
}

void Output::show(bool s)
{
	show_ = s;
	doShow(s);
}

void Output::show(uint32_t nameId, uint32_t arity, bool s)
{
	showMap_[std::make_pair(nameId, arity)] = s;
	doShow(nameId, arity, s);
}

bool Output::show(uint32_t nameId, uint32_t arity)
{
	ShowMap::iterator i = showMap_.find(Signature(nameId, arity));
	if(i != showMap_.end()) return i->second;
	else return show_;
}

void Output::external(uint32_t nameId, uint32_t arity)
{
	external_.insert(Signature(nameId, arity));
}
