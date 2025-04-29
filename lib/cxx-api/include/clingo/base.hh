#pragma once

#include <cassert>
#include <clingo/symbol.hh>

#include <clingo/base.h>

namespace Clingo {

class Atom {
  public:
    explicit Atom(clingo_atom_base_t const *base, size_t index) : base_{base}, index_{index} {}

    [[nodiscard]] auto literal() const -> clingo_literal_t {
        auto lit = clingo_literal_t{0};
        Detail::handle_error(clingo_atom_base_literal(base_, index_, &lit));
        return lit;
    }
    [[nodiscard]] auto symbol() const -> Symbol {
        auto sym = clingo_symbol_t{0};
        Detail::handle_error(clingo_atom_base_symbol(base_, index_, &sym));
        return Symbol{sym, true};
    }

    [[nodiscard]] auto hash() const { return Detail::hash_value(index_); }
    friend auto operator==(Atom const &a, Atom const &b) -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }
    friend auto operator<=>(Atom const &a, Atom const &b) -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_atom_base_t const *base_;
    size_t index_;
};

namespace Detail {

template <typename T> class ArrowProxy {
  public:
    constexpr ArrowProxy(T value) : value_(std::move(value)) {}
    constexpr auto operator->() -> T * { return &value_; }

  private:
    T value_;
};

template <class Seq> class RandomAccessIterator {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename Seq::value_type;
    using size_type = typename Seq::size_type;
    using difference_type = typename Seq::difference_type;
    using pointer = typename Seq::pointer;
    using reference = typename Seq::reference;

    // NOTE: Added to fullfil the sentinel_for concept; should not be used.
    constexpr RandomAccessIterator() : view_{throw std::logic_error("invalid iterator")}, index_{0} {}
    constexpr RandomAccessIterator(Seq container, size_t index) noexcept : view_{std::move(container)}, index_{index} {}
    constexpr auto operator*() const -> reference { return view_.at(index_); }
    constexpr auto operator->() const -> pointer { return view_.at(index_); }
    constexpr auto operator++() -> RandomAccessIterator & {
        ++index_;
        return *this;
    }
    constexpr auto operator++(int) -> RandomAccessIterator {
        auto tmp = *this;
        ++index_;
        return tmp;
    }
    constexpr auto operator--() -> RandomAccessIterator & {
        --index_;
        return *this;
    }
    constexpr auto operator--(int) -> RandomAccessIterator {
        auto tmp = *this;
        --index_;
        return tmp;
    }
    constexpr auto operator-(const RandomAccessIterator &other) const -> difference_type {
        return static_cast<difference_type>(index_) - static_cast<difference_type>(other.index_);
    }
    constexpr auto operator+(difference_type n) const -> RandomAccessIterator {
        return RandomAccessIterator(view_, index_ + n);
    }
    friend constexpr auto operator+(difference_type n, RandomAccessIterator it) -> RandomAccessIterator {
        return RandomAccessIterator(it.view_, it.index_ + n);
    }
    constexpr auto operator-(difference_type n) const -> RandomAccessIterator {
        return RandomAccessIterator(view_, index_ - n);
    }
    constexpr auto operator+=(difference_type n) -> RandomAccessIterator & {
        index_ += n;
        return *this;
    }
    constexpr auto operator-=(difference_type n) -> RandomAccessIterator & {
        index_ -= n;
        return *this;
    }
    constexpr auto operator==(const RandomAccessIterator &other) const -> bool { return index_ == other.index_; }
    constexpr auto operator<=>(const RandomAccessIterator &other) const { return index_ <=> other.index_; }
    constexpr auto operator[](difference_type n) const -> reference { return view_.at(index_ + n); }

  private:
    Seq view_;
    size_t index_;
};

} // namespace Detail

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

    [[nodiscard]] auto size() const -> size_t {
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

    [[nodiscard]] auto begin() const -> Detail::RandomAccessIterator<AtomBase> { return {*this, 0}; }

    [[nodiscard]] auto end() const -> Detail::RandomAccessIterator<AtomBase> { return {*this, size()}; }

  private:
    clingo_atom_base_t const *base_;
};

class Term {
  public:
    explicit Term(clingo_term_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    auto symbol() -> Symbol {
        auto sym = clingo_symbol_t{0};
        Detail::handle_error(clingo_term_base_symbol(base_, index_, &sym));
        return Symbol{sym, true};
    }
    auto condition() -> std::vector<std::vector<clingo_literal_t>> {
        size_t const *sizes = nullptr;
        clingo_literal_t const *const *lits = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_term_base_condition(base_, index_, &sizes, &lits, &size));

        auto res = std::vector<std::vector<clingo_literal_t>>{};
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            res.emplace_back(lits[i], lits[i] + sizes[i]);
        }
        return res;
    }

    [[nodiscard]] auto hash() const -> size_t { return Detail::hash_value(index_); }
    friend auto operator==(Term const &a, Term const &b) -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }
    friend auto operator<=>(Term const &a, Term const &b) -> std::strong_ordering {
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

    TermBase(clingo_term_base_t const &base) : base_{&base} {}

    [[nodiscard]] auto size() const -> size_t {
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

    [[nodiscard]] auto begin() const -> Detail::RandomAccessIterator<TermBase> { return {*this, 0}; }

    [[nodiscard]] auto end() const -> Detail::RandomAccessIterator<TermBase> { return {*this, size()}; }

  private:
    clingo_term_base_t const *base_;
};

class TheoryTerm;
using TheoryTermVec = std::vector<TheoryTerm>;

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
        Detail::handle_error(clingo_theory_base_term_name(base_, index_, &name));
        return name;
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
        char const *str = nullptr;
        Detail::handle_error(clingo_string_builder_string(c_cast(bld), &str, nullptr));
        return str;
    }

    [[nodiscard]] auto hash() const -> size_t { return Detail::hash_value(index_); }
    friend auto operator==(TheoryTerm const &a, TheoryTerm const &b) -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }
    friend auto operator<=>(TheoryTerm const &a, TheoryTerm const &b) -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};

/*
class TheoryElement {
  public:
    TheoryElement(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}
    auto tuple() -> TypeHint<"Sequence[TheoryTerm]">;
    auto condition() -> LitSpan;
    auto condition_id() -> clingo_literal_t;
    auto str() -> char const *;

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
    auto guard() -> std::optional<std::pair<char const *, TheoryTerm>>;
    auto str() -> char const *;

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
    using key_type = std::tuple<char const *, size_t, bool>;
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
    [[nodiscard]] auto contains_short(std::pair<char const *, size_t> const &sig) const -> bool;
    [[nodiscard]] auto contains_symbol(Symbol const &sym) const -> bool;
    [[nodiscard]] auto get(key_type const &sig, std::optional<mapped_type> def) const -> std::optional<mapped_type>;
    [[nodiscard]] auto lookup(key_type const &sig) const -> mapped_type;
    [[nodiscard]] auto lookup_short(std::pair<char const *, size_t> const &sig) const -> mapped_type;
    [[nodiscard]] auto lookup_symbol(Symbol const &sym) const -> Atom;
    [[nodiscard]] auto terms() const -> TermBase;
    [[nodiscard]] auto theory() const -> TheoryBase;

  private:
    clingo_base_t const *base_;
};

using MixedLitSpan = std::span<std::variant<std::pair<Symbol, bool>, Lit_t>>;

auto convert(Base base, MixedLitSpan const &lits, bool flip) -> LitVec;

*/

} // namespace Clingo

namespace std {

template <> struct hash<Clingo::Atom> {
    auto operator()(Clingo::Atom const &atom) const -> size_t { return atom.hash(); }
};

template <> struct hash<Clingo::Term> {
    auto operator()(Clingo::Term const &term) const -> size_t { return term.hash(); }
};

} // namespace std
