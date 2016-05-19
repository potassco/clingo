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

#ifndef PROGRAM_H
#define PROGRAM_H

#include <gringo/gringo.h>
#include <gringo/printable.h>

namespace gringo
{
	class Program : public Printable
	{
	public:
		enum Type {FACT=0, BASIC=1, NORMAL=2};
	public:
		Program(Type type, StatementVector &rules);
		void print(const GlobalStorage *g, std::ostream &out) const;
		Evaluator *getEvaluator();
		bool check(Grounder *g);
		StatementVector *getStatements();
		Type getType();
		~Program();
	private:
		Type type_;
		StatementVector rules_;
		Evaluator *eval_;
	};
}

#endif

