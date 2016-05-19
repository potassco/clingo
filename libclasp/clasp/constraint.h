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
#ifndef CLASP_CONSTRAINT_H_INCLUDED
#define CLASP_CONSTRAINT_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/literal.h>
#include <utility>  // std::pair
#include <cassert>

/*!
 * \file 
 * Defines the base classes for boolean constraints.
 * 
 */
namespace Clasp {

class Solver;

/**
 * \defgroup constraint Boolean Constraints
 */
//@{

//! Constraint types distinguished by a Solver.
struct Constraint_t {
	enum Type { native_constraint = 0, learnt_conflict = 1, learnt_loop = 2, max_value = learnt_loop};
};  
typedef Constraint_t::Type ConstraintType;


//! Base class for (boolean) constraints to be used in a Solver.
/*!
 * Base class for (boolean) constraints like e.g. clauses. Concrete constraint classes define
 * representations for constraints over boolean variables.
 * Each constraint class must define a method for inference (derive information from an assignment),
 * it must be able to detect conflicts (i.e. detect when the current assignment makes the constraint unsatisfiable)
 * and to return a reason for inferred literals as well as conflicts (as a set of literals). 
 */
class Constraint {
public:
	/*!
	 * Type used as return type for Constraint::propagate.
	 * \see Constraint::propagate for a description of the meaning of PropResult's members.
	 */
	typedef std::pair<bool, bool> PropResult;

	Constraint();

	/*!
	 * propagate is called for each constraint that watches p. propagate shall enqueue 
	 * all consequences of p becoming true.
	 * \pre p is true
	 * \param p a literal watched by this constraint that recently became true.
	 * \param data the data-blob that this constraint passed when the watch for p was registered.
	 * \param s the solver in which p is true.
	 * \return PropResult is a bool-pair where:
	 *  - PropResult.first:  false iff the constraint is now conflicting. Otherwise true.
	 *  - PropResult.second: false iff the constraint no longer wants to watch p. Otherwise true.
	 *  .
	 */
	virtual PropResult propagate(const Literal& p, uint32& data, Solver& s) = 0;

	//! called when the solver undid a decision level watched by this constraint.
	/*!
	 * \param s the solver in which the decision level is undone.
	 */
	virtual void undoLevel(Solver& s);

	/*!
	 * \pre This constraint is the reason for p being true.
	 * \post The literals implying p were added to lits
	 */
	virtual void reason(const Literal& p, LitVec& lits) = 0;
	
	/*!
	 * simplify this constraint.
	 * \pre s.decisionLevel() == 0 and the current assignment is fully propagated.
	 * \return
	 *  true if this constraint can be ignored (e.g. is satisfied)
	 *  false otherwise
	 * \post
	 * if simplify returned true, this constraint has previously removed all its watches
	 * from the solver.
	 *
	 * \note the default implementation is a noop and returns false.
	 */
	virtual bool simplify(Solver& s, bool reinit = false);
	
	//! default is to call delete this
	virtual void destroy();

	//! returns the type of this learnt constraint.
	virtual ConstraintType type() const = 0;

	virtual uint32 estimateComplexity(const Solver& s) const;
protected:
	virtual ~Constraint();
private:
	Constraint(const Constraint&);
	Constraint& operator=(const Constraint&);
};

//! Base class for learnt constraints
/*!
 * Base class for constraints that can be learnt during search. A learnt constraint 
 * is a constraint with the difference that it can be created and deleted dynamically 
 * during the search-process and is subject to nogood-deletion.
 * Typical examples are conflict clauses which are created during conflict analysis 
 * and loop formulas which are created during unfounded-set checks. 
 * 
 * A learnt constraint must additionally define the method locked in order to tell a solver
 * if the constraint can be safely deleted or not. 
 * Furthermore whenever a solver needs to delete some learnt constraints it will first
 * delete those with a low activity. A learnt constraint may therefore bump its activity 
 * whenever it sees fit in order to delay its deletion.
 *
 */
class LearntConstraint : public Constraint {
public:
	LearntConstraint();
	/*!
	 * Shall return true if this constraint can't be deleted because it 
	 * currently implies one ore more literals and false otherwise.
	 */
	virtual bool locked(const Solver& s) const = 0;

	/*!
	 * Called once before this learnt constraint is removed from the solver's 
	 * learnt db. Shall remove all watches this constraint has registered with the solver.
	 * \pre locked() == false
	 */
	virtual void removeWatches(Solver& s) = 0;

	//! returns the activity of the constraint.
	/*!
	 * \note A solver uses the activity-value in order to establish an ordering
	 * of learnt constraints. Whenever a solver needs to delete some learnt constraints it
	 * selects from the unlocked ones those with a low activity-value.
	 * \note The default-implementation always returns 0
	 */
	virtual uint32 activity() const;
	
	//! Asks the constraint to decrease its activity.
	virtual void decreaseActivity();

	//! Shall return true if this constraint is satisfied w.r.t the current assignment
	/*!
	 * If this function returns false, freeLits shall contain some (or all) currently
	 * unassigned literals of this constraint.
	 */
	virtual bool isSatisfied(const Solver& s, LitVec& freeLits) const = 0;
protected:
	~LearntConstraint();
};

//! Base class for all post propagators.
/*!
 * Post propagators are called after a solver finished unit-propagation and 
 * once after a total assignment is found.
 * They may add more elaborate propagation mechanisms.
 * The typical asp example would be an unfounded set check.
 *
 * \note Post propagators are called in priority order. For this,
 *       the function PostPropagator::priority() is used. 
 *       Currently, the solver distinguishes *three* classes of 
 *       post propagators:
 *       - det_single: deterministic post propagators that only work on 
 *         the current decision level. During propagation, these post 
 *         propagators must neither backtrack nor create new levels. 
 *         They shall have a priority in the range: [priority_highest, priority_lookahead)
 *       - det_multi: post propagators that use some form of lookahead. They are only
 *         called once all det_single post propagators finished. Furthermore,
 *         they are disabled during lookahead operations. They shall have
 *         a priority in the range [priority_lookahead, priority_general).
 *       - multi: post propagators that are non-deterministic or work on many
 *         decision levels shall have a priority in the range [priority_general, priority_lowest).
 */
class PostPropagator : public Constraint {
public:
	PostPropagator();
	virtual ~PostPropagator();
	using Constraint::propagate;  // Enable overloading!

	PostPropagator* next; // for supporting lists of post propagators
	//! Default priorities for post propagators
	enum Priority {
		priority_highest     = 0,   /**< highest possible priority  */
		priority_single_high = 50,  /**< high det_single priority   */
		priority_single      = 100, /**< default priority           */
		priority_single_low  = 150, /**< low det_single priority    */
		priority_lookahead   = 200, /**< default lookahead priority */
		priority_general     = 300, /**< default general priority   */
		priority_lowest      = 1024 /**< lowest possible priority   */
		
	};
	//! returns a value representing the priority of this propagator
	/*!
	 * The priority is used to order sequences of post propagators and to
	 * classify post propagators w.r.t the three classes: det_single, det_multi, and
	 * multi.
	 * \note The default implementation returns priority_single
	 * \note See class description for an overview of the three priority classes.
	 */
	virtual uint32 priority() const;

	//! shall enqueue new assignments to be propagated
	/*!
	 * \pre    the assignment is fully propagated.
	 * \param  s The solver in which this object is used
	 * \return 
	 *   - false if propagation led to conflict
	 *   - true otherwise
	 *
	 * \note 
	 *   The function is called in the context of
	 *   PostPropagator::propagateFixpoint(Solver& s).
	 *   It must not call s.propagate() or any other function
	 *   that would result in a recursive chain of propagate()
	 *   calls.
	 *
	 * \see PostPropagator::propagateFixpoint(Solver& s)
	 */
	virtual bool propagate(Solver& s) = 0;

	
	//! clears internal state of this object and aborts any active propagation.
	/*!
	 * The solver containing this object calls reset() whenever a conflict is
	 * detected during propagation.
	 * \note The default implementation is a noop
	 */
	virtual void reset();

	//! is the current total assignment a model?
	/*!
	 * \pre the assignment is total and not conflicting
	 * \return
	 *  - true if the assignment is a model w.r.t this post propagator
	 *  - false otherwise
	 * \post if false is returned, the post propagator must have forced a
	 *  conflict and at least one conflicting literal must be assigned on
	 *  the current decision level.
	 */
	virtual bool isModel(Solver& s);

	//! checks whether there is a model that is symmetric to the current model
	/*!
	 * The function shall check for and (optionally) expand symmetric models, 
	 * i.e. models that differ only in the assignment of variables outside of the 
	 * solver's assignment. 
	 * \param s      The solver in which this object is used
	 * \param expand If true, symmetric models shall be expanded. Otherwise they
	 *               they shall be ignored.
	 * \pre the current assignment is a model
	 * \return 
	 *   - true, if there are symmetric models and expand is true
	 *   - false, otherwise.
	 * \note the default implementation always returns false
	 */ 
	virtual bool nextSymModel(Solver& s, bool expand);


	//! propagates the current assignment until a fixpoint is reached w.r.t this post propagator
	/*!
	 * This function implements a basic propagation loop that calls
	 * PostPropagator::propagate(Solver& s) until a fixpoint is reached or
	 * a conflict is detected.
	 * \note
	 *   Normally, subclasses only need to implement 
	 *   PostPropagator::propagate(Solver& s).
	 *
	 * \param  s The solver in which this object is used
	 * \return false if propagation led to conflict, true otherwise
	 * \pre    the assignment is fully propagated.
	 * \post   a fixpoint was reached or propagation led to a conflict.
	 * \note   The default implementation implements the following loop:
	 *   while (propagate() && s.queueSize() > 0) 
	 *     if (!s.propagateUntil(this)) return false;
	 *   return true
	 * \note   
	 */
	virtual bool propagateFixpoint(Solver& s);
protected:
	// Constraint interface - noops
	PropResult     propagate(const Literal&, uint32&, Solver&);
	void           reason(const Literal& p, LitVec& lits);
	ConstraintType type() const;
private:
	PostPropagator(const PostPropagator&);
	PostPropagator& operator=(const PostPropagator&);
};
//@}

//! Stores a reference to the constraint that implied a literal.
/*!
 * Stores a reference to the constraint that implied a certain literal or
 * null if the literal has no antecedent (i.e. is a decision literal or a top-level fact).
 * 
 * \note 
 * The constraint that implied a literal can have three different representations:
 * - it can be a single literal (binary clause constraint)
 * - it can be two literals (ternary clause constraint)
 * - it can be a pointer to a constraint (generic constraint)
 * .
 *
 * \par Implementation:
 *
 * The class stores all three representations in one 64-bit integer and makes
 * the following platform assumptions:
 * - there is a 64-bit integer type
 * - pointers are either 32- or 64-bits wide.
 * - the alignment of pointers to dynamically allocated objects is a multiple of 4 
 * - casting between 64-bit integers and pointers is safe and won't change the underlying value.
 * .
 * These assumptions are not guaranteed by the C++ Standard but should nontheless
 * hold on most 32- and 64-bit platforms.
 * 
 * From the 64-bits the first 2-bits encode the type stored:
 *  - 00: Pointer to constraint
 *  - 01: binary constraint (i.e. one literal stored in the highest 31 bits)
 *  - 11: ternary constraint (i.e. two literals stored in the remaining 62 bits). 
 * 
 */
class Antecedent {
public:
	enum Type { generic_constraint = 0, binary_constraint = 1, ternary_constraint = 3};
	//! creates a null Antecedent. 
	/*!
	 * \post: isNull() == true && type == generic_constraint
	 */
	Antecedent() : data_(0) {}
	
	//! creates an Antecedent from the literal p
	/*!
	 * \post: type() == binary_constraint && firstLiteral() == p
	 */
	Antecedent(const Literal& p) {
		// first lit is stored in high dword
		data_ = (((uint64)p.index()) << 33) + binary_constraint;
		assert(type() == binary_constraint && firstLiteral() == p);
	}
	
	//! creates an Antecedent from the literals p and q
	/*!
	 * \post type() == ternary_constraint && firstLiteral() == p && secondLiteral() == q
	 */
	Antecedent(const Literal& p, const Literal& q) {
		// first lit is stored in high dword
		// second lit is stored in low dword
		data_ = (((uint64)p.index()) << 33) + (((uint64)q.index()) << 2) + ternary_constraint;
		assert(type() == ternary_constraint && firstLiteral() == p && secondLiteral() == q);
	}

	//! creates an Antecedent from the Constraint con
	/*!
	 * \post type() == generic_constraint && constraint() == con
	 */
	Antecedent(Constraint* con) : data_((uintp)con) {
		assert(type() == generic_constraint && constraint() == con);
	}

	//! returns true if this antecedent does not refer to any constraint.
	bool isNull() const {
		return data_ == 0;
	}

	//! returns the antecedent's type.
	Type type() const {
		return Type( data_ & 3 );
	}
	
	//! extracts the constraint-pointer stored in this object
	/*!
	 * \pre type() == generic_constraint
	 */
	Constraint* constraint() const {
		assert(type() == generic_constraint);
		return (Constraint*)(uintp)data_;
	}
	
	//! extracts the first literal stored in this object.
	/*!
	 * \pre type() != generic_constraint
	 */
	Literal firstLiteral() const {
		assert(type() != generic_constraint);
		return Literal::fromIndex(static_cast<uint32>(data_ >> 33));
	}
	
	//! extracts the second literal stored in this object.
	/*!
	 * \pre type() == ternary_constraint
	 */
	Literal secondLiteral() const {
		assert(type() == ternary_constraint);
		return Literal::fromIndex( static_cast<uint32>(data_>>1) >> 1 );
	}

	//! returns the reason for p
	/*!
	 * \pre !isNull()
	 */
	void reason(const Literal& p, LitVec& lits) const {
		assert(!isNull());
		Type t = type();
		if (t == generic_constraint) {
			constraint()->reason(p, lits);
		}
		else {
			lits.push_back(firstLiteral());
			if (t == ternary_constraint) {
				lits.push_back(secondLiteral());
			}
		}
	}

	//! returns true iff the antecedent refers to the constraint con
	bool operator==(const Constraint* con) const {
		return data_ == reinterpret_cast<uintp>(con);
	}

	//! checks whether the imlementation-assumptions hold on this platform
	/*! 
	 * throws PlatformError on error
	 */
	static bool checkPlatformAssumptions(); 
private:
	uint64 data_;
};

class PlatformError : public ClaspError {
public:
	explicit PlatformError(const char* msg);
};

}
#endif
