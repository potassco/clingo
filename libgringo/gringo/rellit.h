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

class RelLit : public Lit
{
public:
	enum Type { ASSIGN, GREATER, LOWER, EQUAL, GTHAN, LTHAN, INEQUAL };
public:
	RelLit(const Loc &loc, Type t, Term *a, Term *b);
	void normalize(Grounder *g, Expander *expander);
	bool fact() const { return true; }
	bool isFalse(Grounder *g);
	void accept(Printer *v);
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	void visit(PrgVisitor *visitor);
	bool match(Grounder *grounder);
	void print(Storage *sto, std::ostream &out) const;
	Lit *clone() const;
	~RelLit();
private:
	Type            t_;
	clone_ptr<Term> a_;
	clone_ptr<Term> b_;
};

