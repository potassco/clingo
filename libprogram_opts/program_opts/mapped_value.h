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
#ifndef PROGRAM_OPTIONS_MAPPED_VALUE_H_INCLUDED
#define PROGRAM_OPTIONS_MAPPED_VALUE_H_INCLUDED
#ifdef _MSC_VER
#pragma warning (disable : 4786)
#pragma warning (disable : 4503)
#endif
#include "typed_value.h"
#include "value_store.h"
#include <string>
#include <cstddef>
#include <map>
#if defined(_MSC_VER) && _MSC_VER <= 1200
namespace std { using ::size_t; }
#endif

namespace ProgramOptions { 
///////////////////////////////////////////////////////////////////////////////
// ValueMap
///////////////////////////////////////////////////////////////////////////////
//! Type for storing anonymous values
/*!
 * Maps option names to their values
 */
class ValueMap {
public:
	ValueMap() {}
	~ValueMap(){}
	bool               empty() const { return map_.empty(); }
	size_t             size()  const { return map_.size(); }
	size_t             count(const std::string& name) const { return map_.count(name); }
	void               clear()       { map_.clear(); }
	const ValueStore& operator[](const std::string& name) const {
		MapType::const_iterator it = map_.find(name);
		if (it == map_.end()) {
			throw UnknownOption("ValueMap", name);
		}
		return it->second;
	}
	template <class T>
	static bool add(ValueMap* this_, const std::string& name, const T* value) {
		MapType::iterator it = this_->map_.find(name);
		if (it == this_->map_.end()) {
			it = this_->map_.insert(it, MapType::value_type(name, ValueStore()));
		}
		if (it->second.extract_raw() != value) {
			it->second.assimilate(const_cast<T*>(value));
		}
		return true;
	}
private:
	ValueMap(const ValueMap&);
	ValueMap& operator=(const ValueMap&);
	typedef std::map<std::string, ValueStore> MapType;
	MapType map_;
};

/*!
 * Creates a value that is created on demand and stored in a given value map.
 *
 * \see OptionGroup::addOptions()
 */
template <class T>
inline NotifiedValue<T>* store(ValueMap& map, typename detail::Parser<T>::type p = &string_cast<T>) {
	return notify<T>(&map, &ValueMap::add<T>, p);
}

inline NotifiedValue<bool>* flag(ValueMap& map, FlagAction a = store_true) {
	return flag(&map, &ValueMap::add<bool>, a);
}

}
#endif
