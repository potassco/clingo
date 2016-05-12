// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <gringo/gringo.h>
#include <gringo/sumaggrlit.h>

//! Min or Max Aggregate.
/** Examples:
  * <pre> Y = \#min [ q(X)=X : p(X) ]
  * 2 \#max[ q(1)=2, q(2)=1, not p(1)=3 ]</pre>
  */
class MinMaxAggrLit : public AggrLit
{
public:
	class Printer : public AggrLit::Printer
	{
	public:
		virtual void begin(bool head, bool sign, bool max) = 0;
		virtual void lower(const Val &l) = 0;
		virtual void upper(const Val &u) = 0;
		virtual void end() = 0;
		virtual ~Printer() { }
	};
public:
	/** \param head whether the literal appears in the head of a rule
	  * \param sign the sign of the literal
	  * \param count whether it is a count aggregate (false for sum aggregate).
	  */
	MinMaxAggrLit(const Loc &loc, CondLitVec &conds, bool count);

	bool match(Grounder *grounder);
	void matchAssign(Grounder *grounder, boost::unordered_set<Val> &vals);
	void setValue(const Val &value) { lowerBound_ = upperBound_ = value; }
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	void accept(::Printer *v);
	Lit *clone() const;
	void print(Storage *sto, std::ostream &out) const;
	tribool accumulate(Grounder *g, const Val &weight, Lit &lit) throw(const Val*);
private:
	//! standard comparison between two weights (inverted for \#min)
	int cmp(const Val& a, const Val& b, Storage *s);
	//! infimum (supremum for \#min)
	Val min();
	//! supremum (infimum for \#min)
	Val max();
	//! whether all evaluated literals are facts
	bool    factOnly_;
	//! smallest weight of evaluated facts (biggest for \#min)
	Val fixed_;
	//! whether the aggregate is a \#min or \#max aggregate
	bool    max_;
	//! biggest weight of all evaluated literals (smallest bound for \#min)
	Val valUpper_;
	//! smallest weight of all evaluated literals (biggest bound for \#min)
	Val valLower_;
	//! upper bound (lower bound for \#min)
	Val upperBound_;
	//! lower bound (upper bound for \#min)
	Val lowerBound_;
	//! values to be assigned
	boost::unordered_set<Val> *vals_;
};

