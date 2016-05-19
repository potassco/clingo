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
#ifndef CLASP_SMODELS_CONSTRAINTS_H_INCLUDED
#define CLASP_SMODELS_CONSTRAINTS_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include <clasp/constraint.h>

namespace Clasp {

//! Class implementing smodels-like cardinality- and weight constraints.
/*!
 * \ingroup constraint
 * This class represents a constraint of type W == w1*x1 ... wn*xn >= B,
 * where W and each xi are literals and B and each wi are strictly positive integers.
 * The class is used to represent smodels-like weight constraint, i.e.
 * the body of a basic weight rule. In this case W is the literal associated with the body.
 * A cardinality constraint is handled like a weight constraint where all weights are equal to 1.
 *
 * The class implements the following four inference rules:
 * Let L be the set of literals of the constraint,
 * let sumTrue be the sum of the weights of all literals l in L that are currently true,
 * let sumReach be the sum of the weights of all literals l in L that are currently not false,
 * let U = {l in L | value(l.var()) == value_free}
 * - FTB: If sumTrue >= bound: set W to true.
 * - BFB: If W is false: set false all literals l in U for which sumTrue + weight(l) >= bound.
 * - FFB: If sumReach < bound: set W to false.
 * - BTB: If W is true: set true all literals l in U for which sumReach - weight(l) < bound.
 */
class WeightConstraint : public Constraint {
public:
	//! Creates a new weight constraint from the given weight literals
	/*!
	 * If the right hand side of the weight constraint is initially true/false (FTB/FFB),
	 * W is assigned appropriately but no constraint is created. Otherwise
	 * the new weight constraint is added to the solver.
	 * \param s solver in which the new constraint is to be used.
	 * \param W the literal that is associated with the constraint
	 * \param lits the literals of the weight constraint
	 * \param bound the lower bound of the weight constraint.
	 * \return false if the constraint is initially conflicting w.r.t the current assignment.
	 * \note Cardinality constraint are represented as weight constraints with all weights equal
	 * to 1.
	 */
	static bool newWeightConstraint(Solver& s, Literal W, WeightLitVec& lits, weight_t bound);
	
	// constraint interface
	bool simplify(Solver& s, bool = false);
	void destroy();
	PropResult propagate(const Literal& p, uint32& data, Solver& s);
	void reason(const Literal& p, LitVec& lits);
	void undoLevel(Solver& s);
	ConstraintType type() const { return Constraint_t::native_constraint; }
	uint32 estimateComplexity(const Solver& s) const;
private:
	WeightConstraint(Solver& s, Literal con, const WeightLitVec& lits, uint32 bound, uint32 sumW, bool card);
	~WeightConstraint();
	static weight_t canonicalize(Solver& s, WeightLitVec& lits, weight_t& bound);

	// Logically, we distinguish two constraints:
	// FFB/BTB: (SumW-bound)+1 [~con=1, l1=w1,...,ln=wn]; watches con and ~l1,...,~ln
	// FTB/BFB: bound [con=1, ~l1=w1,...,~ln=wn]; watches ~con and l1,...,ln
	// Physically, we store the literals in one array: ~con=1, l1=w1,...,ln=wn
	enum ActiveConstraint {
		FFB_BTB   = 0,
		FTB_BFB   = 1,
	};
	static const uint32 NOT_ACTIVE = 3u;
	
	// Represents a literal on the undo stack
	// idx()        returns the index of the literal
	// constraint() returns the constraint that added the literal to the undo stack
	// inReason()   returns true if the literal is part of the reason for a future assignment
	//              or false if it was propagated by constraint().
	struct UndoInfo {
		UndoInfo(uint32 d) : data(d) {}
		uint32           idx()        const { return data >> 2; }
		ActiveConstraint constraint() const { return static_cast<ActiveConstraint>((data&2) != 0); }
		bool             inReason()   const { return (data&1) != 0; }
		uint32 data;
	};
	void addWatch(Solver& s, uint32 idx, ActiveConstraint c);
	// Returns the literal of constraint c at index idx i.e.
	//	lits[idx], iff c == FFB_BTB
	// ~lits[idx], iff c == FTB_BFB
	Literal	 lit(uint32 idx, ActiveConstraint c) const {
		assert(wc_ == 0 || (idx&1) == 0);
		return Literal::fromIndex( lits_[idx].index() ^ c );
	}
	// Returns the weight of the literal at the given index.
	// Note: For weight constraints the weight of a literal at index idx is stored
	// at position idx+1. For cardinality constraint no weights are stored and
	// the returned weight is always 1.
	weight_t weight(uint32 index)	const {
		return wc_ == 0 ? weight_t(1) : (weight_t)lits_[index+1].asUint();
	}
	// Updates bound_[c] and adds an undo watch to the solver if necessary.
	// Then adds the literal at position idx to the reason set (and the undo stack).
	void updateConstraint(Solver& s, uint32 idx, ActiveConstraint c);
	// Returns the starting index of the undo stack.
	uint32   undoStart()       const { return size_<<wc_; }
	// Adds the literal at position idx to the undo stack.
	void     undoPush(uint32 idx, ActiveConstraint c, bool inReason) {
		lits_[++undo_].asUint()	= (idx<<2) + (c<<1) + inReason;
	}
	UndoInfo undoTop()         const { return lits_[undo_].asUint(); }
	UndoInfo undoPos(uint32 i) const { assert(i >= undoStart()); return lits_[i].asUint(); }
	// Returns the decision level of the last assigned literal
	// or 0 if no literal was assigned yet.
	inline uint32	highestUndoLevel(Solver&) const;
	// Returns the index of next literal to look at during backward propagation.
	uint32&  getBpIndex(uint32& idx) {
		if (wc_ == 0) return idx;
		return lits_[1].asUint() == 1 ? ++lits_[1].asUint() : lits_[1].asUint();
	}
	uint32   size_   : 30; // number of lits in constraint (counting the literal associated with the constraint)
	uint32   active_ :  2; // which of the two sub-constraints is currently unit?
	uint32   undo_   : 31; // undo position; [undoStart(), undo_) is the undo stack
	uint32   wc_     :  1; // 1 if this is a weight constraint, otherwise 0 
	weight_t bound_[2];    // FFB_BTB: (sumW-bound)+1 / FTB_BFB: bound
	Literal  lits_[0];     // Literals of constraint, followed by undo stack
	// For cardinality constraints with n = size_:
	// [0, n)  stores literals of this aggregate, i.e. ~B, l1, ..., ln-1 and
	// [n, 2n) stores the undo stack
	// For weight constraints with n = size_:
	// [0,2n)   stores literals and weights of this aggregate, i.e. ~B, 1, l1, W1,..., ln-1, Wn-1 and
	// [2n, 3n)	stores the undo stack
};

//! Implements a (meta-)constraint for supporting Smodels-like minimize statements
/*!
 * \ingroup constraint
 * A solver contains at most one minimize constraint, but a minimize constraint
 * may represent several minimize statements. In that case, minimize statements added
 * earlier have higher priority. Note that this is inverse to the order used in lparse,
 * where the minimize statement written last has the highest priority. Otherwise, 
 * priority is handled like in smodels, i.e. statements
 * with lower priority become relevant only when all statements with higher priority
 * currently have an optimal assignment.
 *
 * MinimizeConstraint supports two notions of optimality: if mode is set to
 * compare_less solutions are considered optimal only if they are strictly smaller 
 * than previous solutions, otherwise, if mode is set to compare_less_equal a
 * solution is considered optimal if it is not greater than any previous solution.
 * Example: 
 *  m0: {a, b}
 *  m1: {c, d}
 *  Solutions: {a, c,...}, {a, d,...} {b, c,...}, {b, d,...} {a, b,...} 
 *  Optimal compare_less: {a, c, ...} (m0 = 1, m1 = 1}
 *  Optimal compare_less_equal: {a, c, ...}, {a, d,...}, {b, c,...}, {b, d,...} 
 * 
 */
class MinimizeConstraint : public Constraint {
public:
	typedef int64 WeightSum;
	//! Supported comparison operators
	/*! 
	 * Defines the possible comparison operators used when comparing
	 * (partial) assignments resp. solutions.
	 * If compare_less is used, an optimal solution is one that is
	 * strictly smaller than all previous solutions. Otherwise
	 * an optimal solution is one that is no greater than any previous
	 * solution.
	 */
	enum Mode {
		compare_less,       /**< optimize using < */
		compare_less_equal  /**< optimize using <= */
	};
	
	//! creates an empty minimize-constraint.
	/*!
	 * \note The default comparison mode is Mode::compare_less 
	 */
	MinimizeConstraint();
	~MinimizeConstraint();

	//! number of minimize statements contained in this constraint
	uint32 numRules() const { return (uint32)minRules_.size(); }

	//! Sets the comparison mode used in this constraint
	void setMode(Mode m)  { mode_ = m; }
	Mode mode() const     { return mode_; }

	//! Sets the optimum of a minimize-statement 
	/*!
	 * \param level priority level of the minimize statement for which the optimum should be set
	 * \param opt the new optimum
	 * \return true if the new optimum does not violate the constraint. Otherwise, false.
	 * \note If false is returned, the new optimum is not set
	 */
	bool setOptimum(uint32 level, WeightSum opt);
	
	//! returns the current optimum of a minimize statement
	WeightSum getOptimum(uint32 level) const {
		assert(level < minRules_.size());
		if (minRules_[level]->opt_ != std::numeric_limits<WeightSum>::max() && level == minRules_.size()-1 && mode_ == compare_less) {
			return minRules_[level]->opt_ + 1;
		}
		return minRules_[level]->opt_;
	}
	
	void adjustSum(uint32 level, weight_t s) {
		assert(level < minRules_.size());
		minRules_[level]->sum_ += s;
	}
	
	//! Adds a minimize statement.
	/*!
	 * \pre simplify was not yet called
	 * \note If more than one minimize statement is added those added
	 * earlier have a higher priority.
	 */
	void minimize(Solver& s, const WeightLitVec& literals, bool inHeu = false);
	
	//! Notifies the constraint about a model
	/*!
	 * setModel must be called whenever the solver finds a model.
	 * The found model is recorded as optimum and the decision level
	 * on which search should continue is returned.
	 * \return The decision level on which the search should continue or
	 *  uint32(-1) if search space is exhausted w.r.t. this minimize-constraint.
	 */
	uint32 setModel(Solver& s);

	//! Notifies the constraint about a model and backtracks the solver
	/*!
	 * Calls setModel and backtracks to the returned decision level if possible.
	 * \return
	 *  - true if backtracking was successful and search can continue
	 *  - false otherwise
	 */
	bool backtrackFromModel(Solver& s);

	bool backpropagate(Solver& s);

	//! returns the number of optimal models found so far
	uint64 models() const;
	
	// constraint interface
	PropResult propagate(const Literal& p, uint32& data, Solver& s);
	void undoLevel(Solver& s);
	void reason(const Literal& p, LitVec& lits);
	bool simplify(Solver& s, bool = false);
	ConstraintType type() const {
		return Constraint_t::native_constraint;
	}
	
	bool select(Solver& s) const;
private:
	MinimizeConstraint(const MinimizeConstraint&);
	MinimizeConstraint& operator=(MinimizeConstraint&);
	
	struct MinRule {
		MinRule() : sum_(0), opt_(std::numeric_limits<WeightSum>::max()) {}
		WeightSum      sum_; // current sum
		WeightSum      opt_; // optimial sum so far
		WeightLitVec  lits_; // sorted by decreasing weights
	};
	// A literal occurrence in one MinRule
	struct LitRef {
		uint32 ruleIdx_;
		uint32 litIdx_;
	};
	// A literal that was assigned true by unit propagation or forced
	// to false by this constraint.
	struct UndoLit {
		UndoLit(Literal p, uint32 key, bool pos) : lit_(p), key_(key), pos_(pos) {}
		Literal lit_;       // assigned literal
		uint32  key_ : 31;  // pos_ == 1: index into occurList, else: true lits of rules [0, key] are the reason for lit
		uint32  pos_ : 1;   // 1 if assigned true
	};
	
	typedef PodVector<LitRef>::type LitOccurrence;
	typedef std::vector<LitOccurrence> OccurList;
	typedef PodVector<UndoLit>::type UndoList;
	typedef PodVector<MinRule*>::type Rules;
	typedef PodVector<uint32>::type Index;
	
	Literal   lit(const LitRef& r)    const { return minRules_[r.ruleIdx_]->lits_[r.litIdx_].first; }
	MinRule*  rule(const LitRef& r)   const { return minRules_[r.ruleIdx_]; }
	MinRule*  rule(uint32 pLevel)     const { return minRules_[pLevel]; }
	weight_t  weight(const LitRef& r) const { return minRules_[r.ruleIdx_]->lits_[r.litIdx_].second; }
	uint32    pLevel(const LitRef& r) const { return r.ruleIdx_; }
	void updateSum(uint32 key);
	void addUndo(Solver& s, Literal p, uint32 key, bool forced);
	bool conflict(uint32& level) const;
	bool backpropagate(Solver& s, MinRule* r);

	Index     index_;       // maps between Literal and Occur-List - only used during construction
	OccurList occurList_;   // occurList_[idx] sorted by increasing rule index
	Rules     minRules_;    // all minimize rules in priority-order
	UndoList  undoList_;    // stores indices into occurList
	uint64    models_;      // number of optimal models so far
	uint32    activePL_;    // priority level under consideration
	uint32    activeIdx_;   // index into rule of current priority level
	Mode      mode_;        // how to compare assignments?
};

}

#endif
