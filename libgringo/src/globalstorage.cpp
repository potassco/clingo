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

#include <gringo/globalstorage.h>
#include <gringo/domain.h>

using namespace gringo;

GlobalStorage::GlobalStorage()
{
}

Domain *GlobalStorage::getDomain(int uid) const
{
	return domains_[uid];
}

DomainVector *GlobalStorage::getDomains() const
{
	return const_cast<DomainVector*>(&domains_);
}

GlobalStorage::~GlobalStorage()
{
	stringHash_.clear();
	funcHash_.clear();
	for(StringVector::iterator it = strings_.begin(); it != strings_.end(); it++)
		delete *it;
	for(FuncSymbolVector::iterator it = funcs_.begin(); it != funcs_.end(); it++)
		delete *it;
	for(DomainVector::iterator it = domains_.begin(); it != domains_.end(); it++)
		delete *it;
}

const std::string *GlobalStorage::getString(int uid) const
{
	return strings_[uid];
}

const FuncSymbol  *GlobalStorage::getFuncSymbol(int uid) const
{
	return funcs_[uid];
}

int GlobalStorage::createString(const std::string &s2)
{
	return createString(new std::string(s2));
}

int GlobalStorage::createString(std::string *s)
{
	std::pair<StringHash::iterator, bool> res = stringHash_.insert(std::make_pair(s, strings_.size()));
	if(!res.second)
		delete s;
	else
		strings_.push_back(s);
	return res.first->second;
}

int GlobalStorage::createFuncSymbol(FuncSymbol* fn)
{
	std::pair<FuncSymbolHash::iterator, bool> res = funcHash_.insert(std::make_pair(fn, funcs_.size()));
	if (!res.second)
		delete fn;
	else
		funcs_.push_back(fn);
	return res.first->second;
}

int GlobalStorage::createPred(int id, int arity)
{
	std::pair<SignatureHash::iterator, bool> res = predHash_.insert(std::make_pair(Signature(id, arity), (int)pred_.size()));
	if(res.second)
	{
		pred_.push_back(Signature(id, arity));
		domains_.push_back(new Domain());
	}
	return res.first->second;
}

SignatureVector *GlobalStorage::getPred()
{
	return &pred_;
}

