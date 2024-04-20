#pragma once

#include <type_traits>

//! @addtogroup util_enum
//! @{

namespace Gringo {

namespace Detail {

//! Opt-in concept to enable bit operations for enums.
//!
//! To enable of operations for an enum T, a call to is_bit_set_enum must be well-defined.
template <class T>
concept BitSetEnum = std::is_enum_v<T> && requires(T e) { is_bit_set_enum(e); };

} // namespace Detail

//! Complement of a bit set.
template <Detail::BitSetEnum T> [[nodiscard]] inline auto operator~(T a) -> T {
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    return static_cast<T>(~static_cast<std::underlying_type_t<T>>(a));
}

//! Union of two bit sets.
template <Detail::BitSetEnum T> [[nodiscard]] inline auto operator|(T a, T b) -> T {
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) | static_cast<std::underlying_type_t<T>>(b));
}

//! Intersection of two bit sets.
template <Detail::BitSetEnum T> [[nodiscard]] inline auto operator&(T a, T b) -> T {
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) & static_cast<std::underlying_type_t<T>>(b));
}

//! Difference of two bit sets.
template <Detail::BitSetEnum T> [[nodiscard]] inline auto operator-(T a, T b) -> T { return a & (~b); }

//! Symmetric difference of two bit sets.
template <Detail::BitSetEnum T> [[nodiscard]] inline auto operator^(T a, T b) -> T {
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) ^ static_cast<std::underlying_type_t<T>>(b));
}

//! Union assignment for bit sets.
template <Detail::BitSetEnum T> inline auto operator|=(T &a, T b) -> T & {
    a = a | b;
    return a;
}

//! Intersection assignment for bit sets.
template <Detail::BitSetEnum T> inline auto operator&=(T &a, T b) -> T & {
    a = a & b;
    return a;
}

//! Symmetric difference assignment for bit sets.
template <Detail::BitSetEnum T> inline auto operator^=(T &a, T b) -> T & {
    a = a ^ b;
    return a;
}

//! Difference assignment for bit sets.
template <Detail::BitSetEnum T> inline auto operator-=(T &a, T b) -> T & {
    a = a - b;
    return a;
}

//! Test if a is a superset of b.
template <Detail::BitSetEnum T> [[nodiscard]] inline auto test(T a, T b) -> bool { return (a & b) == b; }

} // namespace Gringo
