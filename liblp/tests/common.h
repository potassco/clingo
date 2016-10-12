// 
// Copyright (c) 2015, Benjamin Kaufmann
// 
// This file is part of Potassco.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 
#ifndef LIBLP_TEST_COMMON_H_INCLUDED
#define LIBLP_TEST_COMMON_H_INCLUDED
#include <vector>
#include <potassco/basic_types.h>
namespace Potassco {
namespace Test {
template <class T>
using Vec = std::vector<T>;

struct Rule {
	Head_t           ht;
	Vec<Atom_t>      head;
	Body_t           bt;
	Weight_t         bnd;
	Vec<WeightLit_t> body;
	bool operator==(const Rule& rhs) const {
		return ht == rhs.ht && head == rhs.head && bt == rhs.bt && bnd == rhs.bnd && body == rhs.body;
	}
};
struct Edge {
	int s;
	int t;
	Vec<Lit_t> cond;
	bool operator==(const Edge& rhs) const {
		return s == rhs.s && t == rhs.t && cond == rhs.cond;
	}
};
struct Heuristic {
	Atom_t      atom;
	Heuristic_t type;
	int         bias;
	unsigned    prio;
	Vec<Lit_t>  cond;
	bool operator==(const Heuristic& rhs) const {
		return atom == rhs.atom && type == rhs.type && bias == rhs.bias && prio == rhs.prio && cond == rhs.cond;
	}
};

class ReadObserver : public AbstractProgram {
public:
	virtual void initProgram(bool inc) override {
		incremental = inc;
	}
	virtual void beginStep() override {
		++nStep;
	}
	virtual void endStep() override {}
	
	virtual void heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& cond) override {
		heuristics.push_back({a, t, bias, prio, {begin(cond), end(cond)}});
	}
	virtual void acycEdge(int s, int t, const LitSpan& cond) override {
		edges.push_back({s, t, {begin(cond), end(cond)}});
	}
	Vec<Heuristic> heuristics;
	Vec<Edge>      edges;
	int  nStep = 0;
	bool incremental = false;
};


}}

#endif
