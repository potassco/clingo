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
#ifndef CLASP_SOLVER_H_INCLUDED
#define CLASP_SOLVER_H_INCLUDED
#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/solver_types.h>
#include <string>
#include <utility>

namespace Clasp { 
///////////////////////////////////////////////////////////////////////////////
// classes used by the Solver
///////////////////////////////////////////////////////////////////////////////
class   DecisionHeuristic;
class   PostPropagator;

//! Base class for preprocessors working on clauses only
class SatPreprocessor {
public:
	SatPreprocessor() { verbose_ = 0; }
	virtual ~SatPreprocessor();
	void setSolver(Solver& s) { solver_ = &s; }
	virtual bool addClause(const LitVec& cl) = 0;
	virtual bool preprocess() = 0;
	virtual void extendModel(Assignment& m) = 0;
	virtual void clearModel() = 0;
	virtual bool hasSymModel() const = 0;
	virtual void setEnumerate(bool b) = 0;
	enum State { pre_start, iter_start, iter_done, pre_done, pre_stopped };
	typedef void (*StateFun)(State, uint32);
	void setVerbose(StateFun v) { verbose_ = v; }
protected:
	Solver*  solver_;
	StateFun verbose_;  /**< print progress information? */
	void setState(State st, uint32 data) {
		if (verbose_) verbose_(st, data);
	}
private:
	SatPreprocessor(const SatPreprocessor&);
	SatPreprocessor& operator=(const SatPreprocessor&);
};
///////////////////////////////////////////////////////////////////////////////

/**
 * \defgroup solver Solver
 */
//@{

//! Parameter-Object for configuring a solver.
struct SolverStrategies {
	typedef std::auto_ptr<DecisionHeuristic>  Heuristic;
	typedef std::auto_ptr<SatPreprocessor>    SatPrePro;
	typedef std::auto_ptr<AtomIndex>          SymbolTable;
	//! Clasp's two general search strategies
	enum SearchStrategy {
		use_learning, /*!< Analyze conflicts and learn First-1-UIP-clause */
		no_learning   /*!< Don't analyze conflicts - chronological backtracking */
	};
	//! Antecedents to consider during conflict clause minimization
	enum CflMinAntes {
		no_antes              = 0,  /*!< Don't minimize first-uip-clauses */
		binary_antes          = 2,  /*!< Consider only binary antecedents         */
		binary_ternary_antes  = 6,  /*!< Consider binary- and ternary antecedents */
		all_antes             = 7   /*!< Consider all antecedents                 */
	};
	SatPrePro           satPrePro;
	Heuristic           heuristic;
	SymbolTable         symTab; 
	SearchStrategy      search;
	int                 saveProgress;
	CflMinAntes         cflMinAntes;
	bool                strengthenRecursive; /*!< If true, use more expensive recursive nogood minimization */
	bool                randomWatches;
	
	uint32              compress()    const { return compress_; }
	void setCompressionStrategy(uint32 len) {
		compress_ = len == 0 ? static_cast<uint32>(-1) : len;
	}
private:
	friend class Solver;
	SolverStrategies(const SolverStrategies&);
	SolverStrategies& operator=(const SolverStrategies&);
	//! creates a default-initialized object.
	SolverStrategies();
	uint32 compress_;
};

//! clasp's Solver class
/*!
 * A Solver-object maintains the state and provides the functions 
 * necessary to implement our CDNL-algorithm. 
 *
 * The interface supports incremental solving (search under assumptions) as well as
 * solution enumeration. To make this possible the solver maintains two special  
 * decision levels: the root-level and the backtrack-level.
 *
 * The root-level is the lowest decision level to which a search
 * can return. Conflicts on the root-level are non-resolvable and end a search - the
 * root-level therefore acts as an artificial top-level during search.
 * Incremental solving is then implemented by first adding a list of unit assumptions
 * and second initializing the root-level to the current decision level.
 * Once search terminates assumptions can be undone by calling clearAssumptions
 * and a new a search can be started using different assumptions.
 *
 * For model enumeration the solver maintains a backtrack-level which restricts
 * backjumping in order to prevent repeating already enumerated solutions.
 * The solver will never backjump above that level and conflicts on the backtrack-level 
 * are resolved by backtracking, i.e. flipping the corresponding decision literal.
 *
 * \see "Conflict-Driven Answer Set Enumeration" for a detailed description of this approach. 
 *
 */
class Solver {
public:
	/*!
	 * \name construction/destruction/setup
	 */
	//@{
	//! creates an empty solver object with all strategies set to their default value.
	Solver();
	
	//! destroys the solver object and all contained constraints.
	~Solver();
	
	//! resets a solver object to the state it had after default-construction.
	void reset();

	//! shuffle constraints upon next simplification
	void shuffleOnNextSimplify() { shuffle_ = true; }
	
	//! returns the strategies used by this solver-object. 
	SolverStrategies& strategies() { return strategy_; }

	/*!
	 * \overload SolverStrategies& Solver::strategies()
	 */
	const SolverStrategies& strategies() const { return strategy_; }
	//@}

	/*!
	 * \name Problem specification
	 * Functions for adding variables and constraints.
	 * \note Problem specification is a three-stage process:
	 * - First all variables must be added.
	 * - Second startAddConstraints must be called and problem constraints can be added.
	 * - Finally endAddConstraints must be called to finished problem-specification.
	 * .
	 * \note Problem constraints can only be added in stage two. Afterwards only learnt constraints
	 * may be added to the solver.
	 * \note This process can be repeated, i.e. if incremental solving is used,
	 * one can add new variables and problem constraints by calling
	 * addVar/startAddConstraints/endAddConstraints again after a search terminated.
	 */
	//@{
	
	//! reserve space for at least numVars variables
	/*!
	 * \note if the number of variables is known upfront, calling this method
	 * avoids repeated regrowing of the state data structures
	 */
	void reserveVars(uint32 numVars);  

	//! adds a new variable of type t to the solver.
	/*!
	 * \pre startAddConstraints was not yet called.
	 * \param t  type of the new variable (either Var_t::atom_var or Var_t::body_var)
	 * \param eq true if var represents both an atom and a body. In that case
	 *           t is the variable's primary type and determines the preferred literal.
	 * \return The index of the new variable
	 * \note The problem variables are numbered from 1 onwards!
	 */
	Var addVar(VarType t, bool eq = false);

	//! returns the type of variable v.
	/*!
	 * If v was added with parameter eq=true, the return value
	 * is Var_t::atom_body_var.
	 */
	VarType type(Var v) const {
		assert(validVar(v));
		return info_.isSet(v, VarInfo::EQ)
			? Var_t::atom_body_var
			: VarType(Var_t::atom_var + info_.isSet(v, VarInfo::BODY));
	}
	
	//! returns the preferred decision literal of variable v w.r.t its type
	/*!
	 * \return 
	 *  - posLit(v) if type(v) == body_var
	 *  - negLit(v) if type(v) == atom_var
	 * \note if type(v) is atom_body_var, the preferred literal is determined
	 *       by v's primary type, i.e. the one that was initially passed to addVar().
	 */
	Literal preferredLiteralByType(Var v) const {
		assert(validVar(v));
		return Literal(v, !info_.isSet(v, VarInfo::BODY));
	}
	//! returns true if v is currently eliminated, i.e. no longer part of the problem
	bool eliminated(Var v)  const     { assert(validVar(v)); return info_.isSet(v, VarInfo::ELIM); }
	//! returns true if v is excluded from variable elimination
	bool frozen(Var v)      const     { assert(validVar(v)); return info_.isSet(v, VarInfo::FROZEN); }
	bool project(Var v)     const     { assert(validVar(v)); return info_.isSet(v, VarInfo::PROJECT);}
	bool nant(Var v)        const     { assert(validVar(v)); return info_.isSet(v, VarInfo::NANT);}
	
	//! eliminates/ressurrects the variable v
	/*!
	 * \pre if elim is true: v must not occur in any constraint 
	 *  and frozen(v) == false and value(v) == value_free
	 */
	void eliminate(Var v, bool elim);
	void setFrozen(Var v, bool b)     { assert(validVar(v)); if (b != info_.isSet(v, VarInfo::FROZEN)) info_.toggle(v, VarInfo::FROZEN);   }
	void setProject(Var v, bool b)    { assert(validVar(v)); if (b != info_.isSet(v, VarInfo::PROJECT)) info_.toggle(v, VarInfo::PROJECT); }
	void setNant(Var v, bool b)       { assert(validVar(v)); if (b != info_.isSet(v, VarInfo::NANT)) info_.toggle(v, VarInfo::NANT);       }

	//! returns true if var represents a valid variable in this solver.
	/*!
	 * \note The range of valid variables is [1..numVars()]. The variable 0
	 * is a special sentinel variable that is always true. 
	 */
	bool validVar(Var var) const { return var <= numVars(); }

	//! Must be called once before constraints can be added.
	void startAddConstraints();
	
	//! adds the constraint c to the solver.
	/*!
	 * \pre endAddConstraints was not called.
	 */
	void add(Constraint* c) { constraints_.push_back(c); }

	//! adds the unary constraint p to the solver.
	/*!
	 * \note unary constraints are immediately asserted.
	 * \return false if asserting p leads to a conflict.
	 */
	bool addUnary(Literal p);

	//! adds the binary constraint (p, q) to the solver.
	/*!
	 * \pre if asserting is true, q must be false w.r.t the current assignment
	 * \return !asserting || force(p, Antecedent(~q));
	 */
	bool addBinary(Literal p, Literal q, bool asserting);
	
	//! adds the ternary constraint (p, q, r) to the solver.
	/*!
	 * \pre if asserting is true, q and r must be false w.r.t the current assignment
	 * \return !asserting || force(p, Antecedent(~q, ~r));
	 */
	bool addTernary(Literal p, Literal q, Literal r, bool asserting);

	//! adds p as post propagator to this solver
	/*!
	 * \pre p was not added previously and is not part of any other solver
	 * \note post propagators are stored in priority order
	 * \see PostPropagator::priority()
	 */
	void addPost(PostPropagator* p)    { post_.add(p); }
	
	//! removes p from the solver's list of post propagators
	/*!
	 * \note removePost(p) shall only be called during propagation
	 *       of p or if no propagation is currently active.
	 */
	void removePost(PostPropagator* p) { post_.remove(p); }

	//! initializes the solver
	/*!
	 * endAddConstraints must be called once before a search is started. After endAddConstraints
	 * was called learnt constraints may be added to the solver.
	 * \return 
	 *  - false if the constraints are initially conflicting. True otherwise.
	 * \note
	 * The solver can't recover from top-level conflicts, i.e. if endAddConstraints
	 * returned false, the solver is in an unusable state.
	 */
	bool endAddConstraints();

	//! estimates the problem complexity
	/*!
	 * \return sum of c->estimateComplexity(*this) for each problem 
	 *         constraint c.
	 */
	uint32 problemComplexity() const;

	//! adds the learnt constraint c to the solver.
	/*!
	 * \pre endAddConstraints was called.
	 */
	void addLearnt(LearntConstraint* c, uint32 size) {
		learnts_.push_back(c);
		stats.solve.addLearnt(size, c->type()); 
	}
	//@}
	/*!
	 * \name watch management
	 * Functions for setting/removing watches.
	 * \pre validVar(v)
	 */
	//@{
	//! returns the number of constraints watching the literal p
	uint32 numWatches(Literal p) const;
	//! returns true if the constraint c watches the literal p
	bool    hasWatch(Literal p, Constraint* c) const;
	//! returns c's watch-structure associated with p
	/*!
	 * \note returns 0, if hasWatch(p, c) == false
	 */
	Watch*  getWatch(Literal p, Constraint* c) const;

	//! Adds c to the watch-list of p.
	/*!
	 * When p becomes true, c->propagate(p, data, *this) is called.
	 * \post hasWatch(p, c) == true
	 */
	void addWatch(Literal p, Constraint* c, uint32 data = 0) {
		assert(validWatch(p));
		watches_[p.index()].push_back(Watch(c, data));
	}
	
	//! removes c from p's watch-list.
	/*!
	 * \post hasWatch(p, c) == false
	 */
	void removeWatch(const Literal& p, Constraint* c);

	//! adds c to the watch-list of decision-level dl
	/*!
	 * Constraints in the watch-list of a decision level are
	 * notified when that decision level is about to be backtracked.
	 * \pre dl != 0 && dl <= decisionLevel()
	 */
	void addUndoWatch(uint32 dl, Constraint* c) {
		assert(dl != 0 && dl <= decisionLevel() );
		if (levels_[dl-1].second != 0) {
			levels_[dl-1].second->push_back(c);
		}
		else {
			levels_[dl-1].second = allocUndo(c);
		}
	}
	
	//! removes c from the watch-list of the decision level dl
	void removeUndoWatch(uint32 dl, Constraint* c);

	void initSavedValue(Var v, ValueRep val) {
		assert(validVar(v));
		if (assign_.saved(v) == value_free) {
			assign_.setSavedValue(v, val);
		}
	}
	//@}

	/*!
	 * \name state inspection
	 * Functions for inspecting the state of the search.
	 * \note validVar(v) is a precondition for all functions that take a variable as 
	 * parameter.
	 */
	//@{
	//! returns the number of problem variables.
	/*!
	 * \note The special sentine-var 0 is not counted, i.e. numVars() returns
	 * the number of problem-variables.
	 * To iterate over all problem variables use a loop like:
	 * \code
	 * for (Var i = 1; i <= s.numVars(); ++i) {...}
	 * \endcode
	 */
	uint32 numVars() const { return info_.numVars() - 1; }
	//! returns the number of assigned variables.
	/*!
	 * \note The special sentinel-var 0 is not counted.
	 */
	uint32 numAssignedVars()  const { return assign_.assigned(); }
	
	//! returns the number of free variables.
	/*!
	 * The number of free variables is the number of vars that are neither
	 * assigned nor eliminated.
	 */
	uint32 numFreeVars()      const { return numVars()-(assign_.assigned()+eliminated_); }

	//! returns the number of eliminated vars
	uint32 numEliminatedVars()const { return eliminated_; }

	//! returns the number of initial problem constraints stored in the solver
	uint32 numConstraints()   const { return (uint32)constraints_.size() + binCons_ + ternCons_; }
	//! returns the number of binary constraints stored in the solver
	uint32 numBinaryConstraints() const { return binCons_; }
	//! returns the number of ternary constraints stored in the solver
	uint32 numTernaryConstraints() const { return ternCons_; }
	
	//! returns the number of constraints that are currently in the learnt database.
	/*!
	 * \note binary and ternary constraints are not counted as learnt constraint.
	 */
	uint32 numLearntConstraints() const { return (uint32)learnts_.size(); }
	
	//! returns the value of v w.r.t the current assignment
	ValueRep value(Var v) const {
		assert(validVar(v));
		return assign_.value(v);
	}
	//! returns the previous value of v
	ValueRep savedValue(Var v) const {
		assert(validVar(v));
		return assign_.saved(v);
	}
	//! returns true if p is true w.r.t the current assignment
	bool isTrue(Literal p) const {
		assert(validVar(p.var()));
		return assign_.value(p.var()) == trueValue(p);
	}
	//! returns true if p is false w.r.t the current assignment
	bool isFalse(Literal p) const {
		assert(validVar(p.var()));
		return assign_.value(p.var()) == falseValue(p);
	}
	//! returns the decision level on which v was assigned.
	/*!
	 * \note the returned value is only meaningful if value(v) != value_free
	 */
	uint32 level(Var v) const {
		assert(validVar(v));
		return assign_.level(v);
	}

	//! returns the reason for p being true
	/*!
	 * \pre p is true w.r.t the current assignment
	 * \note if solver is used in no-learning mode, the reason is always null
	 */
	const Antecedent& reason(Literal p) const {
		assert(isTrue(p));
		return assign_.reason(p.var());
	}

	//! returns the literal of v being true in the current assignment
	/*!
	 * \pre v is assigned a value in the current assignment
	 */
	Literal trueLit(Var v) const {
		assert(value(v) != value_free);
		return Literal(v, value(v) == value_false);
	}

	//! returns true if v is currently marked as seen.
	/*!
	 * Note: variables assigned on level 0 are always marked.
	 */
	bool seen(Var v) const {
		assert(validVar(v));
		return assign_.seen(v, 3u);
	}
	//! returns true if the literal p is currently marked as seen
	bool seen(Literal p) const {
		assert(validVar(p.var()));
		return assign_.seen(p.var(), uint8(1+p.sign()));
	}
	void markSeen(Var v)    { assert(validVar(v)); assign_.setSeen(v, 3u); }
	void markSeen(Literal p){ assert(validVar(p.var())); assign_.setSeen(p.var(), uint8(1+p.sign())); }
	void clearSeen(Var v)   { assert(validVar(v)); assign_.clearSeen(v);  }

	//! returns the current decision level.
	uint32 decisionLevel() const { return (uint32)levels_.size(); }

	//! returns the starting position of decision level dl in the trail.
	/*!
	 * \pre dl != 0 && dl <= decisionLevel()
	 */
	uint32 levelStart(uint32 dl) const { 
		assert(dl != 0 && dl <= decisionLevel() );
		return levels_[dl-1].first;
	}

	//! returns the decision literal of the decision level dl.
	/*!
	 * \pre dl != 0 && dl <= decisionLevel()
	 */
	Literal decision(uint32 dl) const {
		assert(dl != 0 && dl <= decisionLevel() );
		return assign_.trail[ levels_[dl-1].first ];
	}
	
	//! returns the current (partial) assignment as a set of true literals
	/*!
	 * \note although the special var 0 always has a value it is not considered to be
	 * part of the assignment.
	 */
	const LitVec& assignment() const { return assign_.trail; }
	
	//! returns true, if the current assignment is conflicting
	bool hasConflict() const { return !conflict_.empty(); }
	//! returns the current conflict as a set of literals
	const LitVec& conflict() const { return conflict_; }
	
	SolverStatistics  stats;  /**< stores some statistics about the solving process */
	//@}

	/*!
	 * \name Basic DPLL-functions
	 */
	//@{
	//! initializes the root-level to dl
	/*!
	 * The root-level is similar to the top-level in that it can not be
	 * undone during search, i.e. the solver will not resolve conflicts that are on or
	 * above the root-level. 
	 */
	void setRootLevel(uint32 dl) { 
		rootLevel_  = std::min(decisionLevel(), dl); 
		btLevel_    = std::max(btLevel_, rootLevel_);
	}
	//! returns the current root level
	uint32 rootLevel() const { return rootLevel_; }

	//! sets the backtracking level to dl
	void setBacktrackLevel(uint32 dl) {
		btLevel_    = std::max(std::min(dl, decisionLevel()), rootLevel_);
	}
	uint32 backtrackLevel() const { return btLevel_; }

	//! removes implications made between the top-level and the root-level.
	/*!
	 * The function also resets the current backtrack-level and re-asserts learnt
	 * facts.
	 */
	bool clearAssumptions();

	//! If called on top-level removes SAT-clauses + Constraints for which Constraint::simplify returned true
	/*!
	 * \note if this method is called on a decision-level > 0 it is a noop and will
	 * simply return true.
	 * \return fallse, if a top-level conflict is detected. Otherwise, true.
	 */
	bool simplify();

	//! sets the literal p to true and schedules p for propagation.
	/*!
	 * Setting a literal p to true means assigning the appropriate value to
	 * p's variable. That is: value_false if p is a negative literal and value_true
	 * if p is a positive literal.
	 * \param p the literal that should become true
	 * \param a the reason for the literal to become true or 0 if no reason exists.
	 * 
	 * \return
	 *  - false if p is already false
	 *  - otherwise true.
	 *
	 * \pre hasConflict() == false
	 * \pre a.isNull() == false || decisionLevel() <= rootLevel() || SearchStrategy == no_learning
	 * \post
	 *  p.var() == trueValue(p) || p.var() == falseValue(p) && hasConflict() == true
	 *
	 * \note if setting p to true leads to a conflict, the nogood that caused the
	 * conflict can be requested using the conflict() function.
	 */
	bool force(const Literal& p, const Antecedent& a);

	//! assumes the literal p.
	/*!
	 * Sets the literal p to true and starts a new decision level.
	 * \pre value(p) == value_free && validVar(p.var()) == true
	 * \param p the literal to assume
	 * \return always true
	 * \post if decisionLevel() was n before the call then decisionLevel() is n + 1 afterwards.
	 */
	bool assume(const Literal& p);

	//! selects and assumes the next branching literal by calling the installed decision heuristic.
	/*!
	 * \pre queueSize() == 0
	 * \note the next decision literal will be selected randomly with a
	 * probability set using the initRandomHeuristic method.
	 * \return 
	 *  - true if the assignment is not total and a literal was assumed (or forced).
	 *  - false otherwise
	 *  .
	 * \see DecisionHeuristic
	 */
	bool decideNextBranch();

	//! Marks the literals in cfl as conflicting.
	/*! 
	 * \pre !cfl.empty() && !hasConflict()
	 * \post cfl.empty() && hasConflict()
	 */
	void setConflict(LitVec& cfl) { conflict_.swap(cfl); }

	//! Sets a conflict that forces the solver to terminate its search
	/*!
	 * \post hasConflict()
	 *
	 * \note 
	 *   To prevent the solver from resolving the stop conflict, the
	 *   function sets the root level to the current decision level. 
	 *   Hence, a stop conflict can be removed only via a call to clearAssumptions().
	 */
	void setStopConflict();

	/*!
	 * propagates all enqueued literals. If a conflict arises during propagation
	 * propagate returns false and the current conflict (as a set of literals)
	 * is stored in the solver's conflict variable.
	 * \pre !hasConflict()
	 * \see Solver::force
	 * \see Solver::assume
	 * \note shall not be called recursively
	 */
	bool propagate();

	/*!
	 * Does unit propagation and calls x->propagateFixpoint(*this)
	 * for all post propagators up to but not including p.
	 * \note The function is meant to be called only in the context of p
	 */
	bool propagateUntil(PostPropagator* p) { return unitPropagate() && (p == post_.head || post_.propagate(*this, p)); }

	//! executes a one-step lookahead on p.
	/*!
	 * Assumes p and propagates this assumption. If propagations leads to
	 * a conflict false is returned. Otherwise the assumption is undone and 
	 * the function returns true.
	 * \param p the literal to test
	 * \param c the constraint that wants to test p (may be 0)
	 * \pre p is free
	 * \note If c is not null and testing p does not lead to a conflict, 
	         c->undoLevel() is called *before* p is undone. Hence, the
			 range [s.levelStart(s.decisionLevel()), s.assignment().size())
			 contains p followed by all literals that were forced because of p.
	 * \note During propagation of p, only post propagators with priority
	 * in the range [priority_highest, priority_lookahead) are called.
	 */
	bool test(Literal p, Constraint* c);

	//! estimates the number of assignments following from setting p to true.
	/*!
	 * \note for the estimate only binary clauses are considered.
	 */ 
	uint32 estimateBCP(const Literal& p, int maxRecursionDepth = 5) const;
	
	//! returns the idx'th learnt constraint
	/*!
	 * \pre idx < numLearntConstraints()
	 */
	const LearntConstraint& getLearnt(uint32 idx) const {
		assert(idx < numLearntConstraints());
		return *static_cast<LearntConstraint*>(learnts_[ idx ]);
	}
	
	//! removes upto remMax percent of the learnt nogoods.
	/*!
	 * \param remMax percantage of learnt nogoods that should be removed ([0.0f, 1.0f])
	 * \note nogoods that are the reason for a literal to be in the assignment
	 * are said to be locked and won't be removed.
	 */
	void reduceLearnts(float remMax);

	//! resolves the active conflict using the selected strategy
	/*!
	 * If the SearchStrategy is set to learning, resolveConflict implements
	 * First-UIP learning and backjumping. Otherwise it simply applies
	 * chronological backtracking.
	 * \pre hasConflict
	 * \return
	 *  - true if the conflict was successfully resolved
	 *  - false otherwise
	 * \note
	 *  if decisionLevel() == rootLevel() false is returned.
	 */
	bool resolveConflict();

	//! backtracks the last decision and sets the backtrack-level to the resulting decision level.
	/*!
	 * \return
	 *  - true if backtracking was possible
	 *  - false if decisionLevel() == rootLevel()
	 */
	bool backtrack();

	//! undoes all assignments up to (but not including) decision level dl.
	/*!
	 * \pre dl > 0 (assignments made on decision level 0 cannot be undone)
	 * \pre dl <= decisionLevel()
	 * \post decisionLevel == max(dl, max(rootLevel(), btLevel))
	 */
	void undoUntil(uint32 dl);
	//@}  

	
	//! searches for a model for at most maxConflict number of conflicts.
	/*!
	 * The search function implements clasp's CDNL-algorithm.
	 * It searches for a model for at most maxConflict number of conflicts.
	 * The number of learnt clauses is kept below maxLearnts. 
	 * \pre endAddConstraint() returned true and !hasConflict()
	 * \param maxConflict stop search after maxConflict (-1 means: infinite)
	 * \param maxLearnts reduce learnt constraints after maxLearnts constraints have been learnt (-1 means: never reduce learnt constraints).
	 * \param randProb pick next decision variable randomly with a probability of randProp
	 * \param localR if true, stop after maxConflict in current branch
	 * \return
	 *  - value_true: if a model was found.
	 *  - value_false: if the problem is unsatisfiable (under assumptions, if any)
	 *  - value_free: if search was stopped because maxConflicts were found.
	 *  .
	 *
	 * \note search treats the root level as top-level, i.e. it
	 * will never backtrack below that level.
	 */
	ValueRep search(uint64 maxConflict, uint32 maxLearnts, double randProb = 0.0, bool localR = false);

	//! number of (unprocessed) literals in the propagation queue
	uint32 queueSize()    const { return (uint32) assign_.qSize(); }
	uint32 lastSimplify() const { return (uint32) lastSimplify_; }

	//! checks whether there is a model that is symmetric to the current model
	/*!
	 * The function checks for symmetric models, i.e. models that differ only in the 
	 * assignment of variables outside of the solver's assignment. 
	 * Typical example: vars eliminated by the SAT-preprocessor
	 * \param expand if false, any symmetric models are ignored. Otherwise, symmetric models
	 *        are expanded and stored in the solver.
	 * \pre the current assignment is a model
	 */ 
	bool nextSymModel(bool expand);
private:
	typedef PodVector<Constraint*>::type      ConstraintDB;
	typedef std::pair<uint32, ConstraintDB*>  LevelInfo;
	typedef PodVector<LevelInfo>::type        TrailLevels;  
	typedef PodVector<ImpliedLiteral>::type   ImpliedLits;
	typedef PodVector<Antecedent>::type       ReasonVec;
	// stub for storing ternary clauses.
	// A ternary clause [x y z] is stored using 3 stubs:
	// ~x, TernStub(y,z)
	// ~y, TernStub(x,z)
	// ~z, TernStub(x,y)
	typedef std::pair<Literal, Literal> TernStub;
	typedef PodVector<Literal>::type  BWL;
	typedef PodVector<TernStub>::type TWL;
	typedef PodVector<Watch>::type    GWL;
	struct WL {
		BWL   binW;
		TWL   ternW;
		GWL   genW;
		GWL::size_type size() const {
			return binW.size() + ternW.size() + genW.size();
		}
		void push_back(const Literal& w)  { binW.push_back(w); }
		void push_back(const TernStub& w) { ternW.push_back(w); }
		void push_back(const Watch& w)    { genW.push_back(w); }
	};
	typedef PodVector<WL>::type Watches;
	struct PPList {
		PPList();
		~PPList();
		void add(PostPropagator* p);
		void remove(PostPropagator* p);
		bool propagate(Solver& s, PostPropagator* p);
		void reset();
		bool isModel(Solver& s);
		bool nextSymModel(Solver& s, bool expand);
		PostPropagator* head;
		PostPropagator* look;
		PostPropagator* saved;
	};
	inline  bool hasStopConflict() const;
	bool    validWatch(Literal p) const { return p.index() < (uint32)watches_.size(); }
	uint32  mark(uint32 s, uint32 e);
	void    initRandomHeuristic(double randFreq);
	void    freeMem();
	bool    simplifySAT();
	void    simplifyShort(Literal p);
	void    simplifyDB(ConstraintDB& db);
	bool    unitPropagate();
	void    undoLevel(bool sp);
	uint32  analyzeConflict(uint32& sw);
	void    minimizeConflictClause(uint32 abstr);
	bool    minimizeLitRedundant(Literal p, uint32 abstr);
	void    undoFree(ConstraintDB* x);
	ConstraintDB* allocUndo(Constraint* c);
	SolverStrategies  strategy_;    // Strategies used by this solver-object
	VarInfo           info_;        // info_[v] stores info about variable v
	Assignment        assign_;      // three-valued assignment;
	ConstraintDB      constraints_; // problem constraints
	ConstraintDB      learnts_;     // learnt constraints
	ImpliedLits       impliedLits_; // Lits that were asserted on current dl but are logically implied earlier
	LitVec            conflict_;    // stores conflict-literals for later analysis
	LitVec            cc_;          // temporary: stores conflict clause within analyzeConflict
	TrailLevels       levels_;      // Stores start-position in trail of every decision level
	Watches           watches_;     // watches_[p.index()] is a list of constraints watching p
	PPList            post_;        // (possibly empty) list of post propagators
	DecisionHeuristic*randHeuristic_;
	VarVec*           levConflicts_;// For a DL d, levConflicts_[d-1] stores num conflicts when d was started
	ConstraintDB*     undoHead_;    // Free list of undo DBs
	uint32            units_;       // number of top-level assignments: always marked as seen
	uint32            binCons_;     // number of binary constraints
	uint32            ternCons_;    // number of ternary constraints
	LitVec::size_type lastSimplify_;// number of top-level assignments on last call to simplify
	uint32            rootLevel_;   // dl on which search started.
	uint32            btLevel_;     // When enumerating models: DL of the last unflipped decision from the current model. Can't backjump below this level.
	uint32            eliminated_;  // number of vars currently eliminated
	bool              shuffle_;     // shuffle program on next simplify?
};
//@}

/**
 * \defgroup heuristic Decision Heuristics
 */
//@{

//! Base class for decision heuristics to be used in a Solver.
/*! 
 * During search the decision heuristic is used whenever the DPLL-procedure must pick 
 * a new variable to branch on. Each concrete decision heuristic can implement a
 * different algorithm for selecting the next decision variable.
 */
class DecisionHeuristic {
public:
	DecisionHeuristic() {}
	virtual ~DecisionHeuristic(); 
	
	/*!
	 * Called once after all problem variables are known to the solver.
	 * The default-implementation is a noop.
	 * \param s The solver in which this heuristic is used.
	 */
	virtual void startInit(const Solver& /* s */) {}  

	/*!
	 * Called once after all problem constraints are known to the solver
	 * and the problem was simplified. 
	 * The default-implementation is a noop.
	 * \param s The solver in which this heuristic is used.
	 */
	virtual void endInit(Solver& /* s */) { }  

	/*!
	 * If the heuristic is used in an incremental setting, enable/disable
	 * reinitialization of existing variables.
	 * The default-implementation is a noop. Hence, heuristics will typically
	 * simply (re-)initialize all variables.
	 */
	virtual void reinit(bool /* b */) {}
	
	/*!
	 * Called for each var that changes its state from eliminated back to normal.
	 * The default-implementation is a noop.
	 * \param s Solver in which v is resurrected
	 * \param v The variable that is resurrected
	 */
	virtual void resurrect(const Solver& /* s */, Var /* v */) {}
	
	/*!
	 * Called on decision level 0. Variables that are assigned on this level
	 * may be removed from any decision heuristic.
	 * \note Whenever the solver returns to dl 0, simplify is only called again
	 * if the solver learnt new facts since the last call to simplify.
	 *
	 * \param s The solver that reached decision level 0.
	 * \param st The position in the trail of the first new learnt fact.
	 */
	virtual void simplify(const Solver& /* s */, LitVec::size_type /* st */) { }
	
	/*!
	 * Called whenever the solver backracks.
	 * Literals in the range [s.trail()[st], s.trail().size()) are subject to backtracking.
	 * The default-implementation is a noop.
	 * \param s The solver that is about to backtrack
	 * \param st Position in the trail of the first literal that will be backtracked.
	 */
	virtual void undoUntil(const Solver& /* s */, LitVec::size_type /* st */) {}
	
	/*!
	 * Called whenever a new constraint is added to the solver s.
	 * The default-implementation is a noop.
	 * \param s The solver to which the constraint is added.
	 * \param first First literal of the new constraint.
	 * \param size Size of the new constraint.
	 * \param t Type of the new constraint.
	 * \note first points to an array of size size.
	 */
	virtual void newConstraint(const Solver&, const Literal* /* first */, LitVec::size_type /* size */, ConstraintType /* t */) {}
	
	/*!
	 * Called for each new reason-set that is traversed during conflict analysis.
	 * The default-implementation is a noop.
	 * \param s the solver in which the conflict is analyzed.
	 * \param lits The current reason-set under inspection.
	 * \param resolveLit The literal that is currently resolved.
	 * \note When a conflict is detected the solver passes the conflicting literals
	 * in lits during the first call to updateReason. On that first call resolveLit
	 * is the sentinel-literal.
	 */
	virtual void updateReason(const Solver& /* s */, const LitVec& /* lits */, Literal /* resolveLit */) {}
	
	/*! 
	 * Called whenever the solver must pick a new variable to branch on. 
	 * \param s The solver that needs a new decision variable.
	 * \return
	 *  - true  : if the decision heuristic asserted a literal 
	 *  - false : if no appropriate decision literal could be found, because assignment is total
	 *  .
	 * \post
	 * if true is returned, the heuristic has asserted a literal.
	 */
	bool select(Solver& s) { return s.numFreeVars() != 0 && s.assume(doSelect(s)); }

	//! implements the actual selection process
	/*!
	 * \pre s.numFreeVars() > 0, i.e. there is at least one variable to branch on.
	 * \return 
	 *  - a literal that is currently free or
	 *  - a sentinel literal. In that case, the heuristic shall have asserted a literal!
	 */ 
	virtual Literal doSelect(Solver& /* s */) = 0;

	/*! 
	 * Shall select one of the literals in the range [first, last).
	 * \param s     The solver that needs a new decision variable.
	 * \param first Pointer to first literal in range
	 * \param last  Pointer to the end of the range
	 * \pre [first, last) is not empty and all literals in the range are currently unassigned.
	 * \note The default implementation returns *first
	 */
	virtual Literal selectRange(Solver& /* s */, const Literal* first, const Literal* /* last */) {
		return *first;
	}
protected:
	Literal savedLiteral(const Solver& s, Var var) const {
		Literal r(0,false); ValueRep v;
		if ((v = s.savedValue(var)) != value_free) {
			r = Literal(var, v == value_false);
		}
		return r;
	}
private:
	DecisionHeuristic(const DecisionHeuristic&);
	DecisionHeuristic& operator=(const DecisionHeuristic&);
};

//! selects the first free literal w.r.t to the initial variable order.
class SelectFirst : public DecisionHeuristic {
private:
	Literal doSelect(Solver& s);
};

//@}
}
#endif
