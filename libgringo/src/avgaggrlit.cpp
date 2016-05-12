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

#include <gringo/avgaggrlit.h>
#include <gringo/term.h>
#include <gringo/predlit.h>
#include <gringo/index.h>
#include <gringo/instantiator.h>
#include <gringo/grounder.h>
#include <gringo/exceptions.h>
#include <gringo/index.h>
#include <gringo/output.h>

AvgAggrLit::AvgAggrLit(const Loc &loc, CondLitVec &conds)
	: AggrLit(loc, conds, false, true)
{
}

bool AvgAggrLit::match(Grounder *grounder)
{
	try
	{
		if(lower_.get()) lowerBound_ = lower_->val(grounder).number();
		else lowerBound_ = std::numeric_limits<int32_t>::min();
		if(upper_.get()) upperBound_ = upper_->val(grounder).number();
		else upperBound_ = std::numeric_limits<int32_t>::max();
	}
	catch(const Val *val)
	{
		std::ostringstream oss;
		oss << "cannot convert ";
		val->print(grounder, oss);
		oss << " to integer";
		std::string str(oss.str());
		oss.str("");
		print(grounder, oss);
		throw TypeException(str, StrLoc(grounder, loc()), oss.str());
	}
	fact_     = false;
	factOnly_ = true;
	valLLower_ = valLUpper_ = valULower_ = valUUpper_ = 0;

	foreach(CondLit &lit, conds_) lit.ground(grounder);

	if(head() && !factOnly_) return true;
	// list contains only facts
	if(factOnly_ ) return (fact_ = (sign_ == (valLUpper_ < 0 || valULower_ > 0))) || head();
	// lowest and highest estimate between borders
	if((valLLower_ >= 0) && (valUUpper_ <= 0)) return (fact_ = !sign_) || head();
	// sum too low or too high
	if((valLUpper_ < 0) || (valULower_ > 0)) return (fact_ = sign_) || head();
	return true;
}

void AvgAggrLit::index(Grounder *g, Groundable *gr, VarSet &bound)
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

void AvgAggrLit::accept(::Printer *v)
{ 
	Printer *printer = v->output()->printer<Printer>();
	printer->begin(head(), sign_);
	if(lower_.get() || assign_) printer->lower(lowerBound_);
	if(upper_.get() || assign_) printer->upper(upperBound_);
	foreach(CondLit &lit, conds_) lit.accept(printer);
	printer->end();
}

Lit *AvgAggrLit::clone() const
{ 
	return new AvgAggrLit(*this);
}

void AvgAggrLit::print(Storage *sto, std::ostream &out) const
{ 
	if(sign_) out << "not ";
	bool comma = false;
	if(lower_.get())
	{
		lower_->print(sto, out);
		out << " ";
	}
	out << "#avg[";
	foreach(const CondLit &lit, conds_)
	{
		if(comma) out << ",";
		else comma = true;
		lit.print(sto, out);
	}
	out << "]";
	if(upper_.get())
	{
		out << " ";
		upper_->print(sto, out);
	}
}

tribool AvgAggrLit::accumulate(Grounder *g, const Val &weight, Lit &lit) throw(const Val*)
{
	(void)g;
	int32_t num = weight.number();
	if(!lit.fact()) factOnly_ = false;
	if(lower_.get())
	{
		if(lit.fact() || num < lowerBound_) valLLower_ += num - lowerBound_;
		if(lit.fact() || num > lowerBound_) valLUpper_ += num - lowerBound_;
	}
	if(upper_.get())
	{
		if(lit.fact() || num < upperBound_) valULower_ += num - upperBound_;
		if(lit.fact() || num > upperBound_) valUUpper_ += num - upperBound_;
	}
	return unknown;
}
