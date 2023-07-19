#pragma once

//! @file
//! The symbol implementation available here is currently a placeholder.
//! Something similar to what is used in gringo should be used.

#include <ostream>
#include <variant>
#include <vector>

#include <util/hash.hh>

namespace Gringo {

//! The supremum and infimum constants.
enum class Constant : int {
    supremum, //!< The supremum (<tt>\#sup</tt>).
    infimum,  //!< The infimum (<tt>\#inf</tt>).
};

//! Output the string representation of the constant.
auto operator<<(std::ostream &out, Constant op) -> std::ostream &;

//! A quoted string.
//!
//! A raw string is stored that is quoted when output.
//! For example: <tt>"foo\nbar"</tt>.
struct QuotedString {
    //! Construct a quoted string.
    explicit QuotedString(std::string value) : value{std::move(value)} {}
    //! Compare two quoted strings.
    friend auto operator==(QuotedString const &a, QuotedString const &b) -> bool;
    //! Output the string in quotes escaping special symbols.
    friend auto operator<<(std::ostream &out, QuotedString const &sym) -> std::ostream &;

    //! The raw string value.
    std::string value;
};

struct Function;

//! A variant for the different symbol types.
//!
//! For example: <tt>f(1,\#inf,"xyz")</tt>.
using Symbol = std::variant<int, Constant, QuotedString, Function>;
//! A vector of symbols.
using SymVec = std::vector<Symbol>;

//! A function symbol.
//!
//! Note that tuples also use this type.
//! For example: <tt>f(x,y)</tt>.
struct Function {
    //! Construct a function symbol.
    explicit Function(std::string name, SymVec args = {}) : name{std::move(name)}, args{std::move(args)} {}
    friend auto operator==(Function const &a, Function const &b) -> bool;
    friend auto operator<<(std::ostream &out, Function const &sym) -> std::ostream &;

    //! Whether the symbol is negated.
    //!
    //! Only functions can be negated not tuples.
    bool has_sign = false;
    //! The name of the function symbol (or empty for tuples).
    std::string name;
    //! The arguments of the function.
    SymVec args;
};

//! Output the given symbol.
auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream &;

//! Check whether the symbol has a sign.
//!
//! Returns true for negative integers and negated functions.
[[nodiscard]] auto has_sign(Symbol const &sym) -> bool;

} // namespace Gringo

GRINGO_HASH_PROTO(Gringo::QuotedString)
GRINGO_HASH_PROTO(Gringo::Function)
