//
//  Copyright (c) Benjamin Kaufmann 2010
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. 
// 
//  This file is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//
// NOTE: ProgramOptions is inspired by Boost.Program_options
//       see: www.boost.org/libs/program_options
//
#ifndef PROGRAM_OPTIONS_VALUE_STORE_IMPL_H_INCLUDED
#define PROGRAM_OPTIONS_VALUE_STORE_IMPL_H_INCLUDED

#include <typeinfo>

namespace detail {
template <class T>
struct VTable {
	static void clone(const void* o, void** out) { 
		*out = new T(*static_cast<const T*>(o));
	}
	static void destroy(const void* o, void** out) { 
		delete static_cast<const T*>(o);
		*out = 0;
	}
	static void typeinfo(const void*, void** out)  { 
		*out = const_cast<void*>( static_cast<const void*>(&typeid(T)) ); 
	}
	static vtable_type vtable_s;
};
template <class T>
struct OptVTable {
	static void clone(const void* o, void** out) { 
		new (&*out) T(*static_cast<const T*>(o));
	}
	static void destroy(const void* o, void** out) { 
		static_cast<const T*>(o)->~T();
		*out = 0;
	}
	static vtable_type vtable_s;
};
template <class T>
vtable_type VTable<T>::vtable_s = {
	  0
	, &VTable<T>::clone
	, &VTable<T>::destroy
	, &VTable<T>::typeinfo
};
template <class T>
vtable_type OptVTable<T>::vtable_s = {
	(vcall_type)0x1
	, &OptVTable<T>::clone
	, &OptVTable<T>::destroy
	, &VTable<T>::typeinfo
};
template <bool> struct bool2type {};
template <class T>
inline vptr_type vtable_select(bool2type<0>, const T* = 0) { 
	return &VTable<T>::vtable_s;
}
template <class T>
inline vptr_type vtable_select(bool2type<1>, const T* = 0) { 
	return &OptVTable<T>::vtable_s;
}
template <class T>
inline vptr_type vtable(const T* x) { 
	return vtable_select(bool2type<sizeof(T)<=sizeof(void*)>(), x);
}

template <class T>
inline vptr_type base_vtable(const T* x) {
	return vtable_select(bool2type<0>(), x);
}

}

#endif
