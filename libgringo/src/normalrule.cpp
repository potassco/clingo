// Copyright (c) 2008, Roland Kaminski
//
// This file is part of GrinGo.
//
// GrinGo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GrinGo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GrinGo.  If not, see <http://www.gnu.org/licenses/>.

#include <gringo/normalrule.h>
#include <gringo/literal.h>
#include <gringo/predicateliteral.h>
#include <gringo/statementdependencygraph.h>
#include <gringo/literaldependencygraph.h>
#include <gringo/statementchecker.h>
#include <gringo/grounder.h>
#include <gringo/output.h>
#include <gringo/dlvgrounder.h>
#include <gringo/evaluator.h>
#include <gringo/output.h>
#include <gringo/gringoexception.h>
#include <gringo/indexeddomain.h>
#include <gringo/variable.h>

using namespace gringo;

NormalRule::NormalRule(Literal *head, LiteralVector *body) : Statement(), head_(head), body_(body), grounder_(0), ground_(1), base_(0), last_(0), lambda_(0), delta_(0), isGround_(0)
{
}

void NormalRule::buildDepGraph(SDG *dg)
{
	SDGNode *rn = dg->createStatementNode(this);
	if(head_)
		head_->createNode(dg, rn, Literal::ADD_HEAD_DEP);
	else
	{
		// we have an integrity constraint " :- B", which can be modeled by "r :- B, not r",
		// where r is a unique identifier
		// this means the rule depends negativly on itself
		rn->addDependency(rn, true);
	}
	if(body_)
	{
		for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
			(*it)->createNode(dg, rn, Literal::ADD_BODY_DEP);
	}
}

void NormalRule::print(const GlobalStorage *g, std::ostream &out) const
{
	if(head_)
		out << pp(g, head_);
	if(body_)
	{
		out << " :- ";
		bool comma = false;
		for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
		{
			if(comma)
				out << ", ";
			else
				comma = true;
			out << pp(g, *it);
		}
	}
	out << ".";
}

bool NormalRule::checkO(LiteralVector &unsolved)
{
	unsolved.clear();
	// there are some literals (predicates with conditionals) which must have domainrestricted parts
	if(head_)
		if(!head_->checkO(unsolved))
			return false;
	if(body_)
		for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
			if(!(*it)->checkO(unsolved))
				return false;
	return true;
}

bool NormalRule::check(Grounder *g, VarVector &free)
{
	free.clear();
	StatementChecker s;
	if(head_)
		head_->createNode(&s, true, false);
	if(body_)
		for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
			(*it)->createNode(&s, false, false);
	if(s.check())
	{
		isGround_ = !s.hasVars();
		if(g->options().binderSplit && !isGround_ && body_)
		{
			VarSet relevant;
			getRelevantVars(s.getVars(), relevant);
			if(body_ && s.getVars().size() != relevant.size())
			{
				int l = body_->size();
				for(int i = 0; i < l; i++)
					(*body_)[i]->binderSplit(this, relevant);
			}
		}
		return true;
	}
	else
	{
		s.getFreeVars(free);
		return false;
	}
}

void NormalRule::getVars(VarSet &vars) const
{
	if(head_)
		head_->getVars(vars);
	if(body_)
		for(LiteralVector::const_iterator it = body_->begin(); it != body_->end(); it++)
			(*it)->getVars(vars);
}

void NormalRule::addDomain(PredicateLiteral *pl)
{
	if(!body_)
		body_ = new LiteralVector();
	body_->push_back(pl);
}

void NormalRule::reset()
{
	if(head_)
		head_->reset();
}

void NormalRule::finish()
{
	if(head_)
		head_->finish();
}

void NormalRule::evaluate()
{
	if(head_)
		head_->evaluate();
}

void NormalRule::getRelevantVars(const VarSet &global, VarSet &relevant)
{
	VarSet all;
	if(head_)
		head_->getVars(all);
	if(body_)
		for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
			if(!(*it)->solved())
				(*it)->getVars(all);
	VarSet::iterator last = relevant.end();
	for(VarSet::iterator it = all.begin(); it != all.end(); it++)
		if(global.find(*it) != global.end())
			last = relevant.insert(last, *it);
}

void NormalRule::getRelevantVars(const VarSet &global, VarVector &relevant)
{
	VarSet all;
	if(head_)
		head_->getVars(all);
	if(body_)
		for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
			if(!(*it)->solved())
				(*it)->getVars(all);
	for(VarSet::iterator it = all.begin(); it != all.end(); it++)
		if(global.find(*it) != global.end())
			relevant.push_back(*it);
}

namespace
{
	void groundOther(Grounder *g, GroundStep step, Literal *head_, LiteralVector *body_)
	{
		if(head_)
			head_->ground(g, step);
		if(body_)
			for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
				(*it)->ground(g, step);
	}
}

bool NormalRule::ground(Grounder *g, GroundStep step)
{
	// ground the query in the last step
	if(last_ && g->getIncStep() == g->options().ifixed)
	{
		last_   = 0;
		ground_ = 1;
		if(step == REINIT)
			step = PREPARE;
	}
	// start grounding cumulative from timestep 1
	if(lambda_ && g->getIncStep() > 0)
	{
		lambda_ = 0;
		ground_ = 1;
		if(step == REINIT)
			step = PREPARE;
	}
	// skip queries until iquery is reached
	if(delta_ && g->getIncStep() >= g->options().iquery)
	{
		delta_  = 0;
		ground_ = 1;
		if(step == REINIT)
			step = PREPARE;
	}
	if(!ground_)
		return true;
	//std::cerr << "grounding: " << pp(g, this) << "(" << step << ")"<< std::endl;
	switch(step)
	{
		case PREPARE:
			// if there are no varnodes we can do sth simpler
			if(body_ && !isGround_)
			{
				LDG dg;
				{
					LDGBuilder dgb(&dg);
					if(head_)
						dgb.addHead(head_);
					if(body_)
						for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
							dgb.addToBody(*it);
					dgb.create();
				}
				//std::cerr << "creating grounder for: " << this << std::endl;
				VarVector relevant;
				getRelevantVars(VarSet(dg.getGlobalVars().begin(), dg.getGlobalVars().end()), relevant);
				dg.sortLiterals(body_);
				grounder_ = new DLVGrounder(g, this, body_, &dg, relevant);
				groundOther(g, step, head_, body_);
			}
			else
			{
				grounder_ = 0;
				groundOther(g, step, head_, body_);
			}
			break;
		case REINIT:
			if(grounder_)
				grounder_->reinit();
			groundOther(g, step, head_, body_);
			break;
		case GROUND:
			//std::cerr << "grounding: " << pp(g, this) << std::endl;
			if(grounder_)
			{
				//std::cerr << "grounding: " << this << std::endl;
				grounder_->ground();
			}
			else
			{
				if(body_)
					for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
						if(!(*it)->match(g))
							return true;
				grounded(g);
			}
			break;
		case RELEASE:
			if(grounder_)
			{
				if(g->isIncGrounding())
				{
					grounder_->release();
				}
				else
				{
					delete grounder_;
					grounder_ = 0;
				}
			}
			groundOther(g, step, head_, body_);
			// ground the base only one time
			if(base_)
				ground_ = 0;
			break;
	}
	return true;
}

void NormalRule::grounded(Grounder *g)
{
	bool hasHead;
	NS_OUTPUT::Object *head;
	NS_OUTPUT::ObjectVector body;
	Evaluator *eval = g->getEvaluator();

	if(head_ && head_->match(g))
	{
		// NOTE: if the head is already a fact this rule is useless!!! of course 
		// except if the head is a choice head since choice heads are always facts
		// currently aggregates are only facts if all their literals are facts
		if(head_->isFact(g))
			return;
		hasHead = true;
		head = head_->convert();
	}
	else
	{
		hasHead = false;
		head    = 0;
	}
	// cache variables in solved predicates
	if(body_)
	{
		for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
		{
			if(!(*it)->isFact(g))
			{
				// flatten conjunctions
				NS_OUTPUT::Object *c = (*it)->convert();
				if(dynamic_cast<NS_OUTPUT::Conjunction*>(c))
				{
					body.insert(
						body.end(), 
						static_cast<NS_OUTPUT::Conjunction*>(c)->lits_.begin(),
						static_cast<NS_OUTPUT::Conjunction*>(c)->lits_.end());
					static_cast<NS_OUTPUT::Conjunction*>(c)->lits_.clear();
					delete c;
				}
				else
					body.push_back(c);
			}
		}
	}
	if(!hasHead && body.size() == 0)
	{
		// TODO: thats not very nice find a better solution
		NS_OUTPUT::Conjunction *c = new NS_OUTPUT::Conjunction(body);
		NS_OUTPUT::Integrity  *i = new NS_OUTPUT::Integrity(c);
		eval->add(i);
	}
	else if(!hasHead)
	{
		NS_OUTPUT::Conjunction *c = new NS_OUTPUT::Conjunction(body);
		NS_OUTPUT::Integrity  *i = new NS_OUTPUT::Integrity(c);
		eval->add(i);
	}
	else if(body.size() == 0)
	{
		NS_OUTPUT::Fact *f = new NS_OUTPUT::Fact(head);
		eval->add(f);
	}
	else
	{
		NS_OUTPUT::Conjunction *c = new NS_OUTPUT::Conjunction(body);
		NS_OUTPUT::Rule *r = new NS_OUTPUT::Rule(head, c);
		eval->add(r);
	}
}

namespace
{
	class NormalRuleExpander : public Expandable
	{
	public:
		NormalRuleExpander(NormalRule *n, Grounder *g, LiteralVector *r) : n_(n), g_(g), r_(r)
		{
		}
		void appendLiteral(Literal *l, ExpansionType type)
		{
			if(type == MATERM)
			{
				LiteralVector *r;
				if(r_)
				{
					r = new LiteralVector();
					for(LiteralVector::iterator it = r_->begin(); it != r_->end(); it++)
						r->push_back((*it)->clone());
				}
				else
					r = 0;
				g_->addStatement(new NormalRule(l, r));
			}
			else
			{
				n_->appendLiteral(l, type);
			}
				
		}
	protected:
		NormalRule    *n_;
		Grounder      *g_;
		LiteralVector *r_;
	};
}

void NormalRule::preprocess(Grounder *g)
{
	if(head_)
	{
		NormalRuleExpander nre(this, g, body_);
		head_->preprocess(g, &nre, true);
	}
	if(body_)
		for(size_t i = 0; i < body_->size(); i++)
			(*body_)[i]->preprocess(g, this, false);
	//std::cerr << this << std::endl;
}

void NormalRule::appendLiteral(Literal *l, ExpansionType type)
{
	if(!body_)
		body_ = new LiteralVector();	
	body_->push_back(l);
}

namespace
{
	class DeltaLiteral : public Literal
	{
	public:
		DeltaLiteral() { }
		DeltaLiteral(const DeltaLiteral &l) : Literal(l) { }
		void getVars(VarSet &vars) const { }
		bool checkO(LiteralVector &unsolved) { return true; }
		void preprocess(Grounder *g, Expandable *e, bool head) { }
		void reset() { FAIL(true); }
		void finish() { FAIL(true); }
		void evaluate() { FAIL(true); }
		bool solved() { return false; }
		void createNode(StatementChecker *dg, bool head, bool delayed) { }
		void addIncParam(Grounder *g, const Value &v) { }
 		virtual ~DeltaLiteral() { }

		SDGNode *createNode(SDG *dg, SDGNode *prev, DependencyAdd todo) 
		{
			// a rule with a goal is always a normal logic program
			assert(todo == ADD_BODY_DEP);
			prev->addDependency(prev, true);
			return 0;
		}

		void createNode(LDGBuilder *dg, bool head) 
		{ 
			assert(!head);
			VarSet empty;
			dg->createNode(this, head, empty, empty);
		}

		bool isFact(Grounder *g)
		{
			if(g->options().ifixed >= 0)
				return true;
			else
				return false;
		}

		Literal* clone() const
		{
			return new DeltaLiteral(*this);
		}

		IndexedDomain *createIndexedDomain(Grounder *g, VarSet &index)
		{
			return new IndexedDomainMatchOnly(this);
		}

		bool match(Grounder *g)
		{
			return true;
		}

		NS_OUTPUT::Object *convert()
		{
			return new NS_OUTPUT::DeltaObject();
		}

		double heuristicValue()
		{
			return 0;
		}

		void print(const GlobalStorage *g, std::ostream &out) const
		{
			out << "delta";
		}
	};
}

void NormalRule::setIncPart(Grounder *g, IncPart part, const Value &v)
{
	switch(part)
	{
		case BASE:
			base_ = 1;
			break;
		case DELTA:
			if(g->options().ibase)
			{
				ground_ = 0;
				break;
			}
			else if(g->options().ifixed >= 0)
			{
				ground_ = 0;
				last_   = 1;
			}
			else
			{
				ground_ = 0;
				delta_  = 1;
			}
			appendLiteral(new DeltaLiteral(), Expandable::COMPLEXTERM);
			// no break! //
		case LAMBDA:
			if(g->options().ibase)
			{
				ground_ = 0;
				break;
			}
			else if(part == LAMBDA)
			{
				ground_ = 0;
				lambda_ = 1;
			}
			if(head_)
				head_->addIncParam(g, v);
			if(body_)
			{
				for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
					(*it)->addIncParam(g, v);
			}
			break;
		default:
			break;
	}
}

NormalRule::~NormalRule()
{
	if(grounder_)
		delete grounder_;
	if(head_)
		delete head_;
	if(body_)
	{
		for(LiteralVector::iterator it = body_->begin(); it != body_->end(); it++)
			delete *it;
		delete body_;
	}
}

