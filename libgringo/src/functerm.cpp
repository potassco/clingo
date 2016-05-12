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

#include <gringo/functerm.h>
#include <gringo/grounder.h>
#include <gringo/storage.h>
#include <gringo/func.h>
#include <gringo/lit.h>

FuncTerm::FuncTerm(const Loc &loc, uint32_t name, TermPtrVec &args)
	: Term(loc)
	, name_(name)
	, args_(args)
{
}

Val FuncTerm::val(Grounder *grounder) const
{
	ValVec vals;
	foreach(const Term &term, args_) vals.push_back(term.val(grounder));
	return Val::create(Val::FUNC, grounder->index(Func(grounder, name_, vals)));
}

void FuncTerm::normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify)
{
	(void)ref;
	for(TermPtrVec::iterator i = args_.begin(); i != args_.end(); i++)
	{
		for(Term::Split s = i->split(); s.first; s = i->split())
		{
			clone_.reset(new FuncTerm(loc(), name_, *s.second));
			expander->expand(parent->clone(), Expander::POOL);
			args_.replace(i, s.first);
		}
		i->normalize(parent, Term::VecRef(args_, i), g, expander, unify);
	}
}

bool FuncTerm::unify(Grounder *grounder, const Val &v, int binder) const
{
	if(v.type != Val::FUNC) return false;
	const Func &f = grounder->func(v.index);
	if(name_ != f.name_) return false;
	if(args_.size() != f.args_.size()) return false;
	for(TermPtrVec::size_type i = 0; i < args_.size(); i++)
		if(!args_[i].unify(grounder, f.args_[i], binder)) return false;
	return true;
}

void FuncTerm::vars(VarSet &v) const
{
	foreach(const Term &t, args_) t.vars(v);
}

void FuncTerm::visit(PrgVisitor *visitor, bool bind)
{
	foreach(Term &t, args_) t.visit(visitor, bind);
}

bool FuncTerm::constant() const
{
	foreach(const Term &t, args_)
		if(!t.constant()) return false;
	return true;
}

void FuncTerm::print(Storage *sto, std::ostream &out) const
{
	out << sto->string(name_);
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

Term *FuncTerm::clone() const
{
	if(clone_.get()) return clone_.release();
	else return new FuncTerm(*this);
}

double FuncTerm::estimate(VarSet const &bound, double size) const {
    if (args_.empty()) { return 1; }
    else {
        double score = std::max(1.0, std::pow(size, 1.0 / args_.size()));
        double ret   = 0;
        foreach (Term const &x, args_) { ret += x.estimate(bound, score); }
        return ret / args_.size();
    }
}

FuncTerm::~FuncTerm()
{
}

