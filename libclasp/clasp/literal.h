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
#pragma warning (disable : 4996)
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

//! A variable is currently nothing more but an integer in the range [0..varMax).
typedef uint32 Var;

//! varMax is not a valid variale, i.e. currently Clasp supports at most 2^30 variables.
const Var varMax = (Var(1) << 30);

//! The variable 0 has a special meaning in the solver.
const Var sentVar = 0;

//! Ids are integers in the range [0..idMax).
const uint32 idMax = UINT32_MAX;

//! Possible types of a variable.
struct Var_t {
	enum Type { atom_var = 1, body_var = 2, atom_body_var = atom_var | body_var};
	static bool isBody(Type t) {
		return (static_cast<uint32>(t) & static_cast<uint32>(body_var)) != 0;
	}
	static bool isAtom(Type t) { 
		return (static_cast<uint32>(t) & static_cast<uint32>(atom_var)) != 0; 
	}
};
typedef Var_t::Type VarType;

//! A signed integer type used to represent weights.
typedef int32 weight_t;
//! A signed integer type used to represent sums of weights.
typedef int64 wsum_t;

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
	
	//! Creates a literal of the variable var with sign s.
	/*!
	 * \param var The literal's variable.
	 * \param s true if new literal should be negative.
	 * \pre var < varMax
	 */
	Literal(Var var, bool sign) : rep_( (var<<2) + (uint32(sign)<<1) ) {
		assert( var < varMax );
	}

	//! Returns the unique index of this literal.
	/*!
	 * \note The watch-flag is ignored and thus the index of a literal can be stored in 31-bits. 
	 */
	uint32 index() const { return rep_ >> 1; }
	
	//! Creates a literal from an index.
	/*!
	 * \pre idx < 2^31
	 */
	static Literal fromIndex(uint32 idx) {
		assert( idx < (uint32(1)<<31) );
		return Literal(idx<<1);
	}

	//! Creates a literal from an unsigned integer.
	static Literal fromRep(uint32 rep) { return Literal(rep); }
	
	uint32& asUint()        { return rep_; }
	uint32  asUint() const  { return rep_; }

	//! Returns the variable of the literal.
	Var var() const { return rep_ >> 2; }
	
	//! Returns the sign of the literal.
	/*!
	 * \return true if the literal is negative. Otherwise false.
	 */
	bool sign() const { return test_bit(rep_, 1); }

	void swap(Literal& other) { std::swap(rep_, other.rep_); }
	
	//! Sets the watched-flag of this literal.
	void watch() { store_set_bit(rep_, 0); }
	
	//! Clears the watched-flag of this literal.
	void clearWatch() { store_clear_bit(rep_, 0); }
	
	//! Returns true if the watched-flag of this literal is set.
	bool watched() const { return test_bit(rep_, 0); }

	//! Returns the complimentary literal of this literal.
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
inline Literal operator^(Literal lhs, bool sign) { return Literal::fromIndex( lhs.index() ^ uint32(sign) ); }
inline Literal operator^(bool sign, Literal rhs) { return rhs ^ sign; }

inline unsigned hashId(unsigned key) {  
	key = ~key + (key << 15);
	key ^= (key >> 11);
	key += (key << 3);
	key ^= (key >> 5);
	key += (key << 10);
	key ^= (key >> 16);
	return key;
}
inline uint32 hashLit(Literal p) { return hashId(p.index()); }
/////////////////////////////////////////////////////////////////////////////////////////
// Common interface
/////////////////////////////////////////////////////////////////////////////////////////

//! Creates the negative literal of variable v.
inline Literal negLit(Var v) { return Literal(v, true);}
//! Creates the positive literal of variable v.
inline Literal posLit(Var v) { return Literal(v, false);}
//! Returns the index of variable v.
/*!
 * \note same as posLit(v).index()
 */
inline uint32 index(Var v) { return v << 1; }

//! Returns true if p represents the special variable 0
inline bool isSentinel(Literal p) { return p.var() == sentVar; }

//! Defines a strict-weak-ordering for Literals.
inline bool operator<(const Literal& lhs, const Literal& rhs) {
	return lhs.index() < rhs.index();
}

//! Returns !(lhs == rhs).
inline bool operator!=(const Literal& lhs, const Literal& rhs) {
	return ! (lhs == rhs);
}

inline void swap(Literal& l, Literal& r) {
	l.swap(r);
}

typedef PodVector<Var>::type VarVec;          /**< A vector of variables.  */
typedef PodVector<Literal>::type LitVec;      /**< A vector of literals.   */
typedef PodVector<weight_t>::type WeightVec;  /**< A vector of weights.    */
typedef PodVector<wsum_t>::type SumVec;       /**< A vector of sums of weights. */

typedef std::pair<Literal, weight_t> WeightLiteral;  /**< A weight-literal. */
typedef PodVector<WeightLiteral>::type WeightLitVec; /**< A vector of weight-literals. */

///////////////////////////////////////////////////////////////////////////////
// Truth values
///////////////////////////////////////////////////////////////////////////////
typedef uint8 ValueRep;           /**< Type of the three value-literals. */
const ValueRep value_true   = 1;  /**< Value used for variables that are true. */
const ValueRep value_false  = 2;  /**< Value used for variables that are false. */
const ValueRep value_free   = 0;  /**< Value used for variables that are unassigned. */
typedef PodVector<ValueRep>::type ValueVec;

//! Returns the value that makes the literal lit true.
/*!
 * \param lit The literal for which the true-value should be determined.
 * \return
 *   - value_true     iff lit is a positive literal
 *   - value_false    iff lit is a negative literal.
 *   .
 */
inline ValueRep trueValue(const Literal& lit) { return 1 + lit.sign(); }

//! Returns the value that makes the literal lit false.
/*!
 * \param lit The literal for which the false-value should be determined.
 * \return
 *   - value_false      iff lit is a positive literal
 *   - value_true       iff lit is a negative literal.
 *   .
 */
inline ValueRep falseValue(const Literal& lit) { return 1 + !lit.sign(); }

//! Returns the sign that matches the value.
/*!
 * \return
 *   - false iff v == value_true
 *   - true  otherwise
 */
inline bool valSign(ValueRep v) { return v != value_true; }
//@}

//! Symbol table that maps external ids to internal literals.
/*!
 * A symbol table can be populated incrementally, but
 * each incremental step must be started with a call to
 * startInit() and stopped with a call to endInit().
 * New symbols only become visible once endInit() was called.
 * 
 * The following invariants must hold but are not fully checked:
 * 1. Symbol ids are added at most once
 * 2. All Ids added in step I+1 are greater than those added in step I
 */
class SymbolTable {
private:
	struct String {
		String(const char* n) : str(n) {}
		bool        empty() const { return str == 0 || !*str; }
		const char* c_str() const { return str; }
		char        operator[](uint32 i) const { return str[i]; }
	private: const char* str;
	};
public:
	typedef uint32                           key_type;
	typedef String                           data_type;
	struct                                   symbol_type {
		symbol_type(Literal x = negLit(0), data_type d = 0)
			: lit(x), name(d) {}
		mutable  Literal lit;
		data_type        name;
	};
	typedef std::pair<key_type, symbol_type> value_type;
	typedef PodVector<value_type>::type      map_type;
	typedef map_type::const_iterator         const_iterator;
	enum MapType { map_direct, map_indirect };

	//! Creates an empty symbol table.
	SymbolTable() : lastSort_(0), lastStart_(0), end_(0), type_(map_indirect), inc_(false) { }
	~SymbolTable() { clear();  }
	void   copyTo(SymbolTable& o) {
		o.clear();
		o.map_.reserve(map_.size());
		for (const_iterator it = map_.begin(), end = map_.end(); it != end; ++it) {
			o.map_.push_back(value_type(it->first, symbol_type(it->second.lit, dupName(it->second.name.c_str()))));
		}
		o.lastSort_ = lastSort_;
		o.lastStart_= lastStart_;
		o.end_      = end_;
		o.type_     = type_;
		o.inc_      = inc_;
		o.domLits   = domLits;
	}
	//! Type of symbol mapping.
	/*!
	 * In a direct mapping, symbol ids directly correspond to solver variables.
	 * In an indirect mapping, symbol ids are independent of solver variables and
	 * are instead mapped to solver literals.
	 */
	MapType type()    const { return type_; }
	//! Returns the number of symbols in this table.
	uint32  size()    const { return type() == map_indirect ? (uint32)map_.size() : end_; }
	//! Returns an iterator pointing to the beginning of this index.
	const_iterator begin()    const { return map_.begin(); }
	//! Returns an iterator pointing to the first atom of the current incremental step.
	const_iterator curBegin() const { return map_.begin() + lastStart_; }
	//! Returns an iterator pointing behind the end of this index.
	const_iterator end()      const { return map_.begin() + lastSort_;  }
	//! Returns the symbol with id i or 0 if no such symbol exists (yet).
	const symbol_type* find(key_type i) const {
		const_iterator it = std::lower_bound(begin(), end(), i, LessKey());
		return it != end() && it->first == i ? &it->second : 0;
	}
	const_iterator lower_bound(const_iterator start, key_type i) const {
		return std::lower_bound(start, end(), i, LessKey());
	}
	bool incremental() const { return inc_; }
	void incremental(bool b) { inc_ = b;    }
	//! Returns the symbol with the given id.
	/*!
	 * \pre find(id) != 0
	 */
	const symbol_type& operator[](key_type id) const { return *find(id); }
	//! Removes all symbols from this table.
	void   clear()      { 
		for (const_iterator it = map_.begin(), end = map_.end(); it != end; ++it) {
			freeName(it->second.name.c_str());
		}
		map_.clear();
		domLits.clear();
		lastSort_ = 0; 
		lastStart_= 0; 
		end_      = 0;
		inc_      = false;
	}
	//! Prepares the symbol table so that new symbols can be added.
	/*!
	 * \param type Type of mapping.
	 */
	void startInit(MapType type) {
		lastStart_ = map_.size();
		lastSort_  = lastStart_;
		type_      = type;
		inc_       = inc_ || size() != 0;
	}
	//! Adds the symbol with the given id to the symbol table.
	/*!
	 * \pre startInit() was called
	 * \pre the symbol table does not yet contain a symbol with the given id
	 * \note The new symbol only becomes visible once endInit() is called.
	 */
	symbol_type& addUnique(key_type id, const char* name) {
		assert((lastSort_ == 0 || map_[lastSort_-1].first < id) && "Symbol table: Invalid id in incremental step!");
		map_.push_back(value_type(id, symbol_type(negLit(0), dupName(name))));
		return map_.back().second;
	}
	//! Adds end of mapped range in direct mapping (i.e. [0, end)).
	void add(Var end) {
		end_ = end;
	}
	//! Freezes the symbol table and prepares it so that lookup operations become valid.
	void endInit() {
		std::sort(map_.begin()+lastSort_, map_.end(), LessKey());
		assert(unique() && "Symbol table: Duplicate atoms are not allowed\n");
		lastSort_ = map_.size();
	}
	LitVec domLits;
private:
	SymbolTable(const SymbolTable&);
	SymbolTable& operator=(const SymbolTable&);
	const char* dupName(const char* n) const {
		if (!n) return 0;
		std::size_t len = std::strlen(n);
		char*       dest= new char[len+1];
		std::strncpy(dest, n, len+1);
		return dest;
	}
	void freeName(const char* n) const { delete [] n; }
	bool unique() const {
		for (const_iterator it = map_.begin() + lastSort_, end = map_.end(), n = it + (it != end); n != end; it = n++) {
			if (it->first == n->first)  { return false; }
		}
		return true;
	}
	struct LessKey {
		bool operator()(const value_type& lhs, const value_type& rhs) const { return lhs.first < rhs.first; }
		bool operator()(const value_type& lhs, key_type i) const { return lhs.first < i; }
		bool operator()(key_type i, const value_type& rhs) const { return i < rhs.first; }
	};
	map_type            map_;
	map_type::size_type lastSort_;
	map_type::size_type lastStart_;
	uint32              end_;
	MapType             type_;
	bool                inc_;
};

class ClaspError : public std::runtime_error {
public:
	explicit ClaspError(const std::string& msg) 
		: std::runtime_error(msg){}
};

}
#endif
