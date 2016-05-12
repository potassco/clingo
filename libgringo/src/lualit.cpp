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

#include <gringo/lualit.h>
#include <gringo/grounder.h>
#include <gringo/varterm.h>
#include <gringo/index.h>
#include <gringo/groundable.h>
#include <gringo/instantiator.h>
#include <gringo/prgvisitor.h>

LuaLit::LuaLit(const Loc &loc, VarTerm *var, TermPtrVec &args, uint32_t name)
	: Lit(loc)
	, var_(var)
	, args_(args.release())
	, name_(name)
{
}

bool LuaLit::match(Grounder *grounder)
{
	ValVec vals;
	ValVec args;
	foreach(Term &term, args_) { args.push_back(term.val(grounder)); }
	grounder->luaCall(loc(), grounder->string(name_).c_str(), args, vals);
	return std::find(vals.begin(), vals.end(), var_->val(grounder)) != vals.end();
}

namespace
{
	class LuaIndex : public Index
	{
	public:
		LuaIndex(uint32_t var, LuaLit *lit, const VarVec &bind);
		std::pair<bool,bool> firstMatch(Grounder *grounder, int binder);
		std::pair<bool,bool> nextMatch(Grounder *grounder, int binder);
		void reset() { finished_ = false; }
		void finish() { finished_ = true; }
		bool hasNew() const { return !finished_; }
	private:
		bool             finished_;
		uint32_t         var_;
		ValVec           vals_;
		ValVec::iterator current_;
		LuaLit          *lit_;
		VarVec           bind_;
	};

	LuaIndex::LuaIndex(uint32_t var, LuaLit *lit, const VarVec &bind)
		: finished_(false)
		, var_(var)
		, lit_(lit)
		, bind_(bind)
	{
	}

	std::pair<bool,bool> LuaIndex::firstMatch(Grounder *grounder, int binder)
	{
		ValVec args;
		vals_.clear();
		foreach(const Term &term, lit_->args()) { args.push_back(term.val(grounder)); }
        grounder->luaCall(lit_->loc(), grounder->string(lit_->name()).c_str(), args, vals_);
		current_ = vals_.begin();
		return nextMatch(grounder, binder);
	}

	std::pair<bool,bool> LuaIndex::nextMatch(Grounder *grounder, int binder)
	{
		if(current_ != vals_.end())
		{
			grounder->val(var_, *current_++, binder);
			return std::make_pair(true, !finished_);
		}
		else return std::make_pair(false, false);
	}
}

void LuaLit::index(Grounder *g, Groundable *gr, VarSet &bound)
{
	(void)g;
	VarSet vars;
	VarVec bind;
	var_->vars(vars);
	std::set_difference(vars.begin(), vars.end(), bound.begin(), bound.end(), std::back_insert_iterator<VarVec>(bind));
	if(bind.size() > 0)
	{
		LuaIndex *p = new LuaIndex(var_->index(), this, bind);
		gr->instantiator()->append(p);
		bound.insert(bind.begin(), bind.end());
	}
	else gr->instantiator()->append(new MatchIndex(this));
}

void LuaLit::accept(Printer *v)
{
	(void)v;
}

void LuaLit::visit(PrgVisitor *v)
{
	v->visit(var_.get(), true);
	foreach(Term &term, args_) { term.visit(v, false); }
}

void LuaLit::print(Storage *sto, std::ostream &out) const
{
	out << "#call(";
	var_->print(sto, out);
	out << ",@" << sto->string(name_) << "(";
	bool comma = false;
	foreach(const Term &term, args_)
	{
		if(comma) { out << ","; }
		else      { comma = true; }
		term.print(sto, out);
	}
	out << "))";
}

void LuaLit::normalize(Grounder *g, Expander *expander)
{
	for(TermPtrVec::iterator it = args_.begin(); it != args_.end(); it++)
	{
		it->normalize(this, Term::VecRef(args_, it), g, expander, false);
	}
}

Lit *LuaLit::clone() const
{
	return new LuaLit(*this);
}

LuaLit::~LuaLit()
{
}

