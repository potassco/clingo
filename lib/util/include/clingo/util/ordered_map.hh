#pragma once

#include <clingo/util/hash.hh>

#include <tsl/ordered_map.h>

namespace CppClingo::Util {

//! @addtogroup util_container
//! @{

//! Alias for ordered maps.
template <class Key, class T, class Hash = value_hasher, class KeyEqual = value_equal_to,
          class Allocator = std::allocator<std::pair<Key, T>>,
          class ValueTypeContainer = std::vector<std::pair<Key, T>, Allocator>, class IndexType = std::uint_least32_t>
using ordered_map = tsl::ordered_map<Key, T, Hash, KeyEqual, Allocator, ValueTypeContainer, IndexType>;

//! @}

} // namespace CppClingo::Util
