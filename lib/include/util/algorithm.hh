#pragma once

#include <functional>
#include <optional>
#include <vector>

namespace Gringo::Util {

namespace Detail {

template <class T> struct is_opt_ : std::false_type {};
template <class T> struct is_opt_<std::optional<T>> : std::true_type {};

template <class T> constexpr bool is_opt_v = is_opt_<std::remove_cvref_t<T>>::value;

template <class T, class M>
using opt_ret_t = decltype(std::make_optional(std::declval<M>()(std::forward<T>(std::declval<T>()).value())));

template <class T> struct is_opt_vec_ : std::false_type {};
template <class T> struct is_opt_vec_<std::optional<std::vector<T>>> : std::true_type {};

template <class T> constexpr bool is_opt_vec_v = is_opt_vec_<std::remove_cvref_t<T>>::value;

template <class T, class M>
using opt_vec_ret_t =
    std::optional<std::vector<std::decay_t<decltype(std::declval<M>()(std::move(std::declval<T>().value().front())))>>>;

} // namespace Detail

auto copy_n(auto const &vec, size_t n) {
    std::decay_t<decltype(vec)> ret;
    ret.reserve(vec.size());
    for (auto it = vec.begin(), ie = it + n; it != ie; ++it) {
        ret.emplace_back(*it);
    }
    return ret;
}

template <class T, class M>
auto map_opt(T &&opt, M &&map) -> std::enable_if_t<Detail::is_opt_v<T>, Detail::opt_ret_t<T, M>> {
    if (opt.has_value()) {
        return std::make_optional(std::invoke(std::forward<M>(map), std::forward<T>(opt).value()));
    }
    return std::nullopt;
}

template <class T, class M>
auto and_then_opt(T &&opt, M &&map) -> decltype(std::invoke(std::forward<M>(map), std::forward<T>(opt).value())) {
    if (opt.has_value()) {
        return std::invoke(std::forward<M>(map), std::forward<T>(opt).value());
    }
    return std::nullopt;
}

template <class T, class M>
auto map_opt_vec(T &&vec, M &&map) -> std::enable_if_t<Detail::is_opt_vec_v<T>, Detail::opt_vec_ret_t<T, M>> {
    return map_opt(std::move(vec), [&map](auto &&vec) {
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

/// Avoids copies of initializer_lists.
template <class T, class... Ts> auto make_vec(Ts &&...args) {
    std::vector<T> res;
    res.reserve(sizeof...(Ts));
    (res.emplace_back(std::forward<Ts>(args)), ...);
    return res;
}

} // namespace Gringo::Util
