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
#include <gringo/term.h>

class ArgTerm : public Term
{
public:
	ArgTerm(const Loc &loc, Term *term, TermPtrVec &terms);
	Split split();
	void normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify) { (void)parent; (void)ref; (void)g; (void)expander; (void)unify; }
	Val val(Grounder *grounder) const { (void)grounder; assert(false); return Val::create(); }
	bool constant() const { assert(false); return false; }
	bool unify(Grounder *grounder, const Val &v, int binder) const { (void)grounder; (void)v; (void)binder; assert(false); return false; }
	void vars(VarSet &v) const { (void)v; assert(false); }
	void visit(PrgVisitor *visitor, bool bind) { (void)visitor; (void)bind; assert(false); }
	void print(Storage *sto, std::ostream &out) const;
	double estimate(VarSet const &bound, double size) const;
	Term *clone() const;
	~ArgTerm();
private:
	clone_ptr<Term> term_;
	TermPtrVec      terms_;
};

