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
#include <gringo/lexer_impl.h>
#include <gringo/locateable.h>
#include <gringo/optimize.h>
#include <gringo/domain.h>

class Parser : public LexerImpl
{
private:
	struct DomStm
	{
		DomStm(const Loc &loc, uint32_t idx, uint32_t id, const VarSigVec &vars);
		Loc       loc;
		uint32_t  idx;
		uint32_t  id;
		VarSigVec vars;
	};
	typedef std::pair<Loc,std::string> ErrorTok;
	typedef std::vector<ErrorTok> ErrorVec;
	typedef boost::ptr_map<uint32_t, Term> ConstMap;
	typedef std::list<DomStm> DomStmList;
	typedef std::auto_ptr<Statement> StatementPtr;

public:
	typedef std::multimap<uint32_t, DomStm*> DomStmMap;
	typedef boost::iterator_range<DomStmMap::iterator> DomStmRng;

public:
	struct Token
	{
		Loc loc() const { return Loc(file, line, column); }
		uint32_t file;
		uint32_t line;
		uint32_t column;
		union
		{
			int number;
			uint32_t index;
		};
	};
	enum iPart { IPART_BASE, IPART_CUMULATIVE, IPART_VOLATILE };

public:
	Parser(Grounder *g, IncConfig &config, Streams &streams, bool compat);
	int lex();
	int lex_compat();
	std::string errorToken();
	void syntaxError();
	void include(uint32_t filename);
	void parseError();
	void parse();
	int  level() const { return level_; }
	void nextLevel() { level_ ++; }
	bool maximize() const { return maximize_; }
	void maximize(bool maximize) { maximize_ = maximize; }
	void optimizeSet(bool set) { optimizeSet_ = set; if(set) optimizeUniques_.reset(new PredLitSet()); }
	void setUniques(Optimize *o) { if(optimizeSet_) o->uniques(optimizeUniques_); }
	void incremental(iPart part, uint32_t index = 0);
	void add(Statement *s);
	Term *term(Val::Type t, const Loc &loc, uint32_t index);
	Grounder *grounder() { return g_; }
	PredLit *predLit(const Loc &loc, uint32_t id, TermPtrVec &terms, bool sign);
	void constTerm(uint32_t index, Term *term);
	void domainStm(const Loc &loc, uint32_t id, const VarSigVec &vars);
	DomStmRng domainStm(uint32_t var);
	~Parser();

private:
	void parse(std::istream &sin, uint32_t filename);
	void include();
	void add();

private:
	Grounder       *g_;
	IncConfig      &config_;
	Streams        &streams_;
	void           *parser_;
	Token           token_;
	bool            error_;
	uint32_t        include_;
	ErrorVec        errors_;
	StatementPtrVec last_;
	// parsing optimize statements
	int          level_;
	bool         maximize_;
	bool         optimizeSet_;
	PredLitSetPtr optimizeUniques_;
	// parsing the incremental part
	iPart        iPart_;
	uint32_t     iId_;
	uint32_t     iVar_;
	Loc          iLoc_;
	bool         iAdded_;
	// parsing const directives
	ConstMap     constMap_;
	// parsing domain statements
	DomStmList   domStmList_;
	DomStmMap    domStmMap_;
	// Lua
	Loc          luaBegin_;
	bool         compat_;
};

