#pragma once

#include <optional>
#include <variant>
#include <vector>

#include <util/algorithm.hh>
#include <util/shared_ptr.hh>

namespace Gringo::Input {

template <class F, class T> struct Trans;

namespace Detail {

template <class T, class F>
using can_apply_t = std::enable_if_t<std::is_invocable_r_v<std::optional<T>, F, T const &>, std::optional<T>>;

template <class T, class F>
using can_not_apply_t = std::enable_if_t<!std::is_invocable_r_v<std::optional<T>, F, T const &>, std::optional<T>>;

template <class T> auto transform_value(auto &&fun, T const &x) -> Detail::can_apply_t<T, decltype(fun)>;

template <class T> auto transform_value(auto &&fun, T const &x) -> Detail::can_not_apply_t<T, decltype(fun)>;

template <class T>
auto transform_value(auto &&fun, std::optional<T> const &opt)
    -> Detail::can_not_apply_t<std::optional<T>, decltype(fun)>;

template <class... T>
auto transform_value(auto &&fun, std::tuple<T...> const &tup)
    -> Detail::can_not_apply_t<std::tuple<T...>, decltype(fun)>;

template <class... T>
auto transform_value(auto &&fun, std::variant<T...> const &var)
    -> Detail::can_not_apply_t<std::variant<T...>, decltype(fun)>;

template <class T>
auto transform_value(auto &&fun, std::vector<T> const &vec) -> Detail::can_not_apply_t<std::vector<T>, decltype(fun)>;

template <class... T, size_t... Indices>
auto transform_tuple(std::tuple<T...> const &tup, std::index_sequence<Indices...> indices,
                     std::optional<T>... transfomed) -> std::optional<std::tuple<T...>> {
    static_cast<void>(indices);
    if ((transfomed.has_value() || ...)) {
        return std::tuple<T...>(std::move(transfomed).value_or(std::get<Indices>(tup))...);
    }
    return std::nullopt;
}

template <class... T, size_t... Indices>
auto transform_tuple(auto &&fun, std::tuple<T...> const &tup, std::index_sequence<Indices...> indices)
    -> std::optional<std::tuple<T...>> {
    return transform_tuple(tup, indices, transform_value(fun, std::get<Indices>(tup))...);
}

template <class T> auto transform_value(auto &&fun, T const &x) -> Detail::can_apply_t<T, decltype(fun)> {
    return fun(x);
}

template <class T> auto transform_value(auto &&fun, T const &x) -> Detail::can_not_apply_t<T, decltype(fun)> {
    static_cast<void>(x);
    return std::nullopt;
}

template <class T>
auto transform_value(auto &&fun, std::optional<T> const &opt)
    -> Detail::can_not_apply_t<std::optional<T>, decltype(fun)> {
    if (opt.has_value()) {
        if (auto ret = transform_value(fun, opt.value()); ret.has_value()) {
            return ret;
        }
    }
    return std::nullopt;
}

template <class T, class U>
auto transform_value(auto &&fun, std::pair<T, U> const &pair)
    -> Detail::can_not_apply_t<std::pair<T, U>, decltype(fun)> {
    auto first = transform_value(fun, pair.first);
    auto second = transform_value(fun, pair.second);
    if (first.has_value() || second.has_value()) {
        return std::pair<T, U>{std::move(first).value_or(pair.first), std::move(second).value_or(pair.second)};
    }
    return std::nullopt;
}

template <class... T>
auto transform_value(auto &&fun, std::tuple<T...> const &tup)
    -> Detail::can_not_apply_t<std::tuple<T...>, decltype(fun)> {
    return Detail::transform_tuple(fun, tup, std::index_sequence_for<T...>{});
}

template <class... T>
auto transform_value(auto &&fun, std::variant<T...> const &var)
    -> Detail::can_not_apply_t<std::variant<T...>, decltype(fun)> {
    return std::visit(
        [&fun](auto const &elem) -> std::optional<std::variant<T...>> {
            auto ret = transform_value(fun, elem);
            if (ret.has_value()) {
                return std::variant<T...>{std::move(ret).value()};
            }
            return std::nullopt;
        },
        var);
}

template <class T>
auto transform_value(auto &&fun, std::vector<T> const &vec) -> Detail::can_not_apply_t<std::vector<T>, decltype(fun)> {
    size_t n = 0;
    std::optional<std::vector<T>> ret;
    for (auto &elem : vec) {
        std::optional<T> transformed = transform_value(fun, elem);
        if (transformed.has_value() && !ret.has_value()) {
            ret = Util::copy_n(vec, n);
        }
        if (ret.has_value()) {
            ret->emplace_back(std::move(transformed).value_or(elem));
        }
        ++n;
    }
    return ret;
}

template <class F, class T> auto transform_construct_apply(Trans<F, T> &arg) {
    return arg.transformed = transform_value(arg.fun, arg.orig);
}

template <class T> auto transform_construct_apply(T &&arg) { static_cast<void>(arg); }

template <class F, class T> auto transform_construct_value(Trans<F, T> &&arg) {
    return std::move(arg.transformed).value_or(arg.orig);
}

template <class T> auto transform_construct_value(T &&arg) { return std::forward<T>(arg); }

template <class T> auto transform_construct_has_value(T const &arg) -> bool {
    static_cast<void>(arg);
    return false;
}

template <class F, class T> auto transform_construct_has_value(Trans<F, T> const &arg) -> bool {
    return arg.transformed.has_value();
}

} // namespace Detail

auto transform(auto &&fun, auto const &x) { return Detail::transform_value(std::forward<decltype(fun)>(fun), x); };

template <class F, class T> struct Trans {
    Trans(T const &arg, F fun) : fun{std::move(fun)}, orig{arg} {}
    F fun;
    T const &orig;
    std::optional<T> transformed;
};

template <class T, class B = T, class... Args>
auto transform_construct_shared(Args &&...args) -> std::optional<Util::shared_ptr<B>> {
    (Detail::transform_construct_apply(args), ...);
    if ((Detail::transform_construct_has_value(args) || ...)) {
        return Util::construct_shared<T, B>(Detail::transform_construct_value(std::forward<Args>(args))...);
    }
    return std::nullopt;
}

template <class T, class... Args> auto transform_construct(Args &&...args) -> std::optional<T> {
    (Detail::transform_construct_apply(args), ...);
    if ((Detail::transform_construct_has_value(args) || ...)) {
        return T{Detail::transform_construct_value(std::forward<Args>(args))...};
    }
    return std::nullopt;
}

} // namespace Gringo::Input
