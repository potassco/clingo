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

#include <gringo/value.h>
#include <gringo/gringoexception.h>
#include <gringo/globalstorage.h>

using namespace gringo;

Value::Value(Type type, int uid) : type_(type), uid_(uid)
{
}

Value::Value() : type_(UNDEF), uid_(0)
{
}

Value::Value(const Value &v) : type_(v.type_), uid_(v.uid_)
{
}

bool Value::equal(const Value &b) const
{
	assert(type_ != UNDEF && b.type_ != UNDEF);
	return type_ == b.type_ && uid_ == b.uid_;
}

int Value::compare(const GlobalStorage *g, const Value &b) const
{
	assert(type_ != UNDEF && b.type_ != UNDEF);
	if(type_ != b.type_) return int(type_) - b.type_;
	
	switch(type_)
	{
		case FUNCSYMBOL:
		case INT:
			return uid_ - b.uid_;
		case STRING:
			return g->getString(uid_)->compare(*g->getString(b.uid_));
		case UNDEF:
			FAIL(true);
	}
	FAIL(true);
}

int Value::toInt() const
{
	if(type_ == INT)
	{
		std::cerr << "return: " << uid_ << std::endl;
		return uid_;
	}
	else if(type_ == STRING)
	{
		throw GrinGoException("error trying to convert string to int");
	}
	else if(type_ == FUNCSYMBOL)
	{
		throw GrinGoException("error trying to convert functionsymbol to int");
	}
	FAIL(true);
	/*
	switch(type_)
	{
		case INT:
			return uid_;
		case STRING:
			throw GrinGoException("error trying to convert string to int");
		case FUNCSYMBOL:
			throw GrinGoException("error trying to convert functionsymbol to int");
		default:
			assert(false);
	}
	*/
}

Value::operator int() const
{
	if(type_ == INT)
	{
		return uid_;
	}
	else if(type_ == STRING)
	{
		throw GrinGoException("error trying to convert string to int");
	}
	else if(type_ == FUNCSYMBOL)
	{
		throw GrinGoException("error trying to convert functionsymbol to int");
	}
	FAIL(true);
	/*
	switch(type_)
	{
		case INT:
			return uid_;
		case STRING:
			throw GrinGoException("error trying to convert string to int");
		case FUNCSYMBOL:
			throw GrinGoException("error trying to convert functionsymbol to int");
		default:
			assert(false);
	}
	*/
}

void Value::print(const GlobalStorage *g, std::ostream &out) const
{
	switch(type_)
	{
		case Value::UNDEF:
			out << "undef";
			break;
		case Value::INT:
			out << uid_;
			break;
		case Value::STRING:
			out << *g->getString(uid_);
			break;
		case Value::FUNCSYMBOL:
			g->getFuncSymbol(uid_)->print(g, out);
			break;
	}
}

