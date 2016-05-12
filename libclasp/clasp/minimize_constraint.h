// 
// Copyright (c) 2010-2012, Benjamin Kaufmann
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
#ifndef CLASP_MINIMIZE_CONSTRAINT_H_INCLUDED
#define CLASP_MINIMIZE_CONSTRAINT_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include <clasp/constraint.h>
#include <clasp/util/atomic.h>
#include <clasp/util/misc_types.h>
#include <cassert>

namespace Clasp {
class MinimizeConstraint;
class WeightConstraint;

//! Supported minimization modes.
/*! 
 * Defines the possible minimization modes used during solving.
 * If optimize is used, a valid candidate model is a solution that is
 * strictly smaller than all previous solutions. Otherwise,
 * solutions with costs no greater than a fixed bound are considered valid.
 */
struct MinimizeMode_t {
	enum Mode {
		ignore    = 0, /**< Ignore optimize statements during solving. */
		optimize  = 1, /**< Optimize via a decreasing bound. */
		enumerate = 2, /**< Enumerate models with cost less or equal to a fixed bound. */
		enumOpt   = 3, /**< Enumerate models with cost equal to optimum. */
	};
	//! Strategy to use when optimization is active.
	enum Strategy {
		opt_bb = 0, /*!< branch and bound based optimization.   */
		opt_usc= 1, /*!< unsatisfiable-core based optimization. */
	};
	//! Options for branch and bound based optimization.
	enum BBOption {
		bb_step_def  = 0u, /*!< branch and bound with fixed step of size 1. */
		bb_step_hier = 1u, /*!< hierarchical branch and bound. */
		bb_step_inc  = 2u, /*!< hierarchical branch and bound with increasing steps. */
		bb_step_dec  = 3u, /*!< hierarchical branch and bound with decreasing steps. */
	};
	//! Options for unsatisfiable-core based optimization.
	enum UscOption {
		usc_preprocess = 1u, /*!< enable (disjoint) preprocessing. */
		usc_imp_only   = 2u, /*!< only add constraints for one direction (instead of eq). */
		usc_clauses    = 4u, /*!< only add clauses (instead of cardinality constraints).  */
	};
	enum Heuristic {
		heu_sign  = 1,  /*!< Use optimize statements in sign heuristic. */ 
		heu_model = 2,  /*!< Apply model heuristic when optimizing.     */
	};
	static bool supportsSplitting(Strategy s) { return s != opt_usc; }
};
typedef MinimizeMode_t::Mode MinimizeMode;

//! A type holding data (possibly) shared between a set of minimize constraints.
/*!
 * \ingroup shared
 */
class SharedMinimizeData {
public:
	typedef SharedMinimizeData ThisType;
	//! A type to represent a weight at a certain level.
	/*!
	 * Objects of this type are used to create sparse vectors of weights. E.g.
	 * a weight vector (w1@L1, w2@L3, w3@L5) is represented as [<L1,1,w1><L3,1,w2><L5,0,w3>], 
	 * where each <level, next, weight>-tuple is an object of type LevelWeight.
	 */
	struct LevelWeight {
		LevelWeight(uint32 l, weight_t w) : level(l), next(0), weight(w) {}
		uint32   level : 31; /**< The level of this weight. */
		uint32   next  :  1; /**< Does this weight belong to a sparse vector of weights? */
		weight_t weight;     /**< The weight at this level. */
	};
	//! A type for holding sparse vectors of level weights of a multi-level constraint.
	typedef PodVector<LevelWeight>::type WeightVec;
	explicit SharedMinimizeData(const SumVec& lhsAdjust, MinimizeMode m = MinimizeMode_t::optimize);
	//! Increases the reference count of this object.
	ThisType*      share()               { ++count_; return this; }
	//! Decreases the object's reference count and destroys it if reference count drops to 0.
	void           release()             { if (--count_ == 0) destroy(); }
	//! Number of minimize statements contained in this constraint.
	uint32         numRules()       const{ return static_cast<uint32>(adjust_.size()); }
	uint32         maxLevel()       const{ return numRules()-1; }
	static wsum_t  maxBound()            { return INT64_MAX; }
	//! Returns the active minimization mode.
	MinimizeMode   mode()           const{ return mode_; }
	//! Returns true if optimization is active.
	bool           optimize()       const{ return optGen_ ? checkNext() : mode_ != MinimizeMode_t::enumerate; }
	//! Returns the lower bound of level x.
	wsum_t         lower(uint32 x)  const{ return lower_[x]; }
	const wsum_t*  lower()          const{ return &lower_[0]; }
	//! Returns the upper bound of level x.
	wsum_t         upper(uint32 x)  const{ return upper()[x]; }
	const wsum_t*  upper()          const{ return &(up_ + (gCount_ & 1u))->front(); }
	//! Returns the sum of level x in the most recent model.
	wsum_t         sum(uint32 x)    const{ return sum()[x]; }
	const wsum_t*  sum()            const{ return (mode_ != MinimizeMode_t::enumerate) ? upper() : &up_[1][0]; }
	//! Returns the adjustment for level x.
	wsum_t         adjust(uint32 x) const{ return adjust_[x]; }
	const wsum_t*  adjust()         const{ return &adjust_[0]; }
	//! Returns the current (ajusted and possibly tentative) optimum for level x.
	wsum_t         optimum(uint32 x)const{ return adjust(x) + sum(x); }
	//! Returns the highest level of the literal with the given index i.
	uint32         level(uint32 i)  const{ return numRules() == 1 ? 0 : weights[lits[i].second].level; }
	//! Returns the most important weight of the literal with the given index i.
	weight_t       weight(uint32 i) const{ return numRules() == 1 ? lits[i].second : weights[lits[i].second].weight; }
	uint32         generation()     const{ return gCount_; }
	//! Returns whether minimization should search for solutions with current or next smaller upper bound.
	bool           checkNext()      const{ return mode() != MinimizeMode_t::enumerate && generation() != optGen_; }
	/*!
	 * \name interface for optimization
	 * The following functions shall not be called concurrently.
	 */
	//@{
	
	//! Sets the enumeration mode and (optionally) an initial bound.
	/*!
	 * \note If m is MinimizeMode::enumerate, the caller should always
	 * set a bound. Otherwise, *all* solutions are considered valid.
	 */
	bool setMode(MinimizeMode m, const wsum_t* bound = 0, uint32 boundSize = 0);
	bool setMode(MinimizeMode m, const SumVec& bound) { return setMode(m, bound.empty() ? 0 : &bound[0], (uint32)bound.size()); }
	void resetBounds();
	
	//! Attaches a new minimize constraint to this data object.
	/*!
	 * \param strat  The optimization strategy to use (see MinimizeMode_t::Strategy).
	 * \param param  Parameter to pass to the optimization strategy.
	 * \param addRef If true, the ref count of the shared object is increased. 
	 *               Otherwise, the new minimize constraint inherits the reference to the shared object.
	 */
	MinimizeConstraint* attach(Solver& s, MinimizeMode_t::Strategy strat, uint32 param = 0, bool addRef = true);
	
	//! Makes opt the new (tentative) optimum.
	/*!
	 * \pre opt is a pointer to an array of size numRules()
	 */
	const SumVec* setOptimum(const wsum_t* opt);
	//! Sets the lower bound of level lev to low.
	void          setLower(uint32 lev, wsum_t low);
	//! Marks the current tentative optimum as the final optimum.
	/*!
	 * \note Once a final optimum is set, further calls to setOptimum()
	 * are ignored until resetBounds() is called.
	 */
	void          markOptimal();
	//@}

	/*!
	 * \name Arithmetic functions on weights.
	 */
	//@{
	//! Computes lhs += weight(lit).
	void add(wsum_t* lhs, const WeightLiteral& lit) const { if (weights.empty()) *lhs += lit.second; else add(lhs, &weights[lit.second]); }
	void add(wsum_t* lhs, const LevelWeight* w)     const { do { lhs[w->level] += w->weight; } while (w++->next); }
	//! Computes lhs -= weight(lit).
	void sub(wsum_t* lhs, const WeightLiteral& lit, uint32& aLev) const { if (weights.empty()) *lhs -= lit.second; else sub(lhs, &weights[lit.second], aLev); }
	void sub(wsum_t* lhs, const LevelWeight* w, uint32& aLev)     const;
	//! Returns (lhs + weight(lit)) > rhs
	bool imp(wsum_t* lhs, const WeightLiteral& lit, const wsum_t* rhs, uint32& lev) const {
		return weights.empty() ? (*lhs+lit.second) > *rhs : imp(lhs, &weights[lit.second],  rhs, lev);
	}
	bool imp(wsum_t* lhs, const LevelWeight* w, const wsum_t* rhs, uint32& lev) const;
	//! Returns the weight of lit at level lev.
	weight_t weight(const WeightLiteral& lit, uint32 lev) const {
		if (numRules() == 1) { return lit.second * (lev == 0); }
		const LevelWeight* w = &weights[lit.second];
		do { if (w->level == lev) return w->weight; } while (w++->next);
		return 0;
	}
	//@}
private:
	typedef Clasp::atomic<uint32> Atomic;
	SumVec       adjust_;  // initial bound adjustments
	SumVec       lower_;   // (unadjusted) lower bound of constraint
	SumVec       up_[2];   // buffers for update via "double buffering"
	MinimizeMode mode_;    // how to compare assignments?
	Atomic       count_;   // number of refs to this object
	Atomic       gCount_;  // generation count - used when updating optimum
	uint32       optGen_;  // generation of optimal bound
public:
	WeightVec     weights; // sparse vectors of weights - only used for multi-level constraints
	WeightLiteral lits[0]; // (shared) literals - terminated with posLit(0)
private: 
	~SharedMinimizeData();
	void destroy() const;
	SharedMinimizeData(const SharedMinimizeData&);
	SharedMinimizeData& operator=(const SharedMinimizeData&);
};

//! Helper class for creating minimize constraints.
class MinimizeBuilder {
public:
	typedef SharedMinimizeData SharedData;
	MinimizeBuilder();
	~MinimizeBuilder();
	
	bool             hasRules() const { return !adjust_.empty(); }
	uint32           numRules() const { return (uint32)adjust_.size(); }
	uint32           numLits()  const { return (uint32)lits_.size(); }
	//! Adds a minimize statement.
	/*!
	 * \param lits the literals of the minimize statement
	 * \param adjustSum the initial sum of the minimize statement
	 */
	MinimizeBuilder& addRule(const WeightLitVec& lits, wsum_t adjustSum = 0);
	MinimizeBuilder& addLit(uint32 lev, WeightLiteral lit);
	
	//! Creates a new data object from previously added minimize statements.
	/*!
	 * The function creates a new minimize data object from 
	 * the previously added minimize statements. The returned
	 * object can be used to attach one or more MinimizeConstraints.
	 * \param ctx A ctx object used to simplify minimize statements.
	 * \return a new data object representing previously added minimize statements
	 *  or 0 if minimize statements are initially inconsistent!
	 */
	SharedData*      build(SharedContext& ctx);
	void clear();
private:
	struct Weight {
		Weight(uint32 lev, weight_t w) : level(lev), weight(w), next(0) {}
		uint32   level;
		weight_t weight;
		Weight*  next;
		static void free(Weight*& w);
	};
	typedef std::pair<Literal, Weight*> LitRep;
	typedef PodVector<LitRep>::type     LitRepVec;
	struct CmpByLit {
		bool operator()(const LitRep& lhs, const LitRep& rhs) const;
	};
	struct CmpByWeight {
		bool operator()(const LitRep& lhs, const LitRep& rhs) const;
		int  compare   (const LitRep& lhs, const LitRep& rhs) const;
	};
	void     unfreeze();
	bool     prepare(SharedContext& ctx);
	void     addTo(LitRep l, SumVec& vec);
	void     mergeReduceWeight(LitRep& x, LitRep& by);
	weight_t addFlattened(SharedData::WeightVec& x, const Weight& w);
	bool     eqWeight(const SharedData::LevelWeight* lhs, const Weight& rhs);
	weight_t addLitImpl(uint32 lev, WeightLiteral lit) {
		if (lit.second > 0) { lits_.push_back(LitRep(lit.first, new Weight(lev, lit.second)));   return 0; }
		if (lit.second < 0) { lits_.push_back(LitRep(~lit.first, new Weight(lev, -lit.second))); return lit.second; }
		return 0;
	}
	LitRepVec lits_;  // all literals
	SumVec    adjust_;// lhs adjustments
	bool      ready_; // prepare was called
};

//! Base class for implementing (mulit-level) minimize statements.
/*!
 * \ingroup constraint
 * A solver contains at most one minimize constraint, but a minimize constraint
 * may represent several minimize statements. In that case, each minimize statement
 * has a unique level (starting at 0) and minimize statements with a lower level
 * have higher priority. Priorities are handled like in smodels, i.e. statements
 * with lower priority become relevant only if all statements with higher priority
 * currently have an optimal assignment.
 *
 * MinimizeConstraint supports two modes of operation: if mode is set to
 * optimize, solutions are considered optimal only if they are strictly smaller 
 * than previous solutions. Otherwise, if mode is set to enumerate a
 * solution is valid if it is not greater than the initially set optimum.
 * Example: 
 *  m0: {a, b}
 *  m1: {c, d}
 *  All models: {a, c,...}, {a, d,...} {b, c,...}, {b, d,...} {a, b,...} 
 *  Mode = optimize: {a, c, ...} (m0 = 1, m1 = 1}
 *  Mode = enumerate and initial opt=1,1: {a, c, ...}, {a, d,...}, {b, c,...}, {b, d,...} 
 * 
 */
class MinimizeConstraint : public Constraint {
public:
	typedef SharedMinimizeData       SharedData;
	typedef const SharedData*        SharedDataP;
	//! Returns a pointer to the shared representation of this constraint.
	SharedDataP shared()  const { return shared_; }	
	//! Attaches this object to the given solver.
	virtual bool attach(Solver& s)    = 0;
	//! Shall activate the minimize constraint by integrating bounds stored in the shared data object.
	virtual bool integrate(Solver& s) = 0;
	//! Shall relax this constraint (i.e. remove any bounds).
	/*!
	 * If reset is true, shall also remove search-path related state.
	 */
	virtual bool relax(Solver& s, bool reset) = 0;
	//! Shall commit the model in s to the shared data object.
	/*!
	 * The return value indicates whether the model is valid w.r.t the
	 * costs stored in the shared data object.
	 */
	virtual bool handleModel(Solver& s) = 0;
	//! Shall handle the unsatisfiable path in s.
	virtual bool handleUnsat(Solver& s, bool upShared, LitVec& restore) = 0;
	virtual bool supportsSplitting() const { return true; }
	// base interface
	void         destroy(Solver*, bool);
	Constraint*  cloneAttach(Solver&) { return 0; }
protected:
	MinimizeConstraint(SharedData* s);
	~MinimizeConstraint();
	bool prepare(Solver& s, bool useTag);
	SharedData* shared_; // common shared data
	Literal     tag_;    // (optional) literal for tagging reasons
};

//! Minimization via branch and bound.
/*!
 * The class supports both basic branch and bound as well as
 * hierarchical branch and bound (with or without varying step sizes).
 */
class DefaultMinimize : public MinimizeConstraint {
public:
	explicit DefaultMinimize(SharedData* d, uint32 strat);
	// base interface
	//! Attaches the constraint to the given solver.
	/*!
	 * \pre s.decisionLevel() == 0
	 * \note If either MinimizeMode_t::enumOpt or hierarchical optimization
	 * is active, s.sharedContext()->tagLiteral() shall be an unassigned literal.
	 */
	bool       attach(Solver& s);
	bool       integrate(Solver& s)           { return integrateBound(s); }
	bool       relax(Solver&, bool reset)     { return relaxBound(reset); }
	bool       handleModel(Solver& s)         { commitUpperBound(s); return true; }
	bool       handleUnsat(Solver& s, bool up, LitVec& out);
	// constraint interface
	PropResult propagate(Solver& s, Literal p, uint32& data);
	void       undoLevel(Solver& s);
	void       reason(Solver& s, Literal p, LitVec& lits);
	bool       minimize(Solver& s, Literal p, CCMinRecursive* r);
	void       destroy(Solver*, bool);
	// own interface
	bool       active()  const { return *opt() != SharedData::maxBound(); }
	//! Number of minimize statements contained in this constraint.
	uint32     numRules()const { return size_; }
	//! Tries to integrate the next tentative bound into this constraint.
	/*!
	 * Starting from the current optimum stored in the shared data object,
	 * the function tries to integrate the next candidate bound into
	 * this constraint.
	 * 
	 * \return The function returns true if integration succeeded. Otherwise
	 * false is returned and s.hasConflict() is true.
	 *
	 * \note If integrateBound() failed, the bound of this constraint
	 *       is relaxed. The caller has to resolve the conflict first
	 *       and then integrateBound() shall be called again. 
	 *
	 * \note The caller has to call s.propagate() to propagate any new information
	 *       from the new bound.
	 *
	 * \note If the tag literal (if any) is not true, the minimize constraint first assumes it.
	 */
	bool       integrateBound(Solver& s);

	//! Sets the current local sum as the global optimum (upper bound).
	/*!
	 * commitUpperBound() shall be called whenever the solver finds a model.
	 * The current local sum is recorded as new optimum in the shared data object.
	 * Once the local bound is committed, the function integrateBound() has to be 
	 * called in order to continue optimization.
	 */
	void       commitUpperBound(const Solver& s);
	//! Sets the current local upper bound as the lower bound of this constraint.
	/*!
	 * commitLowerBound() shall be called on unsat. The function stores
	 * the local upper bound as new lower bound in this constraint. If upShared is true,
	 * the lower bound is also copied to the shared data object.
	 *
	 * Once the local bound is committed, the function integrateBound() has to be 
	 * called in order to continue optimization.
	 * \return false if search-space is exceeded w.r.t this constraint.
	 */
	bool       commitLowerBound(const Solver& s, bool upShared);
	
	//! Removes the local upper bound of this constraint and therefore disables it.
	/*!
	 * If full is true, also removes search-path related state.
	 */
	bool       relaxBound(bool full = false);

	bool       more() const { return step_.lev != size_; }

	// FOR TESTING ONLY!
	wsum_t sum(uint32 i, bool adjust) const { return sum()[i] + (adjust ? shared_->adjust(i):0); }
private:
	enum PropMode  { propagate_new_sum, propagate_new_opt };
	union UndoInfo;
	typedef const WeightLiteral* Iter;
	~DefaultMinimize();
	// bound operations
	wsum_t* opt() const { return bounds_; }
	wsum_t* sum() const { return bounds_ + size_; }
	wsum_t* temp()const { return bounds_ + (size_*2); }
	wsum_t* end() const { return bounds_ + (size_*3); }
	void    assign(wsum_t* lhs, wsum_t* rhs)  const;
	bool    greater(wsum_t* lhs, wsum_t* rhs, uint32 len, uint32& aLev) const;
	// propagation & undo
	uint32  lastUndoLevel(const Solver& s) const;
	bool    litSeen(uint32 i) const;
	bool    propagateImpl(Solver& s, PropMode m);
	uint32  computeImplicationSet(const Solver& s, const WeightLiteral& it, uint32& undoPos);
	void    pushUndo(Solver& s, uint32 litIdx);
	bool    updateBounds(bool applyStep);
	// step
	wsum_t& stepLow() const { return *(end() + step_.lev); }
	void    stepInit(uint32 n);
	wsum_t*      bounds_;  // [upper,sum,temp[,lower]]
	Iter         pos_;     // position of literal to look at next
	UndoInfo*    undo_;    // one "seen" flag for each literal +
	uint32       undoTop_; // undo stack holding assigned literals
	uint32       posTop_;  // stack of saved "look at" positions
	const uint32 size_;    // number of rules
	uint32       actLev_;  // first level to look at when comparing bounds
	struct Step {          // how to reduce next tentative bound
	uint32 size;           //   size of step
	uint32 lev : 30;       //   level on which step is applied
	uint32 type:  2;       //   type of step (one of MinimizeMode_t::BBOption)
	}            step_;
};

//! Minimization via unsat cores.
class UncoreMinimize : public MinimizeConstraint {
public:
	// constraint interface
	PropResult propagate(Solver& s, Literal p, uint32& data);
	void       reason(Solver& s, Literal p, LitVec& lits);
	void       destroy(Solver*, bool);
	bool       simplify(Solver& s, bool reinit = false);
	// base interface
	bool       attach(Solver& s);
	bool       integrate(Solver& s);
	bool       relax(Solver&, bool reset);
	bool       valid(Solver& s);
	bool       handleModel(Solver& s);
	bool       handleUnsat(Solver& s, bool up, LitVec& out);
	bool       supportsSplitting() const { return false; }
private:
	friend class SharedMinimizeData;
	explicit UncoreMinimize(SharedData* d, uint32 options = 0u);
	typedef DefaultMinimize* EnumPtr;
	struct LitData {
		LitData(weight_t w, bool as, uint32 c) : weight(w), coreId(c), assume((uint32)as) {}
		weight_t weight;
		uint32   coreId : 31;
		uint32   assume :  1;
	};
	struct LitPair {
		LitPair(Literal p, uint32 dataId) : lit(p), id(dataId) {}
		Literal lit;
		uint32  id;
	};
	struct Core {
		Core(WeightConstraint* c, weight_t b, weight_t w) : con(c), bound(b), weight(w) {}
		uint32  size()       const;
		Literal at(uint32 i) const;
		Literal tag()        const;
		WeightConstraint* con;
		weight_t          bound;
		weight_t          weight;
	};
	struct WCTemp {
		typedef WeightLitVec WLitVec;
		void     start(weight_t b){ lits.clear(); bound = b; }
		void     add(Solver& s, Literal p);
		bool     unsat() const { return bound > 0 && static_cast<uint32>(bound) > static_cast<uint32>(lits.size()); }
		weight_t bound;
		WLitVec  lits;
	};
	typedef PodVector<LitData>::type     LitTable;
	typedef PodVector<Core>::type        CoreTable;
	typedef PodVector<Constraint*>::type ConTable;
	typedef PodVector<LitPair>::type     LitSet;
	// literal and core management
	bool     hasCore(const LitData& x) const { return x.coreId != 0; }
	LitData& getData(uint32 id)              { return litData_[id-1];}
	Core&    getCore(const LitData& x)       { return open_[x.coreId-1]; }
	LitData& addLit(Literal p, weight_t w);
	void     releaseLits();
	bool     addCore(Solver& s, const LitPair* lits, uint32 size, weight_t weight);
	bool     addCore(Solver& s, const WCTemp& wc, weight_t w);
	bool     addClauses(Solver& s, const LitPair* lits, uint32 size, weight_t weight);
	bool     closeCore(Solver& s, LitData& x, bool sat);
	uint32   allocCore(WeightConstraint* con, weight_t bound, weight_t weight, bool open);
	enum CompType { comp_disj = 0, comp_conj = 1 };
	bool     add(CompType t, Solver& s, Literal head, Literal body1, Literal body2);
	// algorithm
	void     init();
	uint32   initRoot(Solver& s);
	bool     initLevel(Solver& s);
	uint32   analyze(Solver& s, LitVec& cfl, weight_t& minW, LitVec& poppedOther);
	bool     pushPath(Solver& s);
	void     integrateOpt(Solver& s);
	bool     popPath(Solver& s, uint32 dl, LitVec& out);
	bool     fixLit(Solver& s, Literal p);
	bool     fixLevel(Solver& s);
	void     detach(Solver* s, bool b);
	wsum_t*  computeSum(Solver& s) const;
	void     setLower(wsum_t x);
	bool     validLowerBound() const {
		wsum_t cmp = lower_ - upper_;
		return cmp < 0 || (cmp == 0 && level_ == shared_->maxLevel() && !shared_->checkNext());
	}
	// data
	EnumPtr   enum_;      // for supporting (optimal) model enumeration in parallel mode
	wsum_t*   sum_;       // costs of active model
	LitTable  litData_;   // data for active literals (tag lits for cores + lits from active minimize)
	CoreTable open_;      // open cores, i.e. relaxable and referenced by an assumption
	ConTable  closed_;    // closed cores represented as weight constraints
	LitSet    assume_;    // current set of assumptions
	LitSet    todo_;      // core(s) not yet represented as constraint
	LitVec    fix_;       // set of fixed literals
	LitVec    conflict_;  // current conflict
	WCTemp    temp_;      // temporary: used for creating weight constraints
	wsum_t    lower_;     // lower bound of active level
	wsum_t    upper_;     // upper bound of active level
	uint32    auxInit_;   // number of solver aux vars on attach
	uint32    auxAdd_;    // number of aux vars added for cores
	uint32    gen_;       // active generation
	uint32    level_ : 26;// active level
	uint32    valid_ :  1;// valid w.r.t active generation?
	uint32    sat_   :  1;// update because of model
	uint32    pre_   :  1;// preprocessing active?
	uint32    path_  :  1;// push path?
	uint32    next_  :  1;// assume next level?
	uint32    init_  :  1;// init constraint?
	uint32    eRoot_;     // saved root level of solver (initial gp)
	uint32    aTop_;      // saved assumption level (added by us)
	uint32    freeOpen_;  // head of open core free list
	uint32    options_;   // active options
};

} // end namespace Clasp

#endif
