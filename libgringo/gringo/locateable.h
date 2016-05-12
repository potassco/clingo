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

struct Loc
{
	Loc() 
		: file(0)
		, line(0)
		, column(0)
	{ }
	Loc(uint32_t f, uint32_t l, uint32_t c)
		: file(f)
		, line(l)
		, column(c)
	{ }
	uint32_t file;
	uint32_t line;
	uint32_t column;
};

class Locateable
{
public:
	Locateable(const Loc &loc)
		: loc_(loc)
	{ }
	void loc(const Loc &loc) { loc_ = loc; }
	const Loc &loc() const { return loc_; }
private:
	Loc loc_;
};

