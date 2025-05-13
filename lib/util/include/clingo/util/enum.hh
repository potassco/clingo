#pragma once

#include <type_traits> // IWYU pragma: keep

namespace CppClingo {

//! @addtogroup util_enum
//! @{

// NOLINTBEGIN

//! Opt-in macro for enabling bit operations for a given enum type.
//!
//! Use CLINGO_ENABLE_BITSET_ENUM(E) to enable bitwise operators on the underlying type of enum E.
//!
//! @note If E is a class-local enum, CLINGO_ENABLE_BITSET_ENUM(E, friend) can be
//! used to enable bitwise operators from within the class definition.
#define CLINGO_ENABLE_BITSET_ENUM(E, ...)                                                                              \
    [[nodiscard]] CLINGO_ENUM_OP(~, (E a), __VA_ARGS__)->E {                                                           \
        return static_cast<E>(~static_cast<std::underlying_type_t<E>>(a));                                             \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(|, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) | static_cast<std::underlying_type_t<E>>(b));  \
    }                                                                                                                  \
    CLINGO_ENUM_OP(|=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a | b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(&, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) & static_cast<std::underlying_type_t<E>>(b));  \
    }                                                                                                                  \
    CLINGO_ENUM_OP(&=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a & b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(-, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) & static_cast<std::underlying_type_t<E>>(~b)); \
    }                                                                                                                  \
    CLINGO_ENUM_OP(-=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a - b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(^, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) ^ static_cast<std::underlying_type_t<E>>(b));  \
    }                                                                                                                  \
    CLINGO_ENUM_OP(^=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a ^ b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] [[maybe_unused]] inline __VA_ARGS__ constexpr auto intersects(E a, E b) -> bool {                    \
        return static_cast<std::underlying_type_t<E>>(a & b) != 0;                                                     \
    }                                                                                                                  \
    static_assert(std::is_enum_v<E>)

//! Internal helper.
#define CLINGO_ENUM_OP(op, arg, ...) [[maybe_unused]] inline __VA_ARGS__ constexpr auto operator op arg noexcept

// NOLINTEND

//! @}

} // namespace CppClingo
