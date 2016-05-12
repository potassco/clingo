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

class Context
{
private:
	typedef std::vector<int> IntVec;
public:
	Context();
	void reserve(uint32_t vars);
	int binder(uint32_t index) const;
	void unbind(uint32_t index);
	const Val &val(uint32_t index) const;
	void val(uint32_t index, const Val &v, int binder);

protected:
	~Context() { }

private:
	IntVec binder_;
	ValVec val_;
};

