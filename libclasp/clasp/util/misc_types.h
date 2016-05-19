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

#ifndef CLASP_UTIL_MISC_TYPES_H_INCLUDED
#define CLASP_UTIL_MISC_TYPES_H_INCLUDED

#include <clasp/util/platform.h>
#include <utility>    // std::pair
#include <functional> // std::unary_function, std::binary_function
#include <algorithm>
/*!
 * \file 
 * Some utility types and functions not specific to clasp.
 */
namespace Clasp {
/*!
 * \defgroup misc Miscellaneous and Internal Stuff
 */
//@{

inline unsigned hashId(unsigned key) {  
	key = ~key + (key << 15);
	key ^= (key >> 11);
	key += (key << 3);
	key ^= (key >> 5);
	key += (key << 10);
	key ^= (key >> 16);
	return key;
}

// Computes n choose k.
inline uint64 choose(unsigned n, unsigned k) {
	if (k == 0) return 1;
	if (k > n) return 0;
	if (2 * k > n) { return choose(n, n-k);}
	uint64 res = n;
	for (unsigned i = 2 ; i <= k; ++i) {
		res *= (n + 1 - i);
		res /= i;
	}
	return res;
}


extern uint32 randSeed_g; /**< stores the seed of the RNG */

//! Sets the starting point for random-number generation.
/*!
 * The function sets the starting point for generating a series of pseudorandom integers. 
 * To reinitialize the generator, use 1 as the seed argument. Any other value for seed 
 * sets the generator to a random starting point. Calling rand() before any call to srand()
 * generates the same sequence as calling srand() with seed passed as 1.
 */
inline void srand(uint32 seed) { randSeed_g = seed; }

//! Generates a pseudorandom number
/*!
 * The rand function returns a pseudorandom integer in the range 0 to 32767 
 * Use the srand function to seed the pseudorandom-number generator before calling rand.
 *
 * \note This function is a replacement for the standard rand-function. It is provided
 * in order to get reproducible random numbers among different compilers.
 *
 * \attention The rand() function is not thread-safe. Calls to rand() from different
 * threads must be serialized.
 */
inline uint32 rand() {
	return( ((randSeed_g = randSeed_g * 214013L + 2531011L) >> 16) & 0x7fff );
}

//! random floating point number in the range [0, 1.0)
/*!
 * \attention Function is not thread-safe
 */
inline double drand() {
	return Clasp::rand()/static_cast<double>(0x8000u);
}

//! random number in the range [0, max)
/*!
 * \attention Function is not thread-safe
 */
inline unsigned irand(unsigned max) {
	return static_cast<unsigned>(drand() * max);
}

//! An unary operator function that calls p->destroy()
struct DestroyObject {
	template <class T>
	void operator()(T* p) const {
		p->destroy();
	}
};

//! An unary operator function that calls delete p
struct DeleteObject {
	template <class T>
	void operator()(T* p) const {
		delete p;
	}
};

//! A predicate that checks whether a std::pair contains a certain value
template <class T>
struct PairContains {
	PairContains(const T& p) : p_(p) {}
	bool operator()(const std::pair<T, T>& s) const {
		return s.first == p_ || s.second == p_;
	}
	T p_;
};

//! Removes from the container c the first occurrence of a value v for which p(v) returns true
/*!
 * \pre C is a container that provides back() and pop_back()
 * \note Removal is implemented by replacing the element to be removed with 
 * the back()-element followed by a call to pop_back().
 */
template <class C, class P>
void remove_first_if(C& cont, const P& p) {
	for (typename C::iterator it = cont.begin(), end = cont.end(); it != end; ++it) {
		if (p(*it)) {
			*it = cont.back();
			cont.pop_back();
			return;
		}
	}
}

//! An unary operator function that simply returns its argument
template <class T>
struct identity : std::unary_function<T, T>{
	T&        operator()(T& x)      const { return x; }
	const T&  operator()(const T& x)  const { return x; }
};


//! An unary operator function that returns the first value of a std::pair
template <class P>
struct select1st : std::unary_function<P, typename P::first_type> {
	typename P::first_type& operator()(P& x) const {
		return x.first;
	}
	const typename P::first_type& operator()(const P& x) const {
		return x.first;
	}
};

//! An unary operator function that returns the second value of a std::pair
template <class P>
struct select2nd : std::unary_function<P, typename P::second_type> {
	typename P::second_type& operator()(P& x) const {
		return x.second;
	}
	const typename P::second_type& operator()(const P& x) const {
		return x.second;
	}
};

//! An unary operator function that returns Op1(Op2(x))
template <class OP1, class OP2>
struct compose_1 : public std::unary_function<
														typename OP2::argument_type, 
														typename OP1::result_type> {
	compose_1(const OP1& op1, const OP2& op2)
		: op1_(op1)
		, op2_(op2) {}
	
	typename OP1::result_type operator()(const typename OP2::argument_type& x) const {
		return op1_(op2_(x));
	}
protected:
	OP1 op1_;
	OP2 op2_;
};

/*!
 * A template helper function used to construct objects of type compose_1, 
 * where the component types are based on the data types passed as parameters.
 */
template <class OP1, class OP2>
inline compose_1<OP1, OP2> compose1(const OP1& op1, const OP2& op2) {
	return compose_1<OP1, OP2>(op1, op2);
}

//! An unary operator function that returns OP1(OP2(x), OP3(x))
template <class OP1, class OP2, class OP3>
struct compose_2_1 : public std::unary_function<
														typename OP2::argument_type, 
														typename OP1::result_type> {
	compose_2_1(const OP1& op1, const OP2& op2, const OP3& op3)
		: op1_(op1)
		, op2_(op2)
		, op3_(op3) {}
	
	typename OP1::result_type operator()(const typename OP2::argument_type& x) const {
		return op1_(op2_(x), op3_(x));
	}
protected:
	OP1 op1_;
	OP2 op2_;
	OP3 op3_;
};

/*!
 * A template helper function used to construct objects of type compose_2_1, 
 * where the component types are based on the data types passed as parameters.
 */
template <class OP1, class OP2, class OP3>
inline compose_2_1<OP1, OP2,OP3> compose2(const OP1& op1, const OP2& op2, const OP3& op3) {
	return compose_2_1<OP1, OP2, OP3>(op1, op2, op3);
}


//! A binary operator function that returns OP1(OP2(x), OP3(y))
template <class OP1, class OP2, class OP3>
struct compose_2_2 : public std::binary_function<
														typename OP2::argument_type, 
														typename OP3::argument_type,
														typename OP1::result_type> {
	compose_2_2(const OP1& op1, const OP2& op2, const OP3& op3)
		: op1_(op1)
		, op2_(op2)
		, op3_(op3) {}
	
	typename OP1::result_type operator()(const typename OP2::argument_type& x, const typename OP3::argument_type& y) const {
		return op1_(op2_(x), op3_(y));
	}
protected:
	OP1 op1_;
	OP2 op2_;
	OP3 op3_;
};

/*!
 * A template helper function used to construct objects of type compose_2_2, 
 * where the component types are based on the data types passed as parameters.
 */
template <class OP1, class OP2, class OP3>
inline compose_2_2<OP1, OP2,OP3> compose22(const OP1& op1, const OP2& op2, const OP3& op3) {
	return compose_2_2<OP1, OP2, OP3>(op1, op2, op3);
}

template <class T>
class SingleOwnerPtr {
public:
	explicit SingleOwnerPtr(T* ptr) : ptr_( set_bit(uintp(ptr),0) ) {}
	~SingleOwnerPtr() {
		if (is_owner()) { 
			delete release();
		}
	}
	SingleOwnerPtr& operator=(T* ptr) {
		SingleOwnerPtr t(ptr);
		t.swap(*this);
		return *this;
	}
	T& operator*()  const { return *get(); }
	T* operator->() const { return  get(); }
	T*    release()   { store_clear_bit(ptr_, 0); return get();  }
	T*    get() const { return (T*)clear_bit(ptr_, 0); }
	void swap(SingleOwnerPtr& o) {
		std::swap(ptr_, o.ptr_);
	}
	bool is_owner() const { return test_bit(ptr_, 0); }
private:
	SingleOwnerPtr(const SingleOwnerPtr&);
	SingleOwnerPtr& operator=(const SingleOwnerPtr&);
	uintp ptr_;
};


//@}
}

#endif
