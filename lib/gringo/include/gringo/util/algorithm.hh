#pragma once

#include <functional>
#include <optional>
#include <vector>

namespace Gringo::Util {

//! @defgroup core_algorithm Generic Algorithms
//! Generic algorithms used throughout the library.
//!
//! @ingroup core_util
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

//! Remove all elements from the vector matching the given predicate.
auto erase_if(auto &vec, auto pred) {
    auto it = std::remove_if(vec.begin(), vec.end(), std::move(pred));
    auto n = vec.end() - it;
    vec.erase(it, vec.end());
    return n;
}

//! Helper template to ease using std::visit.
#define GRINGO_IS_INSTANCE(x, T) std::is_same_v<std::decay_t<decltype(x)>, T>

//! Helper template to ease using std::visit.
#define GRINGO_MATCH(x, T) if constexpr (std::is_same_v<std::decay_t<decltype(x)>, T>)

//! Helper template to ease using std::visit.
#define GRINGO_MATCH2(x, T, y, U)                                                                                      \
    if constexpr (std::is_same_v<std::decay_t<decltype(x)>, T> && std::is_same_v<std::decay_t<decltype(y)>, U>)

//! Helper to simplify forwarding of auto types.
#define GRINGO_FWD(x) std::forward<decltype(x)>(x)

//! @}

} // namespace Gringo::Util
