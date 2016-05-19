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

#include <gringo/functionterm.h>
#include <gringo/value.h>

using namespace gringo;

FunctionTerm::FunctionTerm(FunctionType type, Term *a, Term *b) : Term(), type_(type), a_(a), b_(b)
{
}

void FunctionTerm::print(const GlobalStorage *g, std::ostream &out) const
{
	switch(type_)
	{
		case ABS:
			out << "abs" << "(" << pp(g, a_) << ")";
			break;
		case PLUS:
			out << "(" << pp(g, a_) << " + " << pp(g, b_) << ")";
			break;
		case MINUS:
			out << "(" << pp(g, a_) << " - " << pp(g, b_) << ")";
			break;
		case POWER:
			out << "(" << pp(g, a_) << " ** " << pp(g, b_) << ")";
			break;
		case TIMES:
			out << "(" << pp(g, a_) << " * " << pp(g, b_) << ")";
			break;
		case DIVIDE:
			out << "(" << pp(g, a_) << " / " << pp(g, b_) << ")";
			break;
		case MOD:
			out << "(" << pp(g, a_) << " mod " << pp(g, b_) << ")";
			break;
		case BITXOR:
			out << "(" << pp(g, a_) << " ^ " << pp(g, b_) << ")";
			break;
		case BITOR:
			out << "(" << pp(g, a_) << " ? " << pp(g, b_) << ")";
			break;
		case BITAND:
			out << "(" << pp(g, a_) << " & " << pp(g, b_) << ")";
			break;
		case COMPLEMENT:
			out << "(~" << pp(g, a_) << ")";
			break;
	}
}

void FunctionTerm::getVars(VarSet &vars) const
{
	a_->getVars(vars);
	if(b_)
		b_->getVars(vars);
}

void FunctionTerm::preprocess(Literal *l, Term *&p, Grounder *g, Expandable *e)
{
	a_->preprocess(l, a_, g, e);
	if(b_)
		b_->preprocess(l, b_, g, e);
}

void FunctionTerm::addIncParam(Grounder *g, Term *&p, const Value &v)
{
	a_->addIncParam(g, a_, v);
	if(b_)
		b_->addIncParam(g, b_, v);
}

bool FunctionTerm::isComplex()
{
	return true;
}

Value FunctionTerm::getConstValue(Grounder *g)
{
	switch(type_)
	{
		case ABS:
			return Value(Value::INT, abs(a_->getConstValue(g)));
		case PLUS:
			return Value(Value::INT, a_->getConstValue(g) + b_->getConstValue(g));
		case MINUS:
			return Value(Value::INT, a_->getConstValue(g) - b_->getConstValue(g));
		case POWER:
			return Value(Value::INT, (int)pow(double(a_->getConstValue(g)), int(b_->getConstValue(g))));
		case TIMES:
			return Value(Value::INT, a_->getConstValue(g) * b_->getConstValue(g));
		case DIVIDE:
			return Value(Value::INT, a_->getConstValue(g) / b_->getConstValue(g));
		case MOD:
			return Value(Value::INT, a_->getConstValue(g) % b_->getConstValue(g));
		case BITXOR:
			return Value(Value::INT, a_->getConstValue(g) ^ b_->getConstValue(g));
		case BITOR:
			return Value(Value::INT, a_->getConstValue(g) | b_->getConstValue(g));
		case BITAND:
			return Value(Value::INT, a_->getConstValue(g) & b_->getConstValue(g));
		case COMPLEMENT:
			return Value(Value::INT, ~a_->getConstValue(g));
	}
	FAIL(true);
}

Value FunctionTerm::getValue(Grounder *g)
{
	switch(type_)
	{
		case ABS:
			return Value(Value::INT, abs(a_->getValue(g)));
		case PLUS:
			return Value(Value::INT, a_->getValue(g) + b_->getValue(g));
		case MINUS:
			return Value(Value::INT, a_->getValue(g) - b_->getValue(g));
		case POWER:
			return Value(Value::INT, (int)pow(double(a_->getValue(g)), int(b_->getValue(g))));
		case TIMES:
			return Value(Value::INT, a_->getValue(g) * b_->getValue(g));
		case DIVIDE:
			return Value(Value::INT, a_->getValue(g) / b_->getValue(g));
		case MOD:
			return Value(Value::INT, a_->getValue(g) % b_->getValue(g));
		case BITXOR:
			return Value(Value::INT, a_->getValue(g) ^ b_->getValue(g));
		case BITOR:
			return Value(Value::INT, a_->getValue(g) | b_->getValue(g));
		case BITAND:
			return Value(Value::INT, a_->getValue(g) & b_->getValue(g));
		case COMPLEMENT:
			return Value(Value::INT, ~a_->getValue(g));
	}
	FAIL(true);
}

FunctionTerm::FunctionTerm(const FunctionTerm &f) : type_(f.type_), a_(f.a_->clone()), b_(f.b_ ? f.b_->clone() : 0)
{
}

Term* FunctionTerm::clone() const
{
	return new FunctionTerm(*this);
}

FunctionTerm::~FunctionTerm()
{
	if(a_)
		delete a_;
	if(b_)
		delete b_;

}

