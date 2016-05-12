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
#include <program_opts/value_store.h>
#include <algorithm>
namespace ProgramOptions {

ValueStore::ValueStore()
	: vptr_(0)
	, value_(0) {
}
ValueStore::ValueStore(const ValueStore& other)
	: vptr_(other.vptr_)
	, value_(0) {
	if (!other.empty()) {
		clone(extract(const_cast<void**>(&other.value_)), &value_);
	}
}
ValueStore::~ValueStore() {
	clear();
}
ValueStore& ValueStore::operator=(ValueStore other) {
	other.swap(*this);
	return *this;
}
void ValueStore::swap(ValueStore& other) {
	std::swap(vptr_ , other.vptr_);
	std::swap(value_, other.value_);
}

const  std::type_info& ValueStore::type() const {
	if (!empty()) {
		void* x;
		(*vptr_)[vcall_typeid](0, &x);
		return *static_cast<const std::type_info*>( x );
	}
	struct internal_empty_type {};
	return typeid(internal_empty_type);
}

void ValueStore::clear() {
	if (!empty()) {
		(*vptr_)[vcall_destroy](extract(&value_), &value_);
		vptr_  = 0;
	}
}

void ValueStore::surrender() {
	vptr_ = 0;
}

void ValueStore::clone(const void* obj, void** out) const {
	(*vptr_)[vcall_clone](obj, out);
}

void* ValueStore::extract(void** v) const {
	if ((*vptr_)[call_extract] == 0) {
		return *v;
	}
	return reinterpret_cast<void*>(v);
}

}

