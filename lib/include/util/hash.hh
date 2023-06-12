#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <typeinfo>
#include <variant>
#include <vector>

#include <util/shared_ptr.hh>

#define HASH(T)                                                                                                        \
    namespace std {                                                                                                    \
                                                                                                                       \
    template <> struct hash<T> {                                                                                       \
        auto operator()(T const &b) const { return b.hash(); }                                                         \
    };                                                                                                                 \
                                                                                                                       \
    } // namespace std

template <class A, class B> auto value_equal_to(A const &a, B const &b) { return a == b; }

template <class A, class B> auto value_equal_to(A const *a, B const *b) { return *a == *b; }

template <class A, class B> auto value_equal_to(std::optional<A> const &a, std::optional<B> const &b) {
    if (a.has_value() && b.has_value()) {
        return value_equal_to(a.value(), b.value());
    }
    return !a.has_value() && !b.has_value();
}

template <class A, class B> auto value_equal_to(std::unique_ptr<A> const &a, std::unique_ptr<B> const &b) {
    return *a == *b;
}

template <class A, class B> auto value_equal_to(shared_ptr<A> const &a, shared_ptr<B> const &b) {
    return value_equal_to(*a, *b);
}

template <class A, class B, class C, class D> auto value_equal_to(std::pair<A, B> const &a, std::pair<C, D> const &b) {
    return value_equal_to(a.first, b.first) && value_equal_to(a.second, b.second);
}

namespace detail {

template <class... T, size_t... Indices>
inline auto value_equal_to_tuple(std::tuple<T...> const &a, std::tuple<T...> const &b,
                                 std::index_sequence<Indices...> indices) -> size_t {
    static_cast<void>(indices);
    return (true && ... && value_equal_to(std::get<Indices>(a), std::get<Indices>(b)));
}

} // namespace detail

template <class... A, class... B> auto value_equal_to(std::tuple<A...> const &a, std::tuple<B...> const &b) {
    static_assert(std::tuple_size_v<std::tuple<A...>> == std::tuple_size_v<std::tuple<B...>>);
    return detail::value_equal_to_tuple(a, b, std::index_sequence_for<A...>{});
}

template <class A, class B> auto value_equal_to(std::vector<A> const &a, std::vector<B> const &b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](auto const &a, auto const &b) { return value_equal_to(a, b); });
}

inline auto value_equal_to() { return true; }

template <class A, class B, class... Args> auto value_equal_to(A const &a, B const &b, Args const &...args) {
    return value_equal_to(a, b) && value_equal_to(args...);
}

inline auto hash_combine(size_t a, size_t b) -> size_t {
    std::array<char, 2 * sizeof(size_t)> buf;
    std::memcpy(buf.data(), &a, sizeof(a));
    std::memcpy(buf.data() + sizeof(a), &b, sizeof(b));
    return std::hash<std::string_view>{}(std::string_view(buf.begin(), buf.end()));
}

template <class T> inline auto value_hash(T const &value) -> size_t { return std::hash<T>{}(value); }

inline auto value_hash(std::type_info const &value) -> size_t { return value.hash_code(); }

template <class T> inline auto value_hash(T const *value) -> size_t { return value_hash(*value); }

template <class T> inline auto value_hash(std::unique_ptr<T> const &value) -> size_t { return value_hash(*value); }

template <class T> inline auto value_hash(shared_ptr<T> const &value) -> size_t { return value_hash(*value); }

template <class A, class B> inline auto value_hash(std::pair<A, B> const &value) -> size_t {
    return value_hash(value.first, value.second);
}

namespace detail {

template <class... T, size_t... Indices>
inline auto value_hash_tuple(std::tuple<T...> const &value, std::index_sequence<Indices...> indices) -> size_t {
    static_cast<void>(indices);
    return value_hash(std::get<Indices>(value)...);
}

} // namespace detail

template <class... T> inline auto value_hash(std::tuple<T...> const &value) -> size_t {
    return detail::value_hash_tuple(value, std::index_sequence_for<T...>{});
}

template <class... T> inline auto value_hash(std::variant<T...> const &value) -> size_t {
    return std::visit([](auto &&arg) { return value_hash(arg); }, value);
}

template <class T> inline auto value_hash(std::vector<T> const &value) -> size_t {
    size_t hash = 0;
    for (auto const &elem : value) {
        hash = hash_combine(hash, value_hash(elem));
    }
    return hash;
}

template <class T, class U, class... Args>
inline auto value_hash(T const &a, U const &b, Args const &...value) -> size_t {
    return hash_combine(value_hash(a), value_hash(b, value...));
}
