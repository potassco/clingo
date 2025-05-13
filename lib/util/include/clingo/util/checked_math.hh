#pragma once

// The code below took inspiration from <https://github.com/dcleblanc/SafeInt>
// and the paper "Division and Modulus for Computer Scientists". In principle,
// one could also just use the SafeInt class as a third party library.
// Currently, it is not included because it is a bit heavy (7k loc) and we need
// only a few functions here. I still have to make up my mind here.

#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

namespace CppClingo::Util {

//! @addtogroup util_math
//! @{

// Casting

//! Check if s of type S can be casted to T without loss.
template <std::signed_integral T, std::signed_integral S> auto check_cast(S in) -> bool {
    return std::in_range<T>(std::move(in));
}

//! Cast S to T if possible.
template <std::integral T, std::integral S> auto safe_cast(S in) -> T {
    if (std::in_range<T, S>(in)) {
        return static_cast<T>(std::move(in));
    }
    throw std::range_error("invalid cast");
}

// Addition

//! Add two integers checking overflows.
//!
//! Fallback for the general case.
template <std::signed_integral S> auto check_add(S a, S b) -> std::optional<S> {
#ifdef __GNUC__
    if (S c; !__builtin_add_overflow(a, b, &c)) {
        return c;
    }
    return std::nullopt;
#else
    if constexpr (std::is_same_v<S, int32_t>) {
        int64_t tmp = static_cast<int64_t>(a) + b;
        if (check_cast<int32_t>(tmp)) {
            return tmp;
        }
        return std::nullopt;
    } else {
        using U = std::make_unsigned_t<S>;
        S tmp = static_cast<S>(static_cast<U>(a) + static_cast<U>(b));
        if ((a >= 0 && b >= 0 && tmp < a) || (a < 0 && b < 0 && tmp > a)) {
            return std::nullopt;
        }
        return tmp;
    }
#endif
}

// Subtraction

//! Subtract two integers checking overflows.
//!
//! Fallback for the general case.
template <std::signed_integral S> auto check_sub(S a, S b) -> std::optional<S> {
#ifdef __GNUC__
    if (S c; !__builtin_sub_overflow(a, b, &c)) {
        return c;
    }
    return std::nullopt;
#else
    if constexpr (std::is_same_v<S, int32_t>) {
        int64_t tmp = static_cast<int64_t>(a) - b;
        if (check_cast<int32_t>(tmp)) {
            return tmp;
        }
        return std::nullopt;
    } else {
        using U = std::make_unsigned_t<S>;
        S tmp = static_cast<S>(static_cast<U>(a) - static_cast<U>(b));
        if ((a >= 0 && b < 0 && tmp < a) || (b >= 0 && tmp > a)) {
            return std::nullopt;
        }
        return tmp;
    }
#endif
}

// Unary Minus

//! Negate an integer checking overflows.
template <std::signed_integral S> auto check_neg(S a) -> std::optional<S> {
    if (a == std::numeric_limits<S>::min()) {
        return std::nullopt;
    }
    return -a;
}

// Absolute

//! The absolute of an integer checking overflows.
template <std::signed_integral S> auto check_abs(S a) -> std::optional<S> {
    if (a == std::numeric_limits<S>::min()) {
        return std::nullopt;
    }
    return a < 0 ? -a : a;
}

// Multiplication

//! Multiply two integers checking overflows.
//!
//! Fallback for the general case.
template <std::signed_integral S> auto check_mul(S a, S b) -> std::optional<S> {
#ifdef __GNUC__
    if (S c; !__builtin_mul_overflow(a, b, &c)) {
        return c;
    }
    return std::nullopt;
#else
    if constexpr (std::is_same_v<S, int32_t>) {
        int64_t tmp = static_cast<int64_t>(a) * b;
        if (check_cast<int32_t>(tmp)) {
            return tmp;
        }
        return std::nullopt;
    } else {
        if (a > 0 && b > 0 && a > std::numeric_limits<S>::max() / b) {
            return std::nullopt;
        }
        if (a > 0 && b < 0 && b < std::numeric_limits<S>::min() / a) {
            return std::nullopt;
        }
        if (a < 0 && b > 0 && a < std::numeric_limits<S>::min() / b) {
            return std::nullopt;
        }
        if (a < 0 && b < 0 && b < std::numeric_limits<S>::max() / a) {
            return std::nullopt;
        }
        return a * b;
    }
#endif
}

// Division

//! Divide two integers checking overflows (truncating toward negative infinity).
template <std::signed_integral S> auto check_div(S a, S b) -> std::optional<S> {
    if (b == 0 || (b == -1 && a == std::numeric_limits<S>::min())) {
        return std::nullopt;
    }
    int d = a / b;
    int r = a % b;
    // The remainder is zero if |b| is 1, so we can always subtract 1.
    if ((r > 0 && b < 0) || (r < 0 && b > 0)) {
        --d;
    }
    return d;
}

// Modulo

//! Modulo of two integers checking overflows (truncating toward negative infinity).
template <std::signed_integral S> auto check_mod(S a, S b) -> std::optional<S> {
    if (b == 0) {
        return std::nullopt;
    }
    if (b == -1) {
        return 0;
    }
    int r = a % b;
    // This never overflows.
    if ((r > 0 && b < 0) || (r < 0 && b > 0)) {
        r += b;
    }
    return r;
}

// Power

//! Power of the given integers checking overflows.
//!
//! Note that <tt>a^0 = 1</tt> for all values of <tt>a</tt> and <tt>a^b=0</tt> whenever <tt>b</tt> is less than zero.
template <std::signed_integral S> auto check_pow(S a, S b) -> std::optional<S> {
    if (b < 0) {
        return std::nullopt;
    }
    int r = 1;
    while (b > 0) {
        if ((b & 1) != 0) {
            auto tmp = check_mul(r, a);
            if (!tmp.has_value()) {
                return std::nullopt;
            }
            r = tmp.value();
        }
        b >>= 1;
        if (b > 0) {
            auto tmp = check_mul(a, a);
            if (!tmp.has_value()) {
                return std::nullopt;
            }
            a = tmp.value();
        }
    }
    return r;
}

//! @}

} // namespace CppClingo::Util
