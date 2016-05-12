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

#include <gringo/domain.h>
#include <gringo/val.h>
#include <gringo/predindex.h>
#include <gringo/grounder.h>
#include <gringo/groundable.h>
#include <gringo/instantiator.h>
#include <gringo/grounder.h>

Domain::ArgSet::TupleCmp::TupleCmp(ArgSet *argSet) :
	argSet(argSet)
{
}

size_t Domain::ArgSet::TupleCmp::operator()(const Index &i) const
{
	return boost::hash_range(argSet->vals_.begin() + i, argSet->vals_.begin() + i + argSet->arity_);
}

bool Domain::ArgSet::TupleCmp::operator()(const Index &a, const Index &b) const
{
	return std::equal(argSet->vals_.begin() + a, argSet->vals_.begin() + a + argSet->arity_, argSet->vals_.begin() + b);
}

namespace
{
	const Domain::Index invalid(std::numeric_limits<uint32_t>::max(), 0);
}

Domain::Index::Index(uint32_t index, bool fact) :
	index(index),
	fact(fact)
{
}

bool Domain::Index::valid() const
{
	return *this != invalid;
}

Domain::ArgSet::ArgSet(uint32_t arity)
	: arity_(arity)
	, valSet_(0, TupleCmp(this), TupleCmp(this))
{
}

Domain::ArgSet::ArgSet(const ArgSet &argSet)
	: arity_(argSet.arity_)
	, valSet_(0, TupleCmp(this), TupleCmp(this))
{
	extend(argSet);
}

const Domain::Index &Domain::ArgSet::find(const ValVec::const_iterator &v) const
{
	Index idx(vals_.size());
	vals_.insert(vals_.end(), v, v + arity_);
	ValSet::iterator i = valSet_.find(idx);
	vals_.resize(idx.index);
	if(i != valSet_.end()) return *i;
	else return invalid;
}

void Domain::ArgSet::insert(const ValVec::const_iterator &v, bool fact)
{
	Index idx(vals_.size(), fact);
	vals_.insert(vals_.end(), v, v + arity_);
	std::pair<ValSet::iterator, bool> res = valSet_.insert(idx);
	if(!res.second)
	{
		vals_.resize(idx.index);
		if(fact) res.first->fact = fact;
	}
}

void Domain::ArgSet::extend(const ArgSet &other)
{
	foreach(const Index &idx, other.valSet_)
	{
		insert(other.vals_.begin() + idx.index, idx.fact);
	}
}

Domain::Domain(uint32_t nameId, uint32_t arity, uint32_t domId)
	: nameId_(nameId)
	, arity_(arity)
	, domId_(domId)
	, vals_(arity)
	, new_(0)
	, complete_(false)
	, external_(false)
{
}

const Domain::Index &Domain::find(const ValVec::const_iterator &v) const
{
	return vals_.find(v);
}

void Domain::insert(Grounder *g, const ValVec::const_iterator &v, bool fact)
{
	int32_t offset;
	if(!g->termExpansion().limit(g, ValRng(v, v + arity_), offset))
	{
		vals_.insert(v, fact);
	}
	else
	{
		valsLarger_.insert( OffsetMap::value_type(offset, ArgSet(arity_)) ).first->second.insert(v, fact);
	}
}

void Domain::enqueue(Grounder *g)
{
	uint32_t end = vals_.size();
	foreach(const PredInfo &info, index_)
	{
		bool modified = false;
		ValVec::const_iterator k = vals_.begin() + arity_ * new_;
		for(uint32_t i = new_; i < vals_.size(); ++i, k+= arity_)
			modified = info.first->extend(g, k) || modified;
		if(modified) g->enqueue(info.second);
	}
	foreach(PredIndex *idx, completeIndex_)
	{
		ValVec::const_iterator k = vals_.begin() + arity_ * new_;
		for(uint32_t i = new_; i < vals_.size(); ++i, k+= arity_)
			idx->extend(g, k);
	}
	new_ = end;
}

void Domain::append(Grounder *g, Groundable *gr, PredIndex *idx)
{
	ValVec::const_iterator j = vals_.begin();
	for(uint32_t i = 0; i < new_; ++i, j+= arity_) idx->extend(g, j);
	assert(!complete_ || new_ == vals_.size());
	if(!complete_) { index_.push_back(PredInfo(idx, gr)); }
	else { completeIndex_.push_back(idx); }
}

void Domain::addOffset(int32_t offset)
{
	OffsetMap::iterator valIterator = valsLarger_.find(offset);
	if (valIterator != valsLarger_.end())
	{
		vals_.extend(valIterator->second);
		valsLarger_.erase(valIterator);
	}
}
