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

#include <gringo/rellit.h>
#include <gringo/term.h>
#include <gringo/index.h>
#include <gringo/grounder.h>
#include <gringo/groundable.h>
#include <gringo/instantiator.h>
#include <gringo/litdep.h>
#include <gringo/varcollector.h>

RelLit::RelLit(const Loc &loc, Type t, Term *a, Term *b)
	: Lit(loc)
	, t_(t)
	, a_(a)
	, b_(b)
{
}

bool RelLit::match(Grounder *grounder)
{
	if(head()) return true;
	return !isFalse(grounder);
}

bool RelLit::isFalse(Grounder *g)
{
	switch(t_)
	{
		case RelLit::GREATER: return a_->val(g).compare(b_->val(g), g) <= 0;
		case RelLit::LOWER:   return a_->val(g).compare(b_->val(g), g) >= 0;
		case RelLit::EQUAL:   return a_->val(g) != b_->val(g);
		case RelLit::GTHAN:   return a_->val(g).compare(b_->val(g), g) < 0;
		case RelLit::LTHAN:   return a_->val(g).compare(b_->val(g), g) > 0;
		case RelLit::INEQUAL: return a_->val(g) == b_->val(g);
		case RelLit::ASSIGN:  return a_->val(g).compare(b_->val(g), g) != 0;
	}
	assert(false);
	return false;
}

namespace
{
	class AssignIndex : public Index
	{
	public:
		AssignIndex(Term *a, Term *b, const VarVec &bind);
		std::pair<bool,bool> firstMatch(Grounder *grounder, int binder);
		std::pair<bool,bool> nextMatch(Grounder *grounder, int binder);
		void reset() { finished_ = false; }
		void finish() { finished_ = true; }
		bool hasNew() const { return !finished_; }
	private:
		bool   finished_;
		Term  *a_;
		Term  *b_;
		VarVec bind_;
	};

	AssignIndex::AssignIndex(Term *a, Term *b, const VarVec &bind)
		: finished_(false)
		, a_(a)
		, b_(b)
		, bind_(bind)
	{
	}

	std::pair<bool,bool> AssignIndex::firstMatch(Grounder *grounder, int binder)
	{
		foreach(uint32_t var, bind_) grounder->unbind(var);
		return std::make_pair(a_->unify(grounder, b_->val(grounder), binder), !finished_);
	}

	std::pair<bool,bool> AssignIndex::nextMatch(Grounder *grounder, int binder)
	{
		(void)grounder;
		(void)binder;
		return std::make_pair(false, false);
	}

}

void RelLit::index(Grounder *g, Groundable *gr, VarSet &bound)
{
	(void)g;
	if(t_ == ASSIGN)
	{
		VarSet vars;
		VarVec bind;
		a_->vars(vars);
		std::set_difference(vars.begin(), vars.end(), bound.begin(), bound.end(), std::back_insert_iterator<VarVec>(bind));
		if(bind.size() > 0)
		{
			AssignIndex *p = new AssignIndex(a_.get(), b_.get(), bind);
			gr->instantiator()->append(p);
			bound.insert(bind.begin(), bind.end());
			return;
		}
	}
	gr->instantiator()->append(new MatchIndex(this));
}

void RelLit::accept(Printer *v)
{
	(void)v;
}

void RelLit::visit(PrgVisitor *v)
{
	if(a_.get()) v->visit(a_.get(), t_ == ASSIGN);
	if(b_.get()) v->visit(b_.get(), false);
}

void RelLit::print(Storage *sto, std::ostream &out) const
{
	a_->print(sto, out);
	switch(t_)
	{
		case RelLit::GREATER: out << ">"; break;
		case RelLit::LOWER:   out << "<"; break;
		case RelLit::EQUAL:   out << "=="; break;
		case RelLit::GTHAN:   out << ">="; break;
		case RelLit::LTHAN:   out << "=<"; break;
		case RelLit::INEQUAL: out << "!="; break;
		case RelLit::ASSIGN:  out << ":="; break;
	}
	b_->print(sto, out);
}

void RelLit::normalize(Grounder *g, Expander *expander)
{
	if(a_.get()) a_->normalize(this, Term::PtrRef(a_), g, expander, t_ == ASSIGN);
	if(b_.get()) b_->normalize(this, Term::PtrRef(b_), g, expander, false);
}

Lit *RelLit::clone() const
{
	return new RelLit(*this);
}

RelLit::~RelLit()
{
}

