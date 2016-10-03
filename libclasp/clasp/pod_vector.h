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

#ifndef CLASP_POD_VECTOR_H_INCLUDED
#define CLASP_POD_VECTOR_H_INCLUDED
#include <clasp/util/platform.h>
#include <clasp/util/pod_vector.h>
#include <vector>
#include <cassert>

namespace Clasp {

#if defined(_DEBUG) && _DEBUG
	template <class Type>
	struct PodVector {
		typedef std::vector<Type> type;
		static void destruct(type& t) {t.clear();}
	};
#else
	//! Type selector for a vector type optimized for storing POD-types.
	template <class Type>
	struct PodVector {
		typedef bk_lib::pod_vector<Type> type;
		static void destruct(type& t) {
			for (typename type::size_type i = 0, end = t.size(); i != end; ++i) {
				t[i].~Type();
			}
			t.clear();
		}
	};
#endif
inline uint32 toU32(std::size_t x) { 
	assert(sizeof(std::size_t) <= sizeof(uint32) || x <= static_cast<uint64>(UINT32_MAX)); 
	return static_cast<uint32>(x);
}
template <class T>
inline uint32 sizeVec(const T& c) { return toU32(c.size()); }
template <class T>
inline void releaseVec(T& t) {
	T().swap(t);
}

template <class T>
inline void shrinkVecTo(T& t, typename T::size_type j) {
	t.erase(t.begin()+j, t.end());
}

template <class T>
inline void growVecTo(T& vec, typename T::size_type j, const typename T::value_type& val = typename T::value_type()) {
	if (vec.size() < j) {
		if (vec.capacity() < j) { vec.reserve(j + j / 2); }
		vec.resize(j, val);
	}
}

template <class T>
void moveDown(T& t, typename T::size_type from, typename T::size_type to) {
	for (typename T::size_type end = t.size(); from != end;) {
		t[to++] = t[from++];
	}
	shrinkVecTo(t, to);
}
//! A simple vector-based fifo queue for storing POD-types.
template <class T>
struct PodQueue {
	typedef typename PodVector<T>::type  vec_type;
	typedef typename vec_type::size_type size_type;
	PodQueue() : qFront(0) {}
	bool      empty() const   { return qFront == vec.size(); }
	size_type size()  const   { return vec.size() - qFront; }
	const T&  front() const   { return vec[qFront]; }
	const T&  back()  const   { return vec.back(); }
	T&        front()         { return vec[qFront]; }
	T&        back()          { return vec.back(); }
	void      push(const T& x){ vec.push_back(x);  }
	void      pop()           { ++qFront; }
	T         pop_ret()       { return vec[qFront++]; }
	void      clear()         { vec.clear(); qFront = 0; }
	void      rewind()        { qFront = 0; }
	vec_type  vec;    // the underlying vector holding the items
	size_type qFront; // front position
};

}

#endif
