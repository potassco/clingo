#pragma once

#include <type_traits>

namespace Gringo::Util {

template <class S, class... L> inline constexpr bool is_among_v = (std::is_same_v<S, L> || ...);

} // namespace Gringo::Util
