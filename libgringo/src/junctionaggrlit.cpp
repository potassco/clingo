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

#include <gringo/junctionaggrlit.h>
#include <gringo/term.h>
#include <gringo/predlit.h>
#include <gringo/index.h>
#include <gringo/instantiator.h>
#include <gringo/grounder.h>
#include <gringo/exceptions.h>
#include <gringo/index.h>
#include <gringo/output.h>
#include <gringo/litdep.h>

JunctionAggrLit::JunctionAggrLit(const Loc &loc, CondLitVec &conds)
	: AggrLit(loc, conds, true, false)
{
}

bool JunctionAggrLit::match(Grounder *grounder)
{
	hasFalse_ = false;
	hasFact_  = false;
	factOnly_ = true;
	uniques_.clear();
	foreach(CondLit &lit, conds_) lit.ground(static_cast<Grounder*>(grounder));
	fact_ = head() ? hasFact_ : factOnly_ && !hasFalse_;
	return !hasFalse_ || head();
}

void JunctionAggrLit::index(Grounder *grounder, Groundable *gr, VarSet &bound)
{
	(void)grounder;
	(void)bound;
	gr->instantiator()->append(new MatchIndex(this));
}

void JunctionAggrLit::accept(::Printer *v)
{
	Printer *printer = v->output()->printer<Printer>();
	printer->begin(head());
	foreach(CondLit &lit, conds_) lit.accept(printer);
	printer->end();
}

Lit *JunctionAggrLit::clone() const
{ 
	return new JunctionAggrLit(*this);
}

void JunctionAggrLit::print(Storage *sto, std::ostream &out) const
{
	bool comma = false;
	foreach(const CondLit &lit, conds_)
	{
		if(comma) { out << (head() ? "|" : ","); }
		else      { comma = true; }
		lit.print(sto, out);
	}
}

tribool JunctionAggrLit::accumulate(Grounder *g, const Val &weight, Lit &lit) throw(const Val*)
{
	(void)weight;
	if(!lit.testUnique(uniques_)) { return true; }
	if(lit.isFalse(g))
	{
		// no head literal can be false
		// because only atoms are allowed
		assert(!head());
		hasFalse_ = true;
		return false;
	}
	if(!lit.fact()) { factOnly_ = false; }
	else            { hasFact_  = true; }
	return lit.fact() && !head() ? tribool(true) : unknown;
}

namespace
{
	class CondHeadExpander : public Expander
	{
	public:
		CondHeadExpander(JunctionAggrLit &aggr, CondLit *cond, Expander &ruleExpander);
		void expand(Lit *lit, Type type);
	private:
		JunctionAggrLit &aggr_;
		CondLit         *cond_;
		Expander        &ruleExp_;
	};

	class CondBodyExpander : public Expander
	{
	public:
		CondBodyExpander(CondLit &cond);
		void expand(Lit *lit, Type type);
	private:
		CondLit &cond_;
	};

	CondHeadExpander::CondHeadExpander(JunctionAggrLit &aggr, CondLit *cond, Expander &ruleExpander)
		: aggr_(aggr)
		, cond_(cond)
		, ruleExp_(ruleExpander)
	{
	}

	void CondHeadExpander::expand(Lit *lit, Expander::Type type)
	{
		switch(type)
		{
			case RANGE:
				ruleExp_.expand(lit, type);
				break;
			case POOL:
			{
				LitPtrVec body;
				foreach(const Lit &l, cond_->body()) body.push_back(l.clone());
				std::auto_ptr<CondLit> cond(new CondLit(cond_->loc(), lit, cond_->weight()->clone(), body));
				if(aggr_.head())
				{
					CondLitVec lits;
					foreach(const CondLit &l, aggr_.conds())
					{
						if(&l == cond_) lits.push_back(cond);
						else lits.push_back(new CondLit(l));
					}
					JunctionAggrLit *lit(new JunctionAggrLit(aggr_.loc(), lits));
					ruleExp_.expand(lit, type);
				}
				else
				{
					aggr_.add(cond.release());
				}
				break;
			}
			case RELATION:
				cond_->body().push_back(lit);
				break;
		}
	}

	CondBodyExpander::CondBodyExpander(CondLit &cond)
		: cond_(cond)
	{
	}

	void CondBodyExpander::expand(Lit *lit, Expander::Type type)
	{
		(void)type;
		cond_.body().push_back(lit);
	}
}

void JunctionAggrLit::normalize(Grounder *g, Expander *expander)
{
	assert(!(assign_ && sign_));
	assert(!(assign_ && head()));
	if(lower_.get()) lower_->normalize(this, Term::PtrRef(lower_), g, expander, assign_);
	if(upper_.get()) upper_->normalize(this, Term::PtrRef(upper_), g, expander, false);
	for(CondLitVec::size_type i = 0; i < conds_.size(); i++)
	{
		CondHeadExpander headExp(*this, &conds_[i], *expander);
		CondBodyExpander bodyExp(conds_[i]);
		conds_[i].normalize(g, &headExp, &bodyExp);
	}
}

