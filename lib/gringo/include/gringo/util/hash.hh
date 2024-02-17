#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <typeinfo>
#include <variant>
#include <vector>

#include <gringo/util/immutable_array.hh>
#include <gringo/util/immutable_value.hh>

namespace Gringo::Util {

//! @defgroup core_hash Hash Functions
//! Generic functions for equality comparison and hash computation.
//!
//! @ingroup core_util
//!
//! @{

//! Helper to declare the value hash struct for a type.
#define GRINGO_HASH_PROTO(T)                                                                                           \
    namespace Gringo::Util {                                                                                           \
    template <> struct value_hasher<T> {                                                                               \
        auto operator()(T const &x) const -> size_t;                                                                   \
    };                                                                                                                 \
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
template <class T> struct value_hasher : std::hash<T> {};

//! Compute a hash using std::hash.
template <class T>
    requires requires(T x) { x.hash(); }
struct value_hasher<T> {
    //! Compute the hash of the given value.
    auto operator()(T const &x) const -> size_t { return x.hash(); }
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
template <class T>
    requires std::is_arithmetic_v<T> || std::is_enum_v<T>
struct value_hasher<T> {
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
        return hash_combine(typeid(T *).hash_code(), value_hasher<T>{}(*value));
    }
};

//! Value hasher for optionals.
template <class T> struct value_hasher<std::optional<T>> {
    //! Operator to compute the hash.
    auto operator()(std::optional<T> const &value) const -> size_t {
        if (value) {
            return hash_combine(typeid(std::optional<T>).hash_code(), value_hasher<T>{}(*value));
        }
        return typeid(std::optional<T>).hash_code();
    }
};

//! See value_hasher<T const *>.
template <class T> struct value_hasher<std::unique_ptr<T>> {
    //! Operator to compute the hash.
    auto operator()(std::unique_ptr<T> const &value) const -> size_t {
        if (value) {
            return hash_combine(typeid(std::unique_ptr<T>).hash_code(), value_hasher<T>{}(*value));
        }
        return typeid(std::unique_ptr<T>).hash_code();
    }
};

//! See value_hasher<T const *>.
template <class T> struct value_hasher<immutable_value<T>> {
    //! Operator to compute the hash.
    auto operator()(immutable_value<T> const &value) const -> size_t {
        if (value) {
            return hash_combine(typeid(immutable_value<T>).hash_code(), value_hasher<T>{}(*value));
        }
        return typeid(Util::immutable_value<T>).hash_code();
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

//! Compute the hash of a vector.
template <class T> struct value_hasher<Util::immutable_array<T>> {
    //! Operator to compute the hash.
    auto operator()(Util::immutable_array<T> const &value) const -> size_t {
        size_t hash = typeid(Util::immutable_array<T>).hash_code();
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
