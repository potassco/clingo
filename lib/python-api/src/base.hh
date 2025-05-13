#pragma once

#include <clingo/base.h>

#include <pybind11/pybind11.h>

#include "iterator.hh" // IWYU pragma: keep
#include "symbol.hh"

#include <span>

namespace PyClingo {

class Atom {
  public:
    Atom(clingo_atom_base_t const *base, size_t index) : base_{base}, index_{index} {}

    auto literal() -> clingo_literal_t;
    auto symbol() -> Symbol;

    [[nodiscard]] auto hash() const { return hash_combine(index_, hash_value(base_)); }
    friend auto operator==(Atom const &a, Atom const &b) -> bool { return a.index_ == b.index_; }
    friend auto operator!=(Atom const &a, Atom const &b) -> bool = default;

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

    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto at(size_t index) const -> value_type;
    [[nodiscard]] auto contains(key_type const &symbol) const -> bool;
    [[nodiscard]] auto get(key_type const &symbol, std::optional<mapped_type> def) const -> std::optional<mapped_type>;

  private:
    clingo_atom_base_t const *base_;
};

class Term {
  public:
    Term(clingo_term_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    auto symbol() -> Symbol;
    auto condition() -> TypeHint<"Sequence[Sequence[int]]">;

    [[nodiscard]] auto hash() const { return hash_combine(index_, hash_value(base_)); }
    friend auto operator==(Term const &a, Term const &b) -> bool { return a.index_ == b.index_; }
    friend auto operator!=(Term const &a, Term const &b) -> bool = default;

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

    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto at(size_t index) const -> value_type;
    [[nodiscard]] auto contains(key_type const &symbol) const -> bool;
    [[nodiscard]] auto get(key_type const &symbol, std::optional<mapped_type> def) const -> std::optional<mapped_type>;

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
    auto name() -> std::string_view;
    auto arguments() -> TypeHint<"Sequence[TheoryTerm]">;
    auto str() -> std::string_view;

    [[nodiscard]] auto hash() const { return hash_combine(index_, hash_value(base_)); }
    friend auto operator==(TheoryTerm const &a, TheoryTerm const &b) -> bool { return a.index_ == b.index_; }
    friend auto operator!=(TheoryTerm const &a, TheoryTerm const &b) -> bool = default;

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};

class TheoryElement {
  public:
    TheoryElement(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}
    auto tuple() -> TypeHint<"Sequence[TheoryTerm]">;
    auto condition() -> LitSpan;
    auto condition_id() -> clingo_literal_t;
    auto str() -> std::string_view;

    [[nodiscard]] auto hash() const { return hash_combine(index_, hash_value(base_)); }
    friend auto operator==(TheoryElement const &a, TheoryElement const &b) -> bool { return a.index_ == b.index_; }
    friend auto operator!=(TheoryElement const &a, TheoryElement const &b) -> bool = default;

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};
using TheoryElementVec = std::vector<TheoryElement>;

class TheoryAtom {
  public:
    TheoryAtom(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}
    auto name() -> TheoryTerm;
    auto elements() -> TypeHint<"Sequence[TheoryElement]">;
    auto literal() -> clingo_literal_t;
    auto guard() -> std::optional<std::pair<std::string_view, TheoryTerm>>;
    auto str() -> std::string_view;

    [[nodiscard]] auto hash() const { return hash_combine(index_, hash_value(base_)); }
    friend auto operator==(TheoryAtom const &a, TheoryAtom const &b) -> bool { return a.index_ == b.index_; }
    friend auto operator!=(TheoryAtom const &a, TheoryAtom const &b) -> bool = default;

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};

class TheoryBase {
  public:
    using value_type = TheoryAtom;

    TheoryBase(clingo_theory_base_t const &base) : base_{&base} {}
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto at(size_t index) const -> value_type;

  private:
    clingo_theory_base_t const *base_;
};

class Base {
  public:
    using key_type = std::tuple<std::string_view, size_t, bool>;
    using mapped_type = AtomBase;
    using value_type = std::pair<key_type, mapped_type>;

    Base(clingo_base_t const *base) : base_{base} {}

    [[nodiscard]] auto is_external(clingo_literal_t lit) const -> bool;
    [[nodiscard]] auto is_fact(clingo_literal_t lit) const -> bool;
    [[nodiscard]] auto is_shown(clingo_literal_t lit) const -> bool;
    [[nodiscard]] auto is_projected(clingo_literal_t lit) const -> bool;
    [[nodiscard]] auto is_current(clingo_literal_t lit) const -> bool;
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto at(size_t index) const -> value_type;
    [[nodiscard]] auto contains(key_type const &sig) const -> bool;
    [[nodiscard]] auto contains_short(std::pair<std::string_view, size_t> const &sig) const -> bool;
    [[nodiscard]] auto contains_symbol(Symbol const &sym) const -> bool;
    [[nodiscard]] auto get(key_type const &sig, std::optional<mapped_type> def) const -> std::optional<mapped_type>;
    [[nodiscard]] auto lookup(key_type const &sig) const -> mapped_type;
    [[nodiscard]] auto lookup_short(std::pair<std::string_view, size_t> const &sig) const -> mapped_type;
    [[nodiscard]] auto lookup_symbol(Symbol const &sym) const -> Atom;
    [[nodiscard]] auto terms() const -> TermBase;
    [[nodiscard]] auto theory() const -> TheoryBase;

  private:
    clingo_base_t const *base_;
};

using MixedLitSpan = std::span<std::variant<std::pair<Symbol, bool>, Lit_t>>;

auto convert(Base base, MixedLitSpan const &lits, bool flip) -> LitVec;

void register_base(pybind11::module &m);

} // namespace PyClingo
