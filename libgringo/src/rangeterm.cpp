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

#include <gringo/rangeterm.h>
#include <gringo/varterm.h>
#include <gringo/rellit.h>
#include <gringo/rangelit.h>
#include <gringo/grounder.h>

RangeTerm::RangeTerm(const Loc &loc, Term *a, Term *b)
	: Term(loc)
	, a_(a)
	, b_(b)
{
}

void RangeTerm::print(Storage *sto, std::ostream &out) const
{
	a_->print(sto, out);
	out << "..";
	b_->print(sto, out);
}

void RangeTerm::normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify)
{
	(void)unify;
	a_->normalize(parent, PtrRef(a_), g, expander, false);
	b_->normalize(parent, PtrRef(b_), g, expander, false);
	uint32_t var = g->createVar();
	expander->expand(new RangeLit(loc(), new VarTerm(a_->loc(), var), a_.release(), b_.release()), Expander::RANGE);
	ref.reset(new VarTerm(loc(), var));
}

Term *RangeTerm::clone() const
{
	return new RangeTerm(*this);
}

double RangeTerm::estimate(VarSet const &, double) const {
    return 1;
}

RangeTerm::~RangeTerm()
{
}

