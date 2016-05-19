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

#include <gringo/conjunctionaggregate.h>
#include <gringo/conditionalliteral.h>
#include <gringo/predicateliteral.h>
#include <gringo/term.h>
#include <gringo/value.h>
#include <gringo/output.h>
#include <gringo/grounder.h>
#include <gringo/statementdependencygraph.h>

using namespace gringo;

ConjunctionAggregate::ConjunctionAggregate(ConditionalLiteralVector *literals) : AggregateLiteral(literals)
{
}

void ConjunctionAggregate::setNeg(bool neg)
{
	// the literal itself will never be negative
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
		(*it)->setNeg(neg);
}

void ConjunctionAggregate::doMatch(Grounder *g)
{
	FAIL(true);
}

bool ConjunctionAggregate::match(Grounder *g)
{
	fact_ = true;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		ConditionalLiteral *p = *it;
		p->ground(g, GROUND);
		for(p->start(); p->hasNext(); p->next())
		{
			if(!p->match())
				return false;
			if(p->isFact())
				p->remove();
			else
				fact_ = false;
		}
	}
	return true;
}

void ConjunctionAggregate::print(const GlobalStorage *g, std::ostream &out) const
{
	bool comma = false;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		if(comma)
			out << ", ";
		else
			comma = true;
		out << pp(g, *it);
	}
}

SDGNode *ConjunctionAggregate::createNode(SDG *dg, SDGNode *prev, DependencyAdd todo)
{
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		// aggregate literals always depend negativly on its literals 
		SDGNode *p = (*it)->createNode(dg, prev, ADD_NOTHING);
		assert(p);
		prev->addDependency(p, (*it)->getNeg());
		// if used in the head it also depends cyclically on its literals
		// but not if the literal is negative
		if(todo == ADD_HEAD_DEP && !(*it)->getNeg())
			p->addDependency(prev);
	}
	return 0;
}

ConjunctionAggregate::ConjunctionAggregate(const ConjunctionAggregate &a) : AggregateLiteral(a)
{
}

void ConjunctionAggregate::preprocess(Grounder *g, Expandable *e, bool head)
{
	assert(literals_);
	for(size_t i = 0; i < literals_->size(); i++)
		(*literals_)[i]->preprocessDisjunction(g, this, e, head);
	assert(!upper_);
	assert(!lower_);
}

NS_OUTPUT::Object *ConjunctionAggregate::convert()
{
	NS_OUTPUT::ObjectVector lits;
	for(ConditionalLiteralVector::iterator it = getLiterals()->begin(); it != getLiterals()->end(); it++)
	{
		ConditionalLiteral *p = *it;
		for(p->start(); p->hasNext(); p->next())
			lits.push_back(p->convert());
	}
	return new NS_OUTPUT::Conjunction(lits);
}

Literal *ConjunctionAggregate::clone() const
{
	return new ConjunctionAggregate(*this);
}

Literal *ConjunctionAggregate::createBody(PredicateLiteral *pred, LiteralVector *list)
{
	if(list == 0 || list->size() == 0)
		return pred;
	else
	{
		ConditionalLiteralVector *clv = new ConditionalLiteralVector();
		clv->push_back(new ConditionalLiteral(pred, list));
		return new ConjunctionAggregate(clv);
	}
}

ConjunctionAggregate::~ConjunctionAggregate()
{
}

