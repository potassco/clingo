#pragma once

#include <optional>
#include <variant>
#include <vector>

#include <util/algorithm.hh>
#include <util/shared_ptr.hh>

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

struct anchor {};

template <class T, class B, class... Args>
auto transform_construct_shared_combine(auto const &fun, anchor a, Args &&...args) -> std::optional<shared_ptr<B>> {
    return construct_shared<T, B>(std::forward<Args>(args)...);
}
template <class T, class B, class Arg, class Trans, class... Args>
auto transform_construct_shared_combine(auto const &fun, Arg const &a, Trans b, Args &&...args)
    -> std::enable_if_t<!std::is_same_v<anchor, Arg>, std::optional<shared_ptr<B>>> {
    return transform_construct_shared_combine<T, B>(fun, std::forward<Args>(args)..., std::move(b).value_or(a));
}

template <class T, class B, class... Args>
auto transform_construct_shared(auto const &fun, bool changed, anchor a, Args &&...args)
    -> std::optional<shared_ptr<B>> {
    if (changed) {
        return transform_construct_shared_combine<T, B>(fun, std::forward<Args>(args)..., a);
    }
    return std::nullopt;
}

template <class T, class B, class Arg, class... Args>
auto transform_construct_shared(auto const &fun, bool changed, Arg const &a, Args &&...args)
    -> std::optional<shared_ptr<B>> {
    auto transformed = transform(fun, a);
    return transform_construct_shared<T, B>(fun, changed || transformed.has_value(), std::forward<Args>(args)..., a,
                                            std::move(transformed));
}

} // namespace detail

template <class T> auto transform(auto const &fun, T const &x) -> std::optional<T> { return std::nullopt; }

template <class T> auto transform(auto const &fun, std::optional<T> const &opt) -> std::optional<std::optional<T>> {
    return transform(fun, opt.value());
}

template <class T> auto transform(auto const &fun, shared_ptr<T> const &ptr) -> std::optional<shared_ptr<T>> {
    static_assert(std::is_same_v<decltype(fun(ptr)), std::optional<shared_ptr<T>>>);
    return fun(ptr);
}

template <class T, class U>
auto transform(auto const &fun, std::pair<T, U> const &pair) -> std::optional<std::pair<T, U>> {
    auto first = transform(fun, pair.first);
    auto second = transform(fun, pair.second);
    if (first.has_value() || second.has_value()) {
        return {std::move(first).value_or(pair.first), std::move(second).value_or(pair.second)};
    }
    return std::nullopt;
}

template <class... T> auto transform(auto const &fun, std::tuple<T...> const &tup) -> std::optional<std::tuple<T...>> {
    return detail::transform_tuple(fun, tup, std::index_sequence_for<T...>{});
}

template <class... T>
auto transform(auto const &fun, std::variant<T...> const &var) -> std::optional<std::variant<T...>> {
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

template <class T> auto transform(auto const &fun, std::vector<T> const &vec) {
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

template <class T, class B, class... Args>
auto transform_construct_shared(auto const &fun, Args const &...args) -> std::optional<shared_ptr<B>> {
    return detail::transform_construct_shared<T, B>(fun, false, args..., detail::anchor{});
}
