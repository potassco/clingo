#pragma once

#include <clingo/util/hash.hh>

#include <tsl/hopscotch_map.h>

namespace CppClingo::Util {

//! @addtogroup util_container
//! @{

//! Alias for unordered maps.
template <class Key, class T, class Hash = value_hasher, class KeyEqual = value_equal_to,
          class Allocator = std::allocator<std::pair<Key, T>>,
          unsigned int NeighborhoodSize = default_neighborhood_size, bool StoreHash = false,
          class GrowthPolicy = tsl::hh::power_of_two_growth_policy<2>>
using unordered_map = tsl::hopscotch_map<Key, T, Hash, KeyEqual, Allocator, NeighborhoodSize, StoreHash, GrowthPolicy>;

//! @}

} // namespace CppClingo::Util
