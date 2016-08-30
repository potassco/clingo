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
#ifndef CLASP_HASH_MAP_H_INCLUDED
#define CLASP_HASH_MAP_H_INCLUDED
#include <utility>
#include <cstring>
#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER > 1500) || (defined(_LIBCPP_VERSION))
#include <unordered_map>
#include <unordered_set>
#define HASH_NS std
#else
#if defined(_MSC_VER)
#include <unordered_map>
#include <unordered_set>
#else
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#endif
#define HASH_NS std::tr1
#endif

namespace Clasp {
	//! Selector for a hash (multi) map.
	/*!
	 * \tparam K Key type of the hash map.
	 * \tparam V Value type of the hash map.
	 * \tparam H Hash function for hashing keys.
	 * \tparam E Comparison function for checking equality of keys.
	 * \note Can be replaced with std::unordered_map/unordered_multimap once we switch to C++11
	 */
	template<class K, class V, class H = HASH_NS::hash<K>, class E = std::equal_to<K>, class A = std::allocator< std::pair<const K, V> > >
	struct HashMap_t { 
		typedef HASH_NS::unordered_map<K, V, H, E,A>      map_type; 
		typedef HASH_NS::unordered_multimap<K, V, H, E,A> multi_map_type; 
	};
	//! Selector for a hash (multi) set.
	template<class K, class H = HASH_NS::hash<K>, class E = std::equal_to<K>, class A = std::allocator< K > >
	struct HashSet_t { 
		typedef HASH_NS::unordered_set<K, H, E,A>      set_type; 
		typedef HASH_NS::unordered_multiset<K, H, E,A> multi_set_type; 
	};

//! Hasher for strings.
/*!
 * \see http://research.microsoft.com/en-us/people/palarson/
 */
struct StrHash {
	std::size_t operator()(const char* str) const {
		std::size_t h = 0;
		for (const char* s = str; *s; ++s) {
			h = h * 101 + static_cast<std::size_t>(*s);
		}
		return h;
	}
};
//! Comparison function for C-strings to be used with hash map/set.
struct StrEq {
	bool operator()(const char* lhs, const char* rhs) const { return std::strcmp(lhs, rhs) == 0; }
};

}
#undef HASH_NS
#endif
