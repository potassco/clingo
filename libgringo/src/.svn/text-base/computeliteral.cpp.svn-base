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

#include <gringo/computeliteral.h>
#include <gringo/conditionalliteral.h>
#include <gringo/value.h>
#include <gringo/grounder.h>
#include <gringo/statementdependencygraph.h>
#include <gringo/literaldependencygraph.h>
#include <gringo/statementchecker.h>
#include <gringo/output.h>

using namespace gringo;

ComputeLiteral::ComputeLiteral(ConditionalLiteralVector *literals, int number) : Literal(), number_(number), literals_(literals)
{
}

bool ComputeLiteral::solved()
{
	FAIL(true);
}

bool ComputeLiteral::isFact(Grounder *g)
{
	FAIL(true);
}

bool ComputeLiteral::checkO(LiteralVector &unsolved) 
{
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		if(!(*it)->checkO(unsolved))
			return false;
	}
	return true;
}

namespace
{
	struct Hash
	{
		Value::VectorHash hash;
		size_t operator()(const std::pair<int, ValueVector> &k) const
		{
			return (size_t)k.first + hash(k.second);
		}
	};
	struct Equal
	{
		Value::VectorEqual equal;
		size_t operator()(const std::pair<int, ValueVector> &a, const std::pair<int, ValueVector> &b) const
		{
			return a.first == b.first && equal(a.second, b.second);
		}
	};
	typedef HashSet<std::pair<int, ValueVector>, Hash, Equal>::type UidValueSet;
}

NS_OUTPUT::Object *ComputeLiteral::convert()
{
	NS_OUTPUT::ObjectVector lits;
	UidValueSet set;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		ConditionalLiteral *p = *it;
		for(p->start(); p->hasNext(); p->next())
		{
			// caution there is no -0
			// match has to be called before isFact
			if(!set.insert(std::make_pair(p->getNeg() ? -1 - p->getUid() : p->getUid(), p->getValues())).second || (p->match() && p->isFact()))
			{
				p->remove();
				continue;
			}
			lits.push_back(p->convert());
		}
	}

	return new NS_OUTPUT::Compute(lits, number_);
}

void ComputeLiteral::ground(Grounder *g, GroundStep step)
{
	for(ConditionalLiteralVector::const_iterator it = literals_->begin(); it != literals_->end(); it++)
		(*it)->ground(g, step);
}

bool ComputeLiteral::match(Grounder *g)
{
	ground(g, GROUND);
	return true;
}

void ComputeLiteral::getVars(VarSet &vars) const
{
	for(ConditionalLiteralVector::const_iterator it = literals_->begin(); it != literals_->end(); it++)
		(*it)->getVars(vars);
}

SDGNode *ComputeLiteral::createNode(SDG *dg, SDGNode *prev, DependencyAdd todo)
{
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		// since there cant be any cycles through these statements 
		// its ok to add only positive dependencies
		// even though negative dependencies wont hurt
		SDGNode *p = (*it)->createNode(dg, prev, ADD_NOTHING);
		assert(p);
		prev->addDependency(p);
	}
	return 0;
}

void ComputeLiteral::createNode(LDGBuilder *dg, bool head)
{
	VarSet needed, provided;
	dg->createNode(this, head, needed, provided, true);
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
		(*it)->createNode(dg, head);
}

void ComputeLiteral::createNode(StatementChecker *dg, bool head, bool delayed)
{
	for(ConditionalLiteralVector::const_iterator it = literals_->begin(); it != literals_->end(); it++)
		(*it)->createNode(dg, false, false);
}

void ComputeLiteral::reset()
{
	FAIL(true);
}

void ComputeLiteral::finish()
{
	FAIL(true);
}

IndexedDomain *ComputeLiteral::createIndexedDomain(Grounder *g, VarSet &index)
{
	FAIL(true);
}

ComputeLiteral::ComputeLiteral(const ComputeLiteral &a) : Literal(a), number_(a.number_)
{
	if(a.literals_)
	{
		literals_ = new ConditionalLiteralVector();
		for(ConditionalLiteralVector::iterator it = a.literals_->begin(); it != a.literals_->end(); it++)
			literals_->push_back((ConditionalLiteral*)(*it)->clone());
	}
	else
		literals_ = 0;
}

Literal *ComputeLiteral::clone() const
{
	return new ComputeLiteral(*this);
}

void ComputeLiteral::appendLiteral(Literal *l, ExpansionType type)
{
	if(!literals_)
		literals_ = new ConditionalLiteralVector();
	assert(dynamic_cast<ConditionalLiteral*>(l));
	literals_->push_back((ConditionalLiteral*)l);
}

void ComputeLiteral::preprocess(Grounder *g, Expandable *e, bool head)
{
	if(literals_)
		for(size_t i = 0; i < literals_->size(); i++)
			(*literals_)[i]->preprocess(g, this, head);
}

void ComputeLiteral::print(const GlobalStorage *g, std::ostream &out) const
{
	out << "compute {";
	bool comma = false;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		if(comma)
			out << ", ";
		else
			comma = true;
		out << pp(g, *it);
	}
	out << "}";
}

double ComputeLiteral::heuristicValue()
{
	FAIL(true);
}

void  ComputeLiteral::addIncParam(Grounder *g, const Value &v)
{
	FAIL(true);
}

ComputeLiteral::~ComputeLiteral()
{
	if(literals_)
	{
		for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
			delete *it;
		delete literals_;
	}
}

