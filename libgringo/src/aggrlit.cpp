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

#include <gringo/aggrlit.h>
#include <gringo/term.h>
#include <gringo/grounder.h>
#include <gringo/prgvisitor.h>
#include <gringo/instantiator.h>
#include <gringo/litdep.h>
#include <gringo/exceptions.h>

AggrLit::AggrLit(const Loc &loc, CondLitVec &conds, bool set, bool optComplete)
	: Lit(loc)
	, sign_(false)
	, assign_(false)
	, fact_(false)
	, conds_(conds.release())
	, set_(set)
	, optComplete_(optComplete)
{
	foreach(CondLit &lit, conds_) lit.aggr_ = this;
}

AggrLit::AggrLit(const AggrLit &aggr)
	: Lit(aggr)
	, sign_(aggr.sign_)
	, assign_(aggr.assign_)
	, fact_(aggr.fact_)
	, lower_(aggr.lower_)
	, upper_(aggr.upper_)
	, conds_(aggr.conds_)
	, set_(aggr.set_)
	, uniques_(aggr.uniques_)
	, optComplete_(aggr.optComplete_)
{
	foreach(CondLit &lit, conds_) lit.aggr_ = this;
}

void AggrLit::lower(Term *l) 
{ 
	lower_.reset(l); 
}

void AggrLit::upper(Term *u)
{ 
	upper_.reset(u);
}

void AggrLit::assign(Term *a)
{
	lower_.reset(a); assign_ = true;
}

void AggrLit::add(CondLit *cond)
{
	cond->aggr_ = this;
	conds_.push_back(cond);
}

void AggrLit::addDomain(Grounder *g, bool fact) 
{ 
	assert(head());
	foreach(CondLit &lit, conds_) 
		lit.addDomain(g, fact && monotonicity() == MONOTONE);
}

void AggrLit::finish(Grounder *g) 
{ 
	foreach(CondLit &lit, conds_) lit.finish(g);
}

void AggrLit::visit(PrgVisitor *visitor)
{
	if(lower_.get()) visitor->visit(lower_.get(), assign_);
	if(upper_.get()) visitor->visit(upper_.get(), false);
	foreach(CondLit &lit, conds_) visitor->visit(&lit, head());
}

void AggrLit::init(Grounder *g, const VarSet &bound)
{
	foreach(CondLit &lit, conds_) lit.init(g, bound);
}

PredLitSet &
AggrLit::uniqueSet()
{
	return this->uniques_;
}

AggrLit::~AggrLit() 
{
}

namespace
{
	class CondHeadExpander : public Expander
	{
	public:
		CondHeadExpander(AggrLit &aggr, CondLit &cond);
		void expand(Lit *lit, Type type);
	private:
		AggrLit &aggr_;
		CondLit &cond_;
	};

	class CondBodyExpander : public Expander
	{
	public:
		CondBodyExpander(CondLit &cond);
		void expand(Lit *lit, Type type);
	private:
		CondLit &cond_;
	};

	CondHeadExpander::CondHeadExpander(AggrLit &aggr, CondLit &cond)
		: aggr_(aggr)
		, cond_(cond)
	{
	}

	void CondHeadExpander::expand(Lit *lit, Expander::Type type)
	{
		switch(type)
		{
			case RANGE:
			case RELATION:
				cond_.add(lit);
				break;
			case POOL:
				LitPtrVec body;
				foreach(const Lit &l, cond_.body()) body.push_back(l.clone());
				CondLit *clone(new CondLit(cond_.loc(), lit, cond_.weight()->clone(), body));
				aggr_.add(clone);
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
		cond_.add(lit);
	}
}

void AggrLit::normalize(Grounder *g, Expander *expander) 
{
	assert(!(assign_ && sign_));
	assert(!(assign_ && head()));
	if(lower_.get()) lower_->normalize(this, Term::PtrRef(lower_), g, expander, assign_);
	if(upper_.get()) upper_->normalize(this, Term::PtrRef(upper_), g, expander, false);
	for(CondLitVec::size_type i = 0; i < conds_.size(); i++)
	{
		CondHeadExpander headExp(*this, conds_[i]);
		CondBodyExpander bodyExp(conds_[i]);
		conds_[i].normalize(g, &headExp, &bodyExp);
	}
}

WeightLit::WeightLit(const Loc &loc, Term *weight)
	: Lit(loc)
	, weight_(weight)
{
}

void WeightLit::visit(PrgVisitor *visitor)
{
	visitor->visit(weight_.get(), false);
}

void WeightLit::print(Storage *sto, std::ostream &out) const
{
	weight_->print(sto, out);
}

Lit *WeightLit::clone() const
{
	return new WeightLit(*this);
}

WeightLit::~WeightLit() 
{
}

CondLit::CondLit(const Loc &loc, Lit *head, Term *weight, LitPtrVec &body) 
	: Locateable(loc)
	, head_(head)
	, weight_(new WeightLit(weight->loc(), weight))
	, body_(body.release())
{
	head_->head(true);
}

void CondLit::normalize(Grounder *g, Expander *headExp, Expander *bodyExp)
{
	bool head = head_->head();
	head_->head(false);
	head_->normalize(g, headExp);
	head_->head(head);
	weight_->weight()->normalize(head_.get(), Term::PtrRef(weight_->weight()), g, headExp, false);
	for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
		body_[i].normalize(g, bodyExp);
}

void CondLit::ground(Grounder *g)
{
	weights_.clear();
	head_->clear();
	if(inst_.get()) 
	{
		inst_->reset();
		inst_->ground(g);
	}
	else
	{
		if(head_->match(g)) grounded(g);
	}
}

void CondLit::visit(PrgVisitor *visitor)
{
	if(head_->complete() && aggr_->optimizeComplete()) head_->head(false);
	visitor->visit(head_.get(), false);
	visitor->visit(weight_.get(), false);
	foreach(Lit &lit, body_) visitor->visit(&lit, true);
}

void CondLit::print(Storage *sto, std::ostream &out) const
{
	head_->print(sto, out);
	if(!aggr_->set())
	{
		out << "=";
		weight_->print(sto, out);
	}
	foreach(const Lit &lit, body_) 
	{
		out << ":";
		lit.print(sto, out);
	}
}

void CondLit::addDomain(Grounder *g, bool fact) 
{ 
	for(size_t p = 0; p < weights_.size(); p++)
	{
		head_->move(p);
		head_->addDomain(g, fact);
	}
}

void CondLit::finish(Grounder *g)
{
	head_->finish(g);
}

void CondLit::init(Grounder *g, const VarSet &b)
{
	if(body_.size() > 0 || vars_.size() > 0)
	{
		inst_.reset(new Instantiator(this));
		if(vars_.size() > 0) litDep_->order(g, b);
		else
		{
			VarSet bound(b);
			foreach(Lit &lit, body_) 
			{
				lit.init(g, bound);
				lit.index(g, this, bound);
			}
			head_->init(g, bound);
			head_->index(g, this, bound);
		}
	}
	else head_->init(g, b);
	litDep_.reset(0);
}

bool CondLit::grounded(Grounder *g)
{
	try
	{
		Val v = weight_->weight()->val(g);
		head_->grounded(g);
		tribool res = aggr_->accumulate(g, v, *head_);
		if(unknown(res))
		{
			head_->push();
			weights_.push_back(v);
		}
		else if(!res) return false;
		return true;
	}
	catch(const Val *val)
	{
		std::ostringstream oss;
		oss << "cannot convert ";
		val->print(g, oss);
		oss << " to integer";
		std::string str(oss.str());
		oss.str("");
		print(g, oss);
		throw TypeException(str, StrLoc(g, loc()), oss.str());
	}
}

void CondLit::accept(AggrLit::Printer *v)
{
	size_t p = 0;
	foreach(Val &val, weights_)
	{
		head_->move(p++);
		head_->accept(v);
		v->weight(val);
	}
}

CondLit::~CondLit()
{
}

namespace boost
{
	template <>
	CondLit* new_clone(const CondLit& a)
	{
		return new CondLit(a);
	}
}
