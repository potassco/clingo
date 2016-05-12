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

#include <gringo/constterm.h>
#include <gringo/grounder.h>

ConstTerm::ConstTerm(const Loc &loc, const Val &v) :
	Term(loc), val_(v)
{
}

Val ConstTerm::val(Grounder *grounder) const
{
	(void)grounder;
	return val_;
}

bool ConstTerm::unify(Grounder *grounder, const Val &v, int binder) const
{
	(void)grounder;
	(void)binder;
	return val_ == v;
}

void ConstTerm::vars(VarSet &vars) const
{
	(void)vars;
}

void ConstTerm::visit(PrgVisitor *v, bool bind)
{
	(void)v;
	(void)bind;
}

void ConstTerm::print(Storage *sto, std::ostream &out) const
{
	val_.print(sto, out);
}

Term *ConstTerm::clone() const
{
	return new ConstTerm(*this);
}

double ConstTerm::estimate(VarSet const &, double) const {
    return 1;
}
