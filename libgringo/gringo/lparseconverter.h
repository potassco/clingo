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

#ifndef LPARSECONVERTER_H
#define LPARSECONVERTER_H

#include <gringo/gringo.h>
#include <gringo/gringoparser.h>
#include <gringo/globalstorage.h>

namespace gringo
{
	class LparseConverter : public GrinGoParser, public GlobalStorage
	{
	public:
		LparseConverter(std::vector<std::istream*> &in);
		LparseConverter(std::istream* = &std::cin);
		bool parse(NS_OUTPUT::Output *output);
		GrinGoLexer *getLexer();
		NS_OUTPUT::Output *getOutput();
		int createPred(int id, int arity);
		virtual ~LparseConverter();
	private:
		PlainLparseLexer *lexer_;
		NS_OUTPUT::Output *output_;
		std::vector<std::istream*> streams_;
		void *pParser;
	};
}

#endif

