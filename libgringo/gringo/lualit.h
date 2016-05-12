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

class LuaLit : public Lit
{
public:
	LuaLit(const Loc &loc, VarTerm *var, TermPtrVec &args, uint32_t name);
	void normalize(Grounder *g, Expander *expander);
	bool fact() const;
	void accept(Printer *v);
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	void visit(PrgVisitor *visitor);
	bool match(Grounder *grounder);
	void print(Storage *sto, std::ostream &out) const;
	Lit *clone() const;
	const TermPtrVec &args() const;
	uint32_t name() const;
	double score(Grounder *, VarSet &) const { return 0; }
	~LuaLit();
private:
	clone_ptr<VarTerm> var_;
	TermPtrVec         args_;
	uint32_t           name_;
};

inline bool LuaLit::fact() const { return true; }
inline const TermPtrVec &LuaLit::args() const { return args_; }
inline uint32_t LuaLit::name() const { return name_; }
