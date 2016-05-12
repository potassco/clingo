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

#include <gringo/minmaxaggrlit.h>
#include <gringo/term.h>
#include <gringo/predlit.h>
#include <gringo/index.h>
#include <gringo/instantiator.h>
#include <gringo/grounder.h>
#include <gringo/exceptions.h>
#include <gringo/index.h>
#include <gringo/output.h>

namespace
{
	class MinMaxIndex : public Index
	{
	public:
		MinMaxIndex(MinMaxAggrLit *lit_, const VarVec &bind);
		std::pair<bool,bool> firstMatch(Grounder *grounder, int binder);
		std::pair<bool,bool> nextMatch(Grounder *grounder, int binder);
		void reset() { finished_ = false; }
		void finish() { finished_ = true; }
		bool hasNew() const { return !finished_; }
	private:
		bool           finished_;
		MinMaxAggrLit *lit_;
		VarVec         bind_;
		//! whether the supremum (true) or infimum (false) should be included
		tribool        border_;
		boost::unordered_set<Val>           vals_;
		boost::unordered_set<Val>::iterator current_;
	};

	MinMaxIndex::MinMaxIndex(MinMaxAggrLit *lit, const VarVec &bind)
		: finished_(false)
		, lit_(lit)
		, bind_(bind)
	{
	}

	std::pair<bool,bool> MinMaxIndex::firstMatch(Grounder *grounder, int binder)
	{
		vals_.clear();
		lit_->matchAssign(grounder, vals_);
		current_ = vals_.begin();
		return nextMatch(grounder, binder);
	}

	std::pair<bool,bool> MinMaxIndex::nextMatch(Grounder *grounder, int binder)
	{
		while(current_ != vals_.end())
		{
			lit_->setValue(*current_);
			foreach(uint32_t var, bind_) grounder->unbind(var);
			if(lit_->assign()->unify(grounder, *current_++, binder))
			{
				return std::make_pair(true, !finished_);
			}
		}
		return std::make_pair(false, false);
	}
}

MinMaxAggrLit::MinMaxAggrLit(const Loc &loc, CondLitVec &conds, bool max)
	: AggrLit(loc, conds, false, true)
	, max_(max)
{
}

int MinMaxAggrLit::cmp(const Val &a, const Val &b, Storage *s)
{
	return max_ ? a.compare(b, s) : -a.compare(b, s);
}

Val MinMaxAggrLit::min()
{
	return max_ ? Val::inf() : Val::sup();
}

Val MinMaxAggrLit::max()
{
	return max_ ? Val::sup() : Val::inf();
}

bool MinMaxAggrLit::match(Grounder *grounder)
{
	lowerBound_ = lower_.get() ? lower_->val(grounder) : Val::inf();
	upperBound_ = upper_.get() ? upper_->val(grounder) : Val::sup();
	if(!max_) std::swap(lowerBound_, upperBound_);
	if(assign_) upperBound_ = lowerBound_;

	fact_     = false;
	factOnly_ = true;
	valLower_ = max();
	valUpper_ = fixed_ = min();
	vals_ = 0;
	
	foreach(CondLit &lit, conds_) lit.ground(grounder);

	if(head() && !factOnly_) return true;
	// all too low (or empty) or facts too high
	if(cmp(valUpper_, lowerBound_, grounder) < 0 || cmp(fixed_, upperBound_, grounder) > 0)  return (fact_ = sign_) || head();
	// facts above lower bound and all under upper bound
	if(cmp(fixed_, lowerBound_, grounder) >= 0 && cmp(valUpper_, upperBound_, grounder) <= 0)  return (fact_ = !sign_) || head();
	// else
	return true;
}

void MinMaxAggrLit::matchAssign(Grounder *grounder, boost::unordered_set<Val> &vals)
{
	assert(!head());
	assert(!sign_);
	fact_     = false;
	factOnly_ = true;
	lowerBound_ = valUpper_ = fixed_ = min();
	upperBound_ = valLower_ = max();
	vals_ = &vals;

	foreach(CondLit &lit, conds_) lit.ground(grounder);

	fact_ = factOnly_;
	if(fixed_ == min()) vals.insert(min());
	else
	{
		boost::unordered_set<Val>::iterator i = vals.begin();
		while(i != vals.end())
		{
			if(cmp(*i, fixed_, grounder) < 0) i = vals.erase(i);
			else ++ i;
		}
	}
}

void MinMaxAggrLit::index(Grounder *g, Groundable *gr, VarSet &bound)
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
			MinMaxIndex *p = new MinMaxIndex(this, bind);
			gr->instantiator()->append(p);
			bound.insert(bind.begin(), bind.end());
			return;
		}
	}
	gr->instantiator()->append(new MatchIndex(this));
}

void MinMaxAggrLit::accept(::Printer *v)
{ 
	Printer *printer = v->output()->printer<Printer>();
	printer->begin(head(), sign_, max_);
	if(lower_.get() || assign_)
	{
		if(max_ && fixed_.compare(lowerBound_, v->output()->storage()) < 0) printer->lower(lowerBound_);
		if(!max_ && valUpper_.compare(upperBound_, v->output()->storage()) < 0) printer->lower(upperBound_);
	}
	if(upper_.get() || assign_)
	{
		if(max_ && valUpper_.compare(upperBound_, v->output()->storage()) > 0) printer->upper(upperBound_);
		if(!max_ && fixed_.compare(lowerBound_, v->output()->storage()) > 0) printer->upper(lowerBound_);
	}
	foreach(CondLit &lit, conds_) lit.accept(printer);
	printer->end();
}

Lit *MinMaxAggrLit::clone() const
{ 
	return new MinMaxAggrLit(*this);
}

void MinMaxAggrLit::print(Storage *sto, std::ostream &out) const
{ 
	if(sign_) out << "not ";
	bool comma = false;
	if(lower_.get())
	{
		lower_->print(sto, out);
		out << (assign_ ? ":=" : " ");
	}
	if(max_) out << "#max[";
	else out << "#min[";
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

tribool MinMaxAggrLit::accumulate(Grounder *g, const Val &weight, Lit &lit) throw(const Val*)
{
	(void)g;
	if(cmp(valLower_, weight, g) > 0) valLower_ = weight;
	if(cmp(valUpper_, weight, g) < 0) valUpper_ = weight;
	if(lit.fact())
	{
		if(cmp(fixed_, weight, g) < 0) fixed_ = weight;
		if(cmp(fixed_, upperBound_, g) > 0) return false;
	}
	else factOnly_ = false;
	if(vals_)
	{
		vals_->insert(Val(weight));
	}
	return unknown;
}
