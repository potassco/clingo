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

class Domain
{
public:
	struct Index
	{
		Index(uint32_t index, bool fact = false);
		uint32_t index : 31;
		mutable uint32_t fact : 1;
		bool valid() const;
		operator uint32_t() const { return index; }
	};
	typedef ValVec::iterator       iterator;
	typedef ValVec::const_iterator const_iterator;

private:
	class ArgSet
	{
	private:
		struct TupleCmp
		{
			TupleCmp(ArgSet *argSet);
			size_t operator()(const Index &i) const;
			bool operator()(const Index &a, const Index &b) const;
			ArgSet *argSet;
		};
		typedef boost::unordered_set<Index, TupleCmp, TupleCmp> ValSet;

	public:
		iterator       begin()       { return vals_.begin(); }
		const_iterator begin() const { return vals_.begin(); }
		iterator       end()         { return vals_.end(); }
		const_iterator end() const   { return vals_.end(); }
		uint32_t       size() const  { return valSet_.size(); }

		ArgSet(uint32_t arity);
		ArgSet(const ArgSet &argSet);
		const Index &find(const ValVec::const_iterator &v) const;
		void insert(const ValVec::const_iterator &v, bool fact = false);
		void extend(const ArgSet &other);

	private:
		uint32_t       arity_;
		mutable ValVec vals_;
		ValSet         valSet_;
	};

private:
	typedef std::map<int32_t, ArgSet> OffsetMap;
public:
	typedef std::pair<PredIndex*,Groundable*> PredInfo;
	typedef std::vector<PredInfo> PredInfoVec;
	typedef std::vector<PredIndex*> PredIndexVec;
	typedef std::vector<Groundable*> GroundableVec;
public:
	Domain(uint32_t nameId, uint32_t arity, uint32_t domId);
	const Index &find(const ValVec::const_iterator &v) const;
	void insert(Grounder *g, const ValVec::const_iterator &v, bool fact = false);
	void enqueue(Grounder *g);
	void append(Grounder *g, Groundable *gr, PredIndex *i);
	uint32_t size() const   { return vals_.size(); }
	bool complete() const   { return complete_ && !external_; }
	void complete(bool c)   { complete_ = c; }
	void external(bool e)   { external_ = e; }
	bool external() const   { return external_; }
	uint32_t arity() const  { return arity_; }
	uint32_t nameId() const { return nameId_; }
	uint32_t domId() const  { return domId_; }
	void addOffset(int32_t offset);
private:
	uint32_t       nameId_;
	uint32_t       arity_;
	uint32_t       domId_;
	ArgSet         vals_;
	OffsetMap      valsLarger_;
	PredInfoVec    index_;
	PredIndexVec   completeIndex_;
	uint32_t       new_;
	bool           complete_;
	bool           external_;
};

