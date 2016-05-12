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

#include <gringo/rule.h>
#include <gringo/lit.h>
#include <gringo/instantiator.h>
#include <gringo/index.h>
#include <gringo/printer.h>
#include <gringo/litdep.h>
#include <gringo/grounder.h>
#include <gringo/output.h>

Rule::Rule(const Loc &loc, Lit *head, LitPtrVec &body)
	: Statement(loc)
	, head_(head)
	, body_(body.release())
	, grounded_(false)
{
	if(head) head->head(true);
}

namespace
{
	class RuleHeadExpander : public Expander
	{
	public:
		RuleHeadExpander(const Loc &loc, LitPtrVec &body, Grounder *g);
		void expand(Lit *lit, Type type);
	private:
		const Loc &loc_;
		LitPtrVec &body_;
		Grounder  *g_;
	};

	class RuleBodyExpander : public Expander
	{
	public:
		RuleBodyExpander(LitPtrVec &body);
		void expand(Lit *lit, Type type);
	private:
		LitPtrVec &body_;
	};

	RuleHeadExpander::RuleHeadExpander(const Loc &loc, LitPtrVec &body, Grounder *g)
		: loc_(loc)
		, body_(body)
		, g_(g)
	{
	}

	void RuleHeadExpander::expand(Lit *lit, Expander::Type type)
	{
		switch(type)
		{
			case RANGE:
			case RELATION:
				body_.push_back(lit);
				break;
			case POOL:
				LitPtrVec body;
				foreach(Lit &l, body_) body.push_back(l.clone());
				g_->addInternal(new Rule(loc_, lit, body));
				break;
		}
	}

	RuleBodyExpander::RuleBodyExpander(LitPtrVec &body)
		: body_(body)
	{
	}

	void RuleBodyExpander::expand(Lit *lit, Expander::Type type)
	{
		(void)type;
		body_.push_back(lit);
	}
}

void Rule::normalize(Grounder *g)
{
	if(head_.get())
	{
		RuleHeadExpander exp(loc(), body_, g);
		head_->normalize(g, &exp);
	}
	if(body_.size() > 0)
	{
		RuleBodyExpander exp(body_);
		for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
			body_[i].normalize(g, &exp);
	}
}

void Rule::init(Grounder *g, const VarSet &b)
{
	if(body_.size() > 0)
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
			if(head_.get()) 
			{
				head_->init(g, bound);
				head_->index(g, this, bound);
			}
		}
	}
	else if(head_.get()) head_->init(g, b);
}

bool Rule::edbFact() const
{
	return body_.size() == 0 && head_.get() && head_->edbFact();
}

void Rule::ground(Grounder *g)
{
	if(inst_.get()) inst_->ground(g);
	else if(!grounded_)
	{
		grounded_ = true;
		if(head_.get()) head_->match(g);
		grounded(g);
	}	
	if(head_.get()) head_->finish(g);
}

bool Rule::grounded(Grounder *g)
{
	if(head_.get())
	{
		head_->grounded(g);
		if(head_->fact()) return true;
	}
	Printer *printer = g->output()->printer<Printer>();
	printer->begin();
	if(head_.get()) head_->accept(printer);
	printer->endHead();
	bool fact = true;
	foreach(Lit &lit, body_)
	{
		lit.grounded(g);
		if(!lit.fact())
		{
			lit.accept(printer);
			fact = false;
		}
		else if(lit.forcePrint()) { lit.accept(printer); }
	}
	if(head_.get()) head_->addDomain(g, fact);
	printer->end();
	return true;
}

void Rule::append(Lit *l)
{
	body_.push_back(l);
}

void Rule::visit(PrgVisitor *v)
{
	for(size_t i = 0, n = body_.size(); i < n; i++) v->visit(&body_[i], false);
	if(head_.get()) v->visit(head_.get(), false);
}

void Rule::print(Storage *sto, std::ostream &out) const
{
	if(head_.get()) head_->print(sto, out);
	if(body_.size() > 0)
	{
		out << ":-";
		bool comma = false;
		foreach(const Lit &lit, body_)
		{
			if(comma) out << ",";
			else comma = true;
			lit.print(sto, out);
		}
	}
	out << ".";
}

Rule::~Rule()
{
}

