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

#ifndef CONDITIONALLITERAL_H
#define CONDITIONALLITERAL_H

#include <gringo/gringo.h>
#include <gringo/literal.h>
#include <gringo/groundable.h>
#include <gringo/expandable.h>

namespace gringo
{
	class ConditionalLiteral : public Literal, public Groundable, public Expandable
	{

	public:
		ConditionalLiteral(PredicateLiteral *pred, LiteralVector *conditionals);
		ConditionalLiteral(const ConditionalLiteral &p);
		virtual Literal* clone() const;
		void setWeight(Term *w);
		virtual SDGNode *createNode(SDG *dg, SDGNode *prev, Literal::DependencyAdd todo);
		virtual void createNode(LDGBuilder *dg, bool head);
		virtual void createNode(StatementChecker *dg, bool head, bool delayed);
		virtual void print(const GlobalStorage *g, std::ostream &out) const;
		virtual void getVars(VarSet &vars) const;
		virtual bool checkO(LiteralVector &unsolved);
		virtual void reset();
		virtual bool solved();
		virtual void finish();
		virtual IndexedDomain *createIndexedDomain(Grounder *g, VarSet &index);
		virtual bool match(Grounder *g);
		virtual bool isFact(Grounder *g);
		void ground(Grounder *g, GroundStep step);
		virtual void grounded(Grounder *g);
		virtual void appendLiteral(Literal *l, ExpansionType type);
		virtual void preprocess(Grounder *g, Expandable *e, bool head);
		virtual double heuristicValue();
		virtual void addIncParam(Grounder *g, const Value &v);
		void setNeg(bool neg);

		bool hasConditionals();
		PredicateLiteral *toPredicateLiteral();
		int getUid();
		
		// functions to access all possible bindings of the conditional literal
		virtual NS_OUTPUT::Object *convert();
		bool isFact();
		bool isEmpty();
		void start();
		bool hasWeight();
		bool hasNext();
		void remove();
		bool match();
		int count();
		void next();
		const ValueVector &getValues();
		int getWeight();

		void clonePredicate(PredicateLiteral *clone);
		void preprocessDisjunction(Grounder *g, AggregateLiteral *a, Expandable *e, bool head);
		virtual ~ConditionalLiteral();
	protected:
		PredicateLiteral *pred_;
		LiteralVector    *conditionals_;
		Term             *weight_;
		DLVGrounder      *grounder_;
		LDG              *dg_;

		std::vector<ValueVector> values_;
		std::vector<int>         weights_;
		size_t                   current_;
		PredicateLiteral        *clone_;
	};
}

#endif

