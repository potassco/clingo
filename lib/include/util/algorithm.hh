#pragma once

#include <functional>
#include <optional>
#include <variant>
#include <vector>

namespace Gringo::Util {

namespace Detail {

template <class T, class M>
using opt_ret_t = decltype(std::make_optional(std::declval<M>()(std::forward<T>(std::declval<T>()).value())));

template <class T, class M>
using and_then_opt_ret_t = std::invoke_result_t<M, decltype(std::forward<T>(std::declval<T>()).value())>;

template <class T, class M>
using opt_vec_ret_t = std::optional<
    std::vector<std::decay_t<std::invoke_result_t<M, decltype(std::move(std::declval<T>().value().front()))>>>>;

} // namespace Detail

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

//! Map the value in the optional with the given predicate.
template <class T, class M> auto map_opt(T &&opt, M &&map) -> Detail::opt_ret_t<T, M> {
    if (opt.has_value()) {
        return std::make_optional(std::invoke(std::forward<M>(map), std::forward<T>(opt).value()));
    }
    return std::nullopt;
}

//! Similar to map_opt() but the predicate can fail by returning an optional.
template <class T, class M> auto and_then_opt(T &&opt, M &&map) -> Detail::and_then_opt_ret_t<T, M> {
    if (opt.has_value()) {
        return std::invoke(std::forward<M>(map), std::forward<T>(opt).value());
    }
    return std::nullopt;
}

//! Map the given predicate over an optional vector.
template <class T, class M> auto map_opt_vec(T &&vec, M &&map) -> Detail::opt_vec_ret_t<T, M> {
    return map_opt(std::forward<T>(vec), [&map](auto &&vec) {
        typename Detail::opt_vec_ret_t<T, M>::value_type ret;
        ret.reserve(vec.size());
        for (auto &&elem : vec) {
            // Note we assume that the vector owns the values and move them out
            // if the input is not an lvalue reference.
            if constexpr (std::is_lvalue_reference_v<decltype(vec)>) {
                ret.emplace_back(map(elem));
            } else {
                ret.emplace_back(map(std::move(elem)));
            }
        }
        return ret;
    });
}

//! Avoids copies of initializer_lists.
template <class T, class... Ts> auto make_vec(Ts &&...args) {
    std::vector<T> res;
    res.reserve(sizeof...(Ts));
    (res.emplace_back(std::forward<Ts>(args)), ...);
    return res;
}

//! Helper template to ease using std::visit.
#define GRINGO_IS_OF_TYPE(x, T) std::is_same_v<std::decay_t<decltype(x)>, T>

//! @}

} // namespace Gringo::Util
