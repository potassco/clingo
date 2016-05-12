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

#include <gringo/context.h>

Context::Context()
{
}

void Context::reserve(uint32_t vars)
{
	if(binder_.size() < vars)
	{
		binder_.resize(vars, -1);
		val_.resize(vars);
	}
}

const Val &Context::val(uint32_t index) const
{
	assert(binder_[index] != -1);
	return val_[index];
}

void Context::val(uint32_t index, const Val &v, int binder)
{
	binder_[index] = binder;
	val_[index] = v;
}

int Context::binder(uint32_t index) const
{
	assert(index < binder_.size());
	return binder_[index];
}

void Context::unbind(uint32_t index)
{
	binder_[index] = -1;
}

