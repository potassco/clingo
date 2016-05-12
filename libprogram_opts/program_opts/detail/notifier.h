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
#ifndef PROGRAM_OPTIONS_NOTIFIER_H_INCLUDED
#define PROGRAM_OPTIONS_NOTIFIER_H_INCLUDED

namespace ProgramOptions { namespace detail {

///////////////////////////////////////////////////////////////////////////////
// Notifier
///////////////////////////////////////////////////////////////////////////////
// HACK:
// This should actually be replaced with a proper and typesafe delegate class.
// At least, it should be parametrized on the actual parameter type since
// the invocation of f via the generic func is not strictly conforming.
// Yet, it should work on all major compilers - we do not mess with
// the address and the alignment of void* should be compatible with that of T*.
template <class ParamT>
struct Notifier {
	typedef bool (*notify_func_type)(void*, const std::string& name, ParamT);
	Notifier() : obj(0), func(0) {}
	template <class O>
	Notifier(O* o, bool (*f)(O*, const std::string&, ParamT)) {
		obj = o;
		func= reinterpret_cast<notify_func_type>(f);
	}
	void*            obj;
	notify_func_type func;
	bool notify(const std::string& name, ParamT val) const {
		return func(obj, name, val);
	}
	bool empty() const { return func == 0; }
}; 

template <class ParamT, class ObjT>
struct Notify {
	typedef bool (*type)(ObjT*, const std::string& name, ParamT);
};

} }
#endif
