#pragma once

#include <functional>
#include <optional>
#include <vector>

namespace Gringo::Util {

//! @defgroup core_algorithm Generic Algorithms
//! @ingroup core_util
//!
//! Generic algorithms used throughout the library.
//!
//! @{

//! Return a vector with the first n elements from the given one.
auto copy_n(auto const &vec, size_t n) {
    std::vector<typename std::decay_t<decltype(vec)>::value_type> ret;
    ret.reserve(vec.size());
    for (auto it = vec.begin(), ie = it + n; it != ie; ++it) {
        ret.emplace_back(*it);
    }
    return ret;
}

//! Avoids copies of initializer_lists.
template <class T, class... Ts> auto make_vec(Ts &&...args) {
    std::vector<T> res;
    res.reserve(sizeof...(Ts));
    (res.emplace_back(std::forward<Ts>(args)), ...);
    return res;
}

//! Helper template to ease using std::visit.
#define GRINGO_MATCH(x, T) if constexpr (std::is_same_v<std::decay_t<decltype(x)>, T>)

//! Helper template to ease using std::visit.
#define GRINGO_MATCH2(x, T, y, U)                                                                                      \
    if constexpr (std::is_same_v<std::decay_t<decltype(x)>, T> && std::is_same_v<std::decay_t<decltype(y)>, U>)

//! Helper to simplify forwarding of auto types.
#define GRINGO_FWD(x) std::forward<decltype(x)>(x)

//! @}

} // namespace Gringo::Util
