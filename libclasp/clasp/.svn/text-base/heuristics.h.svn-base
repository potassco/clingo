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
#ifndef CLASP_HEURISTICS_H_INCLUDED
#define CLASP_HEURISTICS_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

/*!
 * \file 
 * Defines various decision heuristics to be used in clasp.
 * 
 */

#include <clasp/solver.h>
#include <clasp/pod_vector.h>
#include <clasp/util/indexed_priority_queue.h>
#include <list>
namespace Clasp { 

// Some lookback heuristics to be used together with learning.

//! computes a moms-like score for var v
uint32 momsScore(const Solver& s, Var v);

//! A variant of the BerkMin decision heuristic from the BerkMin Sat-Solver
/*!
 * \ingroup heuristic
 * \see E. Goldberg, Y. Navikov: "BerkMin: a Fast and Robust Sat-Solver"
 *
 * \note
 * This version of the BerkMin heuristic varies mostly in three points from the original BerkMin:
 *  -# it considers loop-nogoods if requested (this is the default)
 *  -# it uses a MOMS-like heuristic as long as there are no conflicts (and therefore no activities)
 *  -# it uses a MOMS-like score to break ties whenever multiple variables from an unsatisfied learnt
 * constraint have equal activities
 * 
 * The algorithm is as follows:
 * - For each variable a score s is maintained that is initially 0.
 * - During conflict analysis: For each variable v contained in a constraint CL that
 * participates in the derivation of the asserting clause, s(v) is incremented.
 * - Every 512 decisions, the score of each variable is divided by 4
 *
 * For selecting a literl DL the following algorithm is used:
 * - let CL be the most recently derived conflict clause that is currently not satisfied.
 * - if no such CL exists and look-back loops is enabled let CL be last learnt loop nogood that is currently not satisfied.
 * - if a CL was found:
 *    - let v be a free variable of CL with the highest activity. Break ties using MOMS
 *    - let p be the literal of v that occured more often in learnt nogoods. 
 *      - If both lits of v occurred equally often, set p to the preferred literal of v
 *    - set DL to p
 * - if no CL was found and #conflicts != 0
 *    - let v be the free variable with the highest activity. Break ties randomly.
 *    - let p and ~p be the two literals of v.
 *    - let s1 be the result of estimateBCP(p), let s2 be the result of estimateBCP(~p)
 *    - set DL to p iff s1 > s2, to ~p iff s2 > s1 and to the preferred decision literal of v otherwise.
 * - if #conflicts == 0
 *    - Select DL according to a MOMS-like heuristic
 * - return DL;
 * .
 */
class ClaspBerkmin : public DecisionHeuristic {
public:
	/*!
	 * \param maxBerk Check at most maxBerk candidates when searching for not yet satisfied learnt constraints
	 * \param considerLoops true if learnt loop-formulas should be considered during decision making
	 * \param initMoms use MOMS-like scoring for top-level choices (no conflict information yet)
	 * \param huangScore use Huang's scoring scheme (see: Jinbo Huang: "A Case for Simple SAT Solvers")
	 * \note maxBerk = 0 means check *all* candidates
	 */
	explicit ClaspBerkmin(uint32 maxBerk = 0, bool considerLoops = true, bool initMoms = true, bool huangScore = false);
	//! initializes the heuristic.
	void startInit(const Solver& s);
	void endInit(Solver& s);
	void reinit(bool b) { order_.reinit = b; }
	//! updates occurrence-counters if constraint is a learnt constraint.
	void newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t);
	//! updates activity-counters
	void updateReason(const Solver& s, const LitVec& lits, Literal resolveLit);
	void undoUntil(const Solver&, LitVec::size_type);
	void resurrect(const Solver&, Var) {
		front_ = 1;
		cache_.clear();
		cacheFront_ = cache_.end();
	}
protected:
	Literal doSelect(Solver& s);
private:
	Literal selectLiteral(Solver& s, Var v, bool vsids);
	Literal selectRange(Solver& s, const Literal* first, const Literal* last);
	bool    initHuang() const { return order_.score[0].occ == 1; }
	void    initHuang(bool b) { order_.score[0].occ = b; }
	bool    hasActivities() const { return order_.score[0].act != 0; }
	void    hasActivities(bool b) { order_.score[0].act = b; }
	Var  getMostActiveFreeVar(const Solver& s);
	Var  getTopMoms(const Solver& s);
	bool hasTopUnsat(Solver& s);
	bool hasTopUnsat(Solver& s, uint32& maxIdx, uint32 minIdx, ConstraintType t);
	// Gathers heuristic information for one variable v.
	struct HScore {
		HScore(uint32 d = 0) :  occ(0), act(0), dec(uint16(d)) {}
		void incAct(uint32 gd, bool h, bool sign) {
			occ += h * (1 - (2*sign));
			decay(gd, h);
			++act;
		}
		void incOcc(bool sign) { occ += 1 - (2*sign); }
		uint32 decay(uint32 gd, bool h) {
			if (uint32 x = (gd-dec)) {
				// NOTE: shifts might overflow, i.e.
				// activity is actually shifted by x%32. 
				// We deliberately ignore this "logical inaccuracy"
				// and treat it as random noise ;)
				act >>= x;
				dec   = uint16(gd);
				occ  /= (1<<(x*h));
			}
			return act;
		}
		int32  occ;
		uint16 act;
		uint16 dec;
	};
	typedef PodVector<HScore>::type   Scores;
	typedef VarVec::iterator Pos;
	
	struct Order {
		Order(bool huang_a, bool moms_a, bool loops_a) : decay(0), huang(huang_a), moms(moms_a), loops(loops_a), reinit(true) {}
		struct Compare {
			explicit Compare(Order* o) : self(o) {}
			bool operator()(Var v1, Var v2) const {
				return self->decayedScore(v1) > self->decayedScore(v2)
					|| (self->score[v1].act == self->score[v2].act && v1 < v2);
			}
			Order* self; 
		};
		void    init(const Solver& s);
		uint32  decayedScore(Var v) { return score[v].decay(decay, huang); }
		int32   occ(Var v)   const  { return score[v].occ; }
		void    inc(Literal p)      { score[p.var()].incAct(decay, huang, p.sign()); }
		void    incOcc(Literal p)   {
			if (!huang)score[p.var()].incOcc(p.sign());
			else       score[p.var()].incAct(decay, true, p.sign());
		}
		int     compare(Var v1, Var v2) {
			return int(decayedScore(v1)) - int(decayedScore(v2));
		}
		void    resetDecay();
		Scores  score;        // For each var v score_[v] stores heuristic score of v
		uint32  decay;        // "global" decay counter. Increased every decP_ decisions
		bool    huang;        // Use Huang's scoring scheme
		bool    moms;         // use MOMS-score for top-level choices
		bool    loops;        // Consider loop nogoods when searching for learnt nogoods that are not sat
		bool    reinit;       // reinit score in incremental setting
	private:
		Order(const Order&);
		Order& operator=(const Order&);
	}       order_; 
	VarVec  cache_;         // Caches the most active variables
	LitVec  freeLits_;      // Stores free variables of the last learnt conflict clause that is not sat
	LitVec  freeLoopLits_;  // Stores free variables of the last learnt loop nogood that is not sat
	uint32  topConflict_;   // index into the array of learnt nogoods used when searching for conflict clauses that are not sat
	uint32  topLoop_;       // index into the array of learnt nogoods used when searching for loop nogoods that are not sat
	Var     front_;         // first variable whose truth-value is not already known - reset on backtracking
	Pos     cacheFront_;    // first unprocessed cache position - reset on backtracking
	uint32  cacheSize_;     // cache at most cacheSize_ variables
	uint32  numVsids_;      // number of consecutive vsids-based decisions
	uint32  maxBerkmin_;    // when searching for an open learnt constraint, check at most maxBerkmin_ candidates.
};

//! Variable Move To Front decision strategies inspired by Siege.
/*!
 * \ingroup heuristic
 * \see Lawrence Ryan: "Efficient Algorithms for Clause Learning SAT-Solvers"
 *
 * \note This implementation of VMTF differs from the original implementation in three points:
 *  - if considerLoops is true, it moves to the front a selection of variables from learnt loop nogoods
 *  - it measures variable activity by using a BerkMin-like score scheme
 *  - the initial order of the var list is determined using a MOMs-like score scheme
 */
class ClaspVmtf : public DecisionHeuristic {
public:
	/*!
	 * \param mtf The number of literals from constraints used during conflict resolution that are moved to the front
	 * \param considerLoops true if literals from learnt loop-formulas should be moved, too.
	 */
	explicit ClaspVmtf(LitVec::size_type mtf = 8, bool considerLoops = false);
	//! initializes the heuristic.
	void startInit(const Solver& s);
	void endInit(Solver&);
	void reinit(bool b) { reinit_ = b; }
	/*!
	 * updates occurrence-counters if constraint is not a native constraint. Moves active
	 * vars to the front of the variable list the constraint is a conflict-clause
	 * or a loop-formula and consider loops was set to true.
	 */
	void newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t);
	//! updates activity-counters
	void updateReason(const Solver& s, const LitVec& lits, Literal resolveLit);
	//! removes vars set were assigned on level 0 from the heuristic's var list
	void simplify(const Solver&, LitVec::size_type);
	void undoUntil(const Solver&, LitVec::size_type);
	void resurrect(const Solver&, Var v) {
		if (score_[v].pos_ == vars_.end()) { score_[v].pos_ = vars_.insert(vars_.end(), v); }
		else { front_ = vars_.begin(); }
	}
protected:
	Literal doSelect(Solver& s);
private:
	Literal selectRange(Solver& s, const Literal* first, const Literal* last);
	Literal getLiteral(const Solver& s, Var v) const;
	typedef std::list<Var> VarList;
	typedef VarList::iterator VarPos;
	struct VarInfo {
		VarInfo(VarPos it) : pos_(it), activity_(0), occ_(0), decay_(0) { }
		VarPos  pos_;       // position of var in var list
		uint32  activity_;  // activity of var - initially 0
		int32   occ_;       // which literal of var occurred more often in learnt constraints?
		uint32  decay_;     // counter for lazy decaying activity
		uint32& activity(uint32 globalDecay) {
			if (uint32 x = (globalDecay - decay_)) {
				activity_ >>= (x<<1);
				decay_ = globalDecay;
			}
			return activity_;
		}
	};
	typedef PodVector<VarInfo>::type Score;
	
	struct LessLevel {
		LessLevel(const Solver& s, const Score& sc) : s_(s), sc_(sc) {}
		bool operator()(Var v1, Var v2) const {
			return s_.level(v1) < s_.level(v2) 
				|| (s_.level(v1) == s_.level(v2) && sc_[v1].activity_ > sc_[v2].activity_);
		}
		bool operator()(Literal l1, Literal l2) const {
			return (*this)(l1.var(), l2.var());
		}
	private:
		LessLevel& operator=(const LessLevel&);
		const Solver& s_;
		const Score&  sc_;
		
	};
	Score       score_;     // For each var v score_[v] stores heuristic score of v
	VarList     vars_;      // List of possible choices, initially ordered by MOMs-like score
	VarVec      mtf_;       // Vars to be moved to the front of vars_
	VarPos      front_;     // Current front-position in var list - reset on backtracking 
	uint32      decay_;     // "global" decay counter. Increased every 512 decisions
	const LitVec::size_type MOVE_TO_FRONT;
	bool        loops_;     // Move MOVE_TO_FRONT/2 vars from loop nogoods to the front of vars 
	bool        reinit_;
};

//! A variable state independent decision heuristic favoring variables that were active in recent conflicts.
/*!
 * \ingroup heuristic
 * This heuristic combines ideas from VSIDS and BerkMin. Literal Selection works as
 * in VSIDS but var activities are increased as in BerkMin.
 *
 * \see M. W. Moskewicz, C. F. Madigan, Y. Zhao, L. Zhang, and S. Malik: 
 * "Chaff: Engineering an Efficient SAT Solver"
 * \see E. Goldberg, Y. Navikov: "BerkMin: a Fast and Robust Sat-Solver"
 * 
 * The heuristic works as follows:
 * - For each variable v an activity a(v) is maintained that is initially 0.
 * - For each literal p an occurrence counter c(p) is maintained that is initially 0.
 * - When a new constraint is learnt (conflict or loop), c(p) is incremented 
 * for each literal p in the newly learnt constraint.
 * - During conflict analysis: For each variable v contained in a constraint CL that
 * participates in the derivation of the asserting clause, a(v) is incremented.
 * - Periodically the activity of each variable is divided by a constant that is a power
 * of two (currently every 512 decision variable activity is dived by 4).
 * - When a decision is necessary a free variable with the highest activity is taken
 * and from this variable the literal that occured in more learnt clauses is picked.
 * If c(v) == c(~v) the preferred choice literal of v is selected (v for bodies, ~v for heads).
 */
class ClaspVsids : public DecisionHeuristic {
public:
	/*!
	 * \param considerLoops If true, activity counters are increased for variables that occure in loop-formulas
	 */
	explicit ClaspVsids(bool considerLoops = false);
	void startInit(const Solver& s);
	void endInit(Solver&);
	void reinit(bool b) { reinit_ = b; }
	void newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t);
	void updateReason(const Solver& s, const LitVec& lits, Literal resolveLit);
	void undoUntil(const Solver&, LitVec::size_type);
	void simplify(const Solver&, LitVec::size_type);
	void resurrect(const Solver&, Var v) {
		vars_.update(v);  
	}
protected:
	Literal doSelect(Solver& s);
private:
	Literal selectRange(Solver& s, const Literal* first, const Literal* last);
	void updateVarActivity(Var v) {
		if ( (score_[v].first += inc_) > 1e100 ) {
			for (LitVec::size_type i = 0; i != score_.size(); ++i) {
				score_[i].first *= 1e-100;
			}
			inc_ *= 1e-100;
		}
		if (vars_.is_in_queue(v)) {
			vars_.increase(v);
		}
	}
	// HScore.first: activity of a variable
	// HScore.second: occurrence counter for a variable's literal
	//  > 0: positive literal occurred more often
	//  < 0: negative literal occurred more often
	//  = 0: literals occurred equally often
	typedef std::pair<double, int32>  HScore;
	typedef PodVector<HScore>::type   Scores;
	struct GreaterActivity {
		explicit GreaterActivity(const Scores& s) : sc_(s) {}
		bool operator()(Var v1, Var v2) const {
			return sc_[v1].first > sc_[v2].first;
		}
	private:
		GreaterActivity& operator=(const GreaterActivity&);
		const Scores& sc_;
	};
	typedef bk_lib::indexed_priority_queue<GreaterActivity> VarOrder;
	Scores    score_;
	VarOrder  vars_;
	double    inc_;
	bool      scoreLoops_;
	bool      reinit_;
};

}
#endif
