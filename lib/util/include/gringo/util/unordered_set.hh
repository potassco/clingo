#pragma once

#include <tsl/hopscotch_set.h>

#include <gringo/util/hash.hh>

namespace Gringo::Util {

//! @addtogroup util_container
//! @{

//! Alias for unordered sets.
template <class Key, class Hash = value_hasher, class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<Key>, unsigned int NeighborhoodSize = default_neighborhood_size,
          bool StoreHash = false, class GrowthPolicy = tsl::hh::power_of_two_growth_policy<2>>
using unordered_set = tsl::hopscotch_set<Key, Hash, KeyEqual, Allocator, NeighborhoodSize, StoreHash, GrowthPolicy>;

//! @}

} // namespace Gringo::Util
