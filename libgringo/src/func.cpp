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

#include <gringo/func.h>
#include <gringo/storage.h>

namespace
{
	int32_t termDepth(Storage *s, const ValVec& args)
	{
		int32_t maxDepth = 1;
		foreach(const Val &val, args)
		{
			if (val.type == Val::FUNC)
			{
				int32_t depth = s->func(val.index).getDepth();
				if (depth >= maxDepth) { maxDepth = depth + 1; }
			}
		}
		return maxDepth;
	}
}

Func::Func(Storage *s, uint32_t name, const ValVec& args)
	: name_(name)
	, depth_(termDepth(s, args))
	, args_(args)
{
}

size_t Func::hash() const
{
	size_t seed = name_;
	boost::hash_range(seed, args_.begin(), args_.end());
	return seed;
}

bool Func::operator==(const Func& a) const
{
	if(name_ != a.name_) return false;
	if(args_.size() != a.args_.size()) return false;
	for(ValVec::size_type i = 0; i < args_.size(); i++)
		if(!(args_[i] == a.args_[i])) return false;
	return true;
}

int Func::compare(const Func &b, Storage *s) const
{
    if(args_.size() != b.args_.size())
        return int(b.args_.size()) - args_.size();
    if(name_ != b.name_)
        return s->string(name_).compare(s->string(b.name_));
    for(ValVec::size_type i = 0; i < args_.size(); i++)
    {
       int cmp = args_[i].compare(b.args_[i], s);
       if(cmp != 0)
           return cmp;
    }
    return 0;

}

void Func::print(Storage *sto, std::ostream& out) const
{
	out << sto->string(name_);
	out << "(";
	bool comma = false;
	foreach(const Val &val, args_)
	{
		if(comma) out << ",";
		else comma = true;
		val.print(sto, out);
	}
	out << ")";
}
