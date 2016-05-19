// Copyright (c) 2008, Roland Kaminski
//
// This file is part of GrinGo.
//
// GrinGo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GrinGo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GrinGo.  If not, see <http://www.gnu.org/licenses/>.

#include <gringo/funcsymbolterm.h>
#include <gringo/value.h>
#include <gringo/grounder.h>
#include <gringo/funcsymbol.h>
#include <gringo/assignmentliteral.h>
#include <gringo/variable.h>
#include <gringo/expandable.h>

using namespace gringo;

FuncSymbolTerm::FuncSymbolTerm(Grounder* g, int s, TermVector* tl) : Term(), name_(s), termList_(tl)
{
}

void FuncSymbolTerm::print(const GlobalStorage *g, std::ostream &out) const
{
	out << *g->getString(name_) << "(";
	for (unsigned int i = 0; i != termList_->size()-1; ++i)
	{
		out << pp(g, (*termList_)[i]) << ",";
	}
	out << pp(g, (*termList_)[termList_->size()-1]) << ")";
}

void FuncSymbolTerm::getVars(VarSet &vars) const
{
	for (TermVector::const_iterator i = termList_->begin(); i != termList_->end(); ++i)
		(*i)->getVars(vars);
}

void FuncSymbolTerm::preprocess(Literal *l, Term *&p, Grounder *g, Expandable *e)
{
	for (TermVector::iterator i = termList_->begin(); i != termList_->end(); ++i)
		(*i)->preprocess(l, (*i), g, e);
	for(TermVector::iterator it = termList_->begin(); it != termList_->end(); it++)
		if((*it)->isComplex())
		{
			int var = g->createUniqueVar();
			e->appendLiteral(new AssignmentLiteral(new Variable(g, var), *it), Expandable::COMPLEXTERM);
			*it = new Variable(g, var);
		}
}

bool FuncSymbolTerm::isComplex()
{
	// TODO: change this to false in the new implementation of functionsymbols
	//       i want to keep a working version in the trunk so its better to 
	//       return true for now
	//return true;
	return false;
}

Value FuncSymbolTerm::getConstValue(Grounder *g)
{
	ValueVector args;
	for (unsigned int i = 0; i != termList_->size(); ++i)
	{
		args.push_back((*termList_)[i]->getConstValue(g));
	}

	FuncSymbol* funcSymbol = new FuncSymbol(name_, args);
	return Value(Value::FUNCSYMBOL, g->createFuncSymbol(funcSymbol));
}

Value FuncSymbolTerm::getValue(Grounder *g)
{
	ValueVector args;
	for (unsigned int i = 0; i != termList_->size(); ++i)
	{
		args.push_back((*termList_)[i]->getValue(g));
	}

	FuncSymbol* funcSymbol = new FuncSymbol(name_, args);
	return Value(Value::FUNCSYMBOL, g->createFuncSymbol(funcSymbol));
}

FuncSymbolTerm::FuncSymbolTerm(const FuncSymbolTerm &f) : name_(f.name_)
{
	termList_ = new TermVector();
	for (TermVector::const_iterator i = f.termList_->begin(); i != f.termList_->end(); ++i)
	{
		termList_->push_back((*i)->clone());
	}
}

Term* FuncSymbolTerm::clone() const
{
	return new FuncSymbolTerm(*this);
}

bool FuncSymbolTerm::unify(const GlobalStorage *g, const Value& t, const VarVector& vars, ValueVector& vals) const
{
	if (t.type_ == Value::FUNCSYMBOL)
	{
		int name = g->getFuncSymbol(t.uid_)->getName();
		if (name != name_)
			return false;
		const ValueVector& values = g->getFuncSymbol(t.uid_)->getValues();
		if (values.size() != termList_->size())
			return false;
		TermVector::const_iterator term = termList_->begin();
		for (ValueVector::const_iterator i = values.begin(); i != values.end(); ++i, ++term)
		{
			if (!(*term)->unify(g, *i, vars, vals))
				return false;
		}
		return true;
	}
	else
		return false;

}

void FuncSymbolTerm::addIncParam(Grounder *g, Term *&p, const Value &v)
{
	for (TermVector::iterator i = termList_->begin(); i != termList_->end(); ++i)
		(*i)->addIncParam(g, *i, v);
}

FuncSymbolTerm::~FuncSymbolTerm()
{
	for (TermVector::iterator i = termList_->begin(); i != termList_->end(); ++i)
		delete (*i);
	delete termList_;
}

