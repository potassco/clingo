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

#include <gringo/mathterm.h>
#include <gringo/grounder.h>
#include <gringo/varterm.h>
#include <gringo/constterm.h>
#include <gringo/rellit.h>
#include <gringo/prgvisitor.h>
#include <gringo/exceptions.h>

namespace
{
	int ipow(int a, int b)
	{
		if(b < 0) { return 0; }
		else
		{
			int r = 1;
			while(b > 0)
			{
				if(b & 1) { r *= a; }
				b >>= 1;
				a *= a;
			}
			return r;
		}
	}
}

MathTerm::MathTerm(const Loc &loc, const Func &f, Term *a, Term *b) :
	Term(loc), f_(f), a_(a), b_(b)
{
}

Val MathTerm::val(Grounder *grounder) const
{
	try
	{
		// TODO: what about moving all the functions into Val
		switch(f_)
		{
			case PLUS:  return Val::create(Val::NUM, a_->val(grounder).number() + b_->val(grounder).number()); break;
			case MINUS: return Val::create(Val::NUM, a_->val(grounder).number() - b_->val(grounder).number()); break;
			case MULT:  return Val::create(Val::NUM, a_->val(grounder).number() * b_->val(grounder).number()); break;
			case DIV:   return Val::create(Val::NUM, a_->val(grounder).number() / b_->val(grounder).number()); break;
			case MOD:   return Val::create(Val::NUM, a_->val(grounder).number() % b_->val(grounder).number()); break;
			case POW:   return Val::create(Val::NUM, ipow(a_->val(grounder).number(), b_->val(grounder).number())); break;
			case AND:   return Val::create(Val::NUM, a_->val(grounder).number() & b_->val(grounder).number()); break;
			case XOR:   return Val::create(Val::NUM, a_->val(grounder).number() ^ b_->val(grounder).number()); break;
			case OR:    return Val::create(Val::NUM, a_->val(grounder).number() | b_->val(grounder).number()); break;
			case ABS:   return Val::create(Val::NUM, std::abs(a_->val(grounder).number())); break;
		}
	}
	catch(const Val *val)
	{
		std::ostringstream oss;
		oss << "cannot convert ";
		val->print(grounder, oss);
		oss << " to integer";
		std::string str(oss.str());
		oss.str("");
		print(grounder, oss);
		throw TypeException(str, StrLoc(grounder, loc()), oss.str());
	}
	assert(false);
	return Val::create();
}

bool MathTerm::unify(Grounder *grounder, const Val &v, int binder) const
{
	(void)binder;
	if(constant()) return v == val(grounder);
	assert(false);
	return false;
}

void MathTerm::vars(VarSet &v) const
{
	a_->vars(v);
	if(b_.get()) b_->vars(v);
}

void MathTerm::visit(PrgVisitor *visitor, bool bind)
{
	(void)bind;
	visitor->visit(a_.get(), false);
	if(b_.get()) visitor->visit(b_.get(), false);
}

bool MathTerm::constant() const
{
	return a_->constant() && (!b_.get() || b_->constant());
}

void MathTerm::print(Storage *sto, std::ostream &out) const
{
	if(b_.get()) a_->print(sto, out);
	switch(f_)
	{
		case PLUS:  out << "+"; break;
		case MINUS: out << "-"; break;
		case MULT:  out << "*"; break;
		case DIV:   out << "/"; break;
		case MOD:   out << "\\"; break;
		case POW:   out << "**"; break;
		case AND:   out << "&"; break;
		case XOR:   out << "^"; break;
		case OR:    out << "?"; break;
		case ABS:   break;
	}
	if(b_.get()) b_->print(sto, out);
	else
	{
		out << "|";
		a_->print(sto, out);
		out << "|";
	}
}

void MathTerm::normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify)
{
	if(a_.get()) a_->normalize(parent, PtrRef(a_), g, expander, false);
	if(b_.get()) b_->normalize(parent, PtrRef(b_), g, expander, false);
	if((!a_.get() || a_->constant()) && (!b_.get() || b_->constant()))
	{
		ref.reset(new ConstTerm(loc(), val(g)));
	}
	else if(unify)
	{
		uint32_t var = g->createVar();
		expander->expand(new RelLit(a_->loc(), RelLit::ASSIGN, new VarTerm(a_->loc(), var), this), Expander::RELATION);
		ref.replace(new VarTerm(a_->loc(), var));
	}
}

Term *MathTerm::clone() const
{
	return new MathTerm(*this);
}

double MathTerm::estimate(VarSet const &, double) const {
    return 1;
}

MathTerm::~MathTerm()
{
}

