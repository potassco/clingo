// 
// Copyright (c) 2006-2007, Benjamin Kaufmann
// 
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/ 
// 
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#ifndef CLASP_SATELITE_H_INCLUDED
#define CLASP_SATELITE_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/solver.h>
#include <clasp/util/indexed_priority_queue.h>

namespace Clasp { namespace SatElite {

class Clause; // Special clause class optimized for use with the preprocessor

//! SatElite preprocessor for clauses
/*!
 * \see Niklas Eén, Armin Biere: "Effective Preprocessing in SAT through Variable and Clause Elimination"
 * \note the SatElite preprocessor is adapted from MiniSAT 2.0 beta (Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson)
 */
class SatElite : public Clasp::SatPreprocessor {
public:
	explicit SatElite(Solver* s = 0);
	~SatElite();

	//! adds a clause to the preprocessor
	/*!
	 * \pre clause is not a tautology (i.e. does not contain both l and ~l)
	 * \pre clause is a set (i.e. does not contain l more than once)
	 * \return true if clause was added. False if adding the clause makes the problem UNSAT
	 */
	bool addClause(const LitVec& clause);

	//! Options used by the preprocessor
	struct Options {
		Options() : maxTime(uint32(-1)), maxIters(uint32(-1)), maxOcc(uint32(-1)), elimPure(true) {}
		uint32  maxTime;  /**< maximal runtime, checked after each iteration    */
		uint32  maxIters; /**< maximal number of iterations                     */
		uint32  maxOcc;   /**< skip v, if #occ(v) > maxOcc && #occ(~v) > maxOcc */
		bool    elimPure; /**< eliminate pure literals?                         */
	} options;

	bool preprocess();
	void extendModel( Assignment& m );
	void clearModel() { unconstr_.clear(); }
	bool hasSymModel() const { return !unconstr_.empty(); }
	void setEnumerate(bool b){ options.elimPure = !b; }
	typedef PodVector<Clause*>::type  ClauseList;
private:
	typedef PodVector<uint8>::type    TouchedList;
	SatElite(const SatElite&);
	const SatElite& operator=(const SatElite&);
	struct ElimData {
		typedef Clause** ClauseArray; 
		ElimData(Var v) : eliminated(0), var(v) {}
		ClauseArray eliminated; // null-terminated array of clauses containing var
		Var         var;        // eliminated var
	};
	typedef PodVector<ElimData>::type ElimList;
	// For each var v
	struct OccurList {
		OccurList() : pos(0), dirty(0), neg(0), litMark(0) {}
		LitVec  clauses;    // ids of clauses containing v or ~v  (var() == id, sign() == v or ~v)
		VarVec  watches;    // ids of clauses watching v or ~v (literal 0 is the watched literal)
		uint32  pos:31;     // number of *relevant* clauses containing v
		uint32  dirty:1;    // does clauses contain removed clauses?
		uint32  neg:30;     // number of *relevant* clauses containing v
		uint32  litMark:2;  // 00: no literal of v marked, 01: v marked, 10: ~v marked
		uint32  numOcc()          const { return pos + neg; }
		uint32  cost()            const { return pos * neg; }
		void    clear() {
			this->~OccurList();
			new (this) OccurList();
		}
		void    add(uint32 id, bool sign){
			pos += uint32(!sign);
			neg += uint32(sign);
			clauses.push_back(Literal(id, sign));
		}
		void    remove(uint32 id, bool sign, bool updateClauseList) {
			pos -= uint32(!sign);
			neg -= uint32(sign);
			if (updateClauseList) { 
				LitVec::iterator it = std::find(clauses.begin(), clauses.end(), Literal(id, sign));
				if (it != clauses.end()) { clauses.erase(it); }
			}
			else { dirty = 1; }
		}
		// note: only one literal of v shall be marked at a time
		bool    marked(bool sign) const   { return (litMark & (1+sign)) != 0; }
		void    mark(bool sign)           { litMark = (1+sign); }
		void    unmark()                  { litMark = 0; }
	};
	struct LessOccCost {
		explicit LessOccCost(OccurList*& occ) : occ_(occ) {}
		bool operator()(Var v1, Var v2) const { return occ_[v1].cost() < occ_[v2].cost(); }
	private:
		const LessOccCost& operator=(LessOccCost&);
		OccurList*& occ_;
	};
	typedef bk_lib::indexed_priority_queue<LessOccCost> ElimHeap;
	Clause*         peekSubQueue() const {
		assert(qFront_ < queue_.size());
		return clauses_[ queue_[qFront_] ];
	}
	inline Clause*  popSubQueue();
	inline void     addToSubQueue(uint32 clauseId);
	void            updateHeap(Var v) {
		assert(solver_);
		if (!solver_->frozen(v) && !solver_->eliminated(v)) {
			elimHeap_.update(v);
		}
	}
	inline uint32   findUnmarkedLit(const Clause& c, uint32 x) const;
	void    attach(uint32 cId, bool initialClause);
	void    detach(uint32 cId, bool destroy);
	bool    propagateFacts();
	bool    backwardSubsume();
	Literal subsumes(const Clause& c, const Clause& other, Literal res) const;
	bool    strengthenClause(uint32 clauseId, Literal p);
	bool    subsumed(LitVec& cl);
	bool    eliminateVar(Var v);
	bool    trivialResolvent(const Clause& c1, const Clause& c2, Var v) const;
	bool    addResolvent(const Clause& c1, const Clause& c2, Var v);

	uint32  freeClauseId();
	void    cleanUp();

	typedef std::pair<uint32, uint32> ElimPos;
	OccurList*    occurs_;    // occur list for each variable
	ElimList*     elimList_;  // stores eliminated vars and the clauses in which they occur
	LitVec        unconstr_;  // eliminated vars that are unconstraint w.r.t the current model
	ElimHeap      elimHeap_;  // candidates for variable elimination; ordered by increasing occurrence-cost
	ClauseList    clauses_;   // all clauses
	ClauseList  posT_, negT_; // temporary clause lists used in eliminateVar
	ClauseList    resCands_;  // pairs of clauses to be resolved
	LitVec        resolvent_; // temporary, used in addResolvent
	VarVec        queue_;     // indices of clauses waiting for subsumption-check
	ElimPos       elimPos_;   // position of eliminated clause whose id is to be reused next
	uint32        qFront_;    // front of queue_, i.e. [queue_.begin()+qFront_, queue.end()) is the subsumption queue
	uint32        facts_;     // [facts_, solver.trail.size()): new top-level facts
	uint32        clRemoved_; // number of clauses removed since last compaction
};
}}
#endif
