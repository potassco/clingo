#pragma once

#include <tsl/ordered_set.h>

#include <gringo/util/hash.hh>

namespace Gringo::Util {

template <class Key, class Hash = value_hasher, class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<Key>, class ValueTypeContainer = std::deque<Key, Allocator>,
          class IndexType = std::uint_least32_t>
using ordered_set = tsl::ordered_set<Key, Hash, KeyEqual, Allocator, ValueTypeContainer, IndexType>;

}
