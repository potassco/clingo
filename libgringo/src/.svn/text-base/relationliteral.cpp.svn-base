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

#include <gringo/relationliteral.h>
#include <gringo/term.h>
#include <gringo/value.h>
#include <gringo/indexeddomain.h>
#include <gringo/literaldependencygraph.h>
#include <gringo/statementchecker.h>
#include <gringo/grounder.h>

using namespace gringo;

RelationLiteral::RelationLiteral(RelationType type, Term *a, Term *b) : Literal(), type_(type), a_(a), b_(b)
{

}

SDGNode *RelationLiteral::createNode(SDG *dg, SDGNode *prev, DependencyAdd todo)
{
	return 0;
}

void RelationLiteral::createNode(LDGBuilder *dg, bool head)
{
	assert(!head);
	VarSet needed, provided;
	a_->getVars(needed);
	if(b_)
		b_->getVars(needed);
	dg->createNode(this, head, needed, provided);
}

void RelationLiteral::createNode(StatementChecker *dg, bool head, bool delayed)
{
	assert(!head && !delayed);
	VarSet needed, provided;
	a_->getVars(needed);
	b_->getVars(needed);
	dg->createNode(needed, provided);
}

void RelationLiteral::print(const GlobalStorage *g, std::ostream &out) const
{
	out << pp(g, a_);
	switch(type_)
	{ 
		case EQ:
			out << " == ";
			break;
		case NE:
			out << " != ";
			break;
		case GT:
			out << " > ";
			break;
		case GE:
			out << " >= ";
			break;
		case LE:
			out << " <= ";
			break;
		case LT:
			out << " < ";
			break;
	}
	out << pp(g, b_);
}

bool RelationLiteral::isFact(Grounder *g)
{
	return true;
}

bool RelationLiteral::solved()
{
	return true;
}

void RelationLiteral::getVars(VarSet &vars) const
{
	a_->getVars(vars);
	b_->getVars(vars);
}

bool RelationLiteral::checkO(LiteralVector &unsolved)
{
	return true;
}

void RelationLiteral::reset()
{
	FAIL(true);
}

void RelationLiteral::finish()
{
	FAIL(true);
}

bool RelationLiteral::match(Grounder *g)
{
	switch(type_)
	{ 
		case EQ:
			return a_->getValue(g).equal(b_->getValue(g));
		case NE:
			return !a_->getValue(g).equal(b_->getValue(g));
		case GT:
			return a_->getValue(g).compare(g, b_->getValue(g)) > 0;
		case GE:
			return a_->getValue(g).compare(g, b_->getValue(g)) >= 0;
		case LE:
			return a_->getValue(g).compare(g, b_->getValue(g)) <= 0;
		case LT:
			return a_->getValue(g).compare(g, b_->getValue(g)) < 0;
	}
	FAIL(true);
}

void RelationLiteral::preprocess(Grounder *g, Expandable *e, bool head)
{
	a_->preprocess(this, a_, g, e);
	b_->preprocess(this, b_, g, e);
}

void RelationLiteral::addIncParam(Grounder *g, const Value &v)
{
	a_->addIncParam(g, a_, v);
	b_->addIncParam(g, b_, v);
}

double RelationLiteral::heuristicValue()
{
	// match it as soon as possible
	return 0;
}

IndexedDomain *RelationLiteral::createIndexedDomain(Grounder *g, VarSet &index)
{
	return new IndexedDomainMatchOnly(this);
}

RelationLiteral::RelationLiteral(const RelationLiteral &r) : Literal(r), type_(r.type_), a_(r.a_->clone()), b_(r.b_->clone())
{
}

Literal* RelationLiteral::clone() const
{
	return new RelationLiteral(*this);
}

NS_OUTPUT::Object *RelationLiteral::convert()
{
	FAIL(true);
}

RelationLiteral::~RelationLiteral()
{
	if(a_)
		delete a_;
	if(b_)
		delete b_;
}

