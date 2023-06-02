#pragma once

#include <ostream>
#include <variant>
#include <vector>

// Currently meant as a placeholder for the real thing.

enum class Constant : int {
    supremum,
    infimum,
};

auto operator<<(std::ostream &out, Constant op) -> std::ostream &;

class QuotedString {
  public:
    explicit QuotedString(std::string value) : value{std::move(value)} {}
    friend auto operator<<(std::ostream &out, QuotedString const &sym) -> std::ostream &;

    std::string const value;
};

class Function;

using Symbol = std::variant<int, Constant, QuotedString, Function>;
using SymVec = std::vector<Symbol>;

class Function {
  public:
    explicit Function(std::string name, SymVec args = {}) : name{std::move(name)}, args{std::move(args)} {}
    friend auto operator<<(std::ostream &out, Function const &sym) -> std::ostream &;

    std::string const name;
    SymVec const args;
};

auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream &;
