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

struct StrLoc
{
	StrLoc();
	StrLoc(Storage *sto, const Loc &loc);
	std::string file;
	uint32_t    line;
	uint32_t    column;
};

class UnsafeVarsException : public std::exception
{
private:
	typedef std::pair<StrLoc, std::string> UnsafeVar;
	typedef std::vector<UnsafeVar> UnsafeVars;
public:
	UnsafeVarsException(const StrLoc &loc, const std::string &str);
	void add(const StrLoc &loc, const std::string &name);
	const char *what() const throw();
	~UnsafeVarsException() throw() { }
private:
	const StrLoc        stmLoc_;
	const std::string   stmStr_;
	UnsafeVars          vars_;
	mutable std::string msg_;
};

class UnstratifiedException : public std::exception
{
public:
	UnstratifiedException(const StrLoc &stmLoc, const std::string &stmStr, const StrLoc &predLoc, const std::string &predStr);
	const char *what() const throw();
	~UnstratifiedException() throw() { }
private:
	const StrLoc        stmLoc_;
	const std::string   stmStr_;
	const StrLoc        predLoc_;
	const std::string   predStr_;
	mutable std::string msg_;
};

class ParseException : public std::exception
{
private:
	typedef std::pair<StrLoc, std::string> ErrorTok;
	typedef std::vector<ErrorTok> ErrorVec;
public:
	const char *what() const throw();
	void add(const StrLoc &loc, const std::string &tok);
	~ParseException() throw() { }
private:
	ErrorVec            errors_;
	mutable std::string msg_;
};

class FileException : public std::exception
{
public:
	FileException(const std::string &file);
	const char *what() const throw();
	~FileException() throw() { }
private:
	const std::string   file_;
	mutable std::string msg_;
};

class TypeException : public std::exception
{
public:
	TypeException(const std::string &desc, const StrLoc &loc, const std::string &str);
	const char *what() const throw();
	~TypeException() throw() { }
private:
	const std::string   desc_;
	StrLoc              loc_;
	std::string         str_;
	mutable std::string msg_;
};

template<class Stream>
Stream &operator<<(Stream &stream, const StrLoc &loc)
{
	stream << loc.file << ":" << loc.line << ":" << loc.column;
	return stream;
}
