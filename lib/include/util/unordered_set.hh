#pragma once

#include <tsl/hopscotch_set.h>

namespace Gringo::Util {

template <class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<Key>, unsigned int NeighborhoodSize = 62, bool StoreHash = false,
          class GrowthPolicy = tsl::hh::power_of_two_growth_policy<2>>
using unordered_set = tsl::hopscotch_set<Key, Hash, KeyEqual, Allocator, NeighborhoodSize, StoreHash, GrowthPolicy>;

}
