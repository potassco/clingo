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
#include <gringo/parser.h>
#include <gringo/groundprogrambuilder.h>
#include <gringo/lexer_impl.h>
#include <gringo/locateable.h>

class Converter : public LexerImpl, public GroundProgramBuilder
{
private:
	typedef std::pair<Loc,std::string> ErrorTok;
	typedef std::vector<ErrorTok> ErrorVec;
public:
	struct Token
	{
		Loc loc() const { return Loc(file, line, column); }
		uint32_t file;
		uint32_t line;
		uint32_t column;
		union
		{
			int      number;
			uint32_t index;
		};
	};

public:
	Converter(Output *output, Streams &streams);
	int lex();
	std::string errorToken();
	void syntaxError();
	void parseError();
	void parse();
	int  level() const { return level_; }
	void nextLevel() { level_ ++; }
	bool maximize() const { return maximize_; }
	void maximize(bool maximize) { maximize_ = maximize; }
	void addSigned(uint32_t index, bool sign);
	~Converter();

private:
	void parse(std::istream &sin);

private:
	Streams   &streams_;
	void      *parser_;
	Token      token_;
	bool       error_;
	ErrorVec   errors_;
	int        level_;
	bool       maximize_;
};

