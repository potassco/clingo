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

#include <gringo/instantiator.h>
#include <gringo/groundable.h>
#include <gringo/index.h>
#include <gringo/grounder.h>

Instantiator::Instantiator(Groundable *g) :
	groundable_(g)
{
}

void Instantiator::append(Index *i)
{
	assert(i);
	indices_.push_back(i);
	new_.push_back(0);
}

void Instantiator::ground(Grounder *g)
{
	typedef std::vector<bool> BoolVec;

	assert(indices_.size() > 0);
	int numNew = 0;
	for(IndexPtrVec::size_type i = 0; i < indices_.size(); ++i)
	{
		new_[i] = 2 * indices_[i].hasNew();
		if(new_[i]) ++numNew;
	}
	BoolVec ord(new_.size(), true);
	int l = 0;
	std::pair<bool, bool> matched(true, false);
	while(l >= 0)
	{
		if(matched.first)
			matched = indices_[l].firstMatch(g, l);
		else
			matched = indices_[l].nextMatch(g, l);
		if(matched.first && !matched.second && new_[l] == 2)
		{
			new_[l] = 1;
			--numNew;
		}
		assert(ord[l] || !matched.second);
		if(!matched.second)
			ord[l] = false;
		if(matched.first && numNew > 0)
		{
			if(l + 1 == static_cast<int>(indices_.size()))
			{
				if(!groundable_->grounded(g)) break;
				matched.first = false;
			}
			else
				++l;
		}
		else
		{
			ord[l] = true;
			matched.first = false;
			if(new_[l] == 1)
			{
				++numNew;
				new_[l] = 2;
			}
			--l;
		}
	}
	foreach(uint32_t var, groundable_->vars())
		g->unbind(var);
	foreach(Index &idx, indices_)
		idx.finish();
}

void Instantiator::reset()
{
	foreach(Index &index, indices_) index.reset();
	
}

Instantiator::~Instantiator()
{
}

