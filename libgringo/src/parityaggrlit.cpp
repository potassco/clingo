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

#include <gringo/parityaggrlit.h>
#include <gringo/term.h>
#include <gringo/predlit.h>
#include <gringo/index.h>
#include <gringo/instantiator.h>
#include <gringo/grounder.h>
#include <gringo/exceptions.h>
#include <gringo/index.h>
#include <gringo/output.h>

ParityAggrLit::ParityAggrLit(const Loc &loc, CondLitVec &conds, bool even, bool set)
	: AggrLit(loc, conds, set, true)
	, even_(even)
	, set_(set)
{
}

bool ParityAggrLit::match(Grounder *grounder)
{
	fact_     = false;
	factOnly_ = true;
	fixed_ = true;
	if(set_) uniques_.clear();

	foreach(CondLit &lit, conds_) lit.ground(grounder);

	if(head() && factOnly_) fact_ = (even_ == fixed_);
	else fact_ = factOnly_;
	if(factOnly_) return (even_ == fixed_) != sign_ || head();
	return true;
}

void ParityAggrLit::index(Grounder *g, Groundable *gr, VarSet &bound)
{
	(void)g;
	if(assign_)
	{
		VarSet vars;
		VarVec bind;
		lower_->vars(vars);
		std::set_difference(vars.begin(), vars.end(), bound.begin(), bound.end(), std::back_insert_iterator<VarVec>(bind));
		if(bind.size() > 0)
		{
			bound.insert(bind.begin(), bind.end());
			return;
		}
	}
	gr->instantiator()->append(new MatchIndex(this));
}

void ParityAggrLit::accept(::Printer *v)
{ 
	Printer *printer = v->output()->printer<Printer>();
	printer->begin(head(), sign_, even_==fixed_, set_);
	foreach(CondLit &lit, conds_) lit.accept(printer);
	printer->end();
}

Lit *ParityAggrLit::clone() const
{ 
	return new ParityAggrLit(*this);
}

void ParityAggrLit::print(Storage *sto, std::ostream &out) const
{
	if(sign_) out << "not ";
	if(even_) out << "#even";
	else out << "#odd";
	if(set_) out << "{";
	else out << "[";
	bool comma = false;
	foreach(const CondLit &lit, conds_)
	{
		if(comma) out << ",";
		else comma = true;
		lit.print(sto, out);
	}
	if(set_) out << "}";
	else out << "]";
}

tribool ParityAggrLit::accumulate(Grounder *g, const Val &weight, Lit &lit) throw(const Val*)
{
	(void)g;
	int32_t num = weight.number();
	if(num % 2 == 0 && !head()) return true;
	if(set_ && !lit.testUnique(uniques_)) return true;
	if(!lit.fact()) factOnly_ = false;
	else if (num % 2 != 0) fixed_ = !fixed_;
	return lit.fact() ? tribool(true) : unknown;
}
