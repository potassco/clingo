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

#include <gringo/luaterm.h>
#include <gringo/grounder.h>
#include <gringo/func.h>
#include <gringo/lualit.h>
#include <gringo/varterm.h>

LuaTerm::LuaTerm(const Loc &loc, uint32_t name, TermPtrVec &args)
	: Term(loc)
	, name_(name)
	, args_(args)
{
}

void LuaTerm::normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify)
{
	(void)unify;
	for(TermPtrVec::iterator it = args_.begin(); it != args_.end(); it++)
	{
		it->normalize(parent, Term::VecRef(args_, it), g, expander, false);
	}
	uint32_t var = g->createVar();
	expander->expand(new LuaLit(loc(), new VarTerm(loc(), var), args_, name_), Expander::RANGE);
	ref.reset(new VarTerm(loc(), var));
}

void LuaTerm::print(Storage *sto, std::ostream &out) const
{
	out << "@" << sto->string(name_);
	out << "(";
	bool comma = false;
	foreach(const Term &t, args_)
	{
		if(comma) out << ",";
		else comma = true;
		t.print(sto, out);
	}
	out << ")";
}

Term *LuaTerm::clone() const
{
	return new LuaTerm(*this);
}

double LuaTerm::estimate(VarSet const &, double) const {
    return 1;
}

LuaTerm::~LuaTerm()
{
}
