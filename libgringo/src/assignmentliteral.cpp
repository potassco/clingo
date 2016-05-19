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

#include <gringo/assignmentliteral.h>
#include <gringo/term.h>
#include <gringo/variable.h>
#include <gringo/value.h>
#include <gringo/indexeddomain.h>
#include <gringo/dlvgrounder.h>
#include <gringo/grounder.h>
#include <gringo/literaldependencygraph.h>
#include <gringo/statementchecker.h>

using namespace gringo;

AssignmentLiteral::AssignmentLiteral(Variable *c, Term *t) : Literal(), c_(c), t_(t)
{

}

SDGNode *AssignmentLiteral::createNode(SDG *dg, SDGNode *prev, DependencyAdd todo)
{
	return 0;
}

void AssignmentLiteral::createNode(LDGBuilder *dg, bool head)
{
	assert(!head);
	VarSet needed, provided;
	t_->getVars(needed);
	c_->getVars(provided);
	dg->createNode(this, head, needed, provided);
}

void AssignmentLiteral::createNode(StatementChecker *dg, bool head, bool delayed)
{
	assert(!head && !delayed);
	VarSet needed, provided;
	c_->getVars(provided);
	t_->getVars(needed);
	dg->createNode(needed, provided);
}

void AssignmentLiteral::print(const GlobalStorage *g, std::ostream &out) const
{
	out << pp(g, c_) << " = " << pp(g, t_);
}

bool AssignmentLiteral::isFact(Grounder *g)
{
	return true;
}

bool AssignmentLiteral::solved()
{
	return true;
}

void AssignmentLiteral::getVars(VarSet &vars) const
{
	c_->getVars(vars);
	t_->getVars(vars);
}

bool AssignmentLiteral::checkO(LiteralVector &unsolved)
{
	return true;
}

void AssignmentLiteral::reset()
{
	FAIL(true);
}

void AssignmentLiteral::finish()
{
	FAIL(true);
}

bool AssignmentLiteral::match(Grounder *g)
{
	return c_->getValue(g).equal(t_->getValue(g));
}

void AssignmentLiteral::preprocess(Grounder *g, Expandable *e, bool head)
{
	t_->preprocess(this, t_, g, e);
}

namespace
{
	class IndexedDomainAssign : public IndexedDomain
	{
	public:
		IndexedDomainAssign(int var, Term *t);
		virtual void firstMatch(int binder, DLVGrounder *g, MatchStatus &status);
		virtual void nextMatch(int binder, DLVGrounder *g, MatchStatus &status);
		virtual ~IndexedDomainAssign();
	protected:
		int var_;
		Term *t_;
	};

	IndexedDomainAssign::IndexedDomainAssign(int var, Term *t) : var_(var), t_(t)
	{
	}
	
	void IndexedDomainAssign::firstMatch(int binder, DLVGrounder *g, MatchStatus &status)
	{
		g->g_->setValue(var_, t_->getValue(g->g_), binder);
		status = SuccessfulMatch;
	}

	void IndexedDomainAssign::nextMatch(int binder, DLVGrounder *g, MatchStatus &status)
	{
		status = FailureOnNextMatch;
	}

	IndexedDomainAssign::~IndexedDomainAssign()
	{
	}
}

IndexedDomain *AssignmentLiteral::createIndexedDomain(Grounder *g, VarSet &index)
{
	if(index.find(c_->getUID()) == index.end())
		return new IndexedDomainAssign(c_->getUID(), t_);
	else
		return new IndexedDomainMatchOnly(this);
}

AssignmentLiteral::AssignmentLiteral(const AssignmentLiteral &r) : Literal(r), c_(static_cast<Variable*>(r.c_->clone())), t_(r.t_->clone())
{
}

double AssignmentLiteral::heuristicValue()
{
	// match it as soon as possible
	return 0;
}

Literal* AssignmentLiteral::clone() const
{
	return new AssignmentLiteral(*this);
}

NS_OUTPUT::Object *AssignmentLiteral::convert()
{
	FAIL(true);
}

void AssignmentLiteral::addIncParam(Grounder *g, const Value &v)
{
	// c_ doesnt need to be changed
	t_->addIncParam(g, t_, v);
}

AssignmentLiteral::~AssignmentLiteral()
{
	if(c_)
		delete c_;
	if(t_)
		delete t_;
}

