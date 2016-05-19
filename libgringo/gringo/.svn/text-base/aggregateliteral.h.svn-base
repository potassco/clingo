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

#ifndef AGGREGATELITERAL_H
#define AGGREGATELITERAL_H

#include <gringo/gringo.h>
#include <gringo/literal.h>
#include <gringo/expandable.h>

namespace gringo
{
	class AggregateLiteral : public Literal, public Expandable
	{
	public:
		AggregateLiteral(ConditionalLiteralVector *literals);
		AggregateLiteral(const AggregateLiteral &a);
		virtual SDGNode *createNode(SDG *dg, SDGNode *prev, DependencyAdd todo);
		virtual void createNode(LDGBuilder *dg, bool head);
		virtual void createNode(StatementChecker *dg, bool head, bool delayed);
		virtual void getVars(VarSet &vars) const;
		virtual bool checkO(LiteralVector &unsolved);
		void ground(Grounder *g, GroundStep step);
		virtual void reset();
		virtual void finish();
		virtual void preprocess(Grounder *g, Expandable *e, bool head);
		virtual IndexedDomain *createIndexedDomain(Grounder *g, VarSet &index);
		virtual void appendLiteral(Literal *l, ExpansionType type);
		virtual double heuristicValue();
		virtual void setBounds(Term *lower, Term *upper);
		virtual void setEqual(Variable *equal);
		virtual void setEqual(int bound);
		virtual bool match(Grounder *g);
		virtual void doMatch(Grounder *g) = 0;
		virtual bool checkBounds(Grounder *g);
		/**
		 * \brief This function returns if the aggregate is a fact
		 * This function will/may only return true if all the literals of
		 * the aggregate are facts (or if they dont match all). If there is
		 * at least one unsolved literal the function has to return false.
		 */
		virtual bool isFact(Grounder *g);
		virtual bool solved();
		ConditionalLiteralVector *getLiterals() const;
		Term *getLower() const;
		Term *getUpper() const;
		virtual void addIncParam(Grounder *g, const Value &v);
		virtual ~AggregateLiteral();
	protected:
		ConditionalLiteralVector *literals_;
		Term *lower_;
		Term *upper_;
		Variable *equal_;
		bool fact_;
	public:
		int lowerBound_;
		int upperBound_;
		int minLowerBound_;
		int maxUpperBound_;
		int fixedValue_;
	};
}

#endif
