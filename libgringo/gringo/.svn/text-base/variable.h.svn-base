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

#ifndef VARIABLE_H
#define VARIABLE_H

#include <gringo/gringo.h>
#include <gringo/term.h>
#include <gringo/value.h>

namespace gringo
{
	class Variable : public Term
	{
	public:
		Variable(Grounder *g, int id);
		Variable(const Variable &c);
		virtual void print(const GlobalStorage *g, std::ostream &out) const;
		virtual void getVars(VarSet &vars) const;
		virtual bool isComplex();
		virtual void preprocess(Literal *l, Term *&p, Grounder *g, Expandable *e);
		virtual Value getConstValue(Grounder *g);
		virtual Value getValue(Grounder *g);
		int getUID();
		virtual Term* clone() const;
		bool unify(const GlobalStorage *g, const Value& t, const VarVector& vars, ValueVector& vals) const;
		virtual void addIncParam(Grounder *g, Term *&p, const Value &v);
 		virtual ~Variable();
	protected:
		int id_;
		int uid_;
	};
}

#endif

