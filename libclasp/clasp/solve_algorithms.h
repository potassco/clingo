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
#ifndef CLASP_SOLVE_ALGORITHMS_H_INCLUDED
#define CLASP_SOLVE_ALGORITHMS_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include <clasp/literal.h>
#include <clasp/constraint.h>

/*!
 * \file 
 * Defines top-level functions for solving csp-problems.
 */
namespace Clasp { 

class Solver;
class MinimizeConstraint;
struct SearchLimits {
	int64 conflicts;
	int64 restarts;
	bool  update(uint64 cfl, uint64 r) {
		return (conflicts -= static_cast<int64>(cfl)) > 0
			&&   (restarts  -= static_cast<int64>(r))   > 0;
	}
};

//! Interface for enumerating models
class Enumerator : public Constraint {
public:
	class ProgressReport {
	public:
		ProgressReport();
		virtual ~ProgressReport();
		//! the solver is about to (re-)start
		virtual void reportRestart(const Solver& /* s */, uint64 /* maxCfl */, uint32 /* maxL */) {}
	private:
		ProgressReport(const ProgressReport&);
		ProgressReport& operator=(const ProgressReport&);
	};
	//! Interface used by the enumerator to report important events
	class Report : public ProgressReport {
	public:
		Report();
		//! the solver has found a new model
		virtual void reportModel(const Solver& /* s */, const Enumerator& /* self */) {}
		//! enumeration has terminated
		virtual void reportSolution(const Solver& /* s */, const Enumerator& /* self */, bool /* complete */) {}
	};
	explicit Enumerator(Report* r = 0);
	virtual ~Enumerator();

	enum { LIMIT_BIT = 7 };
	
	//! sets the report callback to be used during enumeration
	/*!
	 * \note Ownership is *not* transferred and r must be valid
	 * during complete search
	 */
	void setReport(Report* r);

	//! enables progress reporting via the given report callback
	void enableProgressReport(ProgressReport* r);

	//! if true, does a search-restart after each model found
	void setRestartOnModel(bool r);
	
	//! Sets the minimize constraint to be used during enumeration
	/*!
	 * \note Ownership is transferred.
	 */
	void setMinimize(MinimizeConstraint* min);
	const MinimizeConstraint* minimize() const { return mini_; }

	//! initializes the enumerator and sets the number of models to compute. 
	/*!
	 * Must be called once before search is started
	 * \note In the incremental setting, init must be called once for each incremental step
	 * \note nm = 0 means compute all models
	 */
	void init(Solver& s, uint64 nm);

	//! sets a search limit for the subsequent search
	/*!
	 * \param maxCfl maximum number of conflicts (<= 0: no limit)
	 * \param maxRestarts maximum number of restarts (<= 0: no limit)
	 */
	void setSearchLimit(int64 maxCfl, int64 maxRestarts);

	//! executes the next search iteration under the current search limit (if any)
	/*!
	 * The function first applies any active search limit to maxCfl,
	 * notifies the installed ProgressReport callback (if set), and then calls
	 * Solver::search() on the given solver with the given parameters.
	 *
	 * \return 
	 *   The result of Solver::search(). If the search limit has been reached, 
	 *   the returned value has the LIMIT_BIT set, i.e. test_bit(r, LIMIT_BIT)
	 *   is true.
	 * \note 
	 *   The function should be called in a loop until either enough models
	 *   have been enumerated, value_false is returned, or the search limit has been reached.
	 */
	ValueRep search(Solver& s, uint64 maxCfl, uint32 maxLearnts, double randFreq, bool localR);

	//! called whenever the solver has found a model.
	/*!
	 * The return value determines how search proceeds.
	 * If false is returned, the search is stopped.
	 * Otherwise the enumerator must have removed at least one
	 * decision level and the search continues from the new
	 * decision level.
	 * \pre The solver contains a model and DL = s.decisionLevel()
	 * \post If true is returned:
	 *  - s.decisionLevel() < DL and s.decisionLevel() >= s.rootLevel()
	 */
	bool backtrackFromModel(Solver& s);
	
	//! Shall be called once after search has stopped
	void endSearch(Solver& s, bool complete);
protected:
	//! Defaults to a noop
	PropResult propagate(const Literal&, uint32&, Solver&);
	//! Defaults to a noop
	ConstraintType reason(const Literal&, LitVec&);
	//! Defaults to return Constraint_t::native_constraint
	ConstraintType type() const;
	
	uint32 getHighestActiveLevel() const { return activeLevel_; }
	bool continueFromModel(Solver& s);
	
	virtual void doInit(Solver& s) = 0;
	virtual bool backtrack(Solver& s) = 0;
	virtual void updateModel(Solver& s);
	virtual bool ignoreSymmetric() const;
	virtual void terminateSearch(Solver& s); 
private:
	Enumerator(const Enumerator&);
	Enumerator& operator=(const Enumerator&);
	bool onModel(Solver& s);
	uint64              numModels_;
	Report*             report_;
	ProgressReport*     progress_;
	MinimizeConstraint* mini_;
	SearchLimits*       limits_;
	uint32              activeLevel_;
	bool                restartOnModel_;
};

///////////////////////////////////////////////////////////////////////////////
// Parameter-object for the solve function
// 
///////////////////////////////////////////////////////////////////////////////

//! Aggregates restart-parameters to configure restarts during search.
/*!
 * clasp currently supports four different restart-strategies:
 *  - fixed-interval restarts: restart every n conflicts
 *  - geometric-restarts: restart every n1 * n2^k conflict (k >= 0)
 *  - inner-outer-geometric: similar to geometric but sequence is repeated once bound outer is reached. Then, outer = outer*n2
 *  - luby's restarts: see: Luby et al. "Optimal speedup of las vegas algorithms."
 *  .
 */
class RestartParams {
private:
	double inc_;
	uint32 base_;
	uint32 outer_;
public:
	RestartParams() : inc_(1.5), base_(100), outer_(0), local(false), bounded(false), resetOnModel(false) {}
	double inc()  const { return inc_;   }
	uint32 base() const { return base_;  }
	uint32 outer()const { return outer_; }
	bool local;         /**< local restarts, i.e. restart if number of conflicts in *one* branch exceed threshold */
	bool bounded;       /**< allow (bounded) restarts after first solution was found */
	bool resetOnModel;  /**< repeat restart strategy after each solution */
	//! configure restart strategy
	/*!
	 * \param base    initial interval or run-length
	 * \param inc     grow factor
	 * \param outer   max restart interval, repeat sequence if reached
	 * \note
	 *  if base is equal to 0, restarts are disabled.
	 *  if inc is equal to 0, luby-restarts are used and base is interpreted as run-length
	 */
	void setStrategy(uint32 base, double inc, uint32 outer = 0) {
		base_    = base;
		outer_   = outer;
		if (inc >= 1.0 || inc == 0.0) {
			inc_   = inc;
		}
	}
};

//! Aggregates parameters for the nogood deletion heuristic used during search
class ReduceParams {
private:
	double frac_;
	double inc_;
	double max_;
	uint32 base_;
public:
	ReduceParams() : frac_(3.0), inc_(1.1), max_(3.0), base_(5000), reduceOnRestart(false), estimate(false), disable(false) {}
	
	//! sets the initial problem size used to compute initial db size
	void setProblemSize(uint32 base) { base_ = base; }

	double inc()  const { return inc_; }
	double frac() const { return frac_; }
	
	uint32 base() const { return static_cast<uint32>(base_/frac_); }
	uint32 max()  const { return static_cast<uint32>(std::min(base_*max_, double(std::numeric_limits<uint32>::max()))); }
	
	bool reduceOnRestart; /**< delete some nogoods on each restart */
	bool estimate;        /**< use estimate of problem complexity to init problem size */
	bool disable;         /**< do not delete any nogoods */
	
	//! configure reduce strategy
	/*!
	 * \param frac     init db size to problemSize/frac
	 * \param inc      grow factor applied after each restart
	 * \param maxF     stop growth once db size is > problemSize*maxF
	 */
	void setStrategy(double frac, double inc, double maxF) {
		frac_ = std::max(0.0001, frac);
		inc_  = std::max(1.0   , inc);
		max_  = std::max(0.0001, maxF);
	}
};

//! Parameter-Object for configuring search-parameters
/*!
 * \ingroup solver
 */
struct SolveParams {
	//! creates a default-initialized object.
	/*!
	 * The following parameters are used:
	 * restart      : quadratic: 100*1.5^k / no restarts after first solution
	 * shuffle      : disabled
	 * deletion     : initial size: vars()/3, grow factor: 1.1, max factor: 3.0, do not reduce on restart
	 * randomization: disabled
	 * randomProp   : 0.0 (disabled)
	 * enumerator   : no
	 */
	SolveParams();
	
	RestartParams restart;
	ReduceParams  reduce;
	
	//! Enumerator to use during search
	/*!
	 * \note Ownership is transferred
	 */
	void setEnumerator(Enumerator* e) { enumerator_.reset(e); }	

	//! sets the shuffle-parameters to use during search.
	/*!
	 * \param first   Shuffle program after first restarts
	 * \param next    Re-Shuffle program every next restarts
	 * \note
	 *  if first is equal to 0, shuffling is disabled.
	 */
	void setShuffleParams(uint32 first, uint32 next) {
		shuffleFirst_ = first;
		shuffleNext_  = next;
	}

	//! sets the randomization-parameters to use during search.
	/*!
	 * \param runs number of initial randomized-runs
	 * \param cfl number of conflicts comprising one randomized-run
	 */
	void setRandomizeParams(uint32 runs, uint32 cfls) {
		if (runs > 0 && cfls > 0) {
			randRuns_ = runs;
			randConflicts_ = cfls;
		}
	}

	//! sets the probability with which choices are made randomly instead of with the installed heuristic.
	void setRandomProbability(double p) {
		if (p >= 0.0 && p <= 1.0) {
			randFreq_ = p;
		}
	}
	double computeReduceBase(const Solver& s) const;

	// accessors
	uint32  randRuns()      const { return randRuns_; }
	uint32  randConflicts() const { return randConflicts_; }
	double  randomProbability() const { return randFreq_; }
	
	uint32      shuffleBase() const { return shuffleFirst_; }
	uint32      shuffleNext() const { return shuffleNext_; }
	Enumerator* enumerator()  const { return enumerator_.get(); }	
private:
	typedef std::auto_ptr<Enumerator> EnumPtr;
	double        randFreq_;
	EnumPtr       enumerator_;
	uint32        randRuns_;
	uint32        randConflicts_;
	uint32        shuffleFirst_;
	uint32        shuffleNext_;
};

//! Search without assumptions
/*!
 * \ingroup solver
 * \relates Solver
 * \param s The Solver containing the problem.
 * \param p solve parameters to use.
 *
 * \return
 *  - true: if the search stopped before the search-space was exceeded.
 *  - false: if the search-space was completely examined.
 * 
 */
bool solve(Solver& s, const SolveParams& p);

//! Search under a set of initial assumptions 
/*!
 * \ingroup solver
 * \relates Solver
 * The use of assumptions allows for incremental solving. Literals contained
 * in assumptions are assumed to be true during search but are undone before solve returns.
 *
 * \param s The Solver containing the problem.
 * \param assumptions list of initial unit-assumptions
 * \param p solve parameters to use.
 *
 * \return
 *  - true: if the search stopped before the search-space was exceeded.
 *  - false: if the search-space was completely examined.
 * 
 */
bool solve(Solver& s, const LitVec& assumptions, const SolveParams& p);

}
#endif
