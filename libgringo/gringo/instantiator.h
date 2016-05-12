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

#pragma once

#include <gringo/gringo.h>

class Instantiator
{
private:
	typedef std::vector<int> IntVec;
public:
	Instantiator(Groundable *g);
	void append(Index *i);
	void ground(Grounder *g);
	void reset();
	~Instantiator();
private:
	Groundable *groundable_;
	IndexPtrVec indices_;
	IntVec new_;
};
   
namespace boost
{
	template <>
	inline Instantiator* new_clone(const Instantiator& a)
	{
		(void)a;
		return 0;
	}
}
