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

class Func
{
	friend class FuncTerm;
public:
	Func(Storage *s, uint32_t name, const ValVec& args);
	size_t hash() const;
	bool operator==(const Func& a) const;
	void print(Storage *sto, std::ostream& out) const;
	int compare(const Func &b, Storage *s) const;
	uint32_t name() const { return name_; }
	const ValVec &args() const { return args_; }
	int32_t getDepth() const { return depth_; }

protected:
	uint32_t name_;
	int32_t  depth_;
	ValVec   args_;
};

inline size_t hash_value(const Func &f)
{
	return f.hash();
}
