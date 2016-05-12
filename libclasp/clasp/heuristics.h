// 
// Copyright (c) 2006-2010, Benjamin Kaufmann
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

//! Computes a moms-like score for var v.
uint32 momsScore(const Solver& s, Var v);

struct HeuParams {
	HeuParams() : initScore(0), otherScore(0), resScore(0) {}
	HeuParams& other(uint32 x)   { otherScore = static_cast<uint8>(x);    return *this; }
	HeuParams& init(uint32 moms) { initScore  = static_cast<uint8>(moms); return *this; }
	HeuParams& score(uint32 x)   { resScore   = static_cast<uint8>(x);    return *this; }
	uint8 initScore; // currently {no, moms}
	uint8 otherScore;// currently {no, loop, all, heu}
	uint8 resScore;  // currently {heu, minSet, litSet, multiset}
};

//! A variant of the BerkMin decision heuristic from the BerkMin Sat-Solver
/*!
 * \ingroup heuristic
 * \see E. Goldberg, Y. Navikov: "BerkMin: a Fast and Robust Sat-Solver"
 *
 * \note
 * This version of the BerkMin heuristic varies mostly in the following points from the original BerkMin:
 *  -# it considers loop-nogoods if requested (this is the default)
 *  -# it uses a MOMS-like heuristic as long as there are no conflicts (and therefore no activities)
 *  -# it uses a MOMS-like score to break ties whenever multiple variables from an unsatisfied learnt constraint have equal activities
 *  -# it uses a lazy decaying scheme that only touches active variables
 */
class ClaspBerkmin : public DecisionHeuristic {
public:
	/*!
	 * \param maxBerk Check at most maxBerk candidates when searching for not yet satisfied learnt constraints.
	 * \note maxBerk = 0 means check *all* candidates.
	 */
	explicit ClaspBerkmin(uint32 maxBerk = 0, const HeuParams& params = HeuParams(), bool berkHuang = false);
	void startInit(const Solver& s);
	void endInit(Solver& s);
	void newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t);
	void updateReason(const Solver& s, const LitVec& lits, Literal resolveLit);
	bool bump(const Solver& s, const WeightLitVec& lits, double adj);
	void undoUntil(const Solver&, LitVec::size_type);
	void updateVar(const Solver& s, Var v, uint32 n);
protected:
	Literal doSelect(Solver& s);
private:
	Literal selectLiteral(Solver& s, Var v, bool vsids) const;
	Literal selectRange(Solver& s, const Literal* first, const Literal* last);
	bool    initHuang() const { return order_.score[0].occ == 1; }
	void    initHuang(bool b) { order_.score[0].occ = b; }
	bool    hasActivities() const { return order_.score[0].act != 0; }
	void    hasActivities(bool b) { order_.score[0].act = b; }
	Var  getMostActiveFreeVar(const Solver& s);
	Var  getTopMoms(const Solver& s);
	bool hasTopUnsat(Solver& s);
	// Gathers heuristic information for one variable v.
	struct HScore {
		HScore(uint32 d = 0) :  occ(0), act(0), dec(uint16(d)) {}
		void incAct(uint32 gd, bool h, bool sign) {
			occ += int(h) * (1 - (2*int(sign)));
			decay(gd, h);
			++act;
		}
		void incOcc(bool sign) { occ += 1 - (2*int(sign)); }
		uint32 decay(uint32 gd, bool h) {
			if (uint32 x = (gd-dec)) {
				// NOTE: shifts might overflow, i.e.
				// activity is actually shifted by x%32. 
				// We deliberately ignore this "logical inaccuracy"
				// and treat it as random noise ;)
				act >>= x;
				dec   = uint16(gd);
				occ  /= (1<<(x*int(h)));
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
		explicit Order(bool scoreHuang, uint8 rScore) : decay(0), huang(scoreHuang), resScore(rScore) {}
		struct Compare {
			explicit Compare(Order* o) : self(o) {}
			bool operator()(Var v1, Var v2) const {
				return self->decayedScore(v1) > self->decayedScore(v2)
					|| (self->score[v1].act == self->score[v2].act && v1 < v2);
			}
			Order* self; 
		};
		uint32  decayedScore(Var v) { return score[v].decay(decay, huang); }
		int32   occ(Var v)   const  { return score[v].occ; }
		void    inc(Literal p)      { score[p.var()].incAct(decay, huang, p.sign()); }
		void    inc(Literal p, uint16 f) { if (f) { score[p.var()].decay(decay, huang); score[p.var()].act += f; } }
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
		bool    huang;        // Use Huang's scoring scheme (see: Jinbo Huang: "A Case for Simple SAT Solvers")
		uint8   resScore;
	private:
		Order(const Order&);
		Order& operator=(const Order&);
	}       order_; 
	VarVec  cache_;         // Caches the most active variables
	LitVec  freeLits_;      // Stores free variables of the last learnt conflict clause that is not sat
	LitVec  freeOtherLits_; // Stores free variables of the last other learnt nogood that is not sat
	uint32  topConflict_;   // Index into the array of learnt nogoods used when searching for conflict clauses that are not sat
	uint32  topOther_;      // Index into the array of learnt nogoods used when searching for other learnt nogoods that are not sat
	Var     front_;         // First variable whose truth-value is not already known - reset on backtracking
	Pos     cacheFront_;    // First unprocessed cache position - reset on backtracking
	uint32  cacheSize_;     // Cache at most cacheSize_ variables
	uint32  numVsids_;      // Number of consecutive vsids-based decisions
	uint32  maxBerkmin_;    // When searching for an open learnt constraint, check at most maxBerkmin_ candidates.
	TypeSet types_;         // When searching for an open learnt constraint, consider these types of nogoods.
	RNG     rng_;
};

//! Variable Move To Front decision strategies inspired by Siege.
/*!
 * \ingroup heuristic
 * \see Lawrence Ryan: "Efficient Algorithms for Clause Learning SAT-Solvers"
 *
 * \note This implementation of VMTF differs from the original implementation in three points:
 *  - it optionally moves to the front a selection of variables from learnt loop nogoods
 *  - it measures variable activity by using a BerkMin-like score scheme
 *  - the initial order of the var list is determined using a MOMs-like score scheme
 */
class ClaspVmtf : public DecisionHeuristic {
public:
	/*!
	 * \param mtf The number of literals from constraints used during conflict resolution that are moved to the front.
	 */
	explicit ClaspVmtf(LitVec::size_type mtf = 8, const HeuParams& params = HeuParams());
	void startInit(const Solver& s);
	void endInit(Solver&);
	void newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t);
	void updateReason(const Solver& s, const LitVec& lits, Literal resolveLit);
	bool bump(const Solver& s, const WeightLitVec& lits, double adj);
	void simplify(const Solver&, LitVec::size_type);
	void undoUntil(const Solver&, LitVec::size_type);
	void updateVar(const Solver& s, Var v, uint32 n);
protected:
	Literal doSelect(Solver& s);
private:
	Literal selectRange(Solver& s, const Literal* first, const Literal* last);
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
	Score   score_; // For each var v score_[v] stores heuristic score of v
	VarList vars_;  // List of possible choices, initially ordered by MOMs-like score
	VarVec  mtf_;   // Vars to be moved to the front of vars_
	VarPos  front_; // Current front-position in var list - reset on backtracking 
	uint32  decay_; // "Global" decay counter. Increased every 512 decisions
	TypeSet types_; // Type of nogoods to score during resolution
	uint32  scType_;// Type of scoring
	const LitVec::size_type MOVE_TO_FRONT;
};

//! Score type for VSIDS heuristic.
/*!
 * \see ClaspVsids
 */
struct VsidsScore {
	typedef VsidsScore SC;
	VsidsScore() : value(0.0) {}
	double get()                  const { return value; }
	bool   operator>(const SC& o) const { return value > o.value; }
	double inc(double f)                { return (value += f); }
	void   set(double f)                { value = f; }
	double value;
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
 * \note The implementation uses the exponential VSIDS scheme from MiniSAT.
 */
template <class ScoreType>
class ClaspVsids_t : public DecisionHeuristic {
public:
	/*!
	 * \param d Set initial inc-factor to 1.0/d.
	 */
	explicit ClaspVsids_t(double d = 0.95, const HeuParams& params = HeuParams());
	void startInit(const Solver& s);
	void endInit(Solver&);
	void newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t);
	void updateReason(const Solver& s, const LitVec& lits, Literal resolveLit);
	bool bump(const Solver& s, const WeightLitVec& lits, double adj);
	void undoUntil(const Solver&, LitVec::size_type);
	void simplify(const Solver&, LitVec::size_type);
	void updateVar(const Solver& s, Var v, uint32 n);
protected:
	Literal doSelect(Solver& s);
	virtual void initScores(Solver& s, bool moms);
	typedef typename PodVector<ScoreType>::type ScoreVec;
	typedef PodVector<int32>::type              OccVec;
	Literal selectRange(Solver& s, const Literal* first, const Literal* last);
	void updateVarActivity(Var v, double f = 1.0) {
		double old = score_[v].get(), n = score_[v].inc((inc_*f));
		if (n > 1e100) { normalize(); }
		if (vars_.is_in_queue(v)) {
			if (n >= old) { vars_.increase(v); }
			else          { vars_.decrease(v); }
		}
	}
	void incOcc(Literal p) { occ_[p.var()] += 1 - (int(p.sign()) << 1); }
	int  occ(Var v) const  { return occ_[v]; }
	void normalize();
	struct CmpScore {
		explicit CmpScore(const ScoreVec& s) : sc_(s) {}
		bool operator()(Var v1, Var v2) const { return sc_[v1] > sc_[v2]; }
	private:
		CmpScore& operator=(const CmpScore&);
		const ScoreVec& sc_;
	};
	typedef bk_lib::indexed_priority_queue<CmpScore> VarOrder;
	ScoreVec     score_;
	OccVec       occ_;
	VarOrder     vars_;
	const double decay_;
	double       inc_;
	TypeSet      types_;
	uint32       scType_;
};
typedef ClaspVsids_t<VsidsScore> ClaspVsids;

//! Score type for DomainHeuristic.
/*!
 * \see DomainHeuristic
 */
struct DomScore {
	typedef DomScore SC;
	DomScore() : value(0.0), level(0), factor(1), domKey(UINT32_MAX) {}
	double get()                  const { return value; }
	bool   operator>(const SC& o) const { return (level > o.level) || (level == o.level && value > o.value); }
	bool   isDom()                const { return domKey != UINT32_MAX; }
	void   setDom(uint32 key)           { domKey = key; }
	double inc(double f)                { return (value += (f*factor)); }
	void   set(double f)                { value = f; }
	double value;  // activity as in VSIDS
	int16  level;  // priority level
	int16  factor; // factor used when bumping activity
	uint32 domKey; // index into dom-table if dom var	
};

//! Domain modification for one literal.
struct DomEntry {
	typedef SymbolTable::symbol_type SymbolType;
	enum Modifier { mod_factor = 0, mod_level = 1, mod_sign = 2, mod_init = 3, mod_tf = 4 };
	struct Cmp { bool operator()(const SymbolType& lhs, const SymbolType& rhs) const; };
	struct Lookup : Cmp {
		using Cmp::operator ();
		bool operator()(const char* head, const SymbolType& domSym) const;
		bool operator()(const SymbolType& domSym, const char* head) const;
	};
	static bool isDomEntry(const SymbolType& sym);
	static bool isHeadOf(const char* head, const SymbolType& domSym);

	void init(const SymbolType& atomSym, const SymbolType& domSym);
	Literal lit;    // literal to which this modification applies
	Literal cond;   // (optional) condition that must be true for this modification to apply
	uint32  mod :30;// one of Modifier
	uint32  pref: 2;// pref value (in case of mod_sign or mod_tf)
	int16   value;  // value of modification
	uint16  prio;   // (optional) priority of modification
};

//! A VSIDS heuristic supporting additional domain knowledge.
/*!
 * \ingroup heuristic
 *
 * \see M. Gebser, B. Kaufmann, R. Otero, J. Romero, T. Schaub, P. Wanko:
 *      "Domain-specific Heuristics in Answer Set Programming",
 *      http://www.cs.uni-potsdam.de/wv/pdfformat/gekaotroscwa13a.pdf
 */
class DomainHeuristic : public ClaspVsids_t<DomScore>
                      , private Constraint {
public:
	typedef ClaspVsids_t<DomScore> BaseType;
	enum GlobalPreference { gpref_atom = 0, gpref_scc  = 1, gpref_hcc = 2, gpref_disj = 4, gpref_min  = 8, gpref_show = 16 };
	enum GlobalModifier   { gmod_none  = 0, gmod_level = 1, gmod_spos = 2, gmod_true  = 3, gmod_sneg  = 4, gmod_false = 5  };
	explicit DomainHeuristic(double d = 0.95, const HeuParams& params = HeuParams());
	~DomainHeuristic();
	void setDefaultMod(GlobalModifier mod, uint32 prefSet);
	virtual void startInit(const Solver& s);
	using BaseType::endInit;

	const DomScore& score(Var v) const { return score_[v]; }
protected:
	// base interface
	virtual Literal     doSelect(Solver& s);
	virtual void        initScores(Solver& s, bool moms);
	virtual void        detach(Solver& s);
	// Constraint interface
	virtual Constraint* cloneAttach(Solver&) { return 0; }
	virtual void        reason(Solver&, Literal, LitVec&){}
	virtual PropResult  propagate(Solver&, Literal, uint32&);
	virtual void        undoLevel(Solver& s);
private:
	struct DomAction {
		uint32 var:29; // dom var to be modified
		uint32 mod: 2; // modification to apply
		uint32 comp:1; // compound action?
		uint32 undo;   // next action in undo list
		int16  val;    // value to apply 
		uint16 prio;   // prio of modification
	};
	struct DomPrio {
		void clear() { prio[0] = prio[1] = prio[2] = prio[3] = 0; }
		uint16  operator[](unsigned i) const { return prio[i]; }
		uint16& operator[](unsigned i)       { return prio[i]; }
		uint16  prio[4];
	};
	struct Frame {
		Frame(uint32 lev, uint32 h) : dl(lev), head(h) {}
		uint32 dl;
		uint32 head;
	};
	typedef PodVector<DomAction>::type ActionVec;
	typedef PodVector<DomPrio>::type   PrioVec;
	typedef PodVector<Frame>::type     FrameVec;
	bool    addAction(Solver& s, const DomEntry& e, int16& init);
	void    addDefAction(Solver& s, Literal x, int16 levVal, uint32 domKey);
	void    pushUndo(Solver& s, uint32 actionId);
	void    applyAction(Solver& s, DomAction& act, uint16& oldPrio);
	uint16& prio(Var v, uint32 mod) { return prios_[score_[v].domKey][mod]; }
	LitVec*   domMin_;  // optional: domain literals to minimize
	PrioVec   prios_;   // priorities for domain vars
	ActionVec actions_; // dynamic modifications
	FrameVec  frames_;  // dynamic undo information
	uint32    symSize_; // offset into symbol table
	uint16    defMod_;  // default modifier
	uint16    defPref_; // default preferences
};
}
#endif
