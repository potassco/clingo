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
#include <gringo/index.h>

class PredIndex : public Index
{
private:
	struct ValCmp
	{
		ValCmp(const ValVec *v, uint32_t s);
		size_t operator()(uint32_t i) const;
		bool operator()(uint32_t a, uint32_t b) const;
		const ValVec *vals;
		uint32_t size;
	};
	typedef boost::unordered_map<uint32_t, uint32_t, ValCmp, ValCmp> IndexMap;
	typedef std::vector<uint32_t> BindSet;
	typedef std::vector<BindSet> BindSets;
public:
	PredIndex(const TermPtrVec &terms, const VarVec &index, const VarVec &bind);
	void bind(Grounder *grounder, int binder);
	bool extend(Grounder *grounder, ValVec::const_iterator vals);
	std::pair<bool, bool> firstMatch(Grounder *grounder, int binder);
	std::pair<bool, bool> nextMatch(Grounder *grounder, int binder);
	void reset();
	void finish();
	bool hasNew() const;
private:
	IndexMap map_;
	BindSets sets_;
	VarVec index_;
	VarVec bind_;
	ValVec indexVec_;
	ValVec bindVec_;
	BindSet::reverse_iterator current_;
	BindSet::reverse_iterator end_;
	uint32_t finished_;
	const TermPtrVec &terms_;
};

