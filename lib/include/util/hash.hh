#pragma once

//! @file
//! This file contains utilities for equality comparision and hashes.

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <typeinfo>
#include <variant>
#include <vector>

#include <util/shared_ptr.hh>

namespace Gringo::Util {

#define HASH_PROTO(T)                                                                                                  \
    namespace Gringo::Util {                                                                                           \
                                                                                                                       \
    template <> struct value_hasher<T> {                                                                               \
        auto operator()(T const &x) const -> size_t;                                                                   \
    };                                                                                                                 \
    }

//! Compare by value with support for pointers, optionals, pairs, tuples, variants, and vectors.
template <class T = std::equal_to<>> struct value_equal_to : public T {
    using T::operator();

    template <class A, class B> auto operator()(A const *a, B const *b) const { return operator()(*a, *b); }

    template <class A, class B> auto operator()(std::optional<A> const &a, std::optional<B> const &b) const {
        if (a.has_value() && b.has_value()) {
            return operator()(a.value(), b.value());
        }
        return !a.has_value() && !b.has_value();
    }

    template <class A, class B> auto operator()(std::unique_ptr<A> const &a, std::unique_ptr<B> const &b) const {
        return operator()(*a, *b);
    }

    template <class A, class B> auto operator()(shared_ptr<A> const &a, shared_ptr<B> const &b) const {
        return operator()(*a, *b);
    }

    template <class A, class B, class C, class D>
    auto operator()(std::pair<A, B> const &a, std::pair<C, D> const &b) const {
        return operator()(a.first, b.first) && operator()(a.second, b.second);
    }

    template <class... A> auto operator()(std::tuple<A...> const &a, std::tuple<A...> const &b) const {
        return value_equal_to_tuple(a, b, std::index_sequence_for<A...>{});
    }

    template <class... A> auto operator()(std::variant<A...> const &a, std::variant<A...> const &b) const {
        return value_equal_to_variant(a, b, std::index_sequence_for<A...>{});
    }

    template <class A, class B> auto operator()(std::vector<A> const &a, std::vector<B> const &b) const {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                          [this](auto const &a, auto const &b) { return this->operator()(a, b); });
    }

  private:
    template <class... A, size_t... Indices>
    inline auto value_equal_to_tuple(std::tuple<A...> const &a, std::tuple<A...> const &b,
                                     std::index_sequence<Indices...> indices) const -> bool {
        static_cast<void>(indices);
        return (true && ... && operator()(std::get<Indices>(a), std::get<Indices>(b)));
    }

    template <class... A, size_t... Indices>
    inline auto value_equal_to_variant(std::variant<A...> const &a, std::variant<A...> const &b,
                                       std::index_sequence<Indices...> indices) const -> bool {
        static_cast<void>(indices);
        return (
            false || ... ||
            (a.index() == Indices && b.index() == Indices && operator()(std::get<Indices>(a), std::get<Indices>(b))));
    }
};

inline auto value_equal() { return true; }

//! Check whether all argument pairs are value equal.
template <class A, class B, class... Args> auto value_equal(A const &a, B const &b, Args const &...args) {
    return value_equal_to{}(a, b) && value_equal(args...);
}

//! Combine the given seeds.
inline auto hash_combine(std::initializer_list<size_t> list) -> size_t {
    return std::hash<std::string_view>{}(
        std::string_view(reinterpret_cast<char const *>(list.begin()), sizeof(size_t) * list.size()));
}

//! Combine the given seeds.
template <class... Args> inline auto hash_combine(Args... args) -> size_t { return hash_combine({args...}); }

//! Perturb the given seed.
inline auto hash_mix(size_t a) -> size_t { return hash_combine({a}); }

//! Perturb the given seed.
template <class T> struct value_hasher : std::hash<T> {
    using std::hash<T>::operator();
};

//! Compute a hash for pointers, optionals, pairs, tuples, variants, and vectors.
template <> struct value_hasher<std::type_info> {
    auto operator()(std::type_info const &value) const -> size_t { return hash_mix(value.hash_code()); }
};

template <class T> struct value_hasher<std::enable_if<std::is_arithmetic_v<T> || std::is_enum_v<T>, T>> {
    auto operator()(T value) const -> size_t { return hash_mix(std::hash<T>{}(value)); }
};

template <class T> struct value_hasher<T const *> {
    auto operator()(T const *value) const -> size_t {
        return hash_combine(typeid(T *).hash_code(), value_hasher<T>{}(value));
    }
};

template <class T> struct value_hasher<std::unique_ptr<T>> {
    auto operator()(std::unique_ptr<T> const &value) const -> size_t {
        return hash_combine(typeid(std::unique_ptr<T>).hash_code(), value_hasher<T>{}(*value));
    }
};

template <class T> struct value_hasher<shared_ptr<T>> {
    auto operator()(shared_ptr<T> const &value) const -> size_t {
        return hash_combine(typeid(shared_ptr<T>).hash_code(), value_hasher<T>{}(*value));
    }
};

template <class T, class U> struct value_hasher<std::pair<T, U>> {
    auto operator()(std::pair<T, U> const &value) const -> size_t {
        return hash_combine(
            {typeid(std::pair<T, U>).hash_code(), value_hasher<T>{}(value.first), value_hasher<U>{}(value.second)});
    }
};

template <class... T> struct value_hasher<std::tuple<T...>> {
    auto operator()(std::tuple<T...> const &value) const -> size_t {
        return value_hash_tuple(value, std::index_sequence_for<T...>{});
    }

  private:
    template <size_t... Indices>
    inline auto value_hash_tuple(std::tuple<T...> const &value, std::index_sequence<Indices...> indices) const
        -> size_t {
        static_cast<void>(indices);
        return hash_combine({typeid(std::tuple<T...>).hash_code(), value_hasher<T>{}(std::get<Indices>(value))...});
    }
};

template <class... T> struct value_hasher<std::variant<T...>> {
    auto operator()(std::variant<T...> const &value) const -> size_t {
        return std::visit(
            [](auto &&arg) {
                using U = std::decay_t<decltype(arg)>;
                return hash_combine({typeid(std::variant<T...>).hash_code(), value_hasher<U>{}(arg)});
            },
            value);
    }
};

template <class T> struct value_hasher<std::vector<T>> {
    auto operator()(std::vector<T> const &value) const -> size_t {
        size_t hash = typeid(std::vector<T>).hash_code();
        for (auto const &elem : value) {
            hash = hash_combine({hash, value_hasher<T>{}(elem)});
        }
        return hash;
    }
};

//! Compute and combine the hashes for the given values.
template <class... Args> inline auto value_hash(Args const &...value) -> size_t {
    return hash_combine(value_hasher<Args>{}(value)...);
}

} // namespace Gringo::Util
