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

#include <gringo/exceptions.h>
#include <gringo/storage.h>
#include <gringo/locateable.h>

StrLoc::StrLoc()
	: line(0)
	, column(0)
{
}

StrLoc::StrLoc(Storage *sto, const Loc &loc)
	: file(sto->string(loc.file))
	, line(loc.line)
	, column(loc.column)
{
}

UnsafeVarsException::UnsafeVarsException(const StrLoc &loc, const std::string &str)
	: stmLoc_(loc)
	, stmStr_(str)
{
}

void UnsafeVarsException::add(const StrLoc &loc, const std::string &name)
{
	vars_.push_back(UnsafeVar(loc, name));
}

const char *UnsafeVarsException::what() const throw()
{
	std::ostringstream oss;
	oss << "unsafe variables in:\n";
	oss << stmLoc_ << ": " << stmStr_ << "\n";
	foreach(const UnsafeVar &var, vars_)
		oss << "\t" << var.first << ": " << var.second << "\n";
	msg_ = oss.str();
	return msg_.c_str();
}

UnstratifiedException::UnstratifiedException(const StrLoc &stmLoc, const std::string &stmStr, const StrLoc &predLoc, const std::string &predStr)
	: stmLoc_(stmLoc)
	, stmStr_(stmStr)
	, predLoc_(predLoc)
	, predStr_(predStr)
{
}

const char *UnstratifiedException::what() const throw()
{
	std::ostringstream oss;
	oss << "unstratified predicate in:\n";
	oss << stmLoc_ << ": " << stmStr_ << "\n";
	oss << "\t" << predLoc_ << ": " << predStr_ << "\n";
	msg_ = oss.str();
	return msg_.c_str();
}

const char *ParseException::what() const throw() 
{ 
	std::ostringstream oss;
	oss << "parsing failed:\n";
	foreach(const ErrorTok &tok, errors_)
		oss << "\t" << tok.first << ": unexpected token: " << tok.second << "\n";
	msg_ = oss.str();
	return msg_.c_str();
}

void ParseException::add(const StrLoc &loc, const std::string &tok)
{
	errors_.push_back(ErrorTok(loc, tok));
}

FileException::FileException(const std::string &file)
	: file_(file)
{
}

const char *FileException::what() const throw() 
{ 
	std::ostringstream oss;
	oss << "could not open: " << file_ << "\n";
	msg_ = oss.str();
	return msg_.c_str();
}

TypeException::TypeException(const std::string &desc, const StrLoc &loc, const std::string &str)
	: desc_(desc)
	, loc_(loc)
	, str_(str)
{
}

const char *TypeException::what() const throw()
{
	std::ostringstream oss;
	oss << desc_ << " in:\n";
	oss << "\t" << loc_ << ": " << str_ << "\n";
	msg_ = oss.str();
	return msg_.c_str();
}

