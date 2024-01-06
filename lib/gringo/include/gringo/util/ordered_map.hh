#pragma once

#include <tsl/ordered_map.h>

namespace Gringo::Util {

template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key, T>>,
          class ValueTypeContainer = std::deque<std::pair<Key, T>, Allocator>, class IndexType = std::uint_least32_t>
using ordered_map = tsl::ordered_map<Key, T, Hash, KeyEqual, Allocator, ValueTypeContainer, IndexType>;

}
