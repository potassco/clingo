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
#include <vector>
#include <clasp/util/pod_vector.h>
namespace Clasp {

#ifdef _DEBUG
	template <class Type>
	struct PodVector {
		typedef std::vector<Type> type;
		static void destruct(type& t) {t.clear();}
	};
#else
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

template <class T>
void releaseVec(T& t) {
	T().swap(t);
}

template <class T>
void shrinkVecTo(T& t, typename T::size_type j) {
	t.erase(t.begin()+j, t.end());
}

}

#endif
