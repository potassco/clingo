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

//! Junction Aggregate.
/** Examples:
  * <pre> q(X) : p(X)
  * X < 3 : p(X)
  * a | b : d | c</pre>
  */
class JunctionAggrLit : public AggrLit
{
public:
	class Printer : public AggrLit::Printer
	{
	public:
		virtual void begin(bool head) = 0;
		virtual void end() = 0;
		virtual ~Printer() { }
	};
public:
	JunctionAggrLit(const Loc &loc, CondLitVec &conds);

	bool match(Grounder *grounder);
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	void accept(::Printer *v);
	Lit *clone() const;
	void print(Storage *sto, std::ostream &out) const;
	tribool accumulate(Grounder *g, const Val &weight, Lit &lit) throw(const Val*);
	virtual void normalize(Grounder *g, Expander *expander);
private:
	//! whether all evaluated literals are facts
	bool    factOnly_;
	bool    hasFact_;
	//! whether one of the evaluated facts is false
	bool    hasFalse_;
};

