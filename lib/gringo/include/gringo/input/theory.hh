#pragma once

#include <gringo/input/literal.hh>
#include <gringo/input/term.hh>

namespace Gringo::Input {

//! @defgroup input_theory Theory Terms and Atoms
//! Data structures and functions to represent theory terms and atoms.
//!
//! @ingroup input_language
//!
//! @{

class TheoryTermSymbol;
class TheoryTermVariable;
class TheoryTermTuple;
class TheoryTermFunction;
class TheoryTermUnparsed;

//! A variant for the different theory terms.
using TheoryTerm =
    std::variant<TheoryTermSymbol, TheoryTermVariable, TheoryTermTuple, TheoryTermFunction, TheoryTermUnparsed>;
//! A vector of theory terms.
using TheoryTermArray = Util::immutable_array<TheoryTerm>;

//! A symbolic theory term.
//!
//! For example: <tt>1</tt>.
class TheoryTermSymbol : public Gringo::Util::Record::Base<TheoryTermSymbol> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermSymbol::loc_, a_value = &TheoryTermSymbol::value_};
    }

    //! Construct a symbolic theory term.
    explicit TheoryTermSymbol(Location loc, Symbol value) : loc_{std::move(loc)}, value_{std::move(value)} {}

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The symbol.
    [[nodiscard]] auto value() const -> Symbol { return value_; }

    //! Compare two theory terms.
    friend auto operator==(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> bool {
        return a.value_ == b.value_;
    }
    //! Compare two theory terms.
    friend auto operator<=>(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> std::strong_ordering {
        return a.value_ <=> b.value_;
    }
    //! Compute hash value.
    friend auto value_hash(TheoryTermSymbol const &x) -> size_t {
        return Gringo::Util::value_hash_record<TheoryTermSymbol>(x.value_);
    }

  private:
    Location loc_;
    Symbol value_;
};

//! A variable theory term.
//!
//! For example: <tt>X</tt>.
class TheoryTermVariable : public Gringo::Util::Record::Base<TheoryTermVariable> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermVariable::loc_, a_name = &TheoryTermVariable::name_,
                          a_anonymous = &TheoryTermVariable::anonymous_};
    }

    //! Construct a variable theory term.
    explicit TheoryTermVariable(Location loc, String name, bool is_anonymous = false)
        : loc_{std::move(loc)}, name_{name}, anonymous_{is_anonymous} {}

    //! The location of the variable.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the variable.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! Whether the variable is anonymous.
    [[nodiscard]] auto anonymous() const -> bool { return anonymous_; }

    //! Compare two theory terms.
    friend auto operator==(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool {
        return a.name_ == b.name_;
    }
    //! Compare two theory terms.
    friend auto operator<=>(TheoryTermVariable const &a, TheoryTermVariable const &b) -> std::strong_ordering {
        return a.name_ <=> b.name_;
    }
    //! Compute hash value.
    friend auto value_hash(TheoryTermVariable const &x) -> size_t {
        return Gringo::Util::value_hash_record<TheoryTermVariable>(x.name_);
    }

  private:
    Location loc_;
    String name_;
    bool anonymous_;
};

//! Enumeration of theory term tuple types.
//!
//! @related TheoryTermTuple
enum class TheoryTermTupleType {
    tuple, //!< A tuple of terms enclosed in parentheses.
    set,   //!< A set of terms enclosed in braces.
    list   //!< A list of terms enclosed in brackets.
};

//! A tuple (set or list) theory term.
//!
//! For example: <tt>f(X,y)</tt>.
class TheoryTermTuple : public Gringo::Util::Record::Base<TheoryTermTuple> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermTuple::loc_, a_type = &TheoryTermTuple::type_,
                          a_elems = &TheoryTermTuple::elems_};
    }

    //! Construct a tuple theory term.
    explicit TheoryTermTuple(Location loc, TheoryTermTupleType type, TheoryTermArray elems);

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the term.
    [[nodiscard]] auto type() const -> TheoryTermTupleType { return type_; }
    //! The elements of the tuple.
    [[nodiscard]] auto elems() const -> TheoryTermArray const & { return elems_; }

    //! Compare two theory terms.
    friend auto operator==(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool;
    //! Compare two theory terms.
    friend auto operator<=>(TheoryTermTuple const &a, TheoryTermTuple const &b) -> std::strong_ordering;
    //! Compute hash value.
    friend auto value_hash(TheoryTermTuple const &x) -> size_t;

  private:
    Location loc_;
    TheoryTermTupleType type_;
    TheoryTermArray elems_;
};

//! A tuple (set or list) theory term.
//!
//! For example: <tt>{f(X,y), Z}</tt>.
class TheoryTermFunction : public Gringo::Util::Record::Base<TheoryTermFunction> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermFunction::loc_, a_name = &TheoryTermFunction::name_,
                          a_args = &TheoryTermFunction::args_};
    }

    //! Construct a function theory term.
    explicit TheoryTermFunction(Location loc, String name);
    //! Construct a function theory term.
    explicit TheoryTermFunction(Location loc, String name, TheoryTermArray args);

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the function.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The arguments of the function.
    [[nodiscard]] auto args() const -> TheoryTermArray const & { return args_; }

    //! Compare two theory terms.
    friend auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool;
    //! Compare two theory terms.
    friend auto operator<=>(TheoryTermFunction const &a, TheoryTermFunction const &b) -> std::strong_ordering;
    //! Compute hash value.
    friend auto value_hash(TheoryTermFunction const &x) -> size_t;

  private:
    Location loc_;
    String name_;
    TheoryTermArray args_;
};

//! An element having the form of a right guard.
using UnparsedElement = std::pair<StringArray, TheoryTerm>;
//! A vector of elements.
//!
//! In this context, it has to have at least length one.
//! Furthermore, all but the first element must have at least one operator.
using UnparsedElementArray = Util::immutable_array<UnparsedElement>;

//! An unparsed theory term.
//!
//! The priorities and associativities of the operators have not yet been applied.
//! They are simply stored as a list.
//!
//! For example: <tt>- X ++ Y << Z</tt>.
class TheoryTermUnparsed : public Gringo::Util::Record::Base<TheoryTermUnparsed> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermUnparsed::loc_, a_elems = &TheoryTermUnparsed::elems_};
    }

    //! Construct an unparsed theory term.
    explicit TheoryTermUnparsed(Location loc, UnparsedElementArray elems);

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The vector of elements.
    [[nodiscard]] auto elems() const -> UnparsedElementArray const & { return elems_; }

    //! Compare two theory terms.
    friend auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool;
    //! Compare two theory terms.
    friend auto operator<=>(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> std::strong_ordering;
    //! Compute hash value.
    friend auto value_hash(TheoryTermUnparsed const &x) -> size_t;

  private:
    Location loc_;
    UnparsedElementArray elems_;
};

//! The optional right guard of the theory atom.
using TheoryRGuard = std::optional<std::pair<String, TheoryTerm>>;

//! An element of the theory atom.
class TheoryElement : public Gringo::Util::Record::Base<TheoryElement> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryElement::loc_, a_tuple = &TheoryElement::tuple_,
                          a_cond = &TheoryElement::cond_};
    }

    //! Construct a theory element.
    explicit TheoryElement(Location loc, TheoryTermArray tuple, LitArray cond)
        : loc_{loc}, tuple_{std::move(tuple)}, cond_{std::move(cond)} {}

    //! The location of the theory element.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The tuple of the theory element.
    [[nodiscard]] auto tuple() const -> TheoryTermArray const & { return tuple_; }
    //! The condition of the theory element.
    [[nodiscard]] auto cond() const -> LitArray const & { return cond_; }

    //! Compare two theory elements.
    friend auto operator==(TheoryElement const &a, TheoryElement const &b) -> bool {
        return std::tie(a.tuple_, a.cond_) == std::tie(b.tuple_, b.cond_);
    }
    //! Compare two theory elements.
    friend auto operator<=>(TheoryElement const &a, TheoryElement const &b) -> std::strong_ordering {
        return std::tie(a.tuple_, a.cond_) <=> std::tie(b.tuple_, b.cond_);
    }
    //! Compute hash value.
    friend auto value_hash(TheoryElement const &x) -> size_t {
        return Gringo::Util::value_hash_record<TheoryElement>(x.tuple_, x.cond_);
    }

  private:
    Location loc_;
    TheoryTermArray tuple_;
    LitArray cond_;
};
//! A vector of theory atom elements.
using TheoryElementArray = Util::immutable_array<TheoryElement>;

//! A theory atom.
//!
//! For example: <tt>&sum { X+Y: p(X), q(Y) } >= 0</tt>.
template <bool HasSign>
class TheoryAtom : public std::conditional_t<HasSign, Signed, Unsigned>,
                   public Gringo::Util::Record::Base<TheoryAtom<HasSign>> {
  public:
    using Base = std::conditional_t<HasSign, Signed, Unsigned>;

    //! The record attributes.
    static constexpr auto attributes() {
        if constexpr (HasSign) {
            return std::tuple{a_loc = &TheoryAtom::loc_, a_sign = &Signed::sign, a_name = &TheoryAtom::name_,
                              a_elems = &TheoryAtom::elems_, a_rhs = &TheoryAtom::rhs_};
        } else {
            return std::tuple{a_loc = &TheoryAtom::loc_, a_name = &TheoryAtom::name_, a_elems = &TheoryAtom::elems_,
                              a_rhs = &TheoryAtom::rhs_};
        }
    }

    //! Construct a theory atom.
    explicit TheoryAtom(Location loc, Term name, TheoryElementArray elems, TheoryRGuard rhs)
        : loc_{std::move(loc)}, name_{std::move(name)}, elems_{std::move(elems)}, rhs_{std::move(rhs)} {
        static_assert(!HasSign);
    }

    //! Construct a theory atom.
    explicit TheoryAtom(Location loc, Sign sign, Term name, TheoryElementArray elems, TheoryRGuard rhs)
        : Signed{sign}, loc_{std::move(loc)}, name_{std::move(name)}, elems_{std::move(elems)}, rhs_{std::move(rhs)} {
        static_assert(HasSign);
    }

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the atom.
    [[nodiscard]] auto name() const -> Term const & { return name_; }
    //! The elements of the atom.
    [[nodiscard]] auto elems() const -> TheoryElementArray const & { return elems_; }
    //! The optional right guard of the atom.
    [[nodiscard]] auto rhs() const -> TheoryRGuard const & { return rhs_; }

    //! Compare two theory atoms.
    friend auto operator==(TheoryAtom const &a, TheoryAtom const &b) -> bool {
        return std::tie(static_cast<Base const &>(a), a.name_, a.elems_, a.rhs_) ==
               std::tie(static_cast<Base const &>(b), b.name_, b.elems_, b.rhs_);
    }
    //! Compare two theory atoms.
    friend auto operator<=>(TheoryAtom const &a, TheoryAtom const &b) -> std::strong_ordering {
        // Note: std::optional does not produce a strong_ordering - bug???
        return Util::make_strong_ordering(std::tie(static_cast<Base const &>(a), a.name_, a.elems_, a.rhs_) <=>
                                          std::tie(static_cast<Base const &>(b), b.name_, b.elems_, b.rhs_));
    }
    //! Compute hash value.
    friend auto value_hash(TheoryAtom const &x) -> size_t {
        using Gringo::Util::value_hash;
        return Gringo::Util::value_hash_record<TheoryAtom>(static_cast<Base const &>(x), x.name_, x.elems_, x.rhs_);
    }

  private:
    Location loc_;
    Term name_;
    TheoryElementArray elems_;
    TheoryRGuard rhs_;
};

//! A head theory atom.
using HdLitTheoryAtom = TheoryAtom<false>;

//! A body theory atom.
using BdLitTheoryAtom = TheoryAtom<true>;

//! @}

// TheoryTermTuple

inline TheoryTermTuple::TheoryTermTuple(Location loc, TheoryTermTupleType type, TheoryTermArray elems)
    : loc_{std::move(loc)}, type_(type), elems_{std::move(elems)} {}

inline auto operator==(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool {
    return std::tie(a.type_, a.elems_) == std::tie(b.type_, b.elems_);
}

inline auto operator<=>(TheoryTermTuple const &a, TheoryTermTuple const &b) -> std::strong_ordering {
    return std::tie(a.type_, a.elems_) <=> std::tie(b.type_, b.elems_);
}

inline auto value_hash(TheoryTermTuple const &x) -> size_t {
    return Gringo::Util::value_hash_record<TheoryTermTuple>(x.type_, x.elems_);
}

// TheoryTermFunction

inline TheoryTermFunction::TheoryTermFunction(Location loc, String name, TheoryTermArray args)
    : loc_{std::move(loc)}, name_(name), args_{std::move(args)} {}

inline TheoryTermFunction::TheoryTermFunction(Location loc, String name)
    : TheoryTermFunction{std::move(loc), name, {}} {}

inline auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool {
    return std::tie(a.name_, a.args_) == std::tie(b.name_, b.args_);
}

inline auto operator<=>(TheoryTermFunction const &a, TheoryTermFunction const &b) -> std::strong_ordering {
    return std::tie(a.name_, a.args_) <=> std::tie(b.name_, b.args_);
}

inline auto value_hash(TheoryTermFunction const &x) -> size_t {
    return Gringo::Util::value_hash_record<TheoryTermFunction>(x.name_, x.args_);
}

// TheoryTermUnparsed

inline TheoryTermUnparsed::TheoryTermUnparsed(Location loc, UnparsedElementArray elems)
    : loc_{std::move(loc)}, elems_{std::move(elems)} {}

inline auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool {
    return a.elems_ == b.elems_;
}

inline auto operator<=>(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> std::strong_ordering {
    return a.elems_ <=> b.elems_;
}

inline auto value_hash(TheoryTermUnparsed const &x) -> size_t {
    return Gringo::Util::value_hash_record<TheoryTermUnparsed>(x.elems_);
}

} // namespace Gringo::Input
