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

#pragma once

#include <gringo/gringo.h>
#include <gringo/statement.h>
#include <gringo/printer.h>

class Rule : public Statement
{
public:
	class Printer : public ::Printer
	{
	public:
		virtual void begin() = 0;
		virtual void endHead() = 0;
		virtual void end() = 0;
		virtual ~Printer() { }
	};
public:
	Rule(const Loc &loc, Lit *head, LitPtrVec &body);
	void normalize(Grounder *g);
	bool grounded(Grounder *g);
	void ground(Grounder *g);
	void init(Grounder *g, const VarSet &bound);
	void visit(PrgVisitor *v);
	void print(Storage *sto, std::ostream &out) const;
	void append(Lit *l);
	bool edbFact() const;
	~Rule();
private:
	std::auto_ptr<Lit> head_;
	LitPtrVec          body_;
	bool               grounded_;
};

