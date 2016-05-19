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

#include <gringo/lparseparser.h>
#include "lparseparser_impl.h"
#include <gringo/lparselexer.h>
#include <gringo/grounder.h>

using namespace gringo;

void *lparseparserAlloc(void *(*mallocProc)(size_t));
void lparseparserFree(void *p, void (*freeProc)(void*));
void lparseparser(void *yyp, int yymajor, std::string* yyminor, LparseParser *pParser);

LparseParser::LparseParser(Grounder *g, std::istream* in) : GrinGoParser(), grounder_(g)
{
	lexer_  = new LparseLexer();
        pParser = lparseparserAlloc (malloc);
	streams_.push_back(in);
}

LparseParser::LparseParser(Grounder *g, std::vector<std::istream*> &in) : GrinGoParser(), grounder_(g)
{
	lexer_  = new LparseLexer();
        pParser = lparseparserAlloc (malloc);
	streams_ = in;
}

bool LparseParser::parse(NS_OUTPUT::Output *output)
{
	int token;
	std::string *lval;
	grounder_->setOutput(output);
	for(std::vector<std::istream*>::iterator it = streams_.begin(); it != streams_.end(); it++)
	{
		lexer_->reset(*it);
		while((token = lexer_->lex(lval)) != LPARSEPARSER_EOI)
		{
			lparseparser(pParser, token, lval, this);
		}
	}
	lparseparser(pParser, 0, lval, this);
	if(getError())
		return false;
	return true;
}

LparseParser::~LparseParser()
{
	delete lexer_;
        lparseparserFree(pParser, free);
}

GrinGoLexer *LparseParser::getLexer()
{
	return lexer_;
}

