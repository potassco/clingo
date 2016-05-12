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

#include <gringo/sumaggrlit.h>
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
	class SumIndex : public Index
	{
	public:
		SumIndex(SumAggrLit *sum_, const VarVec &bind);
		std::pair<bool,bool> firstMatch(Grounder *grounder, int binder);
		std::pair<bool,bool> nextMatch(Grounder *grounder, int binder);
		void reset() { finished_ = false; }
		void finish() { finished_ = true; }
		bool hasNew() const { return !finished_; }
	private:
		bool        finished_;
		SumAggrLit *sum_;
		VarVec      bind_;
		int32_t     current_;
		int32_t     upper_;
		int32_t     fixed_;
	};

	SumIndex::SumIndex(SumAggrLit *sum, const VarVec &bind)
		: finished_(false)
		, sum_(sum)
		, bind_(bind)
	{
	}

	std::pair<bool,bool> SumIndex::firstMatch(Grounder *grounder, int binder)
	{
		boost::tuple<int32_t, int32_t, int32_t> interval = sum_->matchAssign(grounder);
		current_ = interval.get<0>();
		upper_   = interval.get<1>();
		fixed_   = interval.get<2>();
		return nextMatch(grounder, binder);
	}

	std::pair<bool,bool> SumIndex::nextMatch(Grounder *grounder, int binder)
	{
		if(current_ <= upper_)
		{
			sum_->setValue(current_);
			foreach(uint32_t var, bind_) grounder->unbind(var);
			if(sum_->assign()->unify(grounder, Val::create(Val::NUM, current_++ + fixed_), binder))
			{
				return std::make_pair(true, !finished_);
			}
			// if it doesn't match at this point no futher integers will match as well
			// hence, it is safe to return false
			// (a match index would have been used if there was no variable on the left)
		}
		return std::make_pair(false, false);
	}
}

SumAggrLit::SumAggrLit(const Loc &loc, CondLitVec &conds, bool count)
	: AggrLit(loc, conds, count, true)
{
}

bool SumAggrLit::match(Grounder *grounder)
{
	try
	{
		if(lower_.get()) lowerBound_ = lower_->val(grounder).number();
		else lowerBound_ = std::numeric_limits<int32_t>::min();
		if(upper_.get()) upperBound_ = upper_->val(grounder).number();
		else upperBound_ = std::numeric_limits<int32_t>::max();
		if(assign_) upperBound_ = lowerBound_;
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
	valLower_ = valUpper_ = fixed_ = 0;
	checkUpperBound_ = (set() && upper_.get());
	if(set()) uniques_.clear();
	foreach(CondLit &lit, conds_) lit.ground(grounder);
	lowerBound_ = lower_.get() ? std::max(lowerBound_ - fixed_, valLower_) : valLower_;
	upperBound_ = upper_.get() || assign_ ? std::min(upperBound_ - fixed_, valUpper_) : valUpper_;

	if(head() && !factOnly_) return true;
	if(lowerBound_ > upperBound_) return (fact_ = sign_) || head();
	if(valLower_ >= lowerBound_ && valUpper_ <= upperBound_) return (fact_ = !sign_) || head();
	if(valUpper_ < lowerBound_ || valLower_  > upperBound_) return (fact_ = sign_) || head();
	return true;
}

boost::tuple<int32_t, int32_t, int32_t> SumAggrLit::matchAssign(Grounder *grounder)
{
	assert(!head());
	assert(!sign_);
	fact_     = false;
	factOnly_ = true;
	valLower_ = valUpper_ = fixed_ = 0;
	checkUpperBound_ = false;
	if(set()) uniques_.clear();
	foreach(CondLit &lit, conds_) lit.ground(grounder);
	fact_     = factOnly_;
	return boost::tuple<int32_t, int32_t, int32_t>(valLower_, valUpper_, fixed_);
}

void SumAggrLit::index(Grounder *g, Groundable *gr, VarSet &bound) 
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
			SumIndex *p = new SumIndex(this, bind);
			gr->instantiator()->append(p);
			bound.insert(bind.begin(), bind.end());
			return;
		}
	}
	gr->instantiator()->append(new MatchIndex(this));
}

void SumAggrLit::accept(::Printer *v)
{ 
	Printer *printer = v->output()->printer<Printer>();
	printer->begin(head(), sign_, set());
	if(lower_.get() || assign_) printer->lower(lowerBound_);
	if(upper_.get() || assign_) printer->upper(upperBound_);
	foreach(CondLit &lit, conds_) lit.accept(printer);
	printer->end();
}

Lit *SumAggrLit::clone() const
{ 
	return new SumAggrLit(*this);
}

void SumAggrLit::print(Storage *sto, std::ostream &out) const 
{ 
	if(sign_) out << "not ";
	bool comma = false;
	if(lower_.get())
	{
		lower_->print(sto, out);
		out << (assign_ ? ":=" : " ");
	}
	if(set()) out << "#count{";
	else out << "#sum[";
	foreach(const CondLit &lit, conds_)
	{
		if(comma) out << ",";
		else comma = true;
		lit.print(sto, out);
	}
	if(set()) out << "}";
	else out << "]";
	if(upper_.get())
	{
		out << " ";
		upper_->print(sto, out);
	}
}

tribool SumAggrLit::accumulate(Grounder *g, const Val &weight, Lit &lit) throw(const Val*)
{
	(void)g;
	int32_t num = weight.number();
	if(num == 0 && !head()) return true;
	if(set() && !lit.testUnique(uniques_))	return true;
	if(lit.fact())
	{
		fixed_ += num;
		if(checkUpperBound_ && fixed_ > upperBound_) return false;
	}
	else
	{
		factOnly_ = false;
		if(num < 0) valLower_ += num;
		if(num > 0)	valUpper_ += num;
	}
	return lit.fact() ? tribool(true) : unknown;
}
