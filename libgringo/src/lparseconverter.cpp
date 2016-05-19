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

#include <gringo/lparseconverter.h>
#include "lparseconverter_impl.h"
#include <gringo/plainlparselexer.h>
#include <gringo/grounder.h>
#include <gringo/funcsymbol.h>

using namespace gringo;

void *lparseconverterAlloc(void *(*mallocProc)(size_t));
void lparseconverterFree(void *p, void (*freeProc)(void*));
void lparseconverter(void *yyp, int yymajor, std::string* yyminor, LparseConverter *pParser);

LparseConverter::LparseConverter(std::istream* in) : GrinGoParser(), output_(0)
{
	lexer_  = new PlainLparseLexer();
        pParser = lparseconverterAlloc (malloc);
	streams_.push_back(in);
}

LparseConverter::LparseConverter(std::vector<std::istream*> &in) : GrinGoParser()
{
	lexer_  = new PlainLparseLexer();
        pParser = lparseconverterAlloc (malloc);
	streams_ = in;
}

bool LparseConverter::parse(NS_OUTPUT::Output *output)
{
	output_ = output;
	int token;
	std::string *lval;
	for(std::vector<std::istream*>::iterator it = streams_.begin(); it != streams_.end(); it++)
	{
		lexer_->reset(*it);
		while((token = lexer_->lex(lval)) != LPARSECONVERTER_EOI)
		{
			lparseconverter(pParser, token, lval, this);
		}
	}
	lparseconverter(pParser, 0, lval, this);
	if(getError())
		return false;
	else
	{
		output_->finalize(true);
		return true;
	}
}

LparseConverter::~LparseConverter()
{
	delete lexer_;
        lparseconverterFree(pParser, free);
}

GrinGoLexer *LparseConverter::getLexer()
{
	return lexer_;
}

NS_OUTPUT::Output *LparseConverter::getOutput()
{
	return output_;
}

int LparseConverter::createPred(int id, int arity)
{
	std::pair<SignatureHash::iterator, bool> res = predHash_.insert(std::make_pair(Signature(id, arity), (int)pred_.size()));
	if(res.second)
	{
		pred_.push_back(Signature(id, arity));
		output_->addSignature();
	}
	return res.first->second;
}

