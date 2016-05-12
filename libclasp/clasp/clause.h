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
#ifndef CLASP_CLAUSE_H_INCLUDED
#define CLASP_CLAUSE_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include <clasp/constraint.h>
#include <clasp/util/misc_types.h>

namespace Clasp { 

//! A little helper-class for creating/adding clauses.
/*!
 * \ingroup constraint
 * This class simplifies clause creation. It hides the special handling of binary- and 
 * ternary clauses. It also makes sure that learnt clauses watch the literals 
 * from the highest decision levels.
 */
class ClauseCreator {
public:
	//! Creates a new ClauseCreator object
	/*!
	 * \param s the Solver in which to store created clauses.
	 */
	explicit ClauseCreator(Solver* s = 0);
	//! sets the solver in which created clauses are stored.
	void setSolver(Solver& s) { solver_ = &s; }
	//! reserve space for a clause of size s
	void reserve(LitVec::size_type s) { literals_.reserve(s); }
	//! clear the current clause
	void clear() { literals_.clear(); }
	
	//! starts the creation of a new clause
	/*!
	 * \pre s.decisionLevel() == 0 || t != Constraint_t::static_constraint
	 */
	ClauseCreator& start(ConstraintType t = Constraint_t::static_constraint);
	
	//! start the creation of an asserting or unit clause.
	/*!
   * All literals except the one passed to startAsserting() must be
	 * false and at least one of the false literals must be assigned on
	 * the current decision level.
	 * \param t The type of the learnt clause: either conflict or loop.
	 * \param assertingLit the literal to be asserted.
	 * \note position 0 is always reserved for the asserting literal.
	 */
	ClauseCreator& startAsserting(ConstraintType t, const Literal& assertingLit = Literal());
	
	//! adds the literal p to the clause.
	/*!
	 * \note p is only added if p is free or the current DL is > 0
	 * \pre clause neither contains p nor ~p.
	 * \pre if clause was started with startAsserting() p is false!
	 */
	ClauseCreator& add(const Literal& p);
	
	//! adds the clause to the solver if possible
	/*!
	 * Adds the clause only if it is not conflicting.
	 *
	 * \param      add clause even if satisfied w.r.t current assignment?
	 * \param[out] pointer to the newly created clause or 0 if no clause object was created
	 * \return true if the clause was added. Otherwise false.
	 * \note If the clause to be added is empty, false is returned and s.hasConflict() is true.
	 */
	bool end(bool addSat = false, Constraint** newCon = 0);

	//! adds the clause given in lits to the solver
	/*
	 * \param s     The solver to which the clause should be added
	 * \param t     The type of the clause
	 * \param lits  The literals of the clause
	 * \param sw    The position of the second watched literal in lits
	 * \param[out]  A pointer to the newly created clause or 0 if no clause object was created
	 *
	 * \pre !s.hasConflict() and s.decisionLevel() == 0 or t != Constraint_t::static_constraint
	 * \pre lits does not contain duplicate/complementary literals
	 * \pre the clause is not conflicting
	 * \pre lits is empty or lits[0] is either free or true
	 * \pre if lits[sw] is false all lits except for lits[0] are false and no literal was assigned on a higher dl
	 */
	static bool addClause(Solver& s, ConstraintType t, const LitVec& lits, uint32 sw, Constraint** newCon = 0);
	
	//! returns the literal at position i.
	/*!
	 * \pre i < size()
	 */
	Literal& operator[](LitVec::size_type i)       { return literals_[i]; }
	Literal  operator[](LitVec::size_type i) const { return literals_[i]; }
	//! returns the literals of the clause
	const LitVec&     lits()    const { return literals_; }
	LitVec&           lits()          { return literals_; }
	//! returns the current size of the clause.
	LitVec::size_type size()    const { return literals_.size(); }
	bool              empty()   const { return literals_.empty();}
	//! returns the clause's type.
	ConstraintType    type()    const { return type_; }
	//! returns the current position of the second watch
	uint32            sw()      const { return sw_; }
		
	//! Possible status values of a clause
	enum Status {
		status_subsumed, status_sat, status_empty, status_conflicting, status_unit, status_open
	};

	//! returns the current status of the clause
	/*!
	 * 
	 * \return Given a clause with literals [l1,...,ln] returns
	 *  - status_subsumed   : if exists li, s.isTrue(li) && s.level(li) == 0
	 *  - status_sat        : if exists li, s.isTrue(li)
	 *  - status_empty      : if n == 0 or for all li, (s.isFalse(li) && s.level(li) == 0)
	 *  - status_conflicting: if n != 0 and for all li, s.isFalse(li) and for some lj, s.level(lj) > 0
	 *  - status_unit       : if s.isFree(l1) and for all i > 1, s.isFalse(li)
	 *  - status_open       : else
	 */
	Status status() const;

	//! returns the implication level of the unit-resulting literal
	/*! 
	 * \pre  status() == status_unit
	 */
	uint32 implicationLevel() const { return swLev_; }
	//! returns the (numerical) highest DL on which the clause is conflicting
	/*!
	 * \pre status() == status_conflicting || status_empty
	 */
	uint32 conflictLevel()    const { return fwLev_; }

	//! removes duplicate lits. Marks the clause as sat if it contains p and ~p
	void simplify();
private:
	void init(ConstraintType t);
	Solver*         solver_;    // solver in which new clauses are stored
	LitVec          literals_;  // literals of the new clause
	uint32          fwLev_;     // decision level of the first watched literal
	uint32          swLev_;     // decision level of the second watched literal
	uint32          sw_;        // position of the second watch - only needed for learnt clauses
	ConstraintType  type_;      // type of the new clause
	bool            asserting_; // true if clause was started with startAsserting
};

//! Class implementing a clause.
/*!
 * \ingroup constraint
 * A clause watches two of its literals. Whenever one of the watched literals becomes false,
 * propagate tries to find a currently unwatched literal that is not false. If such a literal
 * is found, that literal is watched instead of the one that became false. Otherwise if no such
 * literal is found, the clause is unit and the other watched literal is asserted.
 */
class Clause : public LearntConstraint {
public:
	typedef Constraint::PropResult PropResult;
	
	/*!
	 * Creates a new native clause from the literals contained in lits.
	 * 
	 * \param s solver in which the new clause is to be used.
	 * \param lits The vector containing the literals for the clause.
	 *
	 * \pre lits contains at least two literals and:
	 *  - all literals are currently free.
	 *  - lits does not contain any duplicate literals.
	 *  - if lits contains literal p it must not contain ~p.
	 *  . 
	 * \note The clause must be destroyed using Clause::destroy
	 */ 
	static Constraint*  newClause(Solver& s, const LitVec& lits);
	
	/*!
	 * Creates a new learnt clause from the literals contained in lits.
	 * 
	 * \param s solver in which the new clause is to be used.
	 * \param lits The vector containing the literals for the clause.
	 * \param t The type of the learnt constraint.
	 * \param secondWatch The index of the second literal to watch.
	 *
	 * \pre lits contains at least two literals and:
	 *  - lits does not contain any duplicate literals.
	 *  - if lits contains literal p it must not contain ~p.
	 *  - lits[0] and lits[secondWatch] are valid watches, i.e. 
	 *    - they are either both not false or
	 *    - lits[secondWatch] is false AND
	 *      - for each l != lits[0] in lits, isFalse(l) && level(l) <= level(lits[secondWatch])
	 *      - !isFalse(lits[0]) OR level(lits[0]) >= level(lits[secondWatch])
	 *  . 
	 * \note The clause must be destroyed using Clause::destroy
	 */ 
	static LearntConstraint*  newLearntClause(Solver& s, const LitVec& lits, ConstraintType t, LitVec::size_type secondWatch);

	/*!
	 * Creates a new contracted learnt clause from the literals contained in lits. A contracted clause
	 * consists of an active head and a (false) tail. Propagation is restricted to the head. 
	 * The tail is only needed to compute reasons from assignments.
	 * 
	 * \param s solver in which the new clause is to be used.
	 * \param lits The vector containing the literals for the clause.
	 * \param secondWatch The index of the second literal to watch.
	 * \param The index of the first literal that should be temporarily removed from the clause.
	 * \pre see newLearntClause(Solver&, const LitVec&, ConstraintType, LitVec::size_type);
	 * \note The clause must be destroyed using Clause::destroy
	 */ 
	static LearntConstraint*  newContractedClause(Solver& s, const LitVec& lits, LitVec::size_type secondWatch, LitVec::size_type tailStart);
	
	// Constraint-Interface
	/*!
	 * implements unit propagation using a TWL-algorithm.
	 * The algorithm is as follows:
	 * - a) the watched literal that must be updated is determined and the other watched literal is stored in other.
	 * - b) a literal q != other for which s.isFalse(lit) returns false is searched.
	 * - ca) if such a q exists that literal is watched and the existing watch is removed.
	 * - cb) if no such q exists other is asserted. In this case the clause is either unit or false.
	 * If it is false, asserting other will lead to a conflict.
	 */
	PropResult propagate(const Literal&, uint32& data, Solver& s);

	/*!
	 * for a clause [x y p] the reason for p is ~x and ~y. 
	 * \pre propagate previously asserted p
	 * \note if the clause is a learnt clause, calling reason increases
	 * the clause's activity.
	 */
	ConstraintType reason(const Literal& p, LitVec& lits);

	//! returns true if clause is SAT.
	/*!
	 * removes from the clause all literals that are false.
	 * If the clause is binary or ternary after literal removal it is replaced with a special short-clause.
	 */
	bool simplify(Solver& s, bool = false);

	//! destroys the clause and frees its memory.
	void destroy();
	
	// LearntConstraint interface
	
	//! returns true iff clause is asserting
	bool locked(const Solver& s) const;
	
	//! removes the two watches of this clause from the given solver.
	void removeWatches(Solver& s);

	//! returns true if the clause is currently satisfied.
	/*!
	 * If the clause is currently not satisfied all free literals of
	 * the clause are copied to freeLits.
	 */
	bool isSatisfied(const Solver& s, LitVec& freeLits) const;
	
	//! returns the activity of this clause
	/*!
	 * The activity of a learnt clause is initially one plus the number of restarts
	 * the solver already had performed when the clause was learnt.
	 */
	uint32 activity() const {
		return const_cast<Clause*>(this)->act();
	}
	
	//! halves the activity of this clause
	void decreaseActivity() {
		if (type() != Constraint_t::static_constraint) {
			act() >>= 1;
		}
	}
	
	//! returns the type of this clause.
	ConstraintType type() const {
		return ConstraintType(type_);
	}
	
	// own interface
	//! returns the literal at position id
	Literal operator[](LitVec::size_type id) const {
		assert(id < size_);
		return *(begin() + id);
	}
	//! returns the size of the clause
	/*
	 * \note if the clause is currently contracted, only the literals in the "active" range are counted.
	 */
	uint32 size() const { return size_; }
	
	//! returns a pointer to the clause's first literal
	const Literal* begin()  const { return const_cast<Clause*>(this)->begin(); }
	//! returns a pointer that points just beyond the end of the clause.
	const Literal* end()    const { return const_cast<Clause*>(this)->end(); }
	
	Literal* begin()  { return lits() + 1; }
	Literal* end()    { return begin() + size_; }
protected:
	~Clause() {}
private:
	Clause(Solver& s, const LitVec& lits, LitVec::size_type sw, ConstraintType t, uint32 delLevel);
	// returns the starting position of the literal array, i.e.
	//  * lits_[0] if clause is a native constraint
	//  * lits_[1] if clause is a learnt constraint. In that case, lits_[0] stores the activity
	Literal* lits()   { return lits_ + (type_ != 0); }  
	const Literal* lits() const { return const_cast<Clause*>(this)->lits();}  

	bool isBound(uint32 idx, uint32 d) const {
		return idx == (type_!=0)+(d&1)*(size_+1);
	}
	// Contracts an asserting clause. In such a clause all but one literal is false and typically
	// most of the literals were assigned on earlier decision levels. Since backjumping typically removes
	// only few decision levels, many of the assigned literals stay assigned for a long time.
	// The idea behind contraction is to temporarily remove assigned literals from the clause in order
	// to speed up TWL (fewer literals - faster searches).
	// It is implemented here as follows:
	//  * the literals of the clause are sorted by decreasing decision level.
	//  * The literal array is divided into an "active" and a "passive" part. 
	//  * The "active" part contains the starting sentinel SL and the literals from the highest decision levels
	//  * The "passive" part contains the other literals and the ending sentinel SH
	//  * Let X be first literal of the "passive" part. contract now 
	//    * replaces X with ~X (note that ~X is true) and sets the watch-flag of ~X
	//    * sets the size of the clause to the number of literals between SL and ~X
	//    * adds an "undo"-watch to the decisions level on which X was assigned
	//      -> ~X acts as ending sentinel during TWL as long as it is assigned
	void contract(Solver& s);

	// Extends the "active" part of a currently contracted clause.
	void undoLevel(Solver& s);
	void initWatches(Solver& s);
	void initWatches(Solver& s, uint32 fw, uint32 sw);
	
	uint32& act() { return lits_[0].asUint(); }
	void bumpActivity() {
		if (type_ != Constraint_t::static_constraint) {
			++act();
		}
	}
	uint32  size_ : 30;     // "active" size
	uint32  type_ : 2;      //  if != 0 lits_[0] stores activity
	mutable Literal other_; // stores a literal that was recently true
	// The literals of this clause.
	// Layout: ActOpt SL lit1 ... litn SH
	// SL and SH are sentinel literals, s.th:
	//  * isTrue(SL)    && isTrue(SH)
	//  * SL.watched()  && SH.watched()
	// ActOpt stores the activity of the clause and is only present if clause is learnt
	// At all times two literals in the range [begin(), end()) are watched and have
	// their watched flag set.
	Literal           lits_[0];   
};



//! Constraint for Loop-Formulas
/*!
 * \ingroup constraint
 * Special purpose constraint for loop formulas of the form: R -> ~a1, ~a2, ..., ~an
 * where R is a conjunction (B1,...,Bm) of bodies that are false and a1...an are the atoms of
 * an unfounded set.
 * I.e. such a loop formula is equivalent to the following n clauses:
 * ~a1 v B1 v ... v Bm
 * ...
 * ~an v B1 v ... v Bm
 * Representing loop formulas as n clauses is wasteful because each clause
 * contains the same set of bodies. 
 * 
 * The idea behind LoopFormula is to treat the conjunction of atoms as a special
 * "macro-literal" L with the following properties:
 * - isTrue(L), iff for all ai isTrue(~ai) 
 * - isFalse(L), iff for some ai isFalse(~ai) 
 * - L is watchable, iff not isFalse(L)
 * - Watching L means watching all ai.
 * - setting L to true means setting all ai to false.
 * Using this convention the TWL-algo can be implemented as in a clause.
 * 
 * \par Implementation:
 * - The literal-array is divided into two parts, an "active clause" part and an atom part
 * - The "active clause" contains one atom and all bodies: [B1 ... Bj ~ai]
 * - The atom part contains all atoms: [~a1 ... ~an]
 * - Two of the literals of the "active clause" are watched (again: watching an atom means watching all atoms)
 * - If a watched atom becomes true, it is copied into the "active clause" and the TWL-algo starts.
 */
class LoopFormula : public LearntConstraint {
public:
	/*!
	 * Creates a new loop-formula for numAtoms atoms sharing the literals contained in bodyLits.
	 * 
	 * \param s solver in which the new loop-formula is to be used.
	 * \param bodyLits Pointer to an array of numBodies body-literals
	 * \param numBodies Number of body-literals in bodyLits
	 * \param numAtoms Number of atoms in the loop-formula.
	 *
	 * \pre all body-literals are currently false.
	 */ 
	static LoopFormula* newLoopFormula(Solver& s, Literal* bodyLits, uint32 numBodies, uint32 bodyToWatch, uint32 numAtoms) {
		void* mem = ::operator new( sizeof(LoopFormula) + (numBodies+numAtoms+3) * sizeof(Literal) );
		return new (mem) LoopFormula(s, numBodies+numAtoms, bodyLits, numBodies, bodyToWatch);
	}

	//! Adds an atom to the loop-formula
	/*!
	 * \pre the loop-formula currently contains fewer than numAtoms atoms
	 */
	void addAtom(Literal atom, Solver& s);
	
	//! notifies the installed heuristic about the new constraint.
	void updateHeuristic(Solver& s);
	
	//! returns the size of the loop-formula.
	uint32 size() const;
	
	// Constraint interface
	
	PropResult propagate(const Literal& p, uint32& data, Solver& s);
	ConstraintType reason(const Literal& p, LitVec& lits);
	bool simplify(Solver& s, bool = false);
	void destroy();
	
	// LearntConstraint interface
	bool locked(const Solver& s) const;
	void removeWatches(Solver& s);
	
	//! returns true if the loop-formula is satisfied
	/*!
	 * A loop-formula is satisfied if one body is true or all contained atoms are false.
	 * If the loop-formula is currently not satisfied isSatisfied copies all free literals
	 * to freeLits.
	 */
	bool isSatisfied(const Solver& s, LitVec& freeLits) const;
	
	//! returns the loop-formula's activity
	/*!
	 * The activity of a loop-formula is increased, whenever reason() is called.
	 */
	uint32 activity() const {
		return activity_;
	}
	
	//! halves the loop-formula's activity
	void decreaseActivity() {
		activity_ >>= 1;
	}
	
	//! returns Constraint_t::learnt_loop
	ConstraintType type() const {
		return Constraint_t::learnt_loop;
	}
private:
	LoopFormula(Solver& s, uint32 size, Literal* bodyLits, uint32 numBodies, uint32 bodyToWatch);
	bool watchable(const Solver& s, uint32 idx);
	bool isTrue(const Solver& s, uint32 idx);
	uint32  activity_;    // activity of this loop formula
	uint32  end_;         // position of second sentinel
	uint32  size_;        // size of lits_
	mutable uint32 other_;// stores the position of a literal that was recently true
	Literal lits_[0];     // S B1...Bm ai S a1...an
};
}
#endif
