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
#ifndef CLASP_LITERAL_H_INCLUDED
#define CLASP_LITERAL_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif
#include <clasp/util/platform.h>
#include <clasp/pod_vector.h>
#include <algorithm>  // std::swap
#include <limits>
/*!
 * \file 
 * Contains the definition of the class Literal.
 */
namespace Clasp {
/*!
 * \addtogroup constraint
 */
//@{

//! A variable is currently nothing more but an integer in the range [0..varMax)
typedef uint32 Var;

//! varMax is not a valid variale, i.e. currently Clasp supports at most 2^30 variables.
const Var varMax = (Var(1) << 30);

//! The variable 0 has a special meaning in the solver.
const Var sentVar = 0;

//! A signed integer type used to represent weights
typedef int32 weight_t;

typedef uint8 ValueRep;           /**< Type of the three value-literals */
const ValueRep value_true   = 1;  /**< Value used for variables that are true */
const ValueRep value_false  = 2;  /**< Value used for variables that are false */
const ValueRep value_free   = 0;  /**< Value used for variables that are unassigned */

//! A literal is a variable or its negation.
/*!
 * A literal is determined by two things: a sign and a variable index. 
 *
 * \par Implementation: 
 * A literal's state is stored in a single 32-bit integer as follows:
 *  - 30-bits   : var-index
 *  - 1-bit     : sign, 1 if negative, 0 if positive
 *  - 1-bit     : watch-flag 
 */
class Literal {
public:
	//! The default constructor creates the positive literal of the special sentinel var.
	Literal() : rep_(0) { }
	
	//! creates a literal of the variable var with sign s
	/*!
	 * \param var The literal's variable
	 * \param s true if new literal should be negative.
	 * \pre var < varMax
	 */
	Literal(Var var, bool sign) : rep_( (var<<2) + (uint32(sign)<<1) ) {
		assert( var < varMax );
	}

	//! returns the unique index of this literal
	/*!
	 * \note The watch-flag is ignored and thus the index of a literal can be stored in 31-bits. 
	 */
	uint32 index() const { return rep_ >> 1; }
	
	//! creates a literal from an index.
	/*!
	 * \pre idx < 2^31
	 */
	static Literal fromIndex(uint32 idx) {
		assert( idx < (uint32(1)<<31) );
		return Literal(idx<<1);
	}

	//! creates a literal from an unsigned integer.
	static Literal fromRep(uint32 rep) { return Literal(rep); }
	
	uint32& asUint()        { return rep_; }
	uint32  asUint() const  { return rep_; }

	//! returns the variable of the literal.
	Var var() const { return rep_ >> 2; }
	
	//! returns the sign of the literal.
	/*!
	 * \return true if the literal is negative. Otherwise false.
	 */
	bool sign() const { return test_bit(rep_, 1); }

	void swap(Literal& other) { std::swap(rep_, other.rep_); }
	
	//! sets the watched-flag of this literal
	void watch() { store_set_bit(rep_, 0); }
	
	//! clears the watched-flag of this literal
	void clearWatch() { store_clear_bit(rep_, 0); }
	
	//! returns true if the watched-flag of this literal is set
	bool watched() const { return test_bit(rep_, 0); }

	//! returns the complimentary literal of this literal.
	/*!
	 *  The complementary Literal of a Literal is a Literal referring to the
	 *  same variable but with inverted sign.
	 */
	inline Literal operator~() const {
		return Literal( (rep_ ^ 2) & ~static_cast<uint32>(1u) );
	}

	//! Equality-Comparison for literals.
	/*!
	 * Two Literals p and q are equal, iff
	 * - they both refer to the same variable
	 * - they have the same sign
	 * .
	 */
	inline bool operator==(const Literal& rhs) const {
		return index() == rhs.index();
	}
private:
	Literal(uint32 rep) : rep_(rep) {}
	uint32 rep_;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Common interface
/////////////////////////////////////////////////////////////////////////////////////////

//! creates the negative literal of variable v.
inline Literal negLit(Var v) { return Literal(v, true);}
//! creates the positive literal of variable v.
inline Literal posLit(Var v) { return Literal(v, false);}
//! returns the index of variable v.
/*!
 * \note same as posLit(v).index()
 */
inline uint32 index(Var v) { return v << 1; }

//! returns true if p represents the special variable 0
inline bool isSentinel(Literal p) { return p.var() == sentVar; }

//! defines a strict-weak-ordering for Literals.
inline bool operator<(const Literal& lhs, const Literal& rhs) {
	return lhs.index() < rhs.index();
}

//! returns !(lhs == rhs)
inline bool operator!=(const Literal& lhs, const Literal& rhs) {
	return ! (lhs == rhs);
}

inline void swap(Literal& l, Literal& r) {
	l.swap(r);
}
typedef PodVector<Var>::type VarVec;          /**< a vector of variables  */
typedef PodVector<Literal>::type LitVec;      /**< a vector of literals   */
typedef PodVector<weight_t>::type WeightVec;  /**< a vector of weights    */

typedef std::pair<Literal, weight_t> WeightLiteral;  /**< a weight-literal */
typedef PodVector<WeightLiteral>::type WeightLitVec; /**< a vector of weight-literals */

//@}

//! An atom of a program.
struct Atom {
	//! The default constructor creates an invalid literal
	Atom(const char* n = 0) : name(n?n:"") , lit(negLit(sentVar)) { }
	std::string name;     /**< Name of the atom - typically generated by lparse */
	mutable Literal lit;  /**< Corresponding literal in the solver */
};

//! Symbol table that maps atom ids to Atoms
/*!
 * A symbol table can be populated incrementally, but
 * each incremental step must be started with a call to
 * startInit() and stopped with a call to endInit().
 * Atoms can only be added between calls to startInit()
 * and endInit() and lookup-operations are only valid
 * after a call to endInit().
 * 
 * The following invariants must hold but are not fully checked:
 * 1. Atom ids are added at most once
 * 2. All Ids added in step I+1 are greater than those added in step I
 */
class AtomIndex {
public:
	typedef uint32                      key_type;
	typedef std::pair<key_type, Atom>   value_type;
	typedef PodVector<value_type>::type vec_type;
	typedef vec_type::const_iterator    const_iterator;
	typedef vec_type::iterator          iterator;
	//! creates an empty index
	AtomIndex() : lastSort_(0), lastStart_(0) {}
	~AtomIndex() {
		PodVector<value_type>::destruct(atoms_);
	}
	//! returns the number of atoms in this index
	uint32 size() const { return (uint32)atoms_.size(); }
	//! removes all atoms from this index
	void   clear()      { atoms_.clear(); lastSort_ = 0; }
	//! returns an iterator pointing to the beginning of this index
	const_iterator begin() const { return atoms_.begin(); }
	//! returns an iterator pointing to the first atom of the current incremental step
	const_iterator curBegin() const { return atoms_.begin() + lastStart_; }
	//! returns an iterator pointing behind the end of this index 
	const_iterator end()   const { return atoms_.end();   }

	//! prepares the symbol table so that new atoms can be added
	void startInit() {
		lastStart_ = atoms_.size();
		lastSort_  = lastStart_;
	}

	//! adds the atom with the given id to the symbol table
	/*!
	 * \pre startInit() was called
	 * \pre the symbol table does not yet contain an atom with the given id
	 */
	Atom& addUnique(key_type id, const Atom& a) {
		assert((lastSort_ == 0 || atoms_[lastSort_-1].first < id) && "Symbol table: Invalid id in incremental step!");
		atoms_.push_back(value_type(id, a));
		return atoms_.back().second;
	}
	
	//! freezes the symbol table and prepares it so that lookup operations become valid
	void endInit() {
		std::sort(atoms_.begin()+lastSort_, atoms_.end(), LessKey());
		lastSort_ = atoms_.size();
		assert(unique() && "Symbol table: Duplicate atoms are not allowed\n");
	}

	//! returns the atom with id i or 0 if no such atom exists
	/*!
	 * \pre endInit() was called
	 */
	Atom* find(key_type i) const {
		assert(lastSort_ == atoms_.size());
		const_iterator it = std::lower_bound(atoms_.begin(), atoms_.end(), i, LessKey());
		return it != atoms_.end() && it->first == i
			? const_cast<Atom*>(&it->second)
			: 0;
	}
	
	//! returns the atom with id i or 0 if no such atom exists
	/*!
	 * \pre find(id) != 0
	 */
	Atom& operator[](key_type id) const {
		Atom* a = find(id); assert(a);
		return *a;
	}

	/*!
	 * \pre no atom was added since last call to sortUnique
	 */
	const_iterator lower_bound(const_iterator start, key_type i) const {
		assert(lastSort_ == atoms_.size() && (start == atoms_.begin() || start == atoms_.end() || start->first <= i));
		return std::lower_bound(start, atoms_.end(), i, LessKey());
	}
private:
	bool unique() const {
		vec_type::size_type i = lastSort_;
		for (vec_type::size_type j = lastSort_+1; j < atoms_.size(); ++j) {
			if (atoms_[i].first == atoms_[j].first) {
				return false;
			}
		}
		return true;
	}
	AtomIndex(const AtomIndex&);
	AtomIndex& operator=(const AtomIndex&);
	struct LessKey {
		bool operator()(const value_type& lhs, const value_type& rhs) const { return lhs.first < rhs.first; }
		bool operator()(const value_type& lhs, key_type i) const { return lhs.first < i; }
		bool operator()(key_type i, const value_type& rhs) const { return i < rhs.first; }
	};
	vec_type            atoms_;
	vec_type::size_type lastSort_;
	vec_type::size_type lastStart_;
};

class ClaspError : public std::runtime_error {
public:
	explicit ClaspError(const std::string& msg) 
		: std::runtime_error(msg){}
};

}
#endif
