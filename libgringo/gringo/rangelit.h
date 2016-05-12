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

class RangeLit : public Lit
{
public:
	RangeLit(const Loc &loc, VarTerm *var, Term *a, Term *b);
	void normalize(Grounder *g, Expander *expander);
	bool fact() const { return true; }
	void accept(Printer *v);
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	void visit(PrgVisitor *visitor);
	bool match(Grounder *grounder);
	void print(Storage *sto, std::ostream &out) const;
	double score(Grounder *g, VarSet &) const;
	Lit *clone() const;
	~RangeLit();
private:
	clone_ptr<VarTerm> var_;
	clone_ptr<Term>    a_;
	clone_ptr<Term>    b_;
};

