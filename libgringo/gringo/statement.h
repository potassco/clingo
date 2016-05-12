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
#include <gringo/groundable.h>
#include <gringo/locateable.h>

class Statement : public Groundable, public Locateable
{
public:
	Statement(const Loc &loc);
	virtual void normalize(Grounder *g) = 0;
	virtual void append(Lit *lit) = 0;
	virtual void check(Grounder *g);
	virtual void print(Storage *sto, std::ostream &out) const = 0;
	virtual bool edbFact() const { return false; }
	virtual bool choice() const { return false; }
	virtual ~Statement() { }
};

