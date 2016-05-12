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

#include <stdint.h>
#include <cassert>
#include <boost/functional/hash/hash.hpp>

class Storage;

struct Val
{
	enum Type { INF=0, NUM, ID, FUNC, STRING, UNDEF=200, SUP=20000 };
	static inline Val create();
	static inline Val create(uint32_t t, int num);
	static inline Val create(uint32_t t, uint32_t index);
	static inline Val inf();
	static inline Val sup();
	inline size_t hash() const;
	inline bool operator==(const Val &v) const;
	bool operator!=(const Val &v) const { return !operator ==(v); }
	int compare(const Val &v, Storage *s) const;
	void print(Storage *sto, std::ostream &out) const;
	int number() const;
	uint32_t type;
	union
	{
		int num;
		uint32_t index;
	};
};

Val Val::create()
{
	Val v;
	v.type = UNDEF;
	v.num  = 0;
	return v;
}

Val Val::create(uint32_t t, int n)
{
	Val v;
	v.type = t;
	v.num  = n;
	return v;
}

Val Val::create(uint32_t t, uint32_t i)
{
	Val v;
	v.type  = t;
	v.index = i;
	return v;
}

Val Val::inf()
{
	Val v;
	v.type = INF;
	v.num  = 0;
	return v;
}

Val Val::sup()
{
	Val v;
	v.type = SUP;
	v.num  = 0;
	return v;
}

size_t Val::hash() const
{
	size_t hash = static_cast<size_t>(type);
	switch(type)
	{
		case INF:
		case SUP: { return hash; }
		case NUM: { boost::hash_combine(hash, num); return hash; }
		default:  { boost::hash_combine(hash, index); return hash; }
	}
}

bool Val::operator==(const Val &v) const
{
	if(type != v.type) { return false; }
	switch(type)
	{
		case INF:
		case SUP: { return true; }
		case NUM: { return num == v.num; }
		default:  { return index == v.index; }
	}
}

inline int Val::number() const
{
	if(type == NUM) { return num; }
	else            { throw(this); }
}

inline size_t hash_value(const Val &v) { return v.hash(); }

