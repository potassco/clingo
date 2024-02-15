#pragma once

#include <type_traits>

namespace Gringo::Util {

template <class S, class...> struct is_among : std::false_type {};
template <class S, class H, class... T>
struct is_among<S, H, T...> : std::conditional_t<std::is_same_v<S, H>, std::true_type, is_among<S, T...>> {};

template <class S, class... L> inline constexpr bool is_among_v = is_among<S, L...>::value;

} // namespace Gringo::Util
