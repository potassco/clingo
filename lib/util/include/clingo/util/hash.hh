#pragma once

#include <clingo/util/immutable_array.hh>
#include <clingo/util/immutable_value.hh>
#include <clingo/util/small_vector.hh>

#include <cstring>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <typeinfo>
#include <variant>
#include <vector>

namespace CppClingo::Util {

//! @addtogroup util_hash
//! @{

//! Perturb the given seed.
inline auto hash_mix(size_t h) -> size_t;

//! Combine the given hashes.
template <class... T> inline auto hash_combine(T... a) -> size_t;

//! Compute a hash for type_info.
auto value_hash(std::type_info const &x) -> size_t;

//! Compute a hash using std::hash.
template <class T> auto value_hash(T const &x) -> size_t;

//! Compute hash for pointers.
template <class T> auto value_hash(T *x) -> size_t;

//! Compute hash for optionals.
template <class T> auto value_hash(std::optional<T> const &x) -> size_t;

//! Compute hash for reference_wrapper.
template <class T> auto value_hash(std::reference_wrapper<T> const &x) -> size_t;

//! Compute hash for unique_ptr.
template <class T, class D> auto value_hash(std::unique_ptr<T, D> const &x) -> size_t;

//! Compute hash for immutable_value.
template <class T> auto value_hash(immutable_value<T> const &x) -> size_t;

//! Compute the hash of a pair.
template <class T, class U> auto value_hash(std::pair<T, U> const &x) -> size_t;

//! Compute the hash of a tuple.
template <class... T> auto value_hash(std::tuple<T...> const &x) -> size_t;

//! Compute the hash of a variant.
template <class... T> auto value_hash(std::variant<T...> const &x) -> size_t;

//! Compute the hash of a span.
template <class T, size_t E> auto value_hash(std::span<T, E> const &x) -> size_t;

//! Compute the hash of a vector.
template <class T, class A> auto value_hash(std::vector<T, A> const &x) -> size_t;

//! Compute the hash of an immutable array.
template <class T> auto value_hash(Util::immutable_array<T> const &x) -> size_t;

//! Compute the hash of an immutable array.
template <class T, size_t N> auto value_hash(Util::small_vector<T, N> const &x) -> size_t;

//! Compute the hash of a string.
auto value_hash(char const *x) -> size_t;

//! Compute the hash of a string.
auto value_hash(std::string_view const &x) -> size_t;

//! Compute the hash of a string.
auto value_hash(std::string const &x) -> size_t;

//! Compute the hash for a given range of elements.
template <class T> auto value_hash_range(T const &x) -> size_t;

//! Compute and combine the hashes of the given arguments.
template <class T, class... Args> auto value_hash_record(Args const &...x) -> size_t;

//! Compute a hash using std::hash.
struct value_hasher {
    //! Compute the hash of the given value.
    template <class T> auto operator()(T const &x) const -> size_t { return value_hash(x); }
};

//! Helper class to compare pointers and some STL containers holding pointers by value.
struct value_equal_to {
    //! Mark the comparison operator as transparent.
    using is_transparent = void;

    //! Basic comparison.
    template <class T, class U> auto operator()(T const &a, U const &b) const -> bool { return a == b; }
    //! Compare c-strings by value.
    auto operator()(char const *a, char const *b) const -> bool { return std::strcmp(a, b) == 0; }
    //! Compare reference wrappers by value.
    template <class T, class U>
    auto operator()(std::reference_wrapper<T> const &a, std::reference_wrapper<U> const &b) const -> bool {
        return operator()(a.get(), b.get());
    }
    //! Compare optionals by value.
    template <class T, class U> auto operator()(std::optional<T> const &a, std::optional<U> const &b) const -> bool {
        return (!a && !b) || (a && b && operator()(*a, *b));
    }
    //! Compare pointers by value.
    template <class T, class U> auto operator()(T *a, U *b) const -> bool {
        return a == b || (a != nullptr && b != nullptr && operator()(*a, *b));
    }
    //! Compare unique pointers by value.
    template <class T, class D, class U, class E>
    auto operator()(std::unique_ptr<T, D> const &a, std::unique_ptr<U, E> const &b) const -> bool {
        return operator()(a.get(), b.get());
    }
    //! Compare pairs by value.
    template <class T, class U, class V, class W>
    auto operator()(std::pair<T, U> const &a, std::pair<V, W> const &b) const -> bool {
        return operator()(a.first, b.first) && operator()(a.second, b.second);
    }
    //! Compare tuples by value.
    template <class... T, class... U>
    auto operator()(std::tuple<T...> const &a, std::tuple<U...> const &b) const -> bool {
        static_assert(sizeof...(T) == sizeof...(U));
        return [&, this]<size_t... I>([[maybe_unused]] std::index_sequence<I...> seq) {
            return (this->operator()(std::get<I>(a), std::get<I>(b)) && ...);
        }(std::index_sequence_for<T...>());
    }
    //! Compare variants by value.
    template <class... T, class... U>
    auto operator()(std::variant<T...> const &a, std::variant<U...> const &b) const -> bool {
        static_assert(sizeof...(T) == sizeof...(U));
        auto i = a.index();
        return i == b.index() && [&, this]<size_t... I>([[maybe_unused]] std::index_sequence<I...> seq) {
            return ((i == I && this->operator()(std::get<I>(a), std::get<I>(b))) || ...);
        }(std::index_sequence_for<T...>());
    }
    //! Compare spans by value.
    template <class T, size_t E, class U, size_t F>
    auto operator()(std::span<T, E> const &a, std::span<U, F> const &b) const -> bool {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(), *this);
    }
    //! Compare vectors by value.
    template <class T, class A, class U, class B>
    auto operator()(std::vector<T, A> const &a, std::vector<U, B> const &b) const -> bool {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(), *this);
    }
};

//! Hasher for arrays of dynamic but fixed size.
//!
//! The size must be given upon construction.
class array_hash {
  public:
    //! Initialize with the given size.
    array_hash(size_t size) : size_{size} {}
    //! Get the hash of the symbol array.
    template <class T> auto operator()(T const *sym) const -> size_t { return value_hash(std::span(sym, size_)); }

  private:
    size_t size_;
};

//! Comparison operator for arrays of dynamic but fixed size.
//!
//! The size must be given upon construction.
struct array_equal_to {
  public:
    //! Initialize with the given size.
    array_equal_to(size_t size) : size_{size} {}
    //! Compare two symbols arrays.
    template <class T> auto operator()(T const *a, T const *b) const -> bool {
        return value_equal_to{}(std::span(a, size_), std::span(b, size_));
    }

  private:
    size_t size_;
};

// Implementation of value_hash

namespace Detail {

inline auto hash_combine() -> size_t {
    return 0;
}

inline auto hash_combine(size_t a) -> size_t {
    return a;
}

inline auto hash_combine(size_t seed, size_t h) -> size_t {
    // NOLINTBEGIN(readability-magic-numbers)
    if constexpr (sizeof(size_t) == sizeof(uint64_t)) {
        seed *= 0x87c37b91114253d5;
        seed = (seed >> 31) | (seed << 33);
        seed *= 0x4cf5ad432745937f;
        seed ^= h;
        seed = (seed >> 27) | (seed << 37);
        seed = (seed * 5) + 0x52dce729;
    } else {
        seed *= 0xcc9e2d51;
        seed = (seed >> 17) | (seed << 15);
        seed *= 0x1b873593;
        seed ^= h;
        seed = (seed >> 19) | (seed << 13);
        seed = (seed * 5) + 0xe6546b64;
    }
    return seed;
    // NOLINTEND(readability-magic-numbers)
}

template <class... T> inline auto hash_combine(size_t a, size_t b, T... c) -> size_t {
    return hash_combine(hash_combine(a, b), c...);
}

} // namespace Detail

inline auto hash_mix(size_t h) -> size_t {
    // NOLINTBEGIN(readability-magic-numbers)
    if constexpr (sizeof(size_t) == sizeof(uint64_t)) {
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccd;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53;
        h ^= h >> 33;
    } else {
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;
    }
    return h;
    // NOLINTEND(readability-magic-numbers)
}

template <class... T> inline auto hash_combine(T... a) -> size_t {
    return Detail::hash_combine(a...);
}

template <class T> auto value_hash_range(T const &x) -> size_t {
    // NOLINTBEGIN(readability-magic-numbers)
    // note: we unroll the loop for small sizes, this compiles very well and leads to noticable speedups
    // length 8 should cover typical sizes for argument tuples etc. in clingo
    auto small = []<std::size_t... n>(std::index_sequence<n...>, auto const &x) {
        return Util::hash_combine(Util::value_hash(x[n])...);
    };
    switch (x.size()) {
        case 0:
            return small(std::make_index_sequence<0>(), x);
        case 1:
            return small(std::make_index_sequence<1>(), x);
        case 2:
            return small(std::make_index_sequence<2>(), x);
        case 3:
            return small(std::make_index_sequence<3>(), x);
        case 4:
            return small(std::make_index_sequence<4>(), x);
        case 5:
            return small(std::make_index_sequence<5>(), x);
        case 6:
            return small(std::make_index_sequence<6>(), x);
        case 7:
            return small(std::make_index_sequence<7>(), x);
        case 8:
            return small(std::make_index_sequence<8>(), x);
        default:
            return std::accumulate(
                x.begin() + 1, x.end(), Util::value_hash(*x.begin()),
                [](auto const &seed, auto const &x) { return Util::hash_combine(seed, Util::value_hash(x)); });
    }
    // NOLINTEND(readability-magic-numbers)
}

inline auto value_hash(std::type_info const &x) -> size_t {
    return hash_mix(x.hash_code());
}

template <class T> auto value_hash(T const &x) -> size_t {
    if constexpr (requires { x.hash(); }) {
        return x.hash();
    } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
        return hash_mix(std::hash<T>{}(x));
    } else {
        return std::hash<T>{}(x);
    }
}

template <class T> auto value_hash(T *x) -> size_t {
    if (x != nullptr) {
        return value_hash(*x);
    }
    return 0;
}

template <class T> auto value_hash(std::optional<T> const &x) -> size_t {
    if (x) {
        return value_hash(*x);
    }
    return 0;
}

template <class T> auto value_hash(std::reference_wrapper<T> const &x) -> size_t {
    return value_hash(x.get());
}

template <class T, class D> auto value_hash(std::unique_ptr<T, D> const &x) -> size_t {
    if (x) {
        return value_hash(*x);
    }
    return 0;
}

template <class T> auto value_hash(immutable_value<T> const &x) -> size_t {
    if (x) {
        return value_hash(*x);
    }
    return 0;
}

template <class T, class U> auto value_hash(std::pair<T, U> const &x) -> size_t {
    return hash_combine(value_hash(x.first), value_hash(x.second));
}

template <class... T> auto value_hash(std::tuple<T...> const &x) -> size_t {
    return [&x]<size_t... Indices>([[maybe_unused]] std::index_sequence<Indices...> indices) -> size_t {
        return hash_combine(value_hash(std::get<Indices>(x))...);
    }(std::index_sequence_for<T...>{});
}

template <class... T> auto value_hash(std::variant<T...> const &x) -> size_t {
    return std::visit([](auto &&arg) { return hash_combine(value_hash(typeid(arg)), value_hash(arg)); }, x);
}

template <class T, size_t E> auto value_hash(std::span<T, E> const &x) -> size_t {
    return value_hash_range(x);
}

template <class T, class A> auto value_hash(std::vector<T, A> const &x) -> size_t {
    return value_hash_range(x);
}

template <class T> auto value_hash(Util::immutable_array<T> const &x) -> size_t {
    return value_hash_range(x);
}

template <class T, size_t N> auto value_hash(Util::small_vector<T, N> const &x) -> size_t {
    return value_hash_range(x);
}

inline auto value_hash(char const *x) -> size_t {
    return std::hash<std::string_view>{}(x);
}

inline auto value_hash(std::string_view const &x) -> size_t {
    return std::hash<std::string_view>{}(x);
}

inline auto value_hash(std::string const &x) -> size_t {
    return std::hash<std::string_view>{}(x);
}

template <class T, class... Args> auto value_hash_record(Args const &...x) -> size_t {
    return hash_combine(value_hash(x)...);
}

static constexpr unsigned int default_neighborhood_size = 62;

//! @}

} // namespace CppClingo::Util
