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

#include <gringo/varterm.h>
#include <gringo/varcollector.h>
#include <gringo/litdep.h>
#include <gringo/grounder.h>
#include <gringo/storage.h>

VarTerm::VarTerm(const Loc &loc)
	: Term(loc)
	, nameId_(std::numeric_limits<uint32_t>::max())
	, level_(0)
	, local_(true)
{
}

VarTerm::VarTerm(const Loc &loc, uint32_t nameId)
	: Term(loc)
	, nameId_(nameId)
	, level_(0)
	, local_(true)
{
}

bool VarTerm::anonymous() const
{
	return nameId_ == std::numeric_limits<uint32_t>::max();
}

Val VarTerm::val(Grounder *grounder) const
{
	return grounder->val(index_);
}

bool VarTerm::unify(Grounder *grounder, const Val &v, int binder) const
{
	if(grounder->binder(index_) != -1)
		return grounder->val(index_) == v;
	else
	{
		grounder->val(index_, v, binder);
		return true;
	}
}

void VarTerm::visit(PrgVisitor *v, bool bind)
{
	v->visit(this, bind);
}

void VarTerm::vars(VarSet &vars) const
{
	//if(local_) vars.insert(index_);
	vars.insert(index_);
}

void VarTerm::print(Storage *sto, std::ostream &out) const
{
	if(anonymous()) out << "_";
	else out << sto->string(nameId_);
}

double VarTerm::estimate(VarSet const &bound, double size) const {
    return bound.find(index_) != bound.end() ? 1 : size;
}

Term *VarTerm::clone() const
{
	return new VarTerm(*this);
}

