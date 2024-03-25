#pragma once

#include <gringo/util/hash.hh>

#include <tsl/ordered_set.h>

namespace Gringo::Util {

//! @addtogroup util_container
//! @{

//! Alias for ordered sets.
template <class Key, class Hash = value_hasher, class KeyEqual = value_equal_to, class Allocator = std::allocator<Key>,
          class ValueTypeContainer = std::deque<Key, Allocator>, class IndexType = std::uint_least32_t>
using ordered_set = tsl::ordered_set<Key, Hash, KeyEqual, Allocator, ValueTypeContainer, IndexType>;

//! @}

} // namespace Gringo::Util
