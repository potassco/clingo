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

class Term : public Locateable
{
public:
	typedef std::pair<Term*, TermPtrVec*> Split;
	class Ref
	{
	public:
		virtual void reset(Term *a) const = 0;
		virtual void replace(Term *a) const = 0;
		virtual ~Ref() { }
	};
	class VecRef : public Ref
	{
	public:
		VecRef(TermPtrVec &vec, TermPtrVec::iterator pos) : vec_(vec), pos_(pos) { }
		void reset(Term *a) const { vec_.replace(pos_, a); }
		void replace(Term *a) const { vec_.replace(pos_, a).release(); }
	private:
		TermPtrVec          &vec_;
		TermPtrVec::iterator pos_;
	};
	class PtrRef : public Ref
	{
	public:
		PtrRef(clone_ptr<Term> &ptr) : ptr_(ptr) { }
		void reset(Term *a) const { ptr_.reset(a); }
		void replace(Term *a) const { ptr_.release(); ptr_.reset(a); }
	private:
		clone_ptr<Term> &ptr_;
	};
public:
	Term(const Loc &loc) : Locateable(loc) { }
	virtual Val val(Grounder *grounder) const = 0;
	virtual Split split() { return Split(0, 0); }
	virtual void normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify) = 0;
	virtual bool unify(Grounder *grounder, const Val &v, int binder) const = 0;
	virtual void vars(VarSet &v) const = 0;
	virtual void visit(PrgVisitor *visitor, bool bind) = 0;
	virtual bool constant() const = 0;
	virtual void print(Storage *sto, std::ostream &out) const = 0;
	virtual Term *clone() const = 0;
	virtual double estimate(VarSet const &bound, double size) const = 0;
	virtual ~Term() { }
};

namespace boost
{
	template <>
	inline Term* new_clone(const Term& a)
	{
		return a.clone();
	}
}
