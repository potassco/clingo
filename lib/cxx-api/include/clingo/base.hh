#pragma once

#include <clingo/symbol.hh>

#include <clingo/base.h>

#include <cassert>
#include <tuple>

namespace Clingo {

//! @addtogroup cpp_base
//! Inspection of atoms occurring in ground logic programs.
//! @{

//! Class to provide access to symbolic atoms.
class Atom {
  public:
    //! Construct an atom from its C representation.
    //!
    //! For internal use.
    //!
    //! @param base the associated base
    //! @param index the index of the atom
    explicit Atom(clingo_atom_base_t const *base, size_t index) : base_{base}, index_{index} {}

    //! Get the program literal of the atom.
    //!
    //! @return the program literal
    [[nodiscard]] auto literal() const -> ProgramLiteral {
        return Detail::call<clingo_atom_base_literal>(base_, index_);
    }

    //! Get the atom's symbol.
    //!
    //! @return the atom's symbol
    [[nodiscard]] auto symbol() const -> Symbol {
        return Symbol{Detail::call<clingo_atom_base_symbol>(base_, index_), true};
    }

    //! Get the header of the atom.
    //!
    //! The hash enables usage in hash tables. There is also an associtaed
    //! specialization of `std::hash` for this class.
    //!
    //! @return the atom's hash
    [[nodiscard]] auto hash() const noexcept -> size_t { return Detail::hash_value(index_); }

    //! Compare two atoms for equality.
    //!
    //! @param a the first atom
    //! @param b the second atom
    //! @return whether the two atoms are equal
    friend auto operator==(Atom const &a, Atom const &b) noexcept -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }

    //! Compare two atoms.
    //!
    //! @param a the first atom
    //! @param b the second atom
    //! @return the result of the comparison
    friend auto operator<=>(Atom const &a, Atom const &b) noexcept -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_atom_base_t const *base_;
    size_t index_;
};

//! An atom base that maps symbols to atoms.
class AtomBase {
  public:
    //! The key type.
    using key_type = Symbol;
    //! The mapped type.
    using mapped_type = Atom;
    //! The value type.
    using value_type = std::pair<key_type, mapped_type>;
    //! The size type.
    using size_type = std::size_t;
    //! The difference type.
    using difference_type = std::ptrdiff_t;
    //! The reference type.
    using reference = value_type;
    //! The pointer type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type.
    using iterator = Detail::RandomAccessIterator<AtomBase>;

    //! Construct an atom base from its C representation.
    //!
    //! For internal use.
    //!
    //! @param base the C atom base
    explicit AtomBase(clingo_atom_base_t const *base) : base_{base} {}

    //! The size of the atom base.
    //!
    //! @return the size of the atom base
    [[nodiscard]] auto size() const -> size_type { return Detail::call<clingo_atom_base_size>(base_); }

    //! Get the symbol atom pair at the given index.
    //!
    //! @param index the index of the atom
    //! @return the symbol atom pair
    [[nodiscard]] auto at(size_t index) const -> value_type {
        if (auto atom = Atom{base_, index}; index < size()) {
            return value_type{atom.symbol(), atom};
        }
        throw std::out_of_range{"index out of range"};
    }

    //! Whether the atom base contains the given symbol.
    //!
    //! @param symbol the symbol to check
    //! @return whether the atom base contains the symbol
    [[nodiscard]] auto contains(key_type const &symbol) const -> bool {
        return Detail::call<clingo_atom_base_find>(base_, *c_cast(&symbol)) < size();
    }

    //! Get the atom for the given symbol.
    //!
    //! @param symbol the symbol to look for
    //! @param def the default value to return if the symbol is not found
    //! @return the atom for the symbol, or the default value if not found
    [[nodiscard]] auto get(key_type const &symbol, std::optional<mapped_type> def = std::nullopt) const
        -> std::optional<mapped_type> {
        auto index = Detail::call<clingo_atom_base_find>(base_, *c_cast(&symbol));
        return index < size() ? std::make_optional<mapped_type>(base_, index) : def;
    }

    //! Get an iterator pointing to the first element of the atom base.
    //!
    //! @return an iterator to the first element
    [[nodiscard]] auto begin() const -> iterator { return {*this, 0}; }

    //! Get an iterator pointing to the end of the atom base.
    //!
    //! @return an iterator to the end of the atom base
    [[nodiscard]] auto end() const -> iterator { return {*this, size()}; }

  private:
    clingo_atom_base_t const *base_;
};
static_assert(std::random_access_iterator<AtomBase::iterator>);

//! Class to provide access to terms in a program.
class Term {
  public:
    //! Construct a term from its C representation.
    //!
    //! @param base the C term base
    //! @param index the index of the term
    explicit Term(clingo_term_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    //! Get the symbol of the term.
    //!
    //! @return the symbol of the term
    [[nodiscard]] auto symbol() const -> Symbol {
        return Symbol{Detail::call<clingo_term_base_symbol>(base_, index_), true};
    }

    //! Get the condition of the term.
    //!
    //! @return the conditions as a vector of program literal vectors
    [[nodiscard]] auto condition() const -> std::vector<ProgramLiteralVector> {
        size_t const *sizes = nullptr;
        clingo_literal_t const *const *lits = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_term_base_condition(base_, index_, &sizes, &lits, &size));

        auto res = std::vector<ProgramLiteralVector>{};
        res.reserve(size);
        for (size_t i = 0; i < size; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            res.emplace_back(lits[i], lits[i] + sizes[i]);
        }
        return res;
    }

    //! Get the hash of the term.
    //!
    //! The hash enables usage in hash tables. There is also an associtaed
    //! specialization of `std::hash` for this class.
    //!
    //! @return the term's hash
    [[nodiscard]] auto hash() const noexcept -> size_t { return Detail::hash_value(index_); }

    //! Compare two terms for equality.
    //!
    //! @param a the first term
    //! @param b the second term
    //! @return whether the two terms are equal
    friend auto operator==(Term const &a, Term const &b) noexcept -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }

    //! Compare two terms.
    //!
    //! @param a the first term
    //! @param b the second term
    //! @return the result of the comparison
    friend auto operator<=>(Term const &a, Term const &b) noexcept -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_term_base_t const *base_;
    size_t index_;
};

//! A term base that maps symbols to terms.
class TermBase {
  public:
    //! The key type.
    using key_type = Symbol;
    //! The mapped type.
    using mapped_type = Term;
    //! The value type.
    using value_type = std::pair<key_type, mapped_type>;
    //! The size type.
    using size_type = std::size_t;
    //! The difference type.
    using difference_type = std::ptrdiff_t;
    //! The reference type.
    using reference = value_type;
    //! The pointer type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type.
    using iterator = Detail::RandomAccessIterator<TermBase>;

    //! Construct a term base from its C representation.
    //!
    //! @param base the C term base
    explicit TermBase(clingo_term_base_t const &base) : base_{&base} {}

    //! The size of the term base.
    //!
    //! @return the size
    [[nodiscard]] auto size() const -> size_type { return Detail::call<clingo_term_base_size>(base_); }

    //! Get the symbol term pair at the given index.
    //!
    //! @param index the index of the term
    //! @return the symbol term pair
    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto term = Term{*base_, index};
        return index < size() ? value_type{term.symbol(), term} : throw std::out_of_range{"index out of range"};
    }

    //! Whether the term base contains the given symbol.
    //!
    //! @param symbol the symbol to check
    //! @return whether the term base contains the symbol
    [[nodiscard]] auto contains(key_type const &symbol) const -> bool {
        return Detail::call<clingo_term_base_find>(base_, *c_cast(&symbol)) < size();
    }

    //! Get the term for the given symbol.
    //!
    //! @param symbol the symbol to look for
    //! @param def the default value to return if the symbol is not found
    //! @return the term for the symbol, or the default value if not found
    [[nodiscard]] auto get(key_type const &symbol, std::optional<mapped_type> def = std::nullopt) const
        -> std::optional<mapped_type> {
        auto index = Detail::call<clingo_term_base_find>(base_, *c_cast(&symbol));
        return index < size() ? std::make_optional<mapped_type>(*base_, index) : def;
    }

    //! Get an iterator pointing to the first element of the term base.
    //!
    //! @return the resulting iterator
    [[nodiscard]] auto begin() const -> iterator { return {*this, 0}; }

    //! Get an iterator pointing to the end of the term base.
    //!
    //! @return the resulting iterator
    [[nodiscard]] auto end() const -> iterator { return {*this, size()}; }

  private:
    clingo_term_base_t const *base_;
};
static_assert(std::random_access_iterator<TermBase::iterator>);

//! Enumeration of theory term types.
enum class TheoryTermType {
    tuple = clingo_theory_term_type_tuple,       //!< a tuple term, e.g., `(1,2,3)`
    list = clingo_theory_term_type_list,         //!< a list term, e.g., `[1,2,3]`
    set = clingo_theory_term_type_set,           //!< a set term, e.g., `{1,2,3}`
    function = clingo_theory_term_type_function, //!< a function term, e.g., `f(1,2,3)`
    number = clingo_theory_term_type_number,     //!< a number term, e.g., `42`
    symbol = clingo_theory_term_type_symbol      //!< a symbol term, e.g., `c`
};

class TheoryTerm;
//! A vector of theory terms.
using TheoryTermVector = std::vector<TheoryTerm>;

//! Class to provide access to theory terms.
class TheoryTerm {
  public:
    //! Construct a theory term from its C representation.
    //!
    //! @param base the C theory base
    //! @param index the index of the term
    explicit TheoryTerm(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    //! Get the type of the theory term.
    [[nodiscard]] auto type() const -> TheoryTermType {
        return static_cast<TheoryTermType>(Detail::call<clingo_theory_base_term_type>(base_, index_));
    }

    //! Get the numeric value of the term if it is a number term.
    //!
    //! @return the value
    [[nodiscard]] auto number() const -> int { return Detail::call<clingo_theory_base_term_number>(base_, index_); }

    //! Get the name of the term if it is a constant or function term.
    //!
    //! @return the value
    [[nodiscard]] auto name() const -> std::string_view {
        auto [name, size] = Detail::call<clingo_theory_base_term_name>(base_, index_);
        return {name, size};
    }

    //! Get the arguments of the term if it is a function term.
    //!
    //! @return the arguments as a vector of theory terms
    [[nodiscard]] auto arguments() const -> std::vector<TheoryTerm> {
        size_t size = 0;
        clingo_id_t const *args = nullptr;
        Detail::handle_error(clingo_theory_base_term_arguments(base_, index_, &args, &size));
        return Detail::transform(std::span{args, size}, [this](clingo_id_t id) { return TheoryTerm{*base_, id}; });
    }

    //! Convert the term to a string representation.
    //!
    //! @return the string representation of the term
    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_theory_base_term_to_string(base_, index_, c_cast(bld)));
        return std::string{bld.str()};
    }

    //! Get the hash of the term.
    //!
    //! The hash enables usage in hash tables. There is also an associtaed
    //! specialization of `std::hash` for this class.
    //!
    //! @return the term's hash
    [[nodiscard]] auto hash() const noexcept -> size_t { return Detail::hash_value(index_); }

    //! Compare two theory terms for equality.
    //!
    //! @param a the first term
    //! @param b the second term
    //! @return whether the two terms are equal
    friend auto operator==(TheoryTerm const &a, TheoryTerm const &b) noexcept -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }

    //! Compare two theory terms.
    //!
    //! @param a the first term
    //! @param b the second term
    //! @return the result of the comparison
    friend auto operator<=>(TheoryTerm const &a, TheoryTerm const &b) noexcept -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};

class TheoryElement;
//! A vector of theory elements.
using TheoryElementVector = std::vector<TheoryElement>;

//! Class to provide access to theory elements.
class TheoryElement {
  public:
    //! Constructor from C representation.
    //!
    //! @param base the C theory base
    //! @param index the index of the element
    explicit TheoryElement(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    //! Get the tuple of the theory element.
    //!
    //! @return the tuple as a vector of theory terms
    [[nodiscard]] auto tuple() const -> TheoryTermVector {
        size_t size = 0;
        clingo_id_t const *tuple = nullptr;
        Detail::handle_error(clingo_theory_base_element_tuple(base_, index_, &tuple, &size));
        return Detail::transform(std::span{tuple, size}, [this](clingo_id_t id) { return TheoryTerm{*base_, id}; });
    }

    //! Get the condition of the theory element.
    //!
    //! @return the condition as a span of program literals
    [[nodiscard]] auto condition() const -> ProgramLiteralSpan {
        size_t size = 0;
        clingo_literal_t const *cond = nullptr;
        Detail::handle_error(clingo_theory_base_element_condition(base_, index_, &cond, &size));
        return std::span{cond, size};
    }

    //! Get the condition id of the theory element.
    //!
    //! Condition ids are program literals that can be used in the solver. They
    //! are equivalenet to the condition of the element.
    //!
    //! @return the condition id as a program literal
    [[nodiscard]] auto condition_id() const -> ProgramLiteral {
        return Detail::call<clingo_theory_base_element_condition_id>(base_, index_);
    }

    //! Convert the theory element to a string representation.
    //!
    //! @return the string representation of the theory element
    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder();
        Detail::handle_error(clingo_theory_base_element_to_string(base_, index_, c_cast(bld)));
        return std::string{bld.str()};
    }

    //! Get the hash of the theory element.
    //!
    //! The hash enables usage in hash tables. There is also an associtaed
    //! specialization of `std::hash` for this class.
    //!
    //! @return the theory element's hash
    [[nodiscard]] auto hash() const noexcept -> size_t { return Detail::hash_value(index_); }

    //! Compare two theory elements for equality.
    //!
    //! @param a the first element
    //! @param b the second element
    //! @return whether the two elements are equal
    friend auto operator==(TheoryElement const &a, TheoryElement const &b) noexcept -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }

    //! Compare two theory elements.
    //!
    //! @param a the first element
    //! @param b the second element
    //! @return the result of the comparison
    friend auto operator<=>(TheoryElement const &a, TheoryElement const &b) noexcept -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};

//! Class to provide access to theory atoms.
class TheoryAtom {
  public:
    //! Constructor from C representation.
    //!
    //! @param base the C theory base
    //! @param index the index of the atom
    explicit TheoryAtom(clingo_theory_base_t const &base, size_t index) : base_{&base}, index_{index} {}

    //! Get the name of the theory atom.
    //!
    //! @return the name of the theory atom as a theory term
    [[nodiscard]] auto name() const -> TheoryTerm {
        return TheoryTerm{*base_, Detail::call<clingo_theory_base_atom_term>(base_, index_)};
    }

    //! Get the elements of the theory atom.
    //!
    //! @return the elements of the theory atom as a vector of `TheoryElement`
    [[nodiscard]] auto elements() const -> TheoryElementVector {
        size_t size = 0;
        clingo_id_t const *elems = nullptr;
        Detail::handle_error(clingo_theory_base_atom_elements(base_, index_, &elems, &size));
        return Detail::transform(std::span{elems, size}, [this](clingo_id_t id) { return TheoryElement{*base_, id}; });
    }

    //! Get the literal of the theory atom.
    //!
    //! @return the literal of the theory atom as a program literal
    [[nodiscard]] auto literal() const -> ProgramLiteral {
        return Detail::call<clingo_theory_base_atom_literal>(base_, index_);
    }

    //! Get the guard of the theory atom.
    //!
    //! @return the optional guard as a pair of a string view and a theory term
    [[nodiscard]] auto guard() const -> std::optional<std::pair<std::string_view, TheoryTerm>> {
        auto has_guard = Detail::call<clingo_theory_base_atom_has_guard>(base_, index_);
        if (has_guard) {
            clingo_string_t op;
            clingo_id_t term = 0;
            Detail::handle_error(clingo_theory_base_atom_guard(base_, index_, &op, &term));
            return std::pair{std::string_view{op.data, op.size}, TheoryTerm{*base_, term}};
        }
        return std::nullopt;
    }

    //! Convert the theory atom to a string representation.
    //!
    //! @return the string representation of the theory atom
    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_theory_base_atom_to_string(base_, index_, c_cast(bld)));
        return std::string{bld.str()};
    }

    //! Get the hash of the theory atom.
    //!
    //! The hash enables usage in hash tables. There is also an associtaed
    //! specialization of `std::hash` for this class.
    //!
    //! @return the theory atom's hash
    [[nodiscard]] auto hash() const noexcept -> size_t { return Detail::hash_value(index_); }

    //! Compare two theory atoms for equality.
    //!
    //! @param a the first atom
    //! @param b the second atom
    //! @return whether the two atoms are equal
    friend auto operator==(TheoryAtom const &a, TheoryAtom const &b) noexcept -> bool {
        assert(a.base_ == b.base_);
        return a.index_ == b.index_;
    }

    //! Compare two theory atoms.
    //!
    //! @param a the first atom
    //! @param b the second atom
    //! @return the result of the comparison
    friend auto operator<=>(TheoryAtom const &a, TheoryAtom const &b) noexcept -> std::strong_ordering {
        assert(a.base_ == b.base_);
        return a.index_ <=> b.index_;
    }

  private:
    clingo_theory_base_t const *base_;
    size_t index_;
};

//! A theory base that maps theory atoms.
class TheoryBase {
  public:
    //! The value type.
    using value_type = TheoryAtom;
    //! The size type.
    using size_type = std::size_t;
    //! The difference type.
    using difference_type = std::ptrdiff_t;
    //! The reference type.
    using reference = value_type;
    //! The pointer type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type.
    using iterator = Detail::RandomAccessIterator<TheoryBase>;

    //! Construct a theory base from its C representation.
    //!
    //! @param base the C theory base
    explicit TheoryBase(clingo_theory_base_t const &base) : base_{&base} {}

    //! Get the size of the theory base.
    //!
    //! @return the size
    [[nodiscard]] auto size() const -> size_type { return Detail::call<clingo_theory_base_size>(base_); }

    //! Get the theory atom at the given index.
    //!
    //! @return the theory atom at the index
    [[nodiscard]] auto at(size_t index) const -> value_type {
        return index < size() ? TheoryAtom{*base_, index} : throw std::out_of_range{"atom index out of range"};
    }

    //! Get an iterator pointing to the first element of the theory base.
    //!
    //! @return an iterator to the first element
    [[nodiscard]] auto begin() const -> iterator { return {*this, 0}; }

    //! Get an iterator pointing to the end of the theory base.
    //!
    //! @return an iterator to the end of the theory base
    [[nodiscard]] auto end() const -> iterator { return {*this, size()}; }

  private:
    clingo_theory_base_t const *base_;
};
static_assert(std::random_access_iterator<TheoryBase::iterator>);

//! A base that maps signatures to atom bases, and captures term and theory bases.
class Base {
  public:
    //! The key type.
    using key_type = std::tuple<std::string_view, size_t, bool>;
    //! The mapped type.
    using mapped_type = AtomBase;
    //! The value type.
    using value_type = std::pair<key_type, mapped_type>;
    //! The size type.
    using size_type = std::size_t;
    //! The difference type.
    using difference_type = std::ptrdiff_t;
    //! The reference type.
    using reference = value_type;
    //! The pointer type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type.
    using iterator = Detail::RandomAccessIterator<Base>;

    //! Construct a base from its C representation.
    //!
    //! @param base the C base
    explicit Base(clingo_base_t const *base) : base_{base} {}

    //! Check whether the given program literal is external.
    //!
    //! @param lit the program literal to check
    //! @return whether the literal is external
    [[nodiscard]] auto is_external(ProgramLiteral lit) const -> bool {
        return Detail::call<clingo_base_is_external>(base_, lit);
    }

    //! Check whether the given program literal is a fact.
    //!
    //! @param lit the program literal to check
    //! @return whether the literal is a fact
    [[nodiscard]] auto is_fact(ProgramLiteral lit) const -> bool {
        return Detail::call<clingo_base_is_fact>(base_, lit);
    }

    //! Check whether the given program literal is shown.
    //!
    //! @param lit the program literal to check
    //! @return whether the literal is shown
    [[nodiscard]] auto is_shown(ProgramLiteral lit) const -> bool {
        return Detail::call<clingo_base_is_shown>(base_, lit);
    }

    //! Check whether the (atom of the) given program literal is projected.
    //!
    //! @param lit the program literal to check
    //! @return whether the literal is projected
    [[nodiscard]] auto is_projected(ProgramLiteral lit) const -> bool {
        return Detail::call<clingo_base_is_fact>(base_, lit);
    }

    //! Check whether the program literals belongs to the current solving step.
    //!
    //! @param lit the program literal to check
    //! @return whether the literal belongs to the current step
    [[nodiscard]] auto is_current(ProgramLiteral lit) const -> bool {
        return Detail::call<clingo_base_is_current>(base_, lit);
    }

    //! Get the number of atom bases in the base.
    //!
    //! @return the number of atom bases
    [[nodiscard]] auto size() const -> size_type { return Detail::call<clingo_base_atoms_size>(base_); }

    //! Get the signature atom base pair at the given index.
    //!
    //! @param index the index of the atom base
    //! @return the signature atom base pair at the index
    [[nodiscard]] auto at(size_t index) const -> value_type {
        if (index < size()) {
            auto sig = clingo_signature_t{};
            clingo_atom_base_t const *atoms = nullptr;
            Detail::handle_error(clingo_base_atoms_at(base_, index, &sig, &atoms));
            return std::pair{std::tuple{sig.name, sig.arity, sig.is_positive}, AtomBase{atoms}};
        }
        throw std::out_of_range{"index out of range"};
    }

    //! Check whether the base contains the given signature.
    //!
    //! @param sig the signature to check
    //! @return whether the base contains the signature
    [[nodiscard]] auto contains(key_type const &sig) const -> bool {
        auto csig =
            clingo_signature_t{std::get<0>(sig).data(), std::get<0>(sig).size(), std::get<1>(sig), std::get<2>(sig)};
        return Detail::call<clingo_base_atoms_find>(base_, &csig, nullptr);
    }

    //! Check whether the base contains the given short signature.
    //!
    //! The short signature is a pair of a string view and an arity that is
    //! considered positive.
    //!
    //! @param sig the short signature to check
    //! @return whether the base contains the signature
    [[nodiscard]] auto contains(std::pair<std::string_view, size_t> const &sig) const -> bool {
        return contains({std::get<0>(sig), std::get<1>(sig), true});
    }

    //! Check whether the base contains the given symbol.
    //!
    //! @param sym the symbol to check
    //! @return whether the base contains the symbol
    [[nodiscard]] auto contains(Symbol const &sym) const -> bool {
        if (auto sig = sym.signature(); sig) {
            if (auto base = get(*sig)) {
                return base->contains(sym);
            }
        }
        return false;
    }

    //! Get the atom base for the given signature.
    //!
    //! @param sig the signature to look for
    //! @param def the default value to return if the signature is not found
    //! @return the atom base for the signature, or the default value if not found
    [[nodiscard]] auto get(key_type const &sig, std::optional<mapped_type> def = std::nullopt) const
        -> std::optional<mapped_type> {
        auto csig =
            clingo_signature_t{std::get<0>(sig).data(), std::get<0>(sig).size(), std::get<1>(sig), std::get<2>(sig)};
        clingo_atom_base_t const *atoms = nullptr;
        auto found = false;
        Detail::handle_error(clingo_base_atoms_find(base_, &csig, &atoms, &found));
        return found ? std::make_optional<AtomBase>(atoms) : def;
    }

    //! Get the atom base for the given short signature.
    //!
    //! @param sig the short signature to look for
    //! @param def the default value to return if the signature is not found
    //! @return the atom base for the signature, or the default value if not found
    [[nodiscard]] auto get(std::pair<std::string_view, size_t> const &sig,
                           std::optional<mapped_type> def = std::nullopt) const -> std::optional<mapped_type> {
        return get({std::get<0>(sig), std::get<1>(sig), true}, def);
    }

    //! Get the atom base for the given symbol.
    //!
    //! @param sym the symbol to look for
    //! @param def the default value to return if the symbol is not found
    //! @return the atom base for the symbol, or the default value if not found
    [[nodiscard]] auto get(Symbol const &sym, std::optional<Atom> def = std::nullopt) const -> std::optional<Atom> {
        if (auto sig = sym.signature(); sig) {
            if (auto base = get(*sig)) {
                return base->get(sym, def);
            }
        }
        return def;
    }

    //! Get the term base of the program.
    //!
    //! @return the term base of the program
    [[nodiscard]] auto terms() const -> TermBase { return TermBase{*Detail::call<clingo_base_terms>(base_)}; }

    //! Get the theory base of the program.
    //!
    //! @return the theory base of the program
    [[nodiscard]] auto theory() const -> TheoryBase { return TheoryBase{*Detail::call<clingo_base_theory>(base_)}; }

    //! Get an iterator pointing to the first element of the base.
    //!
    //! @return an iterator to the first element
    [[nodiscard]] auto begin() const -> iterator { return {*this, 0}; }

    //! Get an iterator pointing to the end of the base.
    //!
    //! @return an iterator to the end of the base
    [[nodiscard]] auto end() const -> iterator { return {*this, size()}; }

  private:
    clingo_base_t const *base_;
};
static_assert(std::random_access_iterator<Base::iterator>);

//! @}

} // namespace Clingo

namespace std {

template <> struct hash<Clingo::Atom> {
    auto operator()(Clingo::Atom const &x) const noexcept -> size_t { return x.hash(); }
};

template <> struct hash<Clingo::Term> {
    auto operator()(Clingo::Term const &x) const noexcept -> size_t { return x.hash(); }
};

template <> struct hash<Clingo::TheoryTerm> {
    auto operator()(Clingo::TheoryTerm const &x) const noexcept -> size_t { return x.hash(); }
};

template <> struct hash<Clingo::TheoryElement> {
    auto operator()(Clingo::TheoryElement const &x) const noexcept -> size_t { return x.hash(); }
};

template <> struct hash<Clingo::TheoryAtom> {
    auto operator()(Clingo::TheoryAtom const &x) const noexcept -> size_t { return x.hash(); }
};

} // namespace std
