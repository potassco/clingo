#pragma once

// The code below heavily borows from <https://github.com/dcleblanc/SafeInt>.
// In principle, one could also just use it as a third party library.
// Currently, it is not included because the C++ code is a bit heavy (7k loc).
// I still have to make up my mind here.

#include <optional>
#include <type_traits>

namespace Gringo::Input {

// Casting

//! Check if S can be casted to T without loss.
template <class T, class S>
inline auto check_cast(S in) -> std::enable_if_t<std::is_signed_v<T> && std::is_signed_v<S>, bool> {
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
template <class S> inline auto check_add(S a, S b) -> std::enable_if_t<std::is_signed_v<S>, std::optional<S>> {
    using U = std::make_unsigned_t<S>;
    S tmp = static_cast<S>(static_cast<U>(a) + static_cast<U>(b));
    if ((a >= 0 && b >= 0 && tmp < a) || (a < 0 && b < 0 && tmp > a)) {
        return std::nullopt;
    }
    return tmp;
}

//! Prevent adding arithmetic types not explicitly handled.
template <class S, class T>
inline auto check_add(S a, T b) -> std::enable_if_t<std::is_arithmetic_v<S> && std::is_arithmetic_v<T>> = delete;

// Subtraction

//! Subtract two integers checking overflows.
inline auto check_sub(int32_t a, int32_t b) -> std::optional<int32_t> {
    int64_t tmp = static_cast<int64_t>(a) - b;
    if (check_cast<int32_t>(tmp)) {
        return tmp;
    }
    return std::nullopt;
}

template <class S> inline auto check_sub(S a, S b) -> std::enable_if_t<std::is_signed_v<S>, std::optional<S>> {
    using U = std::make_unsigned_t<S>;
    S tmp = static_cast<S>(static_cast<U>(a) - static_cast<U>(b));
    if ((a >= 0 && b < 0 && tmp < a) || (b >= 0 && tmp > a)) {
        return std::nullopt;
    }
    return tmp;
}

//! Prevent subtracting arithmetic types not explicitly handled.
template <class S, class T>
inline auto check_sub(S a, T b) -> std::enable_if_t<std::is_arithmetic_v<S> && std::is_arithmetic_v<T>> = delete;

// Unary Minus

// Absolute

// Multiplication

// Division

//! Divide two integers checking overflows.
template <class S> inline auto check_div(S a, S b) -> std::enable_if_t<std::is_signed_v<S>, std::optional<S>> {
    if (b == 0 || (b == -1 && a == std::numeric_limits<S>::min())) {
        return std::nullopt;
    }
    // TODO: This is C's integer division
    // For example clingcon uses:
    /*
      template<typename I>
      I floordiv(I n, I m) {
          using std::div;
          auto a = div(n, m);
          if (((n < 0) ^ (m < 0)) && a.rem != 0) {
              a.quot--;
          }
          return a.quot;
      }

      template<typename I>
      I ceildiv(I n, I m) {
          using std::div;
          auto a = div(n, m);
          if (((n < 0) ^ (m < 0)) && a.rem != 0) {
              a.quot++;
          }
          return a.quot;
      }
    */
    return a / b;
}

//! Prevent dividing arithmetic types not explicitly handled.
template <class S, class T>
inline auto check_div(S a, T b) -> std::enable_if_t<std::is_arithmetic_v<S> && std::is_arithmetic_v<T>> = delete;

// Modulo

// TODO: use the standard arithmetic modulus here instead of the C one

// Power

// Exclusive Or

// Bitwise Or

// Bitwise And

// Bitwise Negation

} // namespace Gringo::Input
