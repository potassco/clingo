#pragma once

#include <functional>
#include <optional>
#include <vector>

namespace Gringo::Util {

namespace Detail {

template <class T, class F> using transform_result = std::optional<std::remove_cv_t<std::invoke_result_t<F, T>>>;

template <class T, class F> using and_then_result = std::remove_cvref_t<std::invoke_result_t<F, T>>;

template <class T, class F>
using transform_vec_result = std::optional<std::vector<typename transform_result<T, F>::value_type>>;

} // namespace Detail

//! Implemenatation of std::optional<T>::transform.
template <class T, class F> constexpr auto transform(std::optional<T> &x, F &&f) -> Detail::transform_result<T &, F> {
    if (x.has_value()) {
        return std::invoke(std::forward<F>(f), *x);
    }
    return std::nullopt;
}

//! Implemenatation of std::optional<T>::transform.
template <class T, class F>
constexpr auto transform(std::optional<T> const &x, F &&f) -> Detail::transform_result<T const &, F> {
    if (x.has_value()) {
        return std::invoke(std::forward<F>(f), *x);
    }
    return std::nullopt;
}

//! Map the value in the optional with the given predicate.
//! Implemenatation of std::optional<T>::transform.
template <class T, class F> constexpr auto transform(std::optional<T> &&x, F &&f) -> Detail::transform_result<T, F> {
    if (x.has_value()) {
        return std::invoke(std::forward<F>(f), *std::move(x));
    }
    return std::nullopt;
}

//! Implemenatation of std::optional<T>::transform.
template <class T, class F>
constexpr auto transform(std::optional<T> const &&x, F &&f) -> Detail::transform_result<T const, F> {
    if (x.has_value()) {
        return std::invoke(std::forward<F>(f), *std::move(x));
    }
    return std::nullopt;
}

//! Implemenatation of std::optional<T>::and_then.
template <class T, class F> constexpr auto and_then(std::optional<T> &x, F &&f) -> Detail::and_then_result<T &, F> {
    if (x) {
        return std::invoke(std::forward<F>(f), *x);
    }
    return std::remove_cvref_t<std::invoke_result_t<F, T &>>{};
}

//! Implemenatation of std::optional<T>::and_then.
template <class T, class F>
constexpr auto and_then(std::optional<T> const &x, F &&f) -> Detail::and_then_result<T const &, F> {
    if (x) {
        return std::invoke(std::forward<F>(f), *x);
    }
    return std::remove_cvref_t<std::invoke_result_t<F, T const &>>{};
}

//! Implemenatation of std::optional<T>::and_then.
template <class T, class F> constexpr auto and_then(std::optional<T> &&x, F &&f) -> Detail::and_then_result<T, F> {
    if (x) {
        return std::invoke(std::forward<F>(f), *std::move(x));
    }
    return std::remove_cvref_t<std::invoke_result_t<F, T>>{};
}

//! Implemenatation of std::optional<T>::and_then.
template <class T, class F>
constexpr auto and_then(std::optional<T> const &&x, F &&f) -> Detail::and_then_result<T const, F> {
    if (x) {
        return std::invoke(std::forward<F>(f), *std::move(x));
    }
    return std::remove_cvref_t<std::invoke_result_t<F, T const>>{};
}

//! Map the given predicate over an optional vector.
template <class T, class F>
auto transform_vec(std::optional<std::vector<T>> const &vec, F const &f)
    -> Detail::transform_vec_result<T const &, F const &> {
    return transform(vec, [&f](auto const &vec) {
        typename Detail::transform_vec_result<T const &, F const &>::value_type ret;
        ret.reserve(vec.size());
        for (auto const &elem : vec) {
            ret.emplace_back(std::invoke(f, elem));
        }
        return ret;
    });
}

//! Map the given predicate over an optional vector.
template <class T, class F>
auto transform_vec(std::optional<std::vector<T>> &&vec, F const &f) -> Detail::transform_vec_result<T, F const &> {
    return transform(vec, [&f](auto &vec) {
        typename Detail::transform_vec_result<T, F const &>::value_type ret;
        ret.reserve(vec.size());
        for (auto &elem : vec) {
            ret.emplace_back(std::invoke(f, std::move(elem)));
        }
        return ret;
    });
}

} // namespace Gringo::Util
