#pragma once

#include <algorithm>
#include <iterator>
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

//! Map a range into a vector using a transformation function.
template <class Container, std::input_iterator It, std::sentinel_for<It> Sent, typename Pred>
void into_vec(Container &vec, It first, Sent last, Pred &&pred) {
    vec.clear();
    if constexpr (requires { vec.reserve(std::distance(first, last)); }) {
        vec.reserve(std::distance(first, last));
    }
    std::ranges::transform(first, last, std::back_inserter(vec), std::forward<Pred>(pred));
}

//! Map a range into a vector using a transformation function.
template <class Container, class Rng, class Pred> void into_vec(Container &vec, Rng &&rng, Pred &&pred) { // NOLINT
    into_vec(vec, std::ranges::begin(rng), std::ranges::end(rng), std::forward<Pred>(pred));
}

//! Use a transformation function to build a vector from a range.
template <class Container, std::input_iterator It, std::sentinel_for<It> Sent, typename Pred, typename... Args>
auto to_vec(It begin, Sent end, Pred &&pred, Args &&...args) {
    Container p(std::forward<Args>(args)...);
    if constexpr (requires { p.reserve(std::distance(begin, end)); }) {
        p.reserve(std::distance(begin, end));
    }
    std::transform(begin, end, std::back_inserter(p), std::forward<Pred>(pred));
    return p;
}

//! Use a transformation function to build a vector from a range.
template <template <class, class...> class Container = std::vector, std::input_iterator It, std::sentinel_for<It> Sent,
          typename Pred, typename... Args>
auto to_vec(It begin, Sent end, Pred &&pred, Args &&...args) {
    using InputType = std::iter_value_t<It>;
    using OutputType = std::invoke_result_t<Pred, InputType>;
    return to_vec<Container<OutputType, Args...>>(begin, end, std::forward<Pred>(pred), std::forward<Args>(args)...);
}

//! Use std::transform to build a vector.
template <template <class, class...> class Container = std::vector, std::ranges::input_range Rng, typename Pred,
          typename... Args>
auto to_vec(Rng &&rng, Pred pred, Args &&...args) { // NOLINT
    return to_vec<Container>(std::ranges::begin(rng), std::ranges::end(rng), pred, std::forward<Args>(args)...);
}

//! Use std::transform to build a vector.
template <class Container, std::ranges::input_range Rng, typename Pred, typename... Args>
auto to_vec(Rng &&rng, Pred pred, Args &&...args) { // NOLINT
    return to_vec<Container>(std::ranges::begin(rng), std::ranges::end(rng), pred, std::forward<Args>(args)...);
}

//! Use std::transform to build a vector.
template <class It, class Pred> auto transform_n(It begin, size_t n, Pred pred) {
    return to_vec(begin, begin + n, pred);
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
