#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>

namespace Gringo::Util {

//! @addtogroup util_algorithm
//! @{

//! Return a vector with the first n elements from the given one.
template <class Span> auto copy_n(Span const &vec, size_t n) -> std::vector<typename Span::value_type> {
    std::vector<std::remove_const_t<typename Span::value_type>> ret;
    ret.reserve(vec.size());
    for (auto it = vec.begin(), ie = it + n; it != ie; ++it) {
        ret.emplace_back(*it);
    }
    return ret;
}

//! Avoids copies of initializer_lists.
template <class T, class... Ts> auto make_vec(Ts &&...args) -> std::vector<T> {
    std::vector<T> res;
    res.reserve(sizeof...(Ts));
    (res.emplace_back(std::forward<Ts>(args)), ...);
    return res;
}

//! Remove all elements from the vector matching the given predicate.
template <class Vec, class Pred> auto erase_if(Vec &vec, Pred pred) -> size_t {
    auto it = std::remove_if(vec.begin(), vec.end(), std::move(pred));
    auto n = vec.end() - it;
    vec.erase(it, vec.end());
    return n;
}

#if __cpp_lib_unreachable
using std::unreachable;
#else
[[noreturn]] inline void unreachable() {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else                                        // GCC, Clang
    __builtin_unreachable();
#endif
}
#endif

//! @}

} // namespace Gringo::Util
