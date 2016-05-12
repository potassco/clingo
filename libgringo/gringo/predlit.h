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
#include <gringo/predlitrep.h>

class PredLitSet
{
private:
	typedef std::pair<PredLit*, size_t> PredSig;
	struct PredCmp
	{
		PredCmp(const ValVec &vals);
		size_t operator()(const PredSig &a) const;
		bool operator()(const PredSig &a, const PredSig &b) const;
		const ValVec &vals_;
	};
	typedef boost::unordered_set<PredSig, PredCmp, PredCmp> PredSet;
public:
	PredLitSet();
	PredLitSet(PredLitSet const &other);
	bool insert(PredLit *pred, size_t pos, Val &val);
	void clear();
private:
	PredSet set_;
	ValVec  vals_;
};

class PredLit : public Lit, public PredLitRep
{
public:
	PredLit(const Loc &loc, Domain *dom, TermPtrVec &terms);
	void normalize(Grounder *g, Expander *expander);
	bool fact() const;
	bool isFalse(Grounder *g);
	Monotonicity monotonicity();
	void addDomain(Grounder *g, bool fact) { return PredLitRep::addDomain(g, fact); }
	void accept(Printer *v);
	void finish(Grounder *g);
	/** \note grounded has to be called before reading top_ or vals_. */
	void grounded(Grounder *grounder);
	bool match(Grounder *grounder);
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	void visit(PrgVisitor *visitor);
	bool edbFact() const;
	bool complete() const;
	void print(Storage *sto, std::ostream &out) const;
	void push();
	bool testUnique(PredLitSet &set, Val val=Val::create());
	void pop();
	void move(size_t p);
	void clear();
	double score(Grounder *g, VarSet &) const;
	Lit *clone() const;
private:
	void vars(VarSet &vars) const;
private:
	TermPtrVec terms_;
};

