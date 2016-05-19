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

#include <gringo/constant.h>
#include <gringo/grounder.h>

using namespace gringo;
		
Constant::Constant(const Value &v) : value_(v)
{
}

void Constant::getVars(VarSet &vars) const
{
}

bool Constant::isComplex()
{
	return false;
}

void Constant::print(const GlobalStorage *g, std::ostream &out) const
{
	value_.print(g, out);
}

Value Constant::getConstValue(Grounder *g)
{
	if(value_.type_ == Value::STRING)
		return g->getConstValue(value_.uid_);
	else
		return value_;
}

Value Constant::getValue(Grounder *g)
{
	return value_;
}

Constant::~Constant()
{
}

void Constant::preprocess(Literal *l, Term *&p, Grounder *g, Expandable *e)
{
	if(value_.type_ == Value::STRING)
		value_ = g->getConstValue(value_.uid_);
}

Constant::Constant(const Constant &c) :  value_(c.value_)
{
}

Term* Constant::clone() const
{
	return new Constant(*this);
}

namespace
{
	class IncTerm : public Term
	{
	public:
		IncTerm() {}
		IncTerm(const IncTerm &c) {}
		Term* clone() const
		{
			return new IncTerm(*this);
		}

		bool unify(const GlobalStorage *g, const Value& t, const VarVector& vars, ValueVector& subst) const 
		{
			return t.equal(Value(Value::INT, static_cast<const Grounder *>(g)->getIncStep()));
		}

		Value getValue(Grounder *g) 
		{ 
			return Value(Value::INT, g->getIncStep()); 
		}

		Value getConstValue(Grounder *g) 
		{ 
			return Value(Value::INT, g->getIncStep()); 
		}

		void getVars(VarSet &vars) const { }
		bool isComplex() { return false; }
		void preprocess(Literal *l, Term *&p, Grounder *g, Expandable *e) { }
		void print(const GlobalStorage *g, std::ostream &out) const { out << "incremental"; }
		void addIncParam(Grounder *g, Term *&p, const Value &v) { FAIL(true); }
		~IncTerm() { }
	};
}

void  Constant::addIncParam(Grounder *g, Term *&p, const Value &v)
{
	if(value_.equal(v))
	{
		p = new IncTerm();
		delete this;
	}
}

bool Constant::unify(const GlobalStorage *g, const Value& t, const VarVector& vars, ValueVector& vals) const
{
	return t.equal(value_);
}

