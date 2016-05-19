// Copyright (c) 2008, Roland Kaminski
//
// This file is part of GrinGo.
//
// GrinGo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GrinGo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GrinGo.  If not, see <http://www.gnu.org/licenses/>.

#ifndef DEPENDENCYRELATION_H
#define DEPENDENCYRELATION_H

namespace gringo
{
	class DependencyRelation
	{
	public:
		inline DependencyRelation(int size);
		inline void add(int a, int b);
		inline bool contains(int a, int b);
	protected:
		std::vector<bool> relation_;
	};

	DependencyRelation::DependencyRelation(int size) : relation_(size*(size-1)/2, false)
	{
	}

	void DependencyRelation::add(int a, int b)
	{
		if(a == b)
			return;
		if(a > b)
			assert(false);
		relation_[relation_.size() - (b*(b+1))/2 + a] = true;
		
	}

	bool DependencyRelation::contains(int a, int b)
	{
		if(a == b)
			return true;
		if(a > b)
			return false;
		return relation_[relation_.size() - (b*(b+1))/2 + a];
	}
}

#endif

