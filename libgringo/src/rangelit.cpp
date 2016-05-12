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

#include <gringo/rangelit.h>
#include <gringo/varterm.h>
#include <gringo/index.h>
#include <gringo/grounder.h>
#include <gringo/groundable.h>
#include <gringo/instantiator.h>
#include <gringo/litdep.h>
#include <gringo/varcollector.h>
#include <gringo/exceptions.h>

RangeLit::RangeLit(const Loc &loc, VarTerm *var, Term *a, Term *b)
	: Lit(loc)
	, var_(var)
	, a_(a)
	, b_(b)
{
}

namespace
{
	void numException(Grounder *grounder, RangeLit *lit, const Val *val)
	{
		std::ostringstream oss;
		oss << "cannot convert ";
		val->print(grounder, oss);
		oss << " to integer";
		std::string str(oss.str());
		oss.str("");
		lit->print(grounder, oss);
		throw TypeException(str, StrLoc(grounder, lit->loc()), oss.str());
	}
}


bool RangeLit::match(Grounder *grounder)
{
	try
	{
		int num(var_->val(grounder).number());
		int lower(a_->val(grounder).number());
		int upper(b_->val(grounder).number());
		return lower <= num && num <= upper;
	}
	catch(const Val *val)
	{
		numException(grounder, this, val);
		return false;
	}	
}

namespace
{
	class RangeIndex : public Index
	{
	public:
		RangeIndex(uint32_t var, RangeLit *lit, Term *a, Term *b, const VarVec &bind);
		std::pair<bool,bool> firstMatch(Grounder *grounder, int binder);
		std::pair<bool,bool> nextMatch(Grounder *grounder, int binder);
		void reset() { finished_ = false; }
		void finish() { finished_ = true; }
		bool hasNew() const { return !finished_; }
	private:
		bool      finished_;
		uint32_t  var_;
		int       current_;
		int       upper_;
		RangeLit *lit_;
		Term     *a_;
		Term     *b_;
		VarVec    bind_;
	};

	RangeIndex::RangeIndex(uint32_t var, RangeLit *lit, Term *a, Term *b, const VarVec &bind)
		: finished_(false)
		, var_(var)
		, lit_(lit)
		, a_(a)
		, b_(b)
		, bind_(bind)
	{
	}

	std::pair<bool,bool> RangeIndex::firstMatch(Grounder *grounder, int binder)
	{
		try
		{
			current_ = a_->val(grounder).number();
			upper_   = b_->val(grounder).number();
		}
		catch(const Val *val)
		{
			numException(grounder, lit_, val);
		}	
		return nextMatch(grounder, binder);
	}

	std::pair<bool,bool> RangeIndex::nextMatch(Grounder *grounder, int binder)
	{
		if(current_ <= upper_)
		{
			grounder->val(var_, Val::create(Val::NUM, current_++), binder);
			return std::make_pair(true, !finished_);
		}
		else return std::make_pair(false, !finished_);
	}
}

void RangeLit::index(Grounder *g, Groundable *gr, VarSet &bound)
{
	(void)g;
	VarSet vars;
	VarVec bind;
	var_->vars(vars);
	std::set_difference(vars.begin(), vars.end(), bound.begin(), bound.end(), std::back_insert_iterator<VarVec>(bind));
	if(bind.size() > 0)
	{
		RangeIndex *p = new RangeIndex(var_->index(), this, a_.get(), b_.get(), bind);
		gr->instantiator()->append(p);
		bound.insert(bind.begin(), bind.end());
	}
	else gr->instantiator()->append(new MatchIndex(this));
}

void RangeLit::accept(Printer *v)
{
	(void)v;
}

void RangeLit::visit(PrgVisitor *v)
{
	v->visit(var_.get(), true);
	v->visit(a_.get(), false);
	v->visit(b_.get(), false);
}

void RangeLit::print(Storage *sto, std::ostream &out) const
{
	out << "#range(";
	var_->print(sto, out);
	out << ",";
	a_->print(sto, out);
	out << ",";
	b_->print(sto, out);
	out << ")";
}

void RangeLit::normalize(Grounder *g, Expander *expander)
{
	a_->normalize(this, Term::PtrRef(a_), g, expander, false);
	b_->normalize(this, Term::PtrRef(b_), g, expander, false);
}

double RangeLit::score(Grounder *g, VarSet &bound) const
{
	if(a_->constant() && b_->constant())
	{
		Val l = a_->val(g);
		Val u = b_->val(g);
		if(l.type == Val::NUM && u.type == Val::NUM)
		{
			int diff = u.num - l.num;
			if(diff >= 0) { return diff; }
			else          { return Lit::score(g, bound); }
		}
		else { return Lit::score(g, bound); }
	}
	else { return 0; }
}

Lit *RangeLit::clone() const
{
	return new RangeLit(*this);
}

RangeLit::~RangeLit()
{
}

