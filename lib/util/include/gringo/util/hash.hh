#pragma once

#include <gringo/util/immutable_array.hh>
#include <gringo/util/immutable_value.hh>

#include <memory>
#include <numeric>
#include <optional>
#include <span>
#include <string_view>
#include <typeinfo>
#include <variant>
#include <vector>

namespace Gringo::Util {

//! @addtogroup util_hash
//! @{

//! Combine the given seeds.
inline auto hash_combine(std::initializer_list<size_t> list) -> size_t {
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    return std::hash<std::string_view>{}(
        std::string_view(reinterpret_cast<char const *>(list.begin()), sizeof(size_t) * list.size()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}

//! Combine the given seeds.
template <class... Args> inline auto hash_combine(Args... args) -> size_t { return hash_combine({args...}); }

//! Perturb the given seed.
inline auto hash_mix(size_t a) -> size_t { return hash_combine({a}); }

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

//! Like value_hash but sometimes faster.
//!
//! The resulting hash is intended as an argument for hash_combine and not to
//! be directly used in a container.
template <class T> auto value_hash_fast(T const &x) -> size_t;

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
    //! Basic comparison.
    template <class T> auto operator()(T const &a, T const &b) const -> bool { return a == b; }
    //! Compare reference wrappers by value.
    template <class T>
    auto operator()(std::reference_wrapper<T> const &a, std::reference_wrapper<T> const &b) const -> bool {
        return operator()(a.get(), b.get());
    }
    //! Compare optionals by value.
    template <class T> auto operator()(std::optional<T> const &a, std::optional<T> const &b) const -> bool {
        return (!a && !b) || (a && b && operator()(*a, *b));
    }
    //! Compare pointers by value.
    template <class T> auto operator()(T *a, T *b) const -> bool {
        return a == b || (a != nullptr && b != nullptr && operator()(*a, *b));
    }
    //! Compare unique pointers by value.
    template <class T, class D>
    auto operator()(std::unique_ptr<T, D> const &a, std::unique_ptr<T, D> const &b) const -> bool {
        return operator()(a.get(), b.get());
    }
    //! Compare pairs by value.
    template <class T, class U> auto operator()(std::pair<T, U> const &a, std::pair<T, U> const &b) const -> bool {
        return operator()(a.first, b.first) && operator()(a.second, b.second);
    }
    //! Compare tuples by value.
    template <class... T> auto operator()(std::tuple<T...> const &a, std::tuple<T...> const &b) const -> bool {
        return [&, this]<size_t... I>([[maybe_unused]] std::index_sequence<I...> seq) {
            return (this->operator()(std::get<I>(a), std::get<I>(b)) && ...);
        }(std::index_sequence_for<T...>());
    }
    //! Compare variants by value.
    template <class... T> auto operator()(std::variant<T...> const &a, std::variant<T...> const &b) const -> bool {
        auto i = a.index();
        return i == b.index() && [&, this]<size_t... I>([[maybe_unused]] std::index_sequence<I...> seq) {
            return ((i == I && this->operator()(std::get<I>(a), std::get<I>(b))) || ...);
        }(std::index_sequence_for<T...>());
    }
    //! Compare spans by value.
    template <class T, size_t E> auto operator()(std::span<T, E> const &a, std::span<T, E> const &b) const -> bool {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(), *this);
    }
    //! Compare vectors by value.
    template <class T, class A> auto operator()(std::vector<T, A> const &a, std::vector<T, A> const &b) const -> bool {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(), *this);
    }
};

// Implementation of value_hash

template <class T> auto value_hash_fast(T const &x) -> size_t {
    if constexpr (std::is_same_v<T, std::type_info>) {
        return x.hash_code();
    } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
        return std::hash<T>{}(x);
    } else {
        return value_hash(x);
    }
}

template <class T> auto value_hash_range(T const &x) -> size_t {
    using std::begin;
    using std::end;
    auto ib = std::begin(x);
    auto ie = std::end(x);
    if (ib == ie) {
        return value_hash(typeid(T));
    }
    return std::accumulate(ib, ie, value_hash_fast(typeid(T)),
                           [](auto const &seed, auto const &x) { return hash_combine({seed, value_hash_fast(x)}); });
}

inline auto value_hash(std::type_info const &x) -> size_t { return hash_mix(x.hash_code()); }

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
        return hash_combine({value_hash_fast(typeid(T *)), value_hash_fast(*x)});
    }
    return value_hash(typeid(T *));
}

template <class T> auto value_hash(std::optional<T> const &x) -> size_t {
    if (x) {
        return hash_combine({value_hash_fast(typeid(std::optional<T>)), value_hash_fast(*x)});
    }
    return value_hash(typeid(std::optional<T>));
}

template <class T> auto value_hash(std::reference_wrapper<T> const &x) -> size_t {
    return hash_combine({value_hash_fast(typeid(std::reference_wrapper<T>)), value_hash_fast(x.get())});
}

template <class T, class D> auto value_hash(std::unique_ptr<T, D> const &x) -> size_t {
    if (x) {
        return hash_combine({value_hash_fast(typeid(std::unique_ptr<T, D>)), value_hash_fast(*x)});
    }
    return value_hash(typeid(std::unique_ptr<T, D>));
}

template <class T> auto value_hash(immutable_value<T> const &x) -> size_t {
    if (x) {
        return hash_combine({value_hash_fast(typeid(immutable_value<T>)), value_hash_fast(*x)});
    }
    return value_hash(typeid(immutable_value<T>));
}

template <class T, class U> auto value_hash(std::pair<T, U> const &x) -> size_t {
    return hash_combine(
        {value_hash_fast(typeid(std::pair<T, U>)), value_hash_fast(x.first), value_hash_fast(x.second)});
}

template <class... T> auto value_hash(std::tuple<T...> const &x) -> size_t {
    return [&x]<size_t... Indices>([[maybe_unused]] std::index_sequence<Indices...> indices) -> size_t {
        return hash_combine({value_hash_fast(typeid(std::tuple<T...>)), value_hash_fast(std::get<Indices>(x))...});
    }(std::index_sequence_for<T...>{});
}

template <class... T> auto value_hash(std::variant<T...> const &x) -> size_t {
    return std::visit(
        [](auto &&arg) { return hash_combine({value_hash_fast(typeid(std::variant<T...>)), value_hash_fast(arg)}); },
        x);
}

template <class T, size_t E> auto value_hash(std::span<T, E> const &x) -> size_t { return value_hash_range(x); }

template <class T, class A> auto value_hash(std::vector<T, A> const &x) -> size_t { return value_hash_range(x); }

template <class T> auto value_hash(Util::immutable_array<T> const &x) -> size_t { return value_hash_range(x); }

template <class T, class... Args> auto value_hash_record(Args const &...x) -> size_t {
    if constexpr (sizeof...(Args) == 0) {
        return value_hash(typeid(T));
    } else {
        return hash_combine({value_hash_fast(typeid(T)), value_hash_fast(x)...});
    }
}

static constexpr unsigned int default_neighborhood_size = 62;

//! @}

} // namespace Gringo::Util
