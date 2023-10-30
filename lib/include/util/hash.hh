#pragma once

#include <memory>
#include <optional>
#include <string>
#include <typeinfo>
#include <variant>
#include <vector>

#include <util/shared_ptr.hh>

namespace Gringo::Util {

//! @defgroup core_hash Hash Functions
//! @ingroup core_util
//!
//! Generic functions for equality comparison and hash computation.
//!
//! @{

//! Helper to declare the value hash struct for a type.
#define GRINGO_HASH_PROTO(T)                                                                                           \
    namespace Gringo::Util {                                                                                           \
    template <> struct value_hasher<T> {                                                                               \
        auto operator()(T const &x) const -> size_t;                                                                   \
    };                                                                                                                 \
    }

//! Recursive equality comparison with support for pointers, optionals, pairs, tuples, variants, and vectors.
template <class T = std::equal_to<>> struct value_equal_to : public T {
    using T::operator();

    //! Compare two pointers after dereferencing them.
    template <class A, class B> auto operator()(A const *a, B const *b) const { return operator()(*a, *b); }

    //! Compare the values of two optionals.
    template <class A, class B> auto operator()(std::optional<A> const &a, std::optional<B> const &b) const {
        if (a.has_value() && b.has_value()) {
            return operator()(a.value(), b.value());
        }
        return !a.has_value() && !b.has_value();
    }

    //! Compare two unique pointers after dereferencing them.
    template <class A, class B> auto operator()(std::unique_ptr<A> const &a, std::unique_ptr<B> const &b) const {
        return a == b || operator()(*a, *b);
    }

    //! Compare two shared pointers after dereferencing them.
    template <class A, class B> auto operator()(shared_ptr<A> const &a, shared_ptr<B> const &b) const {
        return a == b || operator()(*a, *b);
    }

    //! Compare two pairs.
    template <class A, class B, class C, class D>
    auto operator()(std::pair<A, B> const &a, std::pair<C, D> const &b) const {
        return &a == &b || (operator()(a.first, b.first) && operator()(a.second, b.second));
    }

    //! Compare two tuples.
    template <class... A> auto operator()(std::tuple<A...> const &a, std::tuple<A...> const &b) const {
        return &a == &b || value_equal_to_tuple(a, b, std::index_sequence_for<A...>{});
    }

    //! Compare two variants.
    template <class... A> auto operator()(std::variant<A...> const &a, std::variant<A...> const &b) const {
        return &a == &b || value_equal_to_variant(a, b, std::index_sequence_for<A...>{});
    }

    //! Compare two vectors.
    template <class A, class B> auto operator()(std::vector<A> const &a, std::vector<B> const &b) const {
        return &a == &b || std::equal(a.begin(), a.end(), b.begin(), b.end(),
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

//! Base case for Gringo::Util::value_equal<class A, class B, class... Args>(A const &a, B const &b, Args const
//! &...args).
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

//! Function object producing perturbed hash values.
template <class T, class H = std::hash<T>> struct mix_hasher : private H {
    //! Compute the hash of the given vale and perturb it.
    auto operator()(T const &x) const -> size_t { return hash_mix(H::operator()(x)); }
};

//! Compute a hash using std::hash.
template <class T> struct value_hasher : std::hash<T> {
    //! Compute the hash of the given value.
    auto operator()(T const &x) const -> size_t { return std::hash<T>::operator()(x); }
};

//! Recursively compute a hash for pointers, optionals, pairs, tuples, variants, and vectors.
//!
//! More specialization can be added.
template <> struct value_hasher<std::type_info> {
    //! Operator to compute the hash.
    auto operator()(std::type_info const &value) const -> size_t { return hash_mix(value.hash_code()); }
};

//! Hasher for arithmetic and enumeration types.
//!
//! This hasher will additionally perturb values.
//! The idea is to improve hash quality in case the STL just static casts the value.
template <class T> struct value_hasher<std::enable_if<std::is_arithmetic_v<T> || std::is_enum_v<T>, T>> {
    //! Operator to compute the hash.
    auto operator()(T value) const -> size_t { return hash_mix(std::hash<T>{}(value)); }
};

//! Hasher for pointers.
//!
//! The hash of the defenced pointer is used.
//! This function assumes that the pointer is not null.
template <class T> struct value_hasher<T const *> {
    //! Operator to compute the hash.
    auto operator()(T const *value) const -> size_t {
        return hash_combine(typeid(T *).hash_code(), value_hasher<T>{}(value));
    }
};

//! See value_hasher<T const *>.
template <class T> struct value_hasher<std::unique_ptr<T>> {
    //! Operator to compute the hash.
    auto operator()(std::unique_ptr<T> const &value) const -> size_t {
        return hash_combine(typeid(std::unique_ptr<T>).hash_code(), value_hasher<T>{}(*value));
    }
};

//! See value_hasher<T const *>.
template <class T> struct value_hasher<shared_ptr<T>> {
    //! Operator to compute the hash.
    auto operator()(shared_ptr<T> const &value) const -> size_t {
        return hash_combine(typeid(shared_ptr<T>).hash_code(), value_hasher<T>{}(*value));
    }
};

//! Compute the hash of a pair.
template <class T, class U> struct value_hasher<std::pair<T, U>> {
    //! Operator to compute the hash.
    auto operator()(std::pair<T, U> const &value) const -> size_t {
        return hash_combine(
            {typeid(std::pair<T, U>).hash_code(), value_hasher<T>{}(value.first), value_hasher<U>{}(value.second)});
    }
};

//! Compute the hash of a tuple.
template <class... T> struct value_hasher<std::tuple<T...>> {
    //! Operator to compute the hash.
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

//! Compute the hash of a variant.
template <class... T> struct value_hasher<std::variant<T...>> {
    //! Operator to compute the hash.
    auto operator()(std::variant<T...> const &value) const -> size_t {
        return std::visit(
            [](auto &&arg) {
                using U = std::decay_t<decltype(arg)>;
                return hash_combine({typeid(std::variant<T...>).hash_code(), value_hasher<U>{}(arg)});
            },
            value);
    }
};

//! Compute the hash of a vector.
template <class T> struct value_hasher<std::vector<T>> {
    //! Operator to compute the hash.
    auto operator()(std::vector<T> const &value) const -> size_t {
        size_t hash = typeid(std::vector<T>).hash_code();
        for (auto const &elem : value) {
            hash = hash_combine({hash, value_hasher<T>{}(elem)});
        }
        return hash;
    }
};

//! Compute and combine the hashes for the given values.
//!
//! The value_hasher is used to compute the hashes,
//! which are combined using hash_combine().
template <class... Args> inline auto value_hash(Args const &...value) -> size_t {
    return hash_combine(value_hasher<Args>{}(value)...);
}

//! @}

} // namespace Gringo::Util
