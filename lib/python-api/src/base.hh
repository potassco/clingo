#pragma once

#include <clingo/base.h>

#include <pybind11/pybind11.h>

#include "symbol.hh"

#include <span>

namespace Clingo::Python {

class Atom {
  public:
    Atom(clingo_atom_base_t base, size_t index) : base_{base}, index_{index} {}

    auto literal() -> clingo_literal_t;
    auto symbol() -> Symbol;
    auto external() -> bool;
    auto fact() -> bool;
    [[nodiscard]] auto index() const -> size_t;

  private:
    clingo_atom_base_t base_;
    size_t index_;
};

class AtomBase {
  public:
    AtomBase(clingo_atom_base_t base) : base_{base} {}

    auto size() -> size_t;
    auto at(size_t index) -> Atom;
    auto index(Symbol const &symbol) -> size_t;
    auto lookup(Symbol const &symbol) -> std::optional<Atom>;

  private:
    clingo_atom_base_t base_;
};

class Term {
  public:
    Term(clingo_term_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    auto symbol() -> Symbol;
    auto condition() -> std::optional<std::span<clingo_literal_t const>>;
    [[nodiscard]] auto index() const -> size_t;

  private:
    clingo_term_base_t const *base_;
    size_t index_;
};

class TermBase {
  public:
    TermBase(clingo_term_base_t const &base) : base_{&base} {}

    auto size() -> size_t;
    auto at(size_t index) -> Term;
    auto index(Symbol const &symbol) -> size_t;
    auto lookup(Symbol const &symbol) -> std::optional<Term>;

  private:
    clingo_term_base_t const *base_;
};

class Base {
  public:
    using value_type = std::pair<std::tuple<std::string, size_t, bool>, AtomBase>;

    Base(clingo_base_t base) : base_{base} {}

    auto size() -> size_t;
    auto at(size_t index) -> value_type;
    auto lookup(std::tuple<char const *, size_t, bool> sig) -> AtomBase;
    auto terms() -> TermBase;

  private:
    clingo_base_t base_;
};

void register_base(pybind11::module &m);

} // namespace Clingo::Python
