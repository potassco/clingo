#pragma once

#include <type_traits>

namespace Gringo::Util {

template <class S, class... L> inline constexpr bool is_among_v = (std::is_same_v<S, L> || ...);

template <class S, class... L> inline constexpr bool matches = (std::is_same_v<std::remove_cvref_t<S>, L> || ...);

} // namespace Gringo::Util
