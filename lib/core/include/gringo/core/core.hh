#pragma once

#include <ostream>
#include <cstdint>

namespace Gringo {

//! Enumeration of signs (default negation).
enum class Sign : uint8_t {
    none,  //!< No sign.
    once,  //!< One sign (not).
    twice, //!< Two signs (not not).
};

//! Negate the sign.
auto operator-(Sign a) -> Sign;
//! Combine two signs.
auto operator+(Sign a, Sign b) -> Sign;
//! Combine two signs.
auto operator+=(Sign &a, Sign b) -> Sign &;

//! Output the given sign.
auto operator<<(std::ostream &out, Sign sign) -> std::ostream &;

//! Enumeration of supported relations.
enum class Relation : uint8_t {
    equal,         //!< The equal to symbol (=).
    not_equal,     //!< The not equal to symbol (!=).
    less,          //!< The less than symbol (<).
    less_equal,    //!< The less than or equal to symbol (<=).
    greater,       //!< The greater than symbol (>).
    greater_equal, //!< The greater than or equal to symbol (>=).
};

//! Return the equivalent relation when arguments are flipped.
[[nodiscard]] auto flip(Relation rel) -> Relation;
//! Return the complement of the given relation.
[[nodiscard]] auto complement(Relation rel) -> Relation;

//! Output the given relation.
auto operator<<(std::ostream &out, Relation rel) -> std::ostream &;

} // namespace Gringo
