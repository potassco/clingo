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

#ifndef LPARSEPARSER_H
#define LPARSEPARSER_H

#include <gringo/gringo.h>
#include <gringo/gringoparser.h>

namespace gringo
{
	class LparseParser : public GrinGoParser 
	{
	public:
		LparseParser(Grounder *g, std::vector<std::istream*> &in);
		LparseParser(Grounder *g, std::istream* = &std::cin);
		bool parse(NS_OUTPUT::Output *output);
		GrinGoLexer *getLexer();
		Grounder *getGrounder() { return grounder_; }
		virtual ~LparseParser();
	private:
		LparseLexer *lexer_;
		Grounder *grounder_;
		std::vector<std::istream*> streams_;
		void *pParser;
	};
}
#endif

