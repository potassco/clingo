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
#include <gringo/output.h>

class ReifiedOutput : public Output
{
public:
	typedef std::vector<std::pair<size_t,Val> > List;
	typedef std::vector<size_t> Set;

private:
	class Node
	{
	public:
		Node(size_t symbol);
		size_t symbol() const;
		void visit(size_t visited);
		uint32_t visited() const;
		void mark();
		bool marked() const;
		void pop();
		bool popped() const;
		Node *next();
		bool root();

	public:
		std::vector<Node*> children;

	private:
		size_t   symbol_;
		uint32_t visited_;
		uint32_t finished_;
	};

	typedef std::vector<List> ListVec;
	typedef std::vector<Set>  SetVec;
	typedef boost::unordered_map<size_t, Node> Graph;
	typedef boost::unordered_map<Set, uint32_t> SetMap;
	typedef boost::unordered_map<List, uint32_t> ListMap;
	typedef boost::unordered_map<int, List> MiniMap;
	typedef boost::multi_index::multi_index_container
	<
		Val, boost::multi_index::indexed_by
		<
			boost::multi_index::random_access<>,
			boost::multi_index::hashed_unique<boost::multi_index::identity<Val> >
		>
	> SymbolMap;

public:
	ReifiedOutput(std::ostream *out);
	void startSet();
	void addToSet(size_t id);
	uint32_t addSet();
	Set &getSet();
	void popSet();
	void startList();
	List &getList();
	void popList();
	void addToList(size_t id, const Val &v);
	uint32_t addList();
	const Val &val(size_t symbol) const;
	size_t symbol(const Val &val);
	size_t symbol(PredLitRep *pred);
	void minimize(size_t sym, int weight, int prio);
	void finalize();
	void doShow(bool s);
	void doShow(uint32_t nameId, uint32_t arity, bool s);
	void pushDep(size_t n = 1);
	void addDep(size_t symbol, size_t n = 1);
	void popDep(bool add, size_t n = 1);
	void addCompute(PredLitRep *l);
	~ReifiedOutput();

private:
	void addDep();
	void tarjan();

private:
	SymbolMap symbols_;
	ListMap   lists_;
	SetMap    sets_;
	Graph     graph_;
	MiniMap   minimize_;
	ListVec   listStack_;
	SetVec    setStack_;
	SetVec    dep_;
};

inline ReifiedOutput::Set &ReifiedOutput::getSet()    { return setStack_.back(); }
inline void ReifiedOutput::popSet()                   { setStack_.pop_back(); }
inline ReifiedOutput::List &ReifiedOutput::getList()  { return listStack_.back(); }
inline void ReifiedOutput::popList()                  { listStack_.pop_back(); }
inline void ReifiedOutput::pushDep(size_t n)          { for(size_t i = 0; i < n; i++) { dep_.push_back(Set()); } }
inline void ReifiedOutput::addDep(size_t s, size_t n) { (dep_.end() - n)->push_back(s); }
