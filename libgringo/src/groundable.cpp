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

#include <gringo/groundable.h>
#include <gringo/instantiator.h>
#include <gringo/litdep.h>

Groundable::Groundable() 
	: enqueued_(false)
	, level_(0)
{
}

void Groundable::instantiator(Instantiator *inst) 
{ 
	inst_.reset(inst); 
}

Instantiator *Groundable::instantiator() const
{
	return inst_.get();
}

void Groundable::litDep(LitDep::GrdNode *litDep)
{ 
	litDep_.reset(litDep); 
}

Groundable::~Groundable() 
{ 
}

