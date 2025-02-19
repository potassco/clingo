#pragma once

#include <clingo/base.h>

#include <pybind11/pybind11.h>

#include "iterator.hh"
#include "symbol.hh"

#include <span>

namespace Clingo::Python {

class Atom {
  public:
    Atom(clingo_atom_base_t const *base, size_t index) : base_{base}, index_{index} {}

    auto literal() -> clingo_literal_t;
    auto symbol() -> Symbol;

  private:
    clingo_atom_base_t const *base_;
    size_t index_;
};

class AtomBase {
  public:
    using key_type = Symbol;
    using mapped_type = Atom;
    using value_type = std::pair<key_type, mapped_type>;

    AtomBase(clingo_atom_base_t const *base) : base_{base} {}

    auto size() -> size_t;
    auto at(size_t index) -> value_type;
    auto contains(key_type const &symbol) -> bool;
    auto lookup(key_type const &symbol) -> mapped_type;
    [[nodiscard]] auto begin() { return RandomAccessIterator{*this, 0}; }
    [[nodiscard]] auto end() { return RandomAccessIterator{*this, size()}; }

  private:
    clingo_atom_base_t const *base_;
};

class Term {
  public:
    Term(clingo_term_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    auto symbol() -> Symbol;
    auto condition() -> std::optional<std::span<clingo_literal_t const>>;

  private:
    clingo_term_base_t const *base_;
    size_t index_;
};

class TermBase {
  public:
    using key_type = Symbol;
    using mapped_type = Term;
    using value_type = std::pair<key_type, mapped_type>;

    TermBase(clingo_term_base_t const &base) : base_{&base} {}

    auto size() -> size_t;
    auto at(size_t index) -> value_type;
    auto contains(key_type const &symbol) -> bool;
    auto lookup(key_type const &symbol) -> mapped_type;
    [[nodiscard]] auto begin() { return RandomAccessIterator{*this, 0}; }
    [[nodiscard]] auto end() { return RandomAccessIterator{*this, size()}; }

  private:
    clingo_term_base_t const *base_;
};

class TheoryTerm;
using TheoryTermVec = std::vector<TheoryTerm>;

class TheoryTerm {
  public:
    TheoryTerm(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}
    auto type() -> clingo_theory_term_type_e;
    auto number() -> int;
    auto name() -> char const *;
    auto arguments() -> TheoryTermVec;

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};

class TheoryElement {
  public:
    TheoryElement(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}
    auto tuple() -> TheoryTermVec;
    auto condition() -> LitSpan;
    auto condition_id() -> clingo_literal_t;

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};
using TheoryElementVec = std::vector<TheoryElement>;

class TheoryAtom {
  public:
    TheoryAtom(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}
    auto name() -> TheoryTerm;
    auto elements() -> TheoryElementVec;
    auto literal() -> clingo_literal_t;
    auto guard() -> std::optional<std::pair<char const *, TheoryTerm>>;

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};

class TheoryBase {
  public:
    using value_type = TheoryAtom;

    TheoryBase(clingo_theory_base_t const &base) : base_{&base} {}
    auto size() -> size_t;
    auto at(size_t index) -> value_type;
    [[nodiscard]] auto begin() { return RandomAccessIterator{*this, 0}; }
    [[nodiscard]] auto end() { return RandomAccessIterator{*this, size()}; }

  private:
    clingo_theory_base_t const *base_;
};

class Base {
  public:
    using key_type = std::tuple<char const *, size_t, bool>;
    using mapped_type = AtomBase;
    using value_type = std::pair<key_type, mapped_type>;

    Base(clingo_base_t const *base) : base_{base} {}

    auto is_external(clingo_literal_t lit) -> bool;
    auto is_fact(clingo_literal_t lit) -> bool;
    auto size() -> size_t;
    auto at(size_t index) -> value_type;
    auto contains(key_type const &sig) -> bool;
    auto contains_short(std::pair<char const *, size_t> const &sig) -> bool;
    auto lookup(key_type const &sig) -> mapped_type;
    auto lookup_short(std::pair<char const *, size_t> const &sig) -> mapped_type;
    auto terms() -> TermBase;
    auto theory() -> TheoryBase;
    [[nodiscard]] auto begin() { return RandomAccessIterator{*this, 0}; }
    [[nodiscard]] auto end() { return RandomAccessIterator{*this, size()}; }

  private:
    clingo_base_t const *base_;
};

using MixedLitlVec = std::vector<std::variant<std::pair<Symbol, bool>, Lit_t>>;

auto convert(Base base, MixedLitlVec const &lits) -> LitVec;

void register_base(pybind11::module &m);

} // namespace Clingo::Python
