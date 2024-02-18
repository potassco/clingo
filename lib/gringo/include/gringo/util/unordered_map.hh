#pragma once

#include <tsl/hopscotch_map.h>

#include <gringo/util/hash.hh>

namespace Gringo::Util {

template <class Key, class T, class Hash = value_hasher, class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key, T>>, unsigned int NeighborhoodSize = 62,
          bool StoreHash = false, class GrowthPolicy = tsl::hh::power_of_two_growth_policy<2>>
using unordered_map = tsl::hopscotch_map<Key, T, Hash, KeyEqual, Allocator, NeighborhoodSize, StoreHash, GrowthPolicy>;

}
