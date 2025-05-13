#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>

namespace CppClingo::Util {

//! @addtogroup util_algorithm
//! @{

//! Return a vector with the first n elements from the given one.
template <class Rng> auto copy_n(Rng const &rng, size_t n) -> std::vector<typename Rng::value_type> {
    std::vector<std::remove_const_t<typename Rng::value_type>> ret;
    ret.reserve(rng.size());
    for (auto it = rng.begin(), ie = it + n; it != ie; ++it) {
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

//! Use std::transform to build a vector.
template <class It, class Pred> auto transform(It begin, It end, Pred pred) {
    auto p = std::vector<std::invoke_result_t<Pred, typename std::iterator_traits<It>::value_type>>{};
    p.reserve(std::distance(begin, end));
    std::transform(begin, end, std::back_inserter(p), pred);
    return p;
}

//! Use std::transform to build a vector.
template <class It, class Pred> auto transform_n(It begin, size_t n, Pred pred) {
    return transform(begin, begin + n, pred);
}

//! Use std::transform to build a vector.
template <class Rng, class Pred> auto transform(Rng const &rng, Pred pred) {
    using std::begin;
    using std::end;
    return transform(begin(rng), end(rng), pred);
}

//! Remove all elements from the vector matching the given predicate.
template <class Vec, class Pred> auto erase_if(Vec &vec, Pred pred) -> size_t {
    auto it = std::ranges::remove_if(vec, std::move(pred));
    auto n = vec.end() - it;
    vec.erase(it, vec.end());
    return n;
}

#if __cpp_lib_unreachable
using std::unreachable;
#else
//! C++23's std::unreachable.
[[noreturn]] inline void unreachable() {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else                                        // GCC, Clang
    __builtin_unreachable();
#endif
}
#endif

//! @}

} // namespace CppClingo::Util
