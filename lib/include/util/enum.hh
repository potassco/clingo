#pragma once

// NOLINTNEXTLINE(unused-includes)
#include <type_traits>

//! @defgroup core_enum Helpers for enumurations
//! @ingroup core_util
//!
//! Currently, just provides a macro to create bitsets.
//!
//! @{

//! Helper to create a bitset from an enum class.
#define GRINGO_ENUM_FLAGS(TYPE)                                                                                        \
                                                                                                                       \
    [[nodiscard]] inline auto operator|(TYPE a, TYPE b) -> TYPE {                                                      \
        return static_cast<TYPE>(static_cast<std::underlying_type_t<TYPE>>(a) |                                        \
                                 static_cast<std::underlying_type_t<TYPE>>(b));                                        \
    }                                                                                                                  \
                                                                                                                       \
    [[nodiscard]] inline auto operator&(TYPE a, TYPE b) -> TYPE {                                                      \
        return static_cast<TYPE>(static_cast<std::underlying_type_t<TYPE>>(a) &                                        \
                                 static_cast<std::underlying_type_t<TYPE>>(b));                                        \
    }                                                                                                                  \
                                                                                                                       \
    [[nodiscard]] inline auto operator^(TYPE a, TYPE b) -> TYPE {                                                      \
        return static_cast<TYPE>(static_cast<std::underlying_type_t<TYPE>>(a) ^                                        \
                                 static_cast<std::underlying_type_t<TYPE>>(b));                                        \
    }                                                                                                                  \
                                                                                                                       \
    [[nodiscard]] inline auto operator~(TYPE a) -> TYPE {                                                              \
        return static_cast<TYPE>(~static_cast<std::underlying_type_t<TYPE>>(a));                                       \
    }                                                                                                                  \
                                                                                                                       \
    inline auto operator|=(TYPE &a, TYPE b) -> TYPE & {                                                                \
        a = a | b;                                                                                                     \
        return a;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    inline auto operator&=(TYPE &a, TYPE b) -> TYPE & {                                                                \
        a = a & b;                                                                                                     \
        return a;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    inline auto operator^=(TYPE &a, TYPE b) -> TYPE & {                                                                \
        a = a ^ b;                                                                                                     \
        return a;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    [[nodiscard]] inline auto test(TYPE a, TYPE b) -> bool { return (a & b) == b; }

//! @}
