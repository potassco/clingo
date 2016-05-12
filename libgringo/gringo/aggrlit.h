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
#include <gringo/lit.h>
#include <gringo/predlit.h>
#include <gringo/printer.h>
#include <gringo/groundable.h>

class AggrLit : public Lit
{
public:
	class Printer : public ::Printer
	{
	public:
		virtual void weight(const Val &v) = 0;
		virtual ~Printer() { }
	};
public:
	/** \param set whether the aggregate's list is a set (or else multiset)
		\param optComplete whether handling of complete predicates can be optimized
	  */
	AggrLit(const Loc &loc, CondLitVec &conds, bool set, bool optComplete);
	AggrLit(const AggrLit &aggr);

	//! sets the lower bound.
	void lower(Term *l);

	//! sets the upper bound.
	void upper(Term *u);

	void assign(Term *a);
	Term *assign() const { assert(assign_); return lower_.get(); }
	void add(CondLit *cond);
	const CondLitVec &conds() { return conds_; }
	void sign(bool s) { sign_ = s; }
	bool fact() const { return fact_; }
	virtual Monotonicity monotonicity() { return NONMONOTONE; }
	virtual void normalize(Grounder *g, Expander *expander);
	virtual tribool accumulate(Grounder *g, const Val &weight, Lit &lit) throw(const Val*) = 0;
	void addDomain(Grounder *g, bool fact);
	void finish(Grounder *g);
	void visit(PrgVisitor *visitor);
	void grounded(Grounder *grounder) { (void)grounder; }
	bool complete() const { assert(false); return false; }
	void init(Grounder *g, const VarSet &bound);
	void push() { assert(false); }
	void pop() { assert(false); }
	void move(size_t p) { (void)p; assert(false); }
	void clear() { assert(false); }
	double score(Grounder *, VarSet &) const { return std::numeric_limits<double>::max(); }

	//! whether the aggregate's list is a set (or else multiset)
	bool set() const { return set_; }

	//! whether complete predicates will be handled differently
	/** Example: If p/1 is complete, then p(X) : q(X) will be interpreted as p(X) : p(X) : q(X)
	  */
	bool optimizeComplete() const { return optComplete_; }

	//! returns the set of already evaluated literals
	PredLitSet &uniqueSet();
	~AggrLit();
protected:
	bool            sign_;
	bool            assign_;
	bool            fact_;
	//! lower bound
	clone_ptr<Term> lower_;
	//! upper bound
	clone_ptr<Term> upper_;
	//! literals in the aggregate's (multi)set
	CondLitVec      conds_;

	//! whether the aggregate represents a set (or else multiset)
	bool            set_;

	//! set for uniqueness check when set_ is true
	PredLitSet      uniques_;

	//! whether complete predicates will be handled differently
	bool            optComplete_;
};

class WeightLit : public Lit
{
public:
	WeightLit(const Loc &loc, Term *weight);
	bool fact() const { return true; }
	bool match(Grounder *grounder) { (void)grounder; return true; }
	void addDomain(Grounder *grounder, bool fact) { (void)grounder; (void)fact; assert(false); }
	void index(Grounder *grounder, Groundable *gr, VarSet &bound) { (void)grounder; (void)gr; (void)bound; }
	bool edbFact() const { return false; }
	void normalize(Grounder *grounder, Expander *expander) { (void)grounder; (void)expander; assert(false); }
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	void accept(Printer *v) { (void)v; }
	clone_ptr<Term> &weight() { return weight_; }
	Lit *clone() const;
	~WeightLit();
private:
	clone_ptr<Term> weight_;
};

namespace boost
{
	template <>
	inline WeightLit* new_clone(const WeightLit& a)
	{
		return static_cast<WeightLit*>(a.clone());
	}
}

class CondLit : public Groundable, public Locateable
{
	friend class AggrLit;
public:
	CondLit(const Loc &loc, Lit *head, Term *weight, LitPtrVec &body);
	Term *weight() { return weight_->weight().get(); }
	Lit *head() const { return head_.get(); }
	LitPtrVec &body() { return body_; }
	void add(Lit *lit) { body_.push_back(lit); }
	bool grounded(Grounder *g);
	void normalize(Grounder *g, Expander *headExp, Expander *bodyExp);
	void init(Grounder *g, const VarSet &bound);
	void ground(Grounder *g);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	void accept(AggrLit::Printer *v);
	void addDomain(Grounder *g, bool fact) ;
	void finish(Grounder *g);
	virtual ~CondLit();
private:
	clone_ptr<Lit>       head_;
	clone_ptr<WeightLit> weight_;
	LitPtrVec            body_;
	ValVec               weights_;
	AggrLit             *aggr_;
};

