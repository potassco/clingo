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

#ifndef RELATIONLITERAL_H
#define RELATIONLITERAL_H

#include <gringo/gringo.h>
#include <gringo/literal.h>

namespace gringo
{
	class RelationLiteral : public Literal
	{
	public:
		enum RelationType { EQ, NE, GT, GE, LE, LT };
	public:
		RelationLiteral(RelationType type, Term *a, Term *b);
		RelationLiteral(const RelationLiteral &r);
		virtual SDGNode *createNode(SDG *dg, SDGNode *prev, DependencyAdd todo);
		virtual void createNode(LDGBuilder *dg, bool head);
		virtual void createNode(StatementChecker *dg, bool head, bool delayed);
		virtual void getVars(VarSet &vars) const;
		virtual bool checkO(LiteralVector &unsolved);
		virtual void reset();
		virtual void finish();
		virtual void preprocess(Grounder *g, Expandable *e, bool head);
		virtual bool solved();
		virtual bool isFact(Grounder *g);
		virtual Literal* clone() const;
		virtual IndexedDomain *createIndexedDomain(Grounder *g, VarSet &index);
		virtual bool match(Grounder *g);
		virtual void print(const GlobalStorage *g, std::ostream &out) const;
		virtual NS_OUTPUT::Object *convert();
		virtual double heuristicValue();
		virtual void addIncParam(Grounder *g, const Value &v);
		virtual ~RelationLiteral();
	protected:
		RelationType type_;
		Term *a_;
		Term *b_;
	};
}

#endif

