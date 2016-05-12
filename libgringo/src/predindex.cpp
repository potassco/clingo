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

#include <gringo/predindex.h>
#include <gringo/domain.h>
#include <gringo/grounder.h>
#include <gringo/term.h>

PredIndex::ValCmp::ValCmp(const ValVec *v, uint32_t s) :
	vals(v),
	size(s)
{
}

size_t PredIndex::ValCmp::operator()(uint32_t i) const
{
	return boost::hash_range(vals->begin() + i, vals->begin() + i + size);
}

bool PredIndex::ValCmp::operator()(uint32_t a, uint32_t b) const
{
	return std::equal(vals->begin() + a, vals->begin() + a + size, vals->begin() + b);
}

PredIndex::PredIndex(const TermPtrVec &terms, const VarVec &index, const VarVec &bind) :
	map_(0, ValCmp(&indexVec_, index.size()), ValCmp(&indexVec_, index.size())),
	index_(index),
	bind_(bind),
	indexVec_(index.size()),
	finished_(0),
	terms_(terms)
{
}

bool PredIndex::extend(Grounder *grounder, ValVec::const_iterator vals)
{
	bool unified = true;
	foreach(const Term &term, terms_)
		if(!term.unify(grounder, *vals++, 0))
		{
			unified = false;
			break;
		}
	if(unified)
	{
		uint32_t find = indexVec_.size() - index_.size();
		ValVec::iterator k = indexVec_.begin() + find;
		foreach(uint32_t var, index_)
			*k++ = grounder->val(var);
		std::pair<IndexMap::iterator, bool> res = map_.insert(IndexMap::value_type(find, sets_.size()));
		if(res.second)
		{
			sets_.push_back(BindSet());
			indexVec_.resize(indexVec_.size() + index_.size());
		}
		sets_[res.first->second].push_back(bindVec_.size());
		// TODO: the easiest way is to push in dummy values here
		//       maybe there is a better way
		if(bind_.size() == 0)
			bindVec_.push_back(Val());
		else foreach(uint32_t var, bind_)
			bindVec_.push_back(grounder->val(var));
	}
	foreach(uint32_t var, index_)
		grounder->unbind(var);
	foreach(uint32_t var, bind_)
		grounder->unbind(var);
	return unified;
}

void PredIndex::bind(Grounder *grounder, int binder)
{
	ValVec::const_iterator j = bindVec_.begin() + *current_;
	foreach(uint32_t var, bind_)
		grounder->val(var, *j++, binder);
}

std::pair<bool, bool> PredIndex::firstMatch(Grounder *grounder, int binder)
{
	uint32_t find = indexVec_.size() - index_.size();
	ValVec::iterator j = indexVec_.begin() + find;
	foreach(uint32_t var, index_)
		*j++ = grounder->val(var);
	IndexMap::iterator k = map_.find(find);
	if(k != map_.end())
	{
		current_ = sets_[k->second].rbegin();
		end_     = sets_[k->second].rend();
		assert(current_ != end_);
		bind(grounder, binder);
		return std::make_pair(true, *current_ >= finished_);
	}
	else return std::make_pair(false, false);
}

std::pair<bool, bool> PredIndex::nextMatch(Grounder *grounder, int binder)
{
	current_++;
	if(current_ != end_)
	{
		bind(grounder, binder);
		return std::make_pair(true, *current_ >= finished_);
	}
	else return std::make_pair(false, false);
}

void PredIndex::reset()
{
	finished_ = 0;
}

void PredIndex::finish()
{
	finished_ = bindVec_.size();
}

bool PredIndex::hasNew() const 
{
	return finished_ < bindVec_.size();
}

