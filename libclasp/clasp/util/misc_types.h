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
 * Some utility types and functions.
 */
namespace Clasp {
class  Solver;
struct Model;

//! Base class for library events.
struct Event {
	enum Subsystem { subsystem_facade = 0, subsystem_load = 1, subsystem_prepare = 2, subsystem_solve = 3 };
	enum Verbosity { verbosity_quiet  = 0, verbosity_low  = 1, verbosity_high    = 2, verbosity_max   = 3 };
	explicit Event(Subsystem sys, uint32 evId, Verbosity verbosity) : system(sys), verb(verbosity), op(0), id(evId) {}
	uint32 system : 2; // one of Event::Subsystem - subsystem that produced the event
	uint32 verb   : 2; // one of Event::Verbosity - the verbosity level of this event
	uint32 op     : 8; // operation that triggered the event
	uint32 id     : 16;// type id of event
	static uint32 nextId();
};
template <class T>
struct Event_t : Event {
	Event_t(Subsystem sys, Verbosity verb) : Event(sys, id_s, verb) {}
	static const uint32 id_s;
};
template <class T> const uint32 Event_t<T>::id_s = Event::nextId();
//! A log message.
struct LogEvent : Event_t<LogEvent> {
	enum LogType { message = 'M', warning = 'W' };
	LogEvent(Subsystem sys, Verbosity verb, LogType t, const Solver* s, const char* what) : Event_t<LogEvent>(sys, verb), solver(s), msg(what) {
		op = static_cast<uint32>(t);
	}
	bool isWarning() const { return op == static_cast<uint32>(warning); }
	const Solver* solver;
	const char*   msg;
};
//! Creates a low priority message event.
inline LogEvent message(Event::Subsystem sys, const char* what, const Solver* s = 0) { return LogEvent(sys, Event::verbosity_high, LogEvent::message, s, what); }
template <Event::Verbosity V>
inline LogEvent message(Event::Subsystem sys, const char* what, const Solver* s = 0) { return LogEvent(sys, V, LogEvent::message, s, what); }
//! Creates a high priority warning event.
inline LogEvent warning(Event::Subsystem sys, const char* what, const Solver* s = 0){ return LogEvent(sys, Event::verbosity_quiet, LogEvent::warning, s, what); }
//! Base class for solving related events.
template <class T>
struct SolveEvent : Event_t<T> {
	SolveEvent(const Solver& s, Event::Verbosity verb) : Event_t<T>(Event::subsystem_solve, verb), solver(&s) {}
	const Solver* solver;
};
//! Event type for reporting models via generic event handler.
struct ModelEvent : SolveEvent<ModelEvent> {
	ModelEvent(const Model& m, const Solver& s) : SolveEvent<ModelEvent>(s, verbosity_quiet), model(&m) {}
	const Model* model;
};

template <class ToType, class EvType> const ToType* event_cast(const EvType& ev) { return ev.id == ToType::id_s ? static_cast<const ToType*>(&ev) : 0; }

class ModelHandler {
public:
	virtual ~ModelHandler() {}
	virtual bool onModel(const Solver&, const Model&) = 0;
};
class EventHandler : public ModelHandler {
public:	
	explicit EventHandler(Event::Verbosity verbosity = Event::verbosity_quiet) {
		for (uint32 i = 0; i != sizeof(verbosity_)/sizeof(verbosity_[0]); ++i) { verbosity_[i] = static_cast<uint8>(verbosity); }
	}
	virtual ~EventHandler();
	void setVerbosity(Event::Subsystem sys, Event::Verbosity verb) {
		verbosity_[sys] = static_cast<uint8>(verb);
	}
	void dispatch(const Event& ev) {
		if (ev.verb <= verbosity_[ev.system]) { onEvent(ev); }
	}
	virtual void onEvent(const Event& /* ev */) {}
	virtual bool onModel(const Solver& s, const Model& m) { onEvent(ModelEvent(m, s)); return true; }
private:
	EventHandler(const EventHandler&);
	EventHandler& operator=(const EventHandler&);
	uint8 verbosity_[4];
};

/*!
 * \defgroup misc Miscellaneous and Internal Stuff not specific to clasp.
 */
//@{
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

//! A very simple but fast Pseudo-random number generator
/*!
 * \note This class is a replacement for the standard rand-function. It is provided
 * in order to get reproducible random numbers among different compilers.
 */
class RNG {
public:
	explicit RNG(uint32 seed = 1) : seed_(seed) {}
	
	//! Sets the starting point for random-number generation.
	/*!
	 * The function sets the starting point for generating a series of pseudorandom integers. 
	 * To reinitialize the generator, use 1 as the seed argument. Any other value for seed 
	 * sets the generator to a random starting point. Calling rand() before any call to srand()
	 * generates the same sequence as calling srand() with seed passed as 1.
	 */	
	void srand(uint32 seed) { seed_ = seed; }
	
	//! Generates a pseudorandom number
	/*!
	 * The rand function returns a pseudorandom integer in the range 0 to 32767 
	 * Use the srand function to seed the pseudorandom-number generator before calling rand.
	 */
	uint32 rand() {
		return( ((seed_ = seed_ * 214013L + 2531011L) >> 16) & 0x7fff );
	}

	//! random floating point number in the range [0, 1.0)
	double drand() {
		return this->rand()/static_cast<double>(0x8000u);
	}

	//! random number in the range [0, max)
	unsigned irand(unsigned max) {
		return static_cast<unsigned>(drand() * max);
	}

	uint32 seed() const { return seed_; }

	uint32 operator()(unsigned max) { return irand(max); }
	uint32 operator()()             { return rand(); }
private:
	uint32 seed_;
};

//! An unary operator function that calls p->destroy()
struct DestroyObject {
	template <class T> void operator()(T* p) const { if (p) p->destroy(); }
};
//! An unary operator function that calls delete p
struct DeleteObject {
	template <class T> void operator()(T* p) const { delete p; }
};
//! An unary operator function that calls p->release()
struct ReleaseObject {
	template <class T> void operator()(T* p) const { if (p) p->release(); }
};

struct IsNull {
	template <class T> bool operator()(const T& p) const { return p == 0; }
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
	compose_2_2(const OP1& op1 = OP1(), const OP2& op2 = OP2(), const OP3& op3 = OP3())
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

template <class T, class D = DeleteObject>
class SingleOwnerPtr {
public:
	         SingleOwnerPtr()       : ptr_(0) {}
	explicit SingleOwnerPtr(T* ptr) : ptr_( set_bit(uintp(ptr),0) ) {}
	~SingleOwnerPtr()       { *this = 0; }
	bool is_owner()   const { return test_bit(ptr_, 0); }
	T*   get()        const { return (T*)clear_bit(ptr_, 0); }
	T&   operator*()  const { return *get(); }
	T*   operator->() const { return  get(); }
	SingleOwnerPtr& operator=(T* ptr) { reset(ptr); return *this; }
	void swap(SingleOwnerPtr& o) { std::swap(ptr_, o.ptr_); }
	T*   release()               { store_clear_bit(ptr_, 0); return get();  }
	T*   acquire()               { store_set_bit(ptr_, 0);   return get();  }
	void reset(T* x)             {
		if (x != get() && is_owner()) { D deleter; deleter(release()); }
		ptr_ = set_bit(uintp(x),0);
	}
private:
	SingleOwnerPtr(const SingleOwnerPtr&);
	SingleOwnerPtr& operator=(const SingleOwnerPtr&);
	uintp ptr_;
};

template <class T>
struct Range {
	Range(T x, T y) : lo(x), hi(y) { if (x > y)  { hi = x;  lo = y; } }
	T clamp(T val) const { 
		if (val < lo) return lo;
		if (val > hi) return hi;
		return val;
	}
	T lo;
	T hi;
};
//@}
}

#endif
