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

#include <gringo/compute.h>
#include <gringo/lit.h>
#include <gringo/term.h>
#include <gringo/output.h>
#include <gringo/predlit.h>
#include <gringo/grounder.h>
#include <gringo/prgvisitor.h>
#include <gringo/litdep.h>
#include <gringo/exceptions.h>
#include <gringo/instantiator.h>

Compute::Compute(const Loc &loc, PredLit *head, LitPtrVec &body)
	: Statement(loc)
	, head_(head)
	, body_(body.release())
	, grounded_(false)
{
}

void Compute::ground(Grounder *g)
{
	head_->clear();
	if(inst_.get()) 
	{
		inst_->reset();
		inst_->ground(g);
	}
	else if(!grounded_)
	{
		grounded_ = true;
		if(head_->match(g)) grounded(g);
	}
	else return;
	Printer *printer = g->output()->printer<Printer>();
	printer->begin();
	printer->print(head_.get());
	printer->end();
}

bool Compute::grounded(Grounder *g)
{
	try
	{
		head_->grounded(g);
		head_->push();
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

namespace
{
	class ComputeHeadExpander : public Expander
	{
	public:
		ComputeHeadExpander(Grounder *g, Compute &min);
		void expand(Lit *lit, Type type);
	private:
		Grounder *g_;
		Compute &com_;
	};

	class ComputeBodyExpander : public Expander
	{
	public:
		ComputeBodyExpander(Compute &min);
		void expand(Lit *lit, Type type);
	private:
		Compute &com_;
	};

	ComputeHeadExpander::ComputeHeadExpander(Grounder *g, Compute &com)
		: g_(g)
		, com_(com)
	{
	}

	void ComputeHeadExpander::expand(Lit *lit, Expander::Type type)
	{
		switch(type)
		{
			case RANGE:
			case RELATION:
				com_.body().push_back(lit);
				break;
			case POOL:
				LitPtrVec body;
				foreach(Lit &l, com_.body()) body.push_back(l.clone());
				Compute *o = new Compute(com_.loc(), static_cast<PredLit*>(lit), body);
				g_->addInternal(o);
				break;
		}
	}

	ComputeBodyExpander::ComputeBodyExpander(Compute &min)
		: com_(min)
	{
	}

	void ComputeBodyExpander::expand(Lit *lit, Expander::Type type)
	{
		(void)type;
		com_.body().push_back(lit);
	}
}

void Compute::normalize(Grounder *g)
{
	ComputeHeadExpander headExp(g, *this);
	ComputeBodyExpander bodyExp(*this);
	head_->normalize(g, &headExp);
	for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
		body_[i].normalize(g, &bodyExp);
}

void Compute::init(Grounder *g, const VarSet &b)
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

void Compute::visit(PrgVisitor *visitor)
{
	visitor->visit(static_cast<Lit*>(head_.get()), false);
	foreach(Lit &lit, body_) visitor->visit(&lit, true);
}

void Compute::print(Storage *sto, std::ostream &out) const
{
	out << "#compute{";
	head_->print(sto, out);
	foreach(const Lit &lit, body_) 
	{
		out << ":";
		lit.print(sto, out);
	}
	out << "}";
}

void Compute::append(Lit *lit)
{
	body_.push_back(lit);
}

Compute::~Compute()
{
}

