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
#include <gringo/aggrlit.h>

//! Sum or Count Aggregate.
/** Examples:
  * <pre> 1 [ q(X) : p(X) ] 2
  * X \#count{ q(1), q(2), not p }</pre>
  */
class SumAggrLit : public AggrLit
{
public:
	class Printer : public AggrLit::Printer
	{
	public:
		virtual void begin(bool head, bool sign, bool count) = 0;
		virtual void lower(int32_t l) = 0;
		virtual void upper(int32_t u) = 0;
		virtual void end() = 0;
		virtual ~Printer() { }
	};
public:
	/** \param count whether it is a count aggregate (false for sum aggregate).
	  */
	SumAggrLit(const Loc &loc, CondLitVec &conds, bool count);

	bool match(Grounder *grounder);
	boost::tuple<int32_t, int32_t, int32_t> matchAssign(Grounder *grounder);
	void setValue(int32_t value) { lowerBound_ = upperBound_ = value; }
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	void accept(::Printer *v);
	Lit *clone() const;
	void print(Storage *sto, std::ostream &out) const;
	tribool accumulate(Grounder *g, const Val &weight, Lit &lit) throw(const Val*);
private:
	//! whether all evaluated literals are facts
	bool    factOnly_;
	//! sum of evaluated weight literals that are facts
	int32_t fixed_;
	//! lower bound for the sum of evaluated weight literals that are not facts
	int32_t valLower_;
	//! upper bound for the sum of evaluated weight literals that are not facts
	int32_t valUpper_;
	//! whether the matching ends after the sum is over the upper bound
	bool    checkUpperBound_;
	//! value of the upper bound
	int32_t upperBound_;
	//! value of the lower bound
	int32_t lowerBound_;
};

