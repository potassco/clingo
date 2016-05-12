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

#include <gringo/poolterm.h>
#include <gringo/prgvisitor.h>
#include <gringo/lit.h>

PoolTerm::PoolTerm(const Loc &loc, Term *a, Term *b)
	: Term(loc)
	, a_(a)
	, b_(b)
	, clone_(true)
{
}

PoolTerm::PoolTerm(const Loc &loc, Term *a)
	: Term(loc)
	, a_(a)
	, b_(0)
	, clone_(true)
{
}

void PoolTerm::print(Storage *sto, std::ostream &out) const
{
	if(b_.get())
	{
		a_->print(sto, out);
		out << ";";
		b_->print(sto, out);
	}
	else a_->print(sto, out);
}

void PoolTerm::normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify)
{
	if(b_.get())
	{
		clone_ = false;
		expander->expand(parent->clone(), Expander::POOL);
		clone_ = true;
		Term *b = b_.get();
		ref.reset(b_.release());
		b->normalize(parent, ref, g, expander, unify);
	}
	else
	{
		Term *a = a_.get();
		ref.reset(a_.release());
		a->normalize(parent, ref, g, expander, unify);
	}
}

Term *PoolTerm::clone() const
{
	if(clone_) return new PoolTerm(*this);
	else return new PoolTerm(loc(), a_.release());
}

double PoolTerm::estimate(VarSet const &, double) const {
    return 1;
}

PoolTerm::~PoolTerm()
{
}

