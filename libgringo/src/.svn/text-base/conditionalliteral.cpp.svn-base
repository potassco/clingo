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

#include <gringo/conditionalliteral.h>
#include <gringo/predicateliteral.h>
#include <gringo/grounder.h>
#include <gringo/term.h>
#include <gringo/indexeddomain.h>
#include <gringo/dlvgrounder.h>
#include <gringo/value.h>
#include <gringo/literaldependencygraph.h>
#include <gringo/statementchecker.h>
#include <gringo/aggregateliteral.h>
#include <gringo/domain.h>

using namespace gringo;
		
ConditionalLiteral::ConditionalLiteral(PredicateLiteral *pred, LiteralVector *conditionals) : Literal(), pred_(pred), conditionals_(conditionals), weight_(0), grounder_(0), dg_(0), clone_(0)
{
	Literal::setNeg(pred_->getNeg());
}

void ConditionalLiteral::setWeight(Term *w)
{
	weight_ = w;
}

void ConditionalLiteral::setNeg(bool neg)
{
	pred_->setNeg(neg);
	Literal::setNeg(neg);
}

void ConditionalLiteral::appendLiteral(Literal *l, ExpansionType type)
{
	if(!conditionals_)
		conditionals_ = new LiteralVector();
	conditionals_->push_back(l);
}

void ConditionalLiteral::getVars(VarSet &vars) const
{
	pred_->getVars(vars);
	if(conditionals_)
		for(LiteralVector::const_iterator it = conditionals_->begin(); it != conditionals_->end(); it++)
			(*it)->getVars(vars);
	if(weight_)
		weight_->getVars(vars);
}

bool ConditionalLiteral::checkO(LiteralVector &unsolved)
{
	if(conditionals_ == 0)
		return true;
	for(LiteralVector::iterator it = conditionals_->begin(); it != conditionals_->end(); it++)
	{
		if(!(*it)->solved())
			unsolved.push_back(*it);
	}
	return unsolved.size() == 0;
}

SDGNode *ConditionalLiteral::createNode(SDG *dg, SDGNode *prev, DependencyAdd todo)
{
	SDGNode *n = pred_->createNode(dg, prev, todo);
	if(conditionals_)
	{
		for(LiteralVector::iterator it = conditionals_->begin(); it != conditionals_->end(); it++)
			(*it)->createNode(dg, prev, ADD_BODY_DEP);
	}
	return n;
}

namespace
{
	// TODO: its better reimplement the LDGBuilder to work similar like the SDGBuilder
	//       to avoid things like this!!!
	class HeadNodeHelper : public Literal
	{
	public:
		HeadNodeHelper(PredicateLiteral *pred, const VarSet &vars) : pred_(pred), vars_(vars) {}
		void createNode(LDGBuilder *dg, bool head) 
		{
			assert(head);
			VarSet needed = vars_, provided;
			if(head || pred_->getNeg() || !pred_->getDomain()->complete())
			{
				if(pred_->getArgs())
					for(TermVector::iterator it = pred_->getArgs()->begin(); it != pred_->getArgs()->end(); it++)
						(*it)->getVars(needed);
			}
			else
			{
				if(pred_->getArgs())
					for(TermVector::iterator it = pred_->getArgs()->begin(); it != pred_->getArgs()->end(); it++)
						if((*it)->isComplex())
							(*it)->getVars(needed);
						else
							(*it)->getVars(provided);
			}
			dg->createNode(pred_, head, needed, provided);
			delete this;
		}

		// not used!
		void getVars(VarSet &vars) const { FAIL(true); };
		bool checkO(LiteralVector &unsolved) { FAIL(true); }
		void preprocess(Grounder *g, Expandable *e, bool head) { FAIL(true); }
		void reset() { FAIL(true); }
		void finish() { FAIL(true); }
		bool solved() { FAIL(true); }
		bool isFact(Grounder *g) { FAIL(true); }
		Literal* clone() const { FAIL(true); }
		IndexedDomain *createIndexedDomain(Grounder *g, VarSet &index) { FAIL(true); }
		bool match(Grounder *g) { FAIL(true); }
		NS_OUTPUT::Object *convert() { FAIL(true); }
		SDGNode *createNode(SDG *dg, SDGNode *prev, DependencyAdd todo) { FAIL(true); }
		void createNode(StatementChecker *dg, bool head, bool delayed) { FAIL(true); }
		double heuristicValue() { FAIL(true); }
		void addIncParam(Grounder *g, const Value &v) { FAIL(true); }
		void print(const GlobalStorage *g, std::ostream &out) const { FAIL(true); }
	private:
		PredicateLiteral *pred_;
		VarSet vars_;
	};
}

void ConditionalLiteral::createNode(LDGBuilder *dgb, bool head)
{
	dg_ = new LDG();
	LDGBuilder *subDg = new LDGBuilder(dg_);
	VarSet vars;
	if(weight_)
		weight_->getVars(vars);
	if(vars.size() > 0)
		subDg->addHead(new HeadNodeHelper(pred_, vars));
	else
		subDg->addHead(pred_);
	if(conditionals_)
		for(LiteralVector::iterator it = conditionals_->begin(); it != conditionals_->end(); it++)
			subDg->addToBody(*it);
	dgb->addGraph(subDg);
}

void ConditionalLiteral::createNode(StatementChecker *dg, bool head, bool delayed)
{
	if(weight_)
	{
		VarSet needed, provided;
		weight_->getVars(needed);
		dg->createNode(needed, provided);
	}
	pred_->createNode(dg, true, false);
	if(conditionals_)
		for(LiteralVector::iterator it = conditionals_->begin(); it != conditionals_->end(); it++)
			(*it)->createNode(dg, false, false);
}

void ConditionalLiteral::print(const GlobalStorage *g, std::ostream &out) const
{
	out << pp(g, pred_);
	if(conditionals_)
		for(LiteralVector::iterator it = conditionals_->begin(); it != conditionals_->end(); it++)
			out << " : " << pp(g, *it);
	if(weight_)
		out << " = " << pp(g, weight_);
}

bool ConditionalLiteral::match(Grounder *g)
{
	FAIL(true);
}

bool ConditionalLiteral::isFact(Grounder *g)
{
	FAIL(true);
}

void ConditionalLiteral::start()
{
	current_ = 0;
}

void ConditionalLiteral::remove()
{
	std::swap(values_[current_], values_.back());
	values_.pop_back();
	if(weight_)
	{
		std::swap(weights_[current_], weights_.back());
		weights_.pop_back();
	}
	current_--;
}

bool ConditionalLiteral::hasNext()
{
	return current_ < values_.size();
}

void ConditionalLiteral::next()
{
	current_++;
}

bool ConditionalLiteral::isFact()
{
	return pred_->isFact(values_[current_]);
}

bool ConditionalLiteral::match()
{
	return pred_->match(values_[current_]);
}

const ValueVector &ConditionalLiteral::getValues()
{
	return values_[current_];
}

bool ConditionalLiteral::hasWeight()
{
	return weight_ != 0;
}

int ConditionalLiteral::getWeight()
{
	if(weight_)
		return weights_[current_];
	else
		return 1;
}

int ConditionalLiteral::count()
{
	return values_.size();
}

void ConditionalLiteral::reset()
{
	if(!getNeg())
		pred_->reset();
}

void ConditionalLiteral::finish()
{
	if(!getNeg())
		pred_->finish();
}

bool ConditionalLiteral::solved()
{
	return pred_->solved();
}

void ConditionalLiteral::ground(Grounder *g, GroundStep step)
{
	switch(step)
	{
		case PREPARE:
			if(conditionals_)
			{
				if(!dg_)
				{
					dg_ = new LDG();
					LDGBuilder dgb(dg_);
					dgb.addHead(pred_);
					if(conditionals_)
						for(LiteralVector::iterator it = conditionals_->begin(); it != conditionals_->end(); it++)
							dgb.addToBody(*it);
					dgb.create();
				}
				dg_->sortLiterals(conditionals_);
				grounder_ = new DLVGrounder(g, this, conditionals_, dg_, dg_->getGlobalVars());
			}
			else
				grounder_ = 0;
			if(dg_)
			{
				delete dg_;
				dg_ = 0;
			}
			break;
		case REINIT:
			if(grounder_)
				grounder_->reinit();
			break;
		case GROUND:
			weights_.clear();
			values_.clear();
			if(grounder_)
				grounder_->ground();
			else
				grounded(g);
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
			break;
	}
}

void ConditionalLiteral::grounded(Grounder *g)
{
	ValueVector values;
	values.reserve(pred_->getArgs()->size());
	for(TermVector::iterator it = pred_->getArgs()->begin(); it != pred_->getArgs()->end(); it++)
		values.push_back((*it)->getValue(g));
	if(weight_)
		weights_.push_back(weight_->getValue(g));
	values_.push_back(ValueVector());

	std::swap(values_.back(), values);
}

bool ConditionalLiteral::isEmpty()
{
	return values_.size() == 0;
}

IndexedDomain *ConditionalLiteral::createIndexedDomain(Grounder *g, VarSet &index)
{
	FAIL(true);
}

ConditionalLiteral::ConditionalLiteral(const ConditionalLiteral &p) : Literal(p), pred_(p.clone_ ? p.clone_ : static_cast<PredicateLiteral*>(p.pred_->clone())), weight_(p.weight_ ? p.weight_->clone() : 0), grounder_(0), dg_(0), clone_(0)
{
	if(p.conditionals_)
	{
		conditionals_ = new LiteralVector();
		for(LiteralVector::const_iterator it = p.conditionals_->begin(); it != p.conditionals_->end(); it++)
			conditionals_->push_back((*it)->clone());
	}
	else
		conditionals_ = 0;
}

Literal* ConditionalLiteral::clone() const
{
	return new ConditionalLiteral(*this);
}

ConditionalLiteral::~ConditionalLiteral()
{
	if(dg_)
		delete dg_;
	if(pred_)
		delete pred_;
	if(conditionals_)
	{
		for(LiteralVector::iterator it = conditionals_->begin(); it != conditionals_->end(); it++)
		{
			delete *it;
		}
		delete conditionals_;
	}
	if(weight_)
		delete weight_;
	if(grounder_)
		delete grounder_;
}

namespace
{
	class ConditionalLiteralExpander : public Expandable
	{
	public:
		ConditionalLiteralExpander(ConditionalLiteral *l, Expandable *e, LiteralVector *c, Term *w) : l_(l), e_(e), c_(c), w_(w)
		{
		}
		void appendLiteral(Literal *l, ExpansionType type)
		{
			if(type == MATERM)
			{
				LiteralVector *c;
				if(c_)
				{
					c = new LiteralVector();
					for(LiteralVector::iterator it = c_->begin(); it != c_->end(); it++)
						c->push_back((*it)->clone());
				}
				else
					c = 0;
				ConditionalLiteral *n = new ConditionalLiteral((PredicateLiteral*)l, c);
				n->setWeight(w_ ? static_cast<Term*>(w_->clone()) : 0);
				e_->appendLiteral(n, type);
			}
			else
			{
				l_->appendLiteral(l, type);
			}
		}
	protected:
		ConditionalLiteral *l_;
		Expandable         *e_;
		LiteralVector      *c_;
		Term               *w_;
	};

	class DisjunctiveConditionalLiteralExpander : public Expandable
	{
	public:
		DisjunctiveConditionalLiteralExpander(ConditionalLiteral *l, AggregateLiteral *a, Expandable *e) : l_(l), a_(a), e_(e)
		{
		}
		void appendLiteral(Literal *l, ExpansionType type)
		{
			switch(type)
			{
				case MATERM:
				{
					l_->clonePredicate(static_cast<PredicateLiteral*>(l));
					e_->appendLiteral(a_->clone(), type);
					l_->clonePredicate(0);
					break;
				}
				case RANGETERM:
					e_->appendLiteral(l, type);
					break;
				default:
					l_->appendLiteral(l, type);
					break;
			}
		}
	protected:
		ConditionalLiteral *l_;
		AggregateLiteral   *a_;
		Expandable         *e_;
	};
}

void ConditionalLiteral::clonePredicate(PredicateLiteral *clone)
{
	clone_ = clone;
}

NS_OUTPUT::Object *ConditionalLiteral::convert()
{
	return pred_->convert(getValues());
}

void ConditionalLiteral::preprocessDisjunction(Grounder *g, AggregateLiteral *a, Expandable *e, bool head)
{
	assert(!weight_);
	DisjunctiveConditionalLiteralExpander cle(this, a, e);
	pred_->preprocess(g, &cle, head);
	if(conditionals_)
	{
		for(size_t i = 0; i < conditionals_->size(); i++)
			(*conditionals_)[i]->preprocess(g, this, false);
	}
}

void ConditionalLiteral::preprocess(Grounder *g, Expandable *e, bool head)
{
	ConditionalLiteralExpander cle(this, e, conditionals_, weight_);
	pred_->preprocess(g, &cle, head);
	if(conditionals_)
	{
		for(size_t i = 0; i < conditionals_->size(); i++)
			(*conditionals_)[i]->preprocess(g, this, false);
	}
	if(weight_)
		weight_->preprocess(this, weight_, g, &cle);
}

void ConditionalLiteral::addIncParam(Grounder *g, const Value &v)
{
	pred_->addIncParam(g, v);
	if(conditionals_)
		for(LiteralVector::iterator it = conditionals_->begin(); it != conditionals_->end(); it++)
			(*it)->addIncParam(g, v);
	if(weight_)
		weight_->addIncParam(g, weight_, v);
}

double ConditionalLiteral::heuristicValue()
{
	FAIL(true);
}

int ConditionalLiteral::getUid()
{
	return pred_->getUid();
}

bool ConditionalLiteral::hasConditionals()
{
	return conditionals_ ? conditionals_->size() > 0 : false;
}

PredicateLiteral *ConditionalLiteral::toPredicateLiteral()
{
	PredicateLiteral *pred = pred_;
	pred_ = 0;
	delete this;
	return pred;
}

