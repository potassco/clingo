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
#ifndef CLASP_SOLVER_TYPES_H_INCLUDED
#define CLASP_SOLVER_TYPES_H_INCLUDED
#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/literal.h>
#include <clasp/constraint.h>

/*!
 * \file 
 * Contains some types used by a Solver
 */
namespace Clasp {

/*!
 * \addtogroup solver
 */
//@{

//! Possible types of a variable.
struct Var_t {
	enum Value { atom_var = 1, body_var = 2, atom_body_var = atom_var | body_var};
};
typedef Var_t::Value VarType;
inline bool isBody(VarType t) { return (static_cast<uint32>(t) & static_cast<uint32>(Var_t::body_var)) != 0; }
inline bool isAtom(VarType t) { return (static_cast<uint32>(t) & static_cast<uint32>(Var_t::atom_var)) != 0; }

///////////////////////////////////////////////////////////////////////////////
// Count a thing or two...
///////////////////////////////////////////////////////////////////////////////
//! A struct for aggregating basic problem statistics
struct ProblemStats {
	ProblemStats() { reset(); }
	uint32  vars;
	uint32  eliminated;
	uint32  constraints[3]; /**< 0: all, 1: binary, 2: ternary */
	void    reset() { std::memset(this, 0, sizeof(*this)); }
	void diff(const ProblemStats& o) {
		vars           = std::max(vars, o.vars)-std::min(vars, o.vars);
		eliminated     = std::max(eliminated, o.eliminated)-std::min(eliminated, o.eliminated);
		for (int i = 0; i != 3; ++i) {
			constraints[i] = std::max(constraints[i], o.constraints[i])-std::min(constraints[i], o.constraints[i]);
		}		
	} 
};

//! A struct for holding core solving statistics used by a solver
struct CoreStats {
	CoreStats() { reset(); }
	void reset(){ std::memset(this, 0, sizeof(*this)); }
	void accu(const CoreStats& o) {
		choices  += o.choices;
		conflicts+= o.conflicts;
		restarts += o.restarts;
		models   += o.models;
		
		binary   += o.binary;
		ternary  += o.ternary;
		deleted  += o.deleted;
		modLits  += o.modLits;
		for (int i = 0; i != Constraint_t::max_value; ++i) {
			learnts[i] += o.learnts[i];
			lits[i]    += o.lits[i];
		}
	}
	void addLearnt(uint32 size, ConstraintType t) {
		assert(t != Constraint_t::native_constraint && t <= Constraint_t::max_value);
		++learnts[t-1];
		lits[t-1] += size;
		binary += (size == 2);
		ternary+= (size == 3);
	}
	void addModel(uint32 size) {
		++models;
		modLits += size;
	}
	
	uint64 choices;   /**< Number of choices performed */
	uint64 conflicts; /**< Number of conflicts found */
	uint64 restarts;  /**< Number of restarts */ 
	uint64 models;    /**< Number of models found */

	uint64 learnts[Constraint_t::max_value]; /**< Number of learnt nogoods of type t-1 */
	uint64 lits[Constraint_t::max_value];    /**< Sum of literals in nogoods of type t-1 */
	uint64 binary;    /**< Number of learnt binary nogoods */
	uint64 ternary;   /**< Number of learnt ternary nogoods*/
	uint64 deleted;   /**< Sum of learnt nogoods removed */
	uint64 modLits;   /**< Sum of decision literals in models */
};

//! A struct for jump statistics
struct JumpStats {
	JumpStats() { reset(); }
	void reset(){ std::memset(this, 0, sizeof(*this)); }
	void accu(const JumpStats& o) {
		jumps   += o.jumps;
		bJumps  += o.bJumps;
		jumpSum += o.jumpSum;
		boundSum+= o.boundSum;
		if (o.maxJump   > maxJump)   maxJump   = o.maxJump;
		if (o.maxJumpEx > maxJumpEx) maxJumpEx = o.maxJumpEx;
		if (o.maxBound  > maxBound)  maxBound  = o.maxBound;
	}
	void    update(uint32 dl, uint32 uipLevel, uint32 bLevel) {
		++jumps;
		jumpSum += dl - uipLevel; 
		maxJump = std::max(maxJump, dl - uipLevel);
		if (uipLevel < bLevel) {
			++bJumps;
			boundSum += bLevel - uipLevel;
			maxJumpEx = std::max(maxJumpEx, dl - bLevel);
			maxBound  = std::max(maxBound, bLevel - uipLevel);
		}
		else { maxJumpEx = maxJump; }
	}
	double avgJumpLen()   const { return jumpSum/std::max(1.0,(double)jumps); }
	double avgJumpLenEx() const { return (jumpSum-boundSum)/std::max(1.0,(double)jumps); }
	uint64  jumps;    /**< Number of backjumps (i.e. number of analyzed conflicts) */
	uint64  bJumps;   /**< Number of backjumps that were bounded */
	uint64  jumpSum;  /**< Number of levels that could be skipped w.r.t first-uip */
	uint64  boundSum; /**< Number of levels that could not be skipped because of backtrack-level*/
	uint32  maxJump;  /**< Longest possible backjump */
	uint32  maxJumpEx;/**< Longest executed backjump (< maxJump if longest jump was bounded) */
	uint32  maxBound; /**< Max difference between uip- and backtrack-level */
};

//! A struct for aggregating statistics of one solve operation
struct SolveStats : public CoreStats {
	SolveStats() : jumps(0) { }
	SolveStats(const SolveStats& o) : CoreStats(o), jumps(0) { if (o.jumps) jumps = new JumpStats(*o.jumps); }
	~SolveStats() { delete jumps; }
	void enableJumpStats()   { if (!jumps) jumps = new JumpStats();   }
	void reset() {  
		CoreStats::reset();
		if (jumps)   jumps->reset();
	}
	void accu(const SolveStats& o) {
		CoreStats::accu(o);
		if (jumps && o.jumps) jumps->accu(*o.jumps);
	}
	void updateJumps(uint32 dl, uint32 uipLevel, uint32 bLevel) {
		if (!jumps) return;
		jumps->update(dl, uipLevel, bLevel);
	}
	JumpStats*  jumps;  /**< optional jump statistics */
private:
	SolveStats& operator=(const SolveStats&);
};

//! A struct for aggregating some solver statistics
struct SolverStatistics{
	SolverStatistics() { }
	SolveStats   solve;
	ProblemStats problem;
};
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// A watch is a constraint and a "data-blob"
///////////////////////////////////////////////////////////////////////////////
//! Represents a watch in a Solver.
struct Watch {
	Watch(Constraint* a_con, uint32 a_data = 0) : con(a_con), data(a_data) {}
	//! calls propagate on the stored constraint and passes the stored data to that constraint.
	Constraint::PropResult propagate(Solver& s, Literal p) { return con->propagate(p, data, s); }
	//! returns true if other contains the same constraint as *this
	bool operator==(const Watch& other) const { return con == other.con; }
	Constraint* con;    /**< The constraint watching a certain literal */
	uint32      data;   /**< Additional data associated with this watch - passed to constraint on update */
};

//! returns the value that makes the literal lit true.
/*!
 * \param lit the literal for which the true-value should be determined.
 * \return
 *   - value_true     iff lit is a positive literal
 *   - value_false    iff lit is a negative literal.
 *   .
 */
inline ValueRep trueValue(const Literal& lit) { return 1 + lit.sign(); }

//! returns the value that makes the literal lit false.
/*!
 * \param lit the literal for which the false-value should be determined.
 * \return
 *   - value_false      iff lit is a positive literal
 *   - value_true       iff lit is a negative literal.
 *   .
 */
inline ValueRep falseValue(const Literal& lit) { return 1 + !lit.sign(); }

//! Stores static information about variables
class VarInfo {
public:
	enum FLAGS {
		CHOICE = 0x1u, // branch on this variable - current always 1?
		NANT   = 0x2u, // if this var is an atom, is it in NAnt(P)
		UNUSED = 0x4u, // currently not used
		PROJECT= 0x8u, // do we project on this var?
		BODY   = 0x10u,// is this var representing a body?
		EQ     = 0x20u,// is the var representing both a body and an atom?
		ELIM   = 0x40u,// was the variable eliminated?
		FROZEN = 0x80  // is the variable frozen?
	};
	VarInfo() {}
	void  reserve(uint32 maxSize) { info_.reserve(maxSize); }
	void  add(bool body) {
		uint8 m = flag(CHOICE) + (!body?0:flag(BODY));
		info_.push_back( m );
	}
	uint32    numVars()              const { return (uint32)info_.size(); }
	bool      isSet(Var v, FLAGS f)  const { return (info_[v] & flag(f)) != 0; }
	void      toggle(Var v, FLAGS f)       { info_[v] ^= flag(f); }
private:
	// Bit:   7     6   5   4    3    2   1     0
	//      frozen elim eq body proj    nant choice
	typedef PodVector<uint8>::type InfoVec;
	static uint8 flag(FLAGS x) { return uint8(x); }
	
	VarInfo(const VarInfo&);
	VarInfo& operator=(const VarInfo&);
	InfoVec info_;
};

//! Stores assignment related information.
/*!
 * For each variable v, the class stores 
 *  - v's current value (value_free if unassigned)
 *  - the decision level on which v was assign (only valid if value(v) != value_free)
 *  - the reason why v is in the assignment (only valid if value(v) != value_free)
 *  .
 * Furthermore, the class stores the sequences of assignments as a set of
 * true literals in its trail-member.
 */
class Assignment {
public:
	Assignment() : front(0) {}
	//! number of variables in the three-valued assignment
	uint32            numVars()    const { return (uint32)assign_.size(); }
	//! number of assigned variables
	uint32            assigned()   const { return (uint32)trail.size();   }
	//! returns the largest possible decision level
	uint32            maxLevel()   const { return (1u<<28)-1; }
	//! returns v's value in the three-valued assignment
	ValueRep          value(Var v) const { return ValueRep(assign_[v] & 3u); }
	//! returns v's previously saved value in the three-valued assignment
	ValueRep          saved(Var v) const { return v < saved_.size() ? saved_[v] : value_free; }
	//! returns the decision level on which v was assigned if value(v) != value_free
	uint32            level(Var v) const { return assign_[v] >> 4u; }
	//! returns the reason for v being assigned if value(v) != value_free
	const Antecedent& reason(Var v)const { return reason_[v]; }
	LitVec            trail; // assignment sequence
	LitVec::size_type front; // "propagation queue"
	bool    qEmpty() const { return front == trail.size(); }
	uint32  qSize()  const { return (uint32)trail.size() - front; }
	Literal qPop()         { return trail[front++]; }
	void    qReset()       { front  = trail.size(); }
	//! reserves space for nv variables
	void reserve(uint32 nv) {
		assign_.resize(nv);
		reason_.resize(nv);
	}
	//! adds var to assignment - initially the new var is unassigned
	Var addVar() {
		assign_.push_back(0);
		reason_.push_back(0);
		return numVars()-1;
	}
	//! assigns p.var() on level lev to the value that makes p true and store x as reason for the assignment
	/*!
	 * \return true if the assignment is consistent. False, otherwise.
	 * \post if true is returned, p is in trail. Otherwise, ~p is.
	 */
	bool assign(Literal p, uint32 lev, const Antecedent& x) {
		const Var      v   = p.var();
		const ValueRep val = value(v);
		if (val == value_free) {
			assign_[v] = (lev<<4) + trueValue(p);
			reason_[v] = x;
			trail.push_back(p);
			return true;
		}
		return val == trueValue(p);
	}
	//! undos all assignments in the range trail[first, last).
	/*!
	 * \param first first assignment to be undone
	 * \param save  if true, previous assignment of a var is saved before it is undone
	 */
	void undoTrail(LitVec::size_type first, bool save) {
		if (!save) { popUntil<&Assignment::clearValue>(trail[first]); }
		else       { saved_.resize(numVars(), 0); popUntil<&Assignment::saveAndClear>(trail[first]); }
		front  = trail.size();
	}
	//! undos the last assignment
	void undoLast() { clearValue(trail.back().var()); trail.pop_back(); }
	//! returns the last assignment as a true literal
	Literal last() const { return trail.back(); }
	Literal&last()       { return trail.back(); }
	//! sets val as "previous value" of v
	void setSavedValue(Var v, ValueRep val) {
		if (saved_.size() <= v) saved_.resize(v+1, 0);
		saved_[v] = val;
	}
	/*!
	 * \name implementation functions
	 * Low-level implementation functions. Use with care and only if you
	 * know what you are doing!
	 */
	//@{
	bool seen(Var v, uint8 m) const { return (assign_[v] & (m<<2)) != 0; }
	void setSeen(Var v, uint8 m)    { assign_[v] |= (m<<2); }
	void clearSeen(Var v)           { assign_[v] &= ~uint32(12); }
	void clearValue(Var v)          { assign_[v] = 0; }
	void setValue(Var v, ValueRep val) {
		assert(value(v) == value_free && (val == value_true || val == value_false));
		assign_[v] = val;
	}
	//@}
private:
	Assignment(const Assignment&);
	Assignment& operator=(const Assignment&);
	typedef PodVector<Antecedent>::type ReasonVec;
	typedef PodVector<uint32>::type     AssignVec;
	typedef PodVector<uint8>::type      SavedVec;
	void    saveAndClear(Var v) { saved_[v] = value(v); clearValue(v); }
	template <void (Assignment::*op)(Var v)>
	void popUntil(Literal stop) {
		Literal p;
		do {
			p = trail.back(); trail.pop_back();
			(this->*op)(p.var());
		} while (p != stop);
	}
	AssignVec assign_;
	ReasonVec reason_;
	SavedVec  saved_;
};

//! Stores information about a literal that is implied on an earlier level than the current decision level.
struct ImpliedLiteral {
	ImpliedLiteral(Literal a_lit, uint32 a_level, const Antecedent& a_ante) 
		: lit(a_lit)
		, level(a_level)
		, ante(a_ante) {
	}
	Literal     lit;    /**< The implied literal */
	uint32      level;  /**< The earliest decision level on which lit is implied */
	Antecedent  ante;   /**< The reason why lit is implied on decision-level level */
};

//@}
}
#endif
