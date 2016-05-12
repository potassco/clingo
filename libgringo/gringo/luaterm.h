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

class LuaTerm : public Term
{
public:
	LuaTerm(const Loc &loc, uint32_t name, TermPtrVec &args);
	void normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify);
	void print(Storage *sto, std::ostream &out) const;
	Term *clone() const;
	uint32_t name() const;
	Val val(Grounder *grounder) const;
	bool constant() const;
	bool unify(Grounder *grounder, const Val &v, int binder) const;
	void vars(VarSet &v) const;
	void visit(PrgVisitor *visitor, bool bind);
	double estimate(VarSet const &bound, double size) const;
	~LuaTerm();
private:
	uint32_t                name_;
	TermPtrVec              args_;
};

inline uint32_t LuaTerm::name() const { return name_; }

inline Val LuaTerm::val(Grounder *) const                      { assert(false); return Val::create(); }
inline bool LuaTerm::constant() const                          { assert(false); return false; }
inline bool LuaTerm::unify(Grounder *, const Val &, int) const { assert(false); return false; }
inline void LuaTerm::vars(VarSet &) const                      { assert(false); }
inline void LuaTerm::visit(PrgVisitor *, bool)                 { assert(false); }
