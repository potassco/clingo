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

#ifndef COMPUTELITERAL_H
#define COMPUTELITERAL_H

#include <gringo/gringo.h>
#include <gringo/literal.h>
#include <gringo/expandable.h>

namespace gringo
{
	class ComputeLiteral : public Literal, public Expandable
	{
	public:
		ComputeLiteral(ConditionalLiteralVector *literals, int number);
		ComputeLiteral(const ComputeLiteral &a);
		virtual SDGNode *createNode(SDG *dg, SDGNode *prev, DependencyAdd todo);
		virtual void createNode(LDGBuilder *dg, bool head);
		virtual void createNode(StatementChecker *dg, bool head, bool delayed);
		virtual void getVars(VarSet &vars) const;
		virtual bool checkO(LiteralVector &unsolved);
		void ground(Grounder *g, GroundStep step);
		virtual bool match(Grounder *g);
		virtual void reset();
		virtual void finish();
		virtual void preprocess(Grounder *g, Expandable *e, bool head);
		virtual IndexedDomain *createIndexedDomain(Grounder *g, VarSet &index);
		virtual void appendLiteral(Literal *l, ExpansionType type);
		virtual double heuristicValue();
		virtual bool isFact(Grounder *g);
		virtual bool solved();
		virtual Literal *clone() const;
		virtual NS_OUTPUT::Object *convert();
		virtual void print(const GlobalStorage *g, std::ostream &out) const;
		virtual void addIncParam(Grounder *g, const Value &v);
		virtual ~ComputeLiteral();
	protected:
		int number_;
		ConditionalLiteralVector *literals_;
	};
}

#endif

