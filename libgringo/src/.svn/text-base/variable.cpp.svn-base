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

#include <gringo/variable.h>
#include <gringo/grounder.h>

using namespace gringo;
		
Variable::Variable(Grounder *g, int id) : id_(id), uid_(g->registerVar(id))
{
}

void Variable::getVars(VarSet &vars) const
{
	vars.insert(uid_);
}

bool Variable::isComplex()
{
	return false;
}

void Variable::print(const GlobalStorage *g, std::ostream &out) const
{
	out << *g->getString(id_);
}

Value Variable::getConstValue(Grounder *g)
{
	FAIL(true);
}

Value Variable::getValue(Grounder *g)
{
	return g->getValue(uid_);
}

int Variable::getUID()
{
	return uid_;
}

Variable::~Variable()
{
}

void Variable::preprocess(Literal *l, Term *&p, Grounder *g, Expandable *e)
{
}

Variable::Variable(const Variable &c) : id_(c.id_), uid_(c.uid_)
{
}

Term* Variable::clone() const
{
	return new Variable(*this);
}

void  Variable::addIncParam(Grounder *g, Term *&p, const Value &v)
{
}

bool Variable::unify(const GlobalStorage *g, const Value& t, const VarVector& vars, ValueVector& vals) const
{
	// TODO: constant access would be nice
	for (size_t i = 0; i < vars.size(); ++i)
	{
		// if variable is bound
		if (vars[i] == uid_)
		{
			// variable is not set, then set, else compare
			if (vals[i].type_ == Value::UNDEF)
			{
				vals[i] = t;
				return true;
			}
			else
				return vals[i].equal(t);
		}
	}
	FAIL(true);
}

