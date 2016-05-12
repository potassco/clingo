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

#include <gringo/val.h>
#include <gringo/storage.h>
#include <gringo/func.h>
#include <gringo/exceptions.h>

void Val::print(Storage *sto, std::ostream &out) const
{
	switch(type)
	{
		case ID:     { out << sto->string(index); break; }
		case STRING: { out << '"' << sto->quote(sto->string(index)) << '"'; break; }
		case NUM:    { out << num; break; }
		case FUNC:   { sto->func(index).print(sto, out); break; }
		case INF:    { out << "#infimum"; break; }
		case SUP:    { out << "#supremum"; break; }
		case UNDEF:  { assert(false); break; }
		default:     { return sto->print(out, type, index); }
	}
}

int Val::compare(const Val &v, Storage *s) const
{
	if(type != v.type) return type < v.type ? -1 : 1;
	switch(type)
	{
		case ID:
		case STRING: { return s->string(index).compare(s->string(v.index)); }
		case NUM:    { return num - v.num; }
		case FUNC:   { return s->func(index).compare(s->func(v.index), s); }
		case INF:
		case SUP:    { return 0; }
		case UNDEF:  { assert(false); break; }
	}
	return s->compare(type, index, v.index);
}
