#pragma once

#include <optional>
#include <variant>
#include <vector>

#include <util/algorithm.hh>
#include <util/shared_ptr.hh>

template <class T> struct Trans {
    Trans(T const &arg, auto const &fun);
    T const &orig;
    std::optional<T> transformed;
};

namespace detail {

template <class... T, size_t... Indices>
auto transform_tuple(auto const &fun, std::tuple<T...> const &tup, std::index_sequence<Indices...> indices,
                     std::optional<T>... transfomed) -> std::optional<std::tuple<T...>> {
    if ((transfomed.has_value() || ...)) {
        return std::tuple<T...>(std::move(transfomed).value_or(std::get<Indices>(tup))...);
    }
    return std::nullopt;
}

template <class... T, size_t... Indices>
auto transform_tuple(auto const &fun, std::tuple<T...> const &tup, std::index_sequence<Indices...> indices)
    -> std::optional<std::tuple<T...>> {
    return transform_tuple(fun, tup, indices, transform(fun, std::get<Indices>(tup))...);
}

template <class T> auto transform_construct_value(Trans<T> &&arg) {
    return std::move(arg.transformed).value_or(arg.orig);
}

template <class T> auto transform_construct_value(T &&arg) { return std::forward<T>(arg); }

template <class T> auto transform_construct_has_value(T const &arg) -> bool { return false; }

template <class T> auto transform_construct_has_value(Trans<T> const &arg) -> bool {
    return arg.transformed.has_value();
}

} // namespace detail

template <class T, class F>
using can_apply_t = std::enable_if_t<std::is_invocable_r_v<std::optional<T>, F, T const &>, std::optional<T>>;

template <class T> auto transform(auto const &fun, T const &x) -> can_apply_t<T, decltype(fun)> { return fun(x); }

template <class T, class F>
using can_not_apply_t = std::enable_if_t<!std::is_invocable_r_v<std::optional<T>, F, T const &>, std::optional<T>>;

template <class T> auto transform(auto const &fun, T const &x) -> can_not_apply_t<T, decltype(fun)> {
    return std::nullopt;
}

template <class T>
auto transform(auto const &fun, std::optional<T> const &opt) -> can_not_apply_t<std::optional<T>, decltype(fun)> {
    return transform(fun, opt.value());
}

template <class T, class U>
auto transform(auto const &fun, std::pair<T, U> const &pair) -> can_not_apply_t<std::pair<T, U>, decltype(fun)> {
    auto first = transform(fun, pair.first);
    auto second = transform(fun, pair.second);
    if (first.has_value() || second.has_value()) {
        return {std::move(first).value_or(pair.first), std::move(second).value_or(pair.second)};
    }
    return std::nullopt;
}

template <class... T>
auto transform(auto const &fun, std::tuple<T...> const &tup) -> can_not_apply_t<std::tuple<T...>, decltype(fun)> {
    return detail::transform_tuple(fun, tup, std::index_sequence_for<T...>{});
}

template <class... T>
auto transform(auto const &fun, std::variant<T...> const &var) -> can_not_apply_t<std::variant<T...>, decltype(fun)> {
    return std::visit(
        [&fun](auto const &elem) -> std::optional<std::variant<T...>> {
            auto ret = transform(fun, elem);
            if (ret.has_value()) {
                return std::variant<T...>{std::move(ret).value()};
            }
            return std::nullopt;
        },
        var);
}

template <class T>
auto transform(auto const &fun, std::vector<T> const &vec) -> can_not_apply_t<std::vector<T>, decltype(fun)> {
    size_t n = 0;
    std::optional<std::vector<T>> ret;
    for (auto &elem : vec) {
        std::optional<T> transformed = transform(fun, elem);
        if (transformed.has_value() && !ret.has_value()) {
            ret = copy_n(vec, n);
        }
        if (ret.has_value()) {
            ret->emplace_back(std::move(transformed).value_or(elem));
        }
        ++n;
    }
    return ret;
}

template <class T> Trans<T>::Trans(T const &arg, auto const &fun) : orig{arg}, transformed{transform(fun, arg)} {}

template <class T, class B, class... Args>
auto transform_construct_shared(Args &&...args) -> std::optional<shared_ptr<B>> {
    if ((detail::transform_construct_has_value(args) || ...)) {
        return construct_shared<T, B>(detail::transform_construct_value(std::forward<Args>(args))...);
    }
    return std::nullopt;
}

template <class T, class... Args> auto transform_construct(Args &&...args) -> std::optional<T> {
    if ((detail::transform_construct_has_value(args) || ...)) {
        return T{detail::transform_construct_value(std::forward<Args>(args))...};
    }
    return std::nullopt;
}
