#pragma once

#include <optional>
#include <variant>
#include <vector>

#include <util/algorithm.hh>
#include <util/shared_ptr.hh>

namespace Gringo::Input {

namespace Detail {

// Note: GCC 11 has some problem with forward defined function and auto
// arguments. Thus, we simply make the functions available via ADL.
struct some_arg {};

template <class T, class F> using can_visit_t = std::enable_if_t<std::is_invocable_r_v<void, F, T const &>>;

template <class T, class F> using can_not_visit_t = std::enable_if_t<!std::is_invocable_r_v<void, F, T const &>>;

template <class... T, size_t... Indices>
auto visit_tuple(auto &&fun, std::tuple<T...> const &tup, std::index_sequence<Indices...> indices) {
    static_cast<void>(indices);
    (visit_value(some_arg{}, fun, std::get<Indices>(tup)), ...);
}

template <class T> auto visit_value(some_arg arg, auto &&fun, T const &x) -> can_visit_t<T, decltype(fun)> {
    static_cast<void>(arg);
    fun(x);
}

template <class T> auto visit_value(some_arg arg, auto &&fun, T const &x) -> can_not_visit_t<T, decltype(fun)> {
    static_cast<void>(arg);
    static_cast<void>(x);
}

template <class T>
auto visit_value(some_arg arg, auto &&fun, std::optional<T> const &opt)
    -> can_not_visit_t<std::optional<T>, decltype(fun)> {
    static_cast<void>(arg);
    if (opt.has_value()) {
        visit_value(some_arg{}, fun, opt.value());
    }
}

template <class T, class U>
auto visit_value(some_arg arg, auto &&fun, std::pair<T, U> const &pair)
    -> can_not_visit_t<std::pair<T, U>, decltype(fun)> {
    static_cast<void>(arg);
    visit_value(some_arg{}, fun, pair.first);
    visit_value(some_arg{}, fun, pair.second);
}

template <class... T>
auto visit_value(some_arg arg, auto &&fun, std::tuple<T...> const &tup)
    -> can_not_visit_t<std::tuple<T...>, decltype(fun)> {
    static_cast<void>(arg);
    return visit_tuple(fun, tup, std::index_sequence_for<T...>{});
}

template <class... T>
auto visit_value(some_arg arg, auto &&fun, std::variant<T...> const &var)
    -> can_not_visit_t<std::variant<T...>, decltype(fun)> {
    static_cast<void>(arg);
    return std::visit([&fun](auto const &elem) { visit_value(some_arg{}, fun, elem); }, var);
}

template <class T>
auto visit_value(some_arg arg, auto &&fun, std::vector<T> const &vec)
    -> can_not_visit_t<std::vector<T>, decltype(fun)> {
    static_cast<void>(arg);
    for (auto &elem : vec) {
        visit_value(some_arg{}, fun, elem);
    }
}

} // namespace Detail

template <class... Args> void visit_rec(auto &&fun, Args const &...args) {
    (Detail::visit_value(Detail::some_arg{}, std::forward<decltype(fun)>(fun), args), ...);
}

} // namespace Gringo::Input
