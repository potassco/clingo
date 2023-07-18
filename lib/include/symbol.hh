#pragma once

#include <ostream>
#include <variant>
#include <vector>

#include <util/hash.hh>

namespace Gringo {

// Currently meant as a placeholder for the real thing.

enum class Constant : int {
    supremum,
    infimum,
};

auto operator<<(std::ostream &out, Constant op) -> std::ostream &;

class QuotedString {
  public:
    explicit QuotedString(std::string value) : value{std::move(value)} {}
    friend auto operator==(QuotedString const &a, QuotedString const &b) -> bool;
    friend auto operator<<(std::ostream &out, QuotedString const &sym) -> std::ostream &;

    std::string value;
};

class Function;

using Symbol = std::variant<int, Constant, QuotedString, Function>;
using SymVec = std::vector<Symbol>;

class Function {
  public:
    explicit Function(std::string name, SymVec args = {}) : name{std::move(name)}, args{std::move(args)} {}
    friend auto operator==(Function const &a, Function const &b) -> bool;
    friend auto operator<<(std::ostream &out, Function const &sym) -> std::ostream &;

    bool has_sign = false;
    std::string name;
    SymVec args;
};

auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream &;

[[nodiscard]] auto has_sign(Symbol const &sym) -> bool;

} // namespace Gringo

HASH_PROTO(Gringo::QuotedString)
HASH_PROTO(Gringo::Function)
