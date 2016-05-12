// 
// Copyright (c) 2010-2012, Benjamin Kaufmann
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

#ifndef CLASP_UTIL_ATOMIC_H_INCLUDED
#define CLASP_UTIL_ATOMIC_H_INCLUDED
#ifdef _MSC_VER
#pragma once
#endif

#if WITH_THREADS
#include <tbb/atomic.h>
namespace Clasp { using tbb::atomic; }
#else
namespace no_multi_threading {
template <class T>
struct atomic {
	typedef T value_type;
	atomic() : value(value_type()) {}
	atomic& operator=(value_type t) { value = t; return *this; }
	operator value_type() const { return value; }
	value_type operator+=(value_type v) { return value += v; }
	value_type operator-=(value_type v) { return value -= v; }
	value_type operator++()             { return ++value;    }
	value_type operator--()             { return --value;    }
	value_type operator->() const       { return value;    }
	value_type fetch_and_store(value_type v) {
		value_type last= value;
		value          = v;
		return last;
	}
	value_type compare_and_swap(value_type y, value_type z) {
		if (value == z) {
			value = y;
			return z;
		}
		return value;
	}
	T value;
};
}
namespace Clasp { using no_multi_threading::atomic; }
#endif


// effect: T temp; a |= mask; return temp
template <class T>
inline T fetch_and_or(Clasp::atomic<T>& a, T mask) {
	T x;
	do {
		x = a;
	} while (a.compare_and_swap(x|mask, x) != x);
	return x;
}

// effect: T temp; a &= mask; return temp
template <class T>
inline T fetch_and_and(Clasp::atomic<T>& a, T mask) {
	T x;
	do {
		x = a;
	} while (a.compare_and_swap(x&mask, x) != x);
	return x;
}

#endif
