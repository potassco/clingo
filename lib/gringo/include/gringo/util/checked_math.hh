#pragma once

// The code below took inspiration from <https://github.com/dcleblanc/SafeInt>
// and the paper "Division and Modulus for Computer Scientists". In principle,
// one could also just use the SafeInt class as a third party library.
// Currently, it is not included because it is a bit heavy (7k loc) and we need
// only a few functions here. I still have to make up my mind here.

#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>

namespace Gringo {

// Casting

//! Check if s of type S can be casted to T without loss.
template <class T, class S>
    requires std::is_signed_v<T> && std::is_signed_v<S>
auto check_cast(S in) -> bool {
    return sizeof(T) >= sizeof(S) || (in >= std::numeric_limits<T>::min() && in <= std::numeric_limits<T>::max());
}

// Addition

//! Add two integers checking overflows.
inline auto check_add(int32_t a, int32_t b) -> std::optional<int32_t> {
    int64_t tmp = static_cast<int64_t>(a) + b;
    if (check_cast<int32_t>(tmp)) {
        return tmp;
    }
    return std::nullopt;
}

//! Add two integers checking overflows.
//!
//! Fallback for the general case.
template <class S>
    requires std::is_signed_v<S>
auto check_add(S a, S b) -> std::optional<S> {
    using U = std::make_unsigned_t<S>;
    S tmp = static_cast<S>(static_cast<U>(a) + static_cast<U>(b));
    if ((a >= 0 && b >= 0 && tmp < a) || (a < 0 && b < 0 && tmp > a)) {
        return std::nullopt;
    }
    return tmp;
}

//! Prevent adding arithmetic types not explicitly handled.
template <class S, class T>
    requires std::is_arithmetic_v<S> && std::is_arithmetic_v<T>
auto check_add(S a, T b) = delete;

// Subtraction

//! Subtract two integers checking overflows.
inline auto check_sub(int32_t a, int32_t b) -> std::optional<int32_t> {
    int64_t tmp = static_cast<int64_t>(a) - b;
    if (check_cast<int32_t>(tmp)) {
        return tmp;
    }
    return std::nullopt;
}

//! Subtract two integers checking overflows.
//!
//! Fallback for the general case.
template <class S>
    requires std::is_signed_v<S>
auto check_sub(S a, S b) -> std::optional<S> {
    using U = std::make_unsigned_t<S>;
    S tmp = static_cast<S>(static_cast<U>(a) - static_cast<U>(b));
    if ((a >= 0 && b < 0 && tmp < a) || (b >= 0 && tmp > a)) {
        return std::nullopt;
    }
    return tmp;
}

//! Prevent subtracting arithmetic types not explicitly handled.
template <class S, class T>
    requires std::is_arithmetic_v<S> && std::is_arithmetic_v<T>
auto check_sub(S a, T b) = delete;

// Unary Minus

//! Negate an integer checking overflows.
template <class S>
    requires std::is_signed_v<S>
auto check_neg(S a) -> std::optional<S> {
    if (a == std::numeric_limits<S>::min()) {
        return std::nullopt;
    }
    return -a;
}

// Absolute

//! The absolute of an integer checking overflows.
template <class S>
    requires std::is_signed_v<S>
auto check_abs(S a) -> std::optional<S> {
    if (a == std::numeric_limits<S>::min()) {
        return std::nullopt;
    }
    return a < 0 ? -a : a;
}

// Multiplication

//! Multiply two integers checking overflows.
inline auto check_mul(int32_t a, int32_t b) -> std::optional<int32_t> {
    int64_t tmp = static_cast<int64_t>(a) * b;
    if (check_cast<int32_t>(tmp)) {
        return tmp;
    }
    return std::nullopt;
}

//! Multiply two integers checking overflows.
//!
//! Fallback for the general case.
template <class S>
    requires std::is_signed_v<S>
auto check_mul(S a, S b) -> std::optional<S> {
#ifdef __GNUC__
    S c;
    if (!__builtin_mul_overflow(a, b, &c)) {
        return std::nullopt;
    }
    return c;
#else
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
#endif
}

//! Prevent multiplying arithmetic types not explicitly handled.
template <class S, class T>
    requires std::is_arithmetic_v<S> && std::is_arithmetic_v<T>
auto check_mul(S a, T b) = delete;

// Division

//! Divide two integers checking overflows (truncating toward negative infinity).
template <class S>
    requires std::is_signed_v<S>
auto check_div(S a, S b) -> std::optional<S> {
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

//! Prevent dividing arithmetic types not explicitly handled.
template <class S, class T>
    requires std::is_arithmetic_v<S> && std::is_arithmetic_v<T>
auto check_div(S a, T b) = delete;

// Modulo

//! Modulo of two integers checking overflows (truncating toward negative infinity).
template <class S>
    requires std::is_signed_v<S>
auto check_mod(S a, S b) -> std::optional<S> {
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

//! Modulo of arithmetic types not explicitly handled.
template <class S, class T>
    requires std::is_arithmetic_v<S> && std::is_arithmetic_v<T>
auto check_mod(S a, T b) = delete;

// Power

//! Power of the given integers checking overflows.
//!
//! Note that <tt>a^0 = 1</tt> for all values of <tt>a</tt> and <tt>a^b=0</tt> whenever <tt>b<tt> is less than zero.
template <class S>
    requires std::is_signed_v<S>
auto check_pow(S a, S b) -> std::optional<S> {
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

} // namespace Gringo
