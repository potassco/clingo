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

#ifndef PREDICATELITERAL_H
#define PREDICATELITERAL_H

#include <gringo/gringo.h>
#include <gringo/literal.h>
#include <gringo/groundable.h>
#include <gringo/expandable.h>
#include <gringo/value.h>

namespace gringo
{
	class PredicateLiteral : public Literal
	{
	public:
		PredicateLiteral(Grounder *g, int id, TermVector *variables);
		PredicateLiteral(const PredicateLiteral &p);
		virtual Literal* clone() const;
		void setWeight(Term *w);
		virtual SDGNode *createNode(SDG *dg, SDGNode *prev, DependencyAdd todo);
		virtual void createNode(LDGBuilder *dg, bool head);
		virtual void createNode(StatementChecker *dg, bool head, bool delayed);
		virtual void print(const GlobalStorage *g, std::ostream &out) const;
		virtual void getVars(VarSet &vars) const;
		virtual bool checkO(LiteralVector &unsolved);
		virtual void reset();
		virtual bool solved();
		virtual bool isFact(Grounder *g);
		bool isFact(const ValueVector &values);
		virtual void finish();
		virtual void evaluate();
		virtual IndexedDomain *createIndexedDomain(Grounder *g, VarSet &index);
		virtual bool match(Grounder *g);
		bool match(const ValueVector &values);
		virtual void preprocess(Grounder *g, Expandable *e, bool head);
		virtual NS_OUTPUT::Object *convert();
		virtual double heuristicValue();
		NS_OUTPUT::Object *convert(const ValueVector &values);
		int getId();
		TermVector *getArgs();
		int getUid();
		int getArity() const;
		void addDomain(ValueVector &values);
		Domain *getDomain() const;
		const ValueVector &getValues();
		virtual void binderSplit(Expandable *e, const VarSet &relevant);
		virtual void addIncParam(Grounder *g, const Value &v);
		virtual ~PredicateLiteral();
	protected:
		int           uid_;
		int           aid_;
		Domain        *predNode_;
		int           id_;
		TermVector    *variables_;

		ValueVector values_;
	};
}

#endif

