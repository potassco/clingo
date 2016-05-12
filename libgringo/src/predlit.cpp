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

#include "gringo/predlit.h"
#include "gringo/term.h"
#include "gringo/printer.h"
#include "gringo/grounder.h"
#include "gringo/domain.h"
#include "gringo/predindex.h"
#include "gringo/groundable.h"
#include "gringo/instantiator.h"
#include "gringo/litdep.h"
#include "gringo/varcollector.h"

PredLitSet::PredCmp::PredCmp(const ValVec &vals)
	: vals_(vals)
{
}

size_t PredLitSet::PredCmp::operator()(const PredSig &a) const
{
	size_t hash = a.first->dom()->domId();
	boost::hash_combine(hash, static_cast<size_t>(a.first->sign()));
	boost::hash_range(hash, vals_.begin() + a.second, vals_.begin() + a.second + a.first->dom()->arity() + 1);
	return hash;
}

bool PredLitSet::PredCmp::operator()(const PredSig &a, const PredSig &b) const
{
	return
		a.first->dom() == b.first->dom() &&
		a.first->sign() == b.first->sign() &&
		ValRng(vals_.begin() + a.second, vals_.begin() + a.second + a.first->dom()->arity() + 1) ==
		ValRng(vals_.begin() + b.second, vals_.begin() + b.second + a.first->dom()->arity() + 1);
}

PredLitSet::PredLitSet()
	: set_(0, PredCmp(vals_), PredCmp(vals_))
{
}

PredLitSet::PredLitSet(PredLitSet const &other)
	: set_(0, PredCmp(vals_), PredCmp(vals_))
{
    assert(other.vals_.empty());
}

bool PredLitSet::insert(PredLit *pred, size_t pos, Val &val)
{
	ValRng rng = pred->vals(pos);
	size_t offset = vals_.size();
	vals_.insert(vals_.end(), rng.begin(), rng.end());
	vals_.insert(vals_.end(), val);
	if(set_.insert(PredSig(pred, offset)).second) return true;
	else
	{
		vals_.resize(offset);
		return false;
	}
}

void PredLitSet::clear()
{
	set_.clear();
	vals_.clear();
}

PredLit::PredLit(const Loc &loc, Domain *dom, TermPtrVec &terms)
	: Lit(loc)
	, PredLitRep(false, dom)
	, terms_(terms.release())
{
	vals_.reserve(terms_.size());
}

bool PredLit::fact() const
{
	assert(top_ + dom_->arity() <= vals_.size());
	if(sign())
	{
		if(dom_->complete()) return !dom_->find(vals_.begin() + top_).valid();
		else return false;
	}
	else return dom_->find(vals_.begin() + top_).fact;
}

bool PredLit::isFalse(Grounder *grounder)
{
	(void)grounder;
	assert(top_ + dom_->arity() <= vals_.size());
	if(!sign())
	{
		if(dom_->complete()) return !dom_->find(vals_.begin() + top_).valid();
		else return false;
	}
	else return dom_->find(vals_.begin() + top_).fact;
}

void PredLit::finish(Grounder *g)
{
	dom_->enqueue(g);
}

Lit::Monotonicity PredLit::monotonicity()
{
	return PredLitRep::sign() || dom()->external() ? ANTIMONOTONE : MONOTONE;
}

void PredLit::visit(PrgVisitor *v)
{
	v->visit(this);
	foreach(Term &a, terms_) v->visit(&a, !sign() && !head() && !dom()->external());
}

void PredLit::vars(VarSet &vars) const
{
	foreach(const Term &a, terms_) a.vars(vars);
}

bool PredLit::complete() const
{
	return dom()->complete();
}

void PredLit::push()
{
	top_ = vals_.size();
    for (size_t i = vals_.size() - terms_.size(), end = vals_.size(); i != end; ++i)
    {
        vals_.push_back(vals_[i]);
    }
}

bool PredLit::testUnique(PredLitSet &set, Val val)
{
	return set.insert(this, top_, val);
}

void PredLit::pop()
{
	top_-= terms_.size();
}

void PredLit::move(size_t p)
{
	top_ = p * dom_->arity();
}

void PredLit::clear()
{
	top_ = 0;
	vals_.clear();
}

bool PredLit::match(Grounder *grounder)
{
	vals_.resize(top_);
	foreach(const Term &term, terms_) vals_.push_back(term.val(grounder));
	if(head()) return true;
	if(sign()) return !dom_->find(vals_.begin() + top_).fact;
	if(dom()->external()) return true;
	else return dom_->find(vals_.begin() + top_).valid();
}

void PredLit::index(Grounder *g, Groundable *gr, VarSet &bound)
{
	if(sign() || head() || dom()->external())
	{
		match_ = true;
		gr->instantiator()->append(new MatchIndex(this));
	}
	else
	{
		match_ = false;
		VarSet vars;
		VarVec index, bind;
		this->vars(vars);
		std::set_intersection(bound.begin(), bound.end(), vars.begin(), vars.end(), std::back_insert_iterator<VarVec>(index));
		std::set_difference(vars.begin(), vars.end(), index.begin(), index.end(), std::back_insert_iterator<VarVec>(bind));
		bound.insert(vars.begin(), vars.end());
		PredIndex *p = new PredIndex(terms_, index, bind);
		dom_->append(g, gr, p);
		gr->instantiator()->append(p);
	}
}

void PredLit::grounded(Grounder *grounder)
{
	if(!match_)
	{
		vals_.resize(top_);
		foreach(const Term &a, terms_) vals_.push_back(a.val(grounder));
	}
}

void PredLit::accept(Printer *v)
{
	v->print(this);
}

bool PredLit::edbFact() const
{
	foreach(const Term &a, terms_)
		if(!a.constant()) return false;
	return true;
}

void PredLit::print(Storage *sto, std::ostream &out) const
{
	if(sign()) out << "not ";
	out << sto->string(dom_->nameId());
	if(terms_.size() > 0)
	{
		out << "(";
		bool comma = false;
		foreach(const Term &a, terms_)
		{
			if(comma) out << ",";
			else comma = true;
			a.print(sto, out);
		}
		out << ")";
	}
}

void PredLit::normalize(Grounder *g, Expander *expander)
{
	for(TermPtrVec::iterator i = terms_.begin(); i != terms_.end(); i++)
	{
		for(Term::Split s = i->split(); s.first; s = i->split())
		{
			PredLit *p = new PredLit(loc(), g->domain(dom_->nameId(), s.second->size()), *s.second);
			p->sign(sign());
			expander->expand(p, Expander::POOL);
			terms_.replace(i, s.first);
		}
		i->normalize(this, Term::VecRef(terms_, i), g, expander, !head() && !sign());
	}
}

double PredLit::score(Grounder *g, VarSet &bound) const
{
	if(sign() || terms_.size() == 0) { return Lit::score(g, bound); }
	else {
        double score = std::pow(dom()->size(), 1.0 / terms_.size());
        double ret   = 0;
        foreach (Term const &x, terms_) { ret += x.estimate(bound, score); }
        return ret / terms_.size();
    }
}

Lit *PredLit::clone() const
{
	return new PredLit(*this);
}

