// 
// Copyright (c) 2006-2012, Benjamin Kaufmann
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

class  SharedContext;
class  Solver;
class  ClauseHead;
struct CCMinRecursive;

/**
 * \defgroup constraint Boolean Constraints
 */
//@{

//! Constraint types distinguished by a Solver.
struct Constraint_t {
	enum Type { 
		static_constraint = 0, /**< an unremovable constraint (e.g. a problem constraint)        */
		learnt_conflict   = 1, /**< a (removable) constraint derived from conflict analysis      */
		learnt_loop       = 2, /**< a (removable) constraint derived from unfounded set checking */
		learnt_other      = 3, /**< a (removable) constraint learnt by some other means          */
		max_value         = learnt_other
	};
	struct Set {
		Set() : m(0) {}
		bool inSet(Type t) const { return (m & (uint32(1)<<t)) != 0; }
		void addSet(Type t)      { m |= (uint32(1)<<t); }
		uint32 m;
	};
};  
typedef Constraint_t::Type ConstraintType;
typedef Constraint_t::Set  TypeSet;
//! Type storing a constraint's activity.
struct Activity {
	enum { LBD_SHIFT = 7, MAX_LBD = (1 << LBD_SHIFT)-1, MAX_ACT = (1 << (32-LBD_SHIFT))-1 };
	Activity(uint32 act, uint32 lbd) : rep( (act << LBD_SHIFT) | lbd ) {}
	uint32 activity() const { return rep >> LBD_SHIFT; }
	uint32 lbd()      const { return rep & uint32(MAX_LBD); }
	void   bumpAct()        { if (rep < uint32(MAX_ACT<<LBD_SHIFT)) rep += (1<<LBD_SHIFT); }
	void   setLbd(uint32 x) { rep &= ~uint32(MAX_LBD); rep |= x; }
	uint32 rep;
};

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
	//! Type used as return type for Constraint::propagate.
	struct PropResult {
		explicit PropResult(bool a_ok = true, bool a_keepWatch = true) : ok(a_ok), keepWatch(a_keepWatch) {}
		bool ok;         /**< true if propagation completes without conflict     */
		bool keepWatch;  /**< true if constraint wants to keep the current watch */
	};
	Constraint();
	//! Returns a clone of this and adds necessary watches to the given solver.
	/*!
	 * The function shall create and return a copy of this constraint
	 * to be used in the given solver. Furthermore, it shall add
	 * necessary watches to the given solver.
	 * \note Return 0 to indicate that cloning is not supported.
	 */
	virtual Constraint* cloneAttach(Solver& other) = 0;
	
	//! Default is to call delete this.
	virtual void destroy(Solver* s = 0, bool detach = false);

	//! Returns the type of this constraint.
	/*!
	 * \note The default implementation returns Constraint_t::static_constraint.
	 */
	virtual ConstraintType type() const;

	/*!
	 * Propagate is called for each constraint that watches p. It shall enqueue 
	 * all consequences of p becoming true.
	 * \pre p is true in s
	 * \param s The solver in which p became true.
	 * \param p A literal watched by this constraint that recently became true.
	 * \param data The data-blob that this constraint passed when the watch for p was registered.
	 */
	virtual PropResult propagate(Solver& s, Literal p, uint32& data) = 0;

	/*!
	 * \pre This constraint is the reason for p being true in s.
	 * \post The literals implying p were added to lits.
	 */
	virtual void reason(Solver& s, Literal p, LitVec& lits) = 0;

	/*!
	 * Called during minimization of learnt clauses.
	 * \pre This constraint is the reason for p being true in s.
	 * \return true if p can be removed from the current learnt clause, false otherwise.
	 * \note The default implementation uses the following inefficient algorithm
	 * \code
	 *   LitVec temp;
	 *   reason(s, p, temp);
	 *   for each x in temp 
	 *     if (!s.ccMinimize(p, rec)) return false;
	 *   return true;
	 * \endcode
	 */ 
	virtual bool minimize(Solver& s, Literal p, CCMinRecursive* rec);

	//! Called when the solver undid a decision level watched by this constraint.
	/*!
	 * \param s the solver in which the decision level is undone.
	 */
	virtual void undoLevel(Solver& s);

	/*!
	 * Simplify this constraint.
	 * \pre s.decisionLevel() == 0 and the current assignment is fully propagated.
	 * \return
	 *  true if this constraint can be ignored (e.g. is satisfied),
	 *  false otherwise.
	 * \post
	 * If simplify returned true, this constraint has previously removed all its watches
	 * from the solver.
	 *
	 * \note The default implementation is a noop and returns false.
	 */
	virtual bool simplify(Solver& s, bool reinit = false);

	//! Returns an estimate of the constraint's complexity relative to a clause (complexity = 1).
	virtual uint32 estimateComplexity(const Solver& s) const;

	//! Shall return this if constraint is a clause, otherwise 0.
	/*!
	 * The default implementation returns 0.
	 */
	virtual ClauseHead* clause();

	//! Shall return whether the constraint is valid (i.e. not conflicting) w.r.t the current assignment in s.
	/*!
	 * \pre The assignment in s is not conflicting and fully propagated.
	 * \post A changed assignment if the assignment was not valid.
	 * \note The default implementation always returns true and assumes
	 *       that conflicts are detected by Constraint::propagate().
	 */
	virtual bool valid(Solver& s);
protected:
	virtual ~Constraint();
private:
	Constraint(const Constraint&);
	Constraint& operator=(const Constraint&);
};

//! Base class for learnt constraints.
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
	//! Returns the type of this learnt constraint.
	/*!
	 * \note The default implementation returns Constraint_t::learnt_conflict.
	 */
	ConstraintType type() const;

	/*!
	 * Shall return true if this constraint can't be deleted because it 
	 * currently implies one ore more literals and false otherwise.
	 */
	virtual bool locked(const Solver& s) const = 0;

	//! Returns the activity of the constraint.
	/*!
	 * \note A solver uses the activity-value in order to establish an ordering
	 * of learnt constraints. Whenever a solver needs to delete some learnt constraints it
	 * selects from the unlocked ones those with a low activity-value.
	 * \note The default-implementation always returns 0.
	 */
	virtual Activity activity() const;
	
	//! Asks the constraint to decrease its activity.
	virtual void decreaseActivity();
	//! Asks the constraint to reset its activity.
	virtual void resetActivity(Activity hint);
	
	//! Shall return 0 if either !t.inSet(type()) or this constraint is satisfied w.r.t the current assignment.
	/*!
	 * If this constraint is currently not satisfied and t.inSet(type()), shall return type()
	 * and freeLits shall contain all currently unassigned literals of this constraint.
	 */
	virtual uint32 isOpen(const Solver& s, const TypeSet& t, LitVec& freeLits) = 0;
protected:
	~LearntConstraint();
};

//! Base class for post propagators.
/*!
 * Post propagators are called after unit propagation on each decision level and
 * once after a total assignment is found.
 * 
 * They may extend a solver's unit-propagation with more elaborate propagation mechanisms.
 * The typical asp example is an unfounded set check.
 *
 * \note Currently, the solver distinguishes \b two classes of post propagators:
 *       - class_simple: deterministic post propagators shall only extend
 *         the current decision level. That is, given DL, the decision level on which propagation started,
 *         these post propagators shall neither backtrack below DL nor permanently add new decision levels. 
 *         Deterministic post propagators are called in priority order. For this,
 *         the function PostPropagator::priority() is used and shall return a priority in the range: <tt>[priority_class_simple, priority_class_general)</tt>
 *       - class_general: post propagators that are non-deterministic or those that are not limited to extending
 *         the current decision level shall have a priority of priority_class_general. They are called in LIFO order but
 *         only after \b all simple post propagators have reached a fixpoint. 
 *
 * \note There are currently three reserved priority values, namely
 *  - priority_reserved_msg for message and termination handler (if any),
 *  - priority_reserved_ufs for the default unfounded set checker (if any),
 *  - and priority_reserved_look for the default lookahead operator (if any).
 *  .      
 */
class PostPropagator : public Constraint {
public:
	PostPropagator();
	virtual ~PostPropagator();
	using Constraint::propagate;  // Enable overloading!

	PostPropagator* next; // main propagation lists of post propagators
	//! Default priorities for post propagators.
	enum Priority {
		priority_class_simple    = 0,    /**< Starting priority of simple post propagators. */
		priority_reserved_msg    = 0,    /**< Reserved priority for message/termination handlers (if any). */
		priority_reserved_ufs    = 10,   /**< Reserved priority for the default unfounded set checker (if any). */
		priority_reserved_look   = 1023, /**< Reserved priority for the default lookahead operator (if any). */
		priority_class_general   = 1024, /**< Priortiy of extended post propagators. */
	};

	//! Shall return a value representing the priority of this propagator.
	/*!
	 * The priority is used to order sequences of post propagators and to
	 * classify post propagators w.r.t the classes: class_simple and class_general.
	 * \note See class description for an overview of the two priority classes.
	 */
	virtual uint32 priority() const = 0;

	//! Called during initialization of s.
	/*!
	 * \note During initialization a post propagator may assign variables
	 *       but it must not yet propagate them.
	 */
	virtual bool   init(Solver& s);
	
	//! Shall enqueue and propagate new assignments implied by this propagator.
	/*!
	 * This function shall enqueue and propagate all assignments currently implied by 
	 * this propagator until a fixpoint is reached w.r.t this post propagator or 
	 * a conflict is detected.
	 * 
	 * \pre   The assignment is fully propagated w.r.t any previous post propagator.
	 * \param s    The solver in which this post propagator is used.
	 * \param ctx  The post propagator from which this post propagator is called or
	 *             0 if no other post propagator is currently active.
	 * \post  s.queueSize() == 0 || s.hasConflict()
	 * \return false if propagation led to conflict, true otherwise
	 * 
	 * \note 
	 *   The function shall not call Solver::propagate()
	 *   or any other function that would result in a recursive chain 
	 *   of propagate() calls. On the other hand, it shall call
	 *   Solver::propagateUntil(this) after enqueuing new assignments 
	 *   to initiate propagation up to this propagator.
	 * 
	 * Typically, propagateFixpoint() should implemet a loop like this:
	 * \code
	 * for (;;) {
	 *   if (!assign_newly_implied_literals(s)){ return false; }
	 *   if (s.queueSize() == 0)               { return true;  }
	 *   if (!s.propagateUntil(this))          { return false; }
	 * }
	 * \endcode
	 */
	virtual bool   propagateFixpoint(Solver& s, PostPropagator* ctx) = 0;

	//! Aborts an active propagation operation.
	/*!
	 * The function reset() is called whenever propagation on the
	 * current decision level is stopped before a fixpoint is reached.
	 * In particular, a solver calls reset() when a conflict is detected 
	 * during propagation.
	 *
	 * \note The default implementation is a noop.
	 */
	virtual void   reset();

	//! Is the current total assignment a model?
	/*!
	 * \pre The assignment is total and not conflicting.
	 * \return
	 *  - true if the assignment is a model w.r.t this post propagator
	 *  - false otherwise
	 * \post If the function returned true:  s.numFreeVars() == 0 && !s.hasConflict().
	 *       If the function returned false: s.numFreeVars() > 0 || s.hasConflict().
	 * \note The default implementation returns Constraint::valid(s);
	 */
	virtual bool   isModel(Solver& s);
protected:
	//! PostPropagators are not clonable by default.
	Constraint* cloneAttach(Solver&) { return 0; }
	// Constraint interface - noops
	PropResult propagate(Solver&, Literal, uint32&);
  void       reason(Solver&, Literal, LitVec& );
private:
	PostPropagator(const PostPropagator&);
	PostPropagator& operator=(const PostPropagator&);
};

class MessageHandler : public PostPropagator {
public:
	MessageHandler();
	virtual bool   handleMessages() = 0;
	virtual uint32 priority()const { return PostPropagator::priority_reserved_msg; }
	virtual bool   propagateFixpoint(Solver&, PostPropagator*) { return handleMessages(); }
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
 *  - 01: ternary constraint (i.e. two literals stored in the remaining 62 bits). 
 *  - 10: binary constraint (i.e. one literal stored in the highest 31 bits)
 * 
 */
class Antecedent {
public:
	enum Type { generic_constraint = 0, ternary_constraint = 1, binary_constraint = 2};
	//! Creates a null Antecedent. 
	/*!
	 * \post: isNull() == true && type == generic_constraint
	 */
	Antecedent() : data_(0) {}
	
	//! Creates an Antecedent from the literal p.
	/*!
	 * \post: type() == binary_constraint && firstLiteral() == p
	 */
	Antecedent(const Literal& p) {
		// first lit is stored in high dword
		data_ = (((uint64)p.index()) << 33) + binary_constraint;
		assert(type() == binary_constraint && firstLiteral() == p);
	}
	
	//! Creates an Antecedent from the literals p and q.
	/*!
	 * \post type() == ternary_constraint && firstLiteral() == p && secondLiteral() == q
	 */
	Antecedent(const Literal& p, const Literal& q) {
		// first lit is stored in high dword
		// second lit is stored in low dword
		data_ = (((uint64)p.index()) << 33) + (((uint64)q.index()) << 2) + ternary_constraint;
		assert(type() == ternary_constraint && firstLiteral() == p && secondLiteral() == q);
	}

	//! Creates an Antecedent from the Constraint con.
	/*!
	 * \post type() == generic_constraint && constraint() == con
	 */
	Antecedent(Constraint* con) : data_((uintp)con) {
		assert(type() == generic_constraint && constraint() == con);
	}

	//! Returns true if this antecedent does not refer to any constraint.
	bool isNull() const {
		return data_ == 0;
	}

	//! Returns the antecedent's type.
	Type type() const {
		return Type( data_ & 3 );
	}
	
	//! Returns true if the antecedent is a learnt nogood.
	bool learnt() const { return data_ && (data_ & 3u) == 0 && constraint()->type() != Constraint_t::static_constraint; }
	
	//! Extracts the constraint-pointer stored in this object.
	/*!
	 * \pre type() == generic_constraint
	 */
	Constraint* constraint() const {
		assert(type() == generic_constraint);
		return (Constraint*)(uintp)data_;
	}
	
	//! Extracts the first literal stored in this object.
	/*!
	 * \pre type() != generic_constraint
	 */
	Literal firstLiteral() const {
		assert(type() != generic_constraint);
		return Literal::fromIndex(static_cast<uint32>(data_ >> 33));
	}
	
	//! Extracts the second literal stored in this object.
	/*!
	 * \pre type() == ternary_constraint
	 */
	Literal secondLiteral() const {
		assert(type() == ternary_constraint);
		return Literal::fromIndex( static_cast<uint32>(data_>>1) >> 1 );
	}

	//! Returns the reason for p.
	/*!
	 * \pre !isNull()
	 */
	void reason(Solver& s, Literal p, LitVec& lits) const {
		assert(!isNull());
		Type t = type();
		if (t == generic_constraint) {
			constraint()->reason(s, p, lits);
		}
		else {
			lits.push_back(firstLiteral());
			if (t == ternary_constraint) {
				lits.push_back(secondLiteral());
			}
		}
	}
	template <class S>
	bool minimize(S& s, Literal p, CCMinRecursive* rec) const {
		assert(!isNull());
		Type t = type();
		if (t == generic_constraint) {
			return constraint()->minimize(s, p, rec);
		}
		else {
			return s.ccMinimize(firstLiteral(), rec)
				&& (t != ternary_constraint || s.ccMinimize(secondLiteral(), rec));
		}
	}

	//! Returns true iff the antecedent refers to the constraint con.
	bool operator==(const Constraint* con) const {
		return static_cast<uintp>(data_) == reinterpret_cast<uintp>(con);
	}

	//! Checks whether the imlementation-assumptions hold on this platform.
	/*! 
	 * throws PlatformError on error
	 */
	static bool checkPlatformAssumptions();

	uint64  asUint() const { return data_; }
	uint64& asUint()       { return data_; }
private:
	uint64 data_;
};


class PlatformError : public ClaspError {
public:
	explicit PlatformError(const char* msg);
};

}
#endif
