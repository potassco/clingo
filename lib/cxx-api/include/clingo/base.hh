#pragma once

#include <clingo/symbol.hh>

#include <clingo/base.h>

#include <cassert>

namespace Clingo {

class Atom {
  public:
    explicit Atom(clingo_atom_base_t const *base, size_t index) : base_{base}, index_{index} {}

    [[nodiscard]] auto literal() const -> Literal {
        auto lit = clingo_literal_t{0};
        Detail::handle_error(clingo_atom_base_literal(base_, index_, &lit));
        return lit;
    }

    [[nodiscard]] auto symbol() const -> Symbol {
        auto sym = clingo_symbol_t{0};
        Detail::handle_error(clingo_atom_base_symbol(base_, index_, &sym));
        return Symbol{sym, true};
    }

    [[nodiscard]] auto hash() const noexcept { return Detail::hash_value(index_); }

    friend auto operator==(Atom const &a, Atom const &b) noexcept -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }

    friend auto operator<=>(Atom const &a, Atom const &b) noexcept -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_atom_base_t const *base_;
    size_t index_;
};

class AtomBase {
  public:
    using key_type = Symbol;
    using mapped_type = Atom;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<AtomBase>;

    explicit AtomBase(clingo_atom_base_t const *base) : base_{base} {}

    [[nodiscard]] auto size() const -> size_type {
        size_t size = 0;
        Detail::handle_error(clingo_atom_base_size(base_, &size));
        return size;
    }

    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto atom = Atom{base_, index};
        return index < size() ? value_type{atom.symbol(), atom} : throw std::out_of_range{"index out of range"};
    }

    [[nodiscard]] auto contains(key_type const &symbol) const -> bool {
        auto index = size_t{0};
        Detail::handle_error(clingo_atom_base_find(base_, *c_cast(&symbol), &index));
        return index < size();
    }

    [[nodiscard]] auto get(key_type const &symbol, std::optional<mapped_type> def) const -> std::optional<mapped_type> {
        auto index = size_t{0};
        Detail::handle_error(clingo_atom_base_find(base_, *c_cast(&symbol), &index));
        return index < size() ? std::make_optional<mapped_type>(base_, index) : def;
    }

    [[nodiscard]] auto begin() const -> iterator { return {*this, 0}; }

    [[nodiscard]] auto end() const -> iterator { return {*this, size()}; }

  private:
    clingo_atom_base_t const *base_;
};

class Term {
  public:
    explicit Term(clingo_term_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    [[nodiscard]] auto symbol() const -> Symbol {
        auto sym = clingo_symbol_t{0};
        Detail::handle_error(clingo_term_base_symbol(base_, index_, &sym));
        return Symbol{sym, true};
    }
    [[nodiscard]] auto condition() const -> std::vector<LiteralVector> {
        size_t const *sizes = nullptr;
        clingo_literal_t const *const *lits = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_term_base_condition(base_, index_, &sizes, &lits, &size));

        auto res = std::vector<LiteralVector>{};
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            res.emplace_back(lits[i], lits[i] + sizes[i]);
        }
        return res;
    }

    [[nodiscard]] auto hash() const noexcept -> size_t { return Detail::hash_value(index_); }

    friend auto operator==(Term const &a, Term const &b) noexcept -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }

    friend auto operator<=>(Term const &a, Term const &b) noexcept -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_term_base_t const *base_;
    size_t index_;
};

class TermBase {
  public:
    using key_type = Symbol;
    using mapped_type = Term;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<TermBase>;

    explicit TermBase(clingo_term_base_t const &base) : base_{&base} {}

    [[nodiscard]] auto size() const -> size_type {
        size_t size = 0;
        Detail::handle_error(clingo_term_base_size(base_, &size));
        return size;
    }

    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto term = Term{*base_, index};
        return index < size() ? value_type{term.symbol(), term} : throw std::out_of_range{"index out of range"};
    }

    [[nodiscard]] auto contains(key_type const &symbol) const -> bool {
        auto index = size_t{0};
        Detail::handle_error(clingo_term_base_find(base_, *c_cast(&symbol), &index));
        return index < size();
    }

    [[nodiscard]] auto get(key_type const &symbol, std::optional<mapped_type> def) const -> std::optional<mapped_type> {
        auto index = size_t{0};
        Detail::handle_error(clingo_term_base_find(base_, *c_cast(&symbol), &index));
        return index < size() ? std::make_optional<mapped_type>(*base_, index) : def;
    }

    [[nodiscard]] auto begin() const -> iterator { return {*this, 0}; }

    [[nodiscard]] auto end() const -> iterator { return {*this, size()}; }

  private:
    clingo_term_base_t const *base_;
};

class TheoryTerm;
using TheoryTermVector = std::vector<TheoryTerm>;

class TheoryTerm {
  public:
    explicit TheoryTerm(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    [[nodiscard]] auto type() const -> clingo_theory_term_type_e {
        clingo_theory_term_type_t type = 0;
        Detail::handle_error(clingo_theory_base_term_type(base_, index_, &type));
        return static_cast<clingo_theory_term_type_e>(type);
    }

    [[nodiscard]] auto number() const -> int {
        int num = 0;
        Detail::handle_error(clingo_theory_base_term_number(base_, index_, &num));
        return num;
    }

    [[nodiscard]] auto name() const -> std::string_view {
        char const *name = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_theory_base_term_name(base_, index_, &name, &size));
        return {name, size};
    }

    [[nodiscard]] auto arguments() const -> std::vector<TheoryTerm> {
        size_t size = 0;
        clingo_id_t const *args = nullptr;
        Detail::handle_error(clingo_theory_base_term_arguments(base_, index_, &args, &size));
        return Detail::transform(std::span{args, size}, [this](clingo_id_t id) { return TheoryTerm{*base_, id}; });
    }

    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_theory_base_term_to_string(base_, index_, c_cast(bld)));
        return std::string{bld.str()};
    }

    [[nodiscard]] auto hash() const noexcept -> size_t { return Detail::hash_value(index_); }

    friend auto operator==(TheoryTerm const &a, TheoryTerm const &b) noexcept -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }

    friend auto operator<=>(TheoryTerm const &a, TheoryTerm const &b) noexcept -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};

class TheoryElement;
using TheoryElementVector = std::vector<TheoryElement>;

class TheoryElement {
  public:
    explicit TheoryElement(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    [[nodiscard]] auto tuple() const -> TheoryTermVector {
        size_t size = 0;
        clingo_id_t const *tuple = nullptr;
        Detail::handle_error(clingo_theory_base_element_tuple(base_, index_, &tuple, &size));
        return Detail::transform(std::span{tuple, size}, [this](clingo_id_t id) { return TheoryTerm{*base_, id}; });
    }

    [[nodiscard]] auto condition() const -> LiteralSpan {
        size_t size = 0;
        clingo_literal_t const *cond = nullptr;
        Detail::handle_error(clingo_theory_base_element_condition(base_, index_, &cond, &size));
        return std::span{cond, size};
    }

    [[nodiscard]] auto condition_id() const -> Literal {
        clingo_literal_t id = 0;
        Detail::handle_error(clingo_theory_base_element_condition_id(base_, index_, &id));
        return id;
    }

    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder();
        Detail::handle_error(clingo_theory_base_element_to_string(base_, index_, c_cast(bld)));
        return std::string{bld.str()};
    }

    [[nodiscard]] auto hash() const noexcept -> size_t { return Detail::hash_value(index_); }

    friend auto operator==(TheoryElement const &a, TheoryElement const &b) noexcept -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }

    friend auto operator<=>(TheoryElement const &a, TheoryElement const &b) noexcept -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};

class TheoryAtom {
  public:
    explicit TheoryAtom(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    [[nodiscard]] auto name() const -> TheoryTerm {
        clingo_id_t id = 0;
        Detail::handle_error(clingo_theory_base_atom_term(base_, index_, &id));
        return TheoryTerm{*base_, id};
    }

    [[nodiscard]] auto elements() const -> TheoryElementVector {
        size_t size = 0;
        clingo_id_t const *elems = nullptr;
        Detail::handle_error(clingo_theory_base_atom_elements(base_, index_, &elems, &size));
        return Detail::transform(std::span{elems, size}, [this](clingo_id_t id) { return TheoryElement{*base_, id}; });
    }

    [[nodiscard]] auto literal() const -> Literal {
        clingo_literal_t lit = 0;
        Detail::handle_error(clingo_theory_base_atom_literal(base_, index_, &lit));
        return lit;
    }

    [[nodiscard]] auto guard() const -> std::optional<std::pair<std::string_view, TheoryTerm>> {
        auto has_guard = false;
        Detail::handle_error(clingo_theory_base_atom_has_guard(base_, index_, &has_guard));
        if (has_guard) {
            char const *op = nullptr;
            size_t size = 0;
            clingo_id_t term = 0;
            Detail::handle_error(clingo_theory_base_atom_guard(base_, index_, &op, &size, &term));
            return std::pair{std::string_view{op, size}, TheoryTerm{*base_, term}};
        }
        return std::nullopt;
    }

    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_theory_base_atom_to_string(base_, index_, c_cast(bld)));
        return std::string{bld.str()};
    }

    [[nodiscard]] auto hash() const noexcept -> size_t { return Detail::hash_value(index_); }

    friend auto operator==(TheoryAtom const &a, TheoryAtom const &b) noexcept -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }

    friend auto operator<=>(TheoryAtom const &a, TheoryAtom const &b) noexcept -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};

class TheoryBase {
  public:
    using value_type = TheoryAtom;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<TheoryBase>;

    explicit TheoryBase(clingo_theory_base_t const &base) : base_{&base} {}

    [[nodiscard]] auto size() const -> size_type {
        size_t size = 0;
        Detail::handle_error(clingo_theory_base_size(base_, &size));
        return size;
    }

    [[nodiscard]] auto at(size_t index) const -> value_type {
        return index < size() ? TheoryAtom{*base_, index} : throw std::out_of_range{"atom index out of range"};
    }

    [[nodiscard]] auto begin() const -> iterator { return {*this, 0}; }

    [[nodiscard]] auto end() const -> iterator { return {*this, size()}; }

  private:
    clingo_theory_base_t const *base_;
};

class Base {
  public:
    using key_type = std::tuple<std::string_view, size_t, bool>;
    using mapped_type = AtomBase;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<Base>;

    Base(clingo_base_t const *base) : base_{base} {}

    [[nodiscard]] auto is_external(Literal lit) const -> bool {
        auto ext = false;
        Detail::handle_error(clingo_base_is_external(base_, lit, &ext));
        return ext;
    }

    [[nodiscard]] auto is_fact(Literal lit) const -> bool {
        auto fact = false;
        Detail::handle_error(clingo_base_is_fact(base_, lit, &fact));
        return fact;
    }

    [[nodiscard]] auto is_shown(Literal lit) const -> bool {
        auto shown = false;
        Detail::handle_error(clingo_base_is_shown(base_, lit, &shown));
        return shown;
    }

    [[nodiscard]] auto is_projected(Literal lit) const -> bool {
        auto projected = false;
        Detail::handle_error(clingo_base_is_fact(base_, lit, &projected));
        return projected;
    }

    [[nodiscard]] auto is_current(Literal lit) const -> bool {
        auto current = false;
        Detail::handle_error(clingo_base_is_current(base_, lit, &current));
        return current;
    }

    [[nodiscard]] auto size() const -> size_type {
        size_t size = 0;
        Detail::handle_error(clingo_base_atoms_size(base_, &size));
        return size;
    }

    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto sig = clingo_signature_t{};
        clingo_atom_base_t const *atoms = nullptr;
        if (index < size()) {
            Detail::handle_error(clingo_base_atoms_at(base_, index, &sig, &atoms));
            return std::pair{std::tuple{sig.name, sig.arity, sig.is_positive}, AtomBase{atoms}};
        }
        throw std::out_of_range{"index out of range"};
    }

    [[nodiscard]] auto contains(key_type const &sig) const -> bool {
        auto csig =
            clingo_signature_t{std::get<0>(sig).data(), std::get<0>(sig).size(), std::get<1>(sig), std::get<2>(sig)};
        auto found = false;
        Detail::handle_error(clingo_base_atoms_find(base_, &csig, nullptr, &found));
        return found;
    }

    [[nodiscard]] auto contains(std::pair<std::string_view, size_t> const &sig) const -> bool {
        return contains({std::get<0>(sig), std::get<1>(sig), true});
    }

    [[nodiscard]] auto contains(Symbol const &sym) const -> bool {
        if (auto sig = sym.signature(); sig) {
            if (auto base = get(*sig)) {
                return base->contains(sym);
            }
        }
        return false;
    }

    [[nodiscard]] auto get(key_type const &sig, std::optional<mapped_type> def = std::nullopt) const
        -> std::optional<mapped_type> {
        auto csig =
            clingo_signature_t{std::get<0>(sig).data(), std::get<0>(sig).size(), std::get<1>(sig), std::get<2>(sig)};
        clingo_atom_base_t const *atoms = nullptr;
        auto found = false;
        Detail::handle_error(clingo_base_atoms_find(base_, &csig, &atoms, &found));
        return found ? std::make_optional<AtomBase>(atoms) : def;
    }

    [[nodiscard]] auto get(std::pair<std::string_view, size_t> const &sig,
                           std::optional<mapped_type> def = std::nullopt) const -> std::optional<mapped_type> {
        return get({std::get<0>(sig), std::get<1>(sig), true}, def);
    }

    [[nodiscard]] auto get(Symbol const &sym, std::optional<Atom> def = std::nullopt) const -> std::optional<Atom> {
        if (auto sig = sym.signature(); sig) {
            if (auto base = get(*sig)) {
                return base->get(sym, def);
            }
        }
        return def;
    }

    [[nodiscard]] auto terms() const -> TermBase {
        auto const *terms = static_cast<clingo_term_base_t const *>(nullptr);
        Detail::handle_error(clingo_base_terms(base_, &terms));
        return TermBase{*terms};
    }

    [[nodiscard]] auto theory() const -> TheoryBase {
        auto const *base = static_cast<clingo_theory_base_t const *>(nullptr);
        Detail::handle_error(clingo_base_theory(base_, &base));
        return TheoryBase{*base};
    }

    [[nodiscard]] auto begin() const -> iterator { return {*this, 0}; }

    [[nodiscard]] auto end() const -> iterator { return {*this, size()}; }

  private:
    clingo_base_t const *base_;
};

} // namespace Clingo

namespace std {

template <> struct hash<Clingo::Atom> {
    auto operator()(Clingo::Atom const &x) const -> size_t { return x.hash(); }
};

template <> struct hash<Clingo::Term> {
    auto operator()(Clingo::Term const &x) const -> size_t { return x.hash(); }
};

template <> struct hash<Clingo::TheoryTerm> {
    auto operator()(Clingo::TheoryTerm const &x) const -> size_t { return x.hash(); }
};

template <> struct hash<Clingo::TheoryElement> {
    auto operator()(Clingo::TheoryElement const &x) const -> size_t { return x.hash(); }
};

template <> struct hash<Clingo::TheoryAtom> {
    auto operator()(Clingo::TheoryAtom const &x) const -> size_t { return x.hash(); }
};

} // namespace std
