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

#include <gringo/statement.h>
#include <gringo/varcollector.h>
#include <gringo/litdep.h>
#include <gringo/varterm.h>
#include <gringo/grounder.h>
#include <gringo/exceptions.h>

Statement::Statement(const Loc &loc)
	: Locateable(loc)
{
}

void Statement::check(Grounder *g)
{
	VarCollector collector(g);
	collector.visit(this, choice());
	uint32_t numVars = collector.collect();
	if(numVars > 0)
	{
		LitDep::Builder builder(numVars);
		builder.visit(this, choice());
		VarTermVec terms;
		if(!builder.check(terms))
		{
			std::ostringstream oss;
			print(g, oss);
			UnsafeVarsException ex(StrLoc(g, loc()), oss.str());
			foreach(VarTerm *term, terms)
			{
				oss.str("");
				term->print(g, oss);
				ex.add(StrLoc(g, term->loc()), oss.str());
			}
			throw ex;
		}
	}
}

