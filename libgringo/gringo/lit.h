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
#include <gringo/locateable.h>

class Expander
{
public:
	enum Type { POOL, RANGE, RELATION };
public:
	virtual void expand(Lit *lit, Type type) = 0;
	virtual ~Expander() { }
};

class Lit : public Locateable
{
public:
	enum Monotonicity { MONOTONE, ANTIMONOTONE, NONMONOTONE };
public:
	Lit(const Loc &loc) : Locateable(loc), head_(false) { }
	virtual void normalize(Grounder *g, Expander *expander) = 0;
	virtual Monotonicity monotonicity() { return MONOTONE; }
	virtual bool fact() const = 0;
	virtual bool forcePrint() { return false; }
	virtual bool match(Grounder *grounder) = 0;
	virtual void addDomain(Grounder *grounder, bool fact) { (void)grounder; (void)fact; assert(false); }
	virtual void finish(Grounder *grounder) { (void)grounder; }
	virtual void grounded(Grounder *grounder) { (void)grounder; }
	virtual bool complete() const { return true; }

	/** whether the literal is the head of a rule. */
	virtual bool head() const { return head_; }

	virtual void head(bool head)  { head_ = head; }
	virtual void index(Grounder *g, Groundable *gr, VarSet &bound) = 0;
	virtual void visit(PrgVisitor *visitor) = 0;
	virtual bool edbFact() const { return false; }
	virtual void print(Storage *sto, std::ostream &out) const = 0;
	virtual void accept(Printer *v) = 0;
	virtual void init(Grounder *grounder, const VarSet &bound) { (void)grounder; (void)bound; }
	virtual Lit *clone() const = 0;
	virtual void push() { }

	/** whether the literal is in the set. Used in aggregates and optimize statements.
	 * \param val additional information (e.g. used to distinguish literals with priorities in optimize statements)
	 */
	virtual bool testUnique(PredLitSet&, Val=Val::create()) { return true; }

	virtual void pop() { }
	virtual void move(size_t p) { (void)p; }
	virtual void clear() { }
	virtual double score(Grounder *, VarSet &) const { return std::numeric_limits<double>::min(); }
	virtual bool isFalse(Grounder *grounder) { (void)grounder; assert(false); return false; }
	virtual ~Lit() { }
private:
	bool head_;
};

namespace boost
{
	template <>
	inline Lit* new_clone(const Lit& a)
	{
		return a.clone();
	}
}
