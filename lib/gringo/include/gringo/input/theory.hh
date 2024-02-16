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
class TheoryTermSymbol {
  public:
    //! Construct a symbolic theory term.
    explicit TheoryTermSymbol(Location loc, Symbol value) : loc_{std::move(loc)}, value_{std::move(value)} {}

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The symbol.
    [[nodiscard]] auto value() const -> Symbol { return value_; }

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_value}, Types{args...});
        return TheoryTermSymbol{select<Opt>(a_loc, loc_, args...), select<Opt>(a_value, value_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two theory terms.
    friend auto operator==(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> bool {
        return a.value_ == b.value_;
    }
    //! Compare two theory terms.
    friend auto operator<=>(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> std::strong_ordering {
        return a.value_ <=> b.value_;
    }

  private:
    friend struct Util::value_hasher<TheoryTermSymbol>;

    Location loc_;
    Symbol value_;
};

//! A variable theory term.
//!
//! For example: <tt>X</tt>.
class TheoryTermVariable {
  public:
    //! Construct a variable theory term.
    explicit TheoryTermVariable(Location loc, String name, bool is_anonymous = false)
        : loc_{std::move(loc)}, name_{name}, anonymous_{is_anonymous} {}

    //! The location of the variable.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the variable.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! Whether the variable is anonymous.
    [[nodiscard]] auto anonymous() const -> bool { return anonymous_; }

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_name, a_anonymous}, Types{args...});
        return TheoryTermVariable{select<Opt>(a_loc, loc_, args...), select<Opt>(a_name, name_, args...),
                                  select<Opt>(a_anonymous, anonymous_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two theory terms.
    friend auto operator==(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool {
        return a.name_ == b.name_;
    }
    //! Compare two theory terms.
    friend auto operator<=>(TheoryTermVariable const &a, TheoryTermVariable const &b) -> std::strong_ordering {
        return a.name_ <=> b.name_;
    }

  private:
    friend struct Util::value_hasher<TheoryTermVariable>;

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
class TheoryTermTuple {
  public:
    //! Construct a tuple theory term.
    explicit TheoryTermTuple(Location loc, TheoryTermTupleType type, TheoryTermArray elems);

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the term.
    [[nodiscard]] auto type() const -> TheoryTermTupleType { return type_; }
    //! The elements of the tuple.
    [[nodiscard]] auto elems() const -> TheoryTermArray const & { return elems_; }

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_type, a_elems}, Types{args...});
        return TheoryTermTuple{select<Opt>(a_loc, loc_, args...), select<Opt>(a_type, type_, args...),
                               select<Opt>(a_elems, elems_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two theory terms.
    friend auto operator==(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool;
    //! Compare two theory terms.
    friend auto operator<=>(TheoryTermTuple const &a, TheoryTermTuple const &b) -> std::strong_ordering;

  private:
    friend struct Util::value_hasher<TheoryTermTuple>;

    Location loc_;
    TheoryTermTupleType type_;
    TheoryTermArray elems_;
};

//! A tuple (set or list) theory term.
//!
//! For example: <tt>{f(X,y), Z}</tt>.
class TheoryTermFunction {
  public:
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

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_name, a_args}, Types{args...});
        return TheoryTermFunction{select<Opt>(a_loc, loc_, args...), select<Opt>(a_name, name_, args...),
                                  select<Opt>(a_args, args_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two theory terms.
    friend auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool;
    //! Compare two theory terms.
    friend auto operator<=>(TheoryTermFunction const &a, TheoryTermFunction const &b) -> std::strong_ordering;

  private:
    friend struct Util::value_hasher<TheoryTermFunction>;

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
class TheoryTermUnparsed {
  public:
    //! Construct an unparsed theory term.
    explicit TheoryTermUnparsed(Location loc, UnparsedElementArray elems);

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The vector of elements.
    [[nodiscard]] auto elems() const -> UnparsedElementArray const & { return elems_; }

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_elems}, Types{args...});
        return TheoryTermUnparsed{select<Opt>(a_loc, loc_, args...), select<Opt>(a_elems, elems_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two theory terms.
    friend auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool;
    //! Compare two theory terms.
    friend auto operator<=>(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> std::strong_ordering;

  private:
    friend struct Util::value_hasher<TheoryTermUnparsed>;

    Location loc_;
    UnparsedElementArray elems_;
};

//! The optional right guard of the theory atom.
using TheoryRGuard = std::optional<std::pair<String, TheoryTerm>>;

//! An element of the theory atom.
class TheoryElement {
  public:
    //! Construct a theory element.
    explicit TheoryElement(Location loc, TheoryTermArray tuple, LitArray cond)
        : loc_{loc}, tuple_{std::move(tuple)}, cond_{std::move(cond)} {}

    //! The location of the theory element.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The tuple of the theory element.
    [[nodiscard]] auto tuple() const -> TheoryTermArray const & { return tuple_; }
    //! The condition of the theory element.
    [[nodiscard]] auto cond() const -> LitArray const & { return cond_; }

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_tuple, a_cond}, Types{args...});
        return TheoryElement{select<Opt>(a_loc, loc_, args...), select<Opt>(a_tuple, tuple_, args...),
                             select<Opt>(a_cond, cond_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two theory elements.
    friend auto operator==(TheoryElement const &a, TheoryElement const &b) -> bool {
        return std::tie(a.tuple_, a.cond_) == std::tie(b.tuple_, b.cond_);
    }
    //! Compare two theory elements.
    friend auto operator<=>(TheoryElement const &a, TheoryElement const &b) -> std::strong_ordering {
        return std::tie(a.tuple_, a.cond_) <=> std::tie(b.tuple_, b.cond_);
    }

  private:
    friend struct Util::value_hasher<TheoryElement>;

    Location loc_;
    TheoryTermArray tuple_;
    LitArray cond_;
};
//! A vector of theory atom elements.
using TheoryElementArray = Util::immutable_array<TheoryElement>;

//! A theory atom.
//!
//! For example: <tt>&sum { X+Y: p(X), q(Y) } >= 0</tt>.
template <bool HasSign> class TheoryAtom : public std::conditional_t<HasSign, Signed, Unsigned> {
  public:
    using Base = std::conditional_t<HasSign, Signed, Unsigned>;

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

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        if constexpr (HasSign) {
            check(Types{a_loc, a_sign, a_name, a_elems, a_rhs}, Types{args...});
            return TheoryAtom{select<Opt>(a_loc, loc_, args...), select<Opt>(a_sign, this->sign(), args...),
                              select<Opt>(a_name, name_, args...), select<Opt>(a_elems, elems_, args...),
                              select<Opt>(a_rhs, rhs_, args...)};
        } else {
            check(Types{a_loc, a_name, a_elems, a_rhs}, Types{args...});
            return TheoryAtom{select<Opt>(a_loc, loc_, args...), select<Opt>(a_name, name_, args...),
                              select<Opt>(a_elems, elems_, args...), select<Opt>(a_rhs, rhs_, args...)};
        }
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

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

  private:
    friend struct Util::value_hasher<TheoryAtom>;

    Location loc_;
    Term name_;
    TheoryElementArray elems_;
    TheoryRGuard rhs_;
};

//! A head theory atom.
using HdLitTheoryAtom = TheoryAtom<false>;

//! A body theory atom.
using BdLitTheoryAtom = TheoryAtom<true>;

inline TheoryTermTuple::TheoryTermTuple(Location loc, TheoryTermTupleType type, TheoryTermArray elems)
    : loc_{std::move(loc)}, type_(type), elems_{std::move(elems)} {}

inline auto operator==(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool {
    return std::tie(a.type_, a.elems_) == std::tie(b.type_, b.elems_);
}

inline auto operator<=>(TheoryTermTuple const &a, TheoryTermTuple const &b) -> std::strong_ordering {
    return std::tie(a.type_, a.elems_) <=> std::tie(b.type_, b.elems_);
}

inline TheoryTermFunction::TheoryTermFunction(Location loc, String name)
    : TheoryTermFunction{std::move(loc), name, {}} {}

inline auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool {
    return std::tie(a.name_, a.args_) == std::tie(b.name_, b.args_);
}

inline auto operator<=>(TheoryTermFunction const &a, TheoryTermFunction const &b) -> std::strong_ordering {
    return std::tie(a.name_, a.args_) <=> std::tie(b.name_, b.args_);
}

inline TheoryTermFunction::TheoryTermFunction(Location loc, String name, TheoryTermArray args)
    : loc_{std::move(loc)}, name_(name), args_{std::move(args)} {}

inline TheoryTermUnparsed::TheoryTermUnparsed(Location loc, UnparsedElementArray elems)
    : loc_{std::move(loc)}, elems_{std::move(elems)} {}

inline auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool {
    return a.elems_ == b.elems_;
}

inline auto operator<=>(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> std::strong_ordering {
    return a.elems_ <=> b.elems_;
}

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::TheoryTermVariable);
GRINGO_HASH_PROTO(Gringo::Input::TheoryTermSymbol);
GRINGO_HASH_PROTO(Gringo::Input::TheoryTermFunction);
GRINGO_HASH_PROTO(Gringo::Input::TheoryTermTuple);
GRINGO_HASH_PROTO(Gringo::Input::TheoryTermUnparsed);
GRINGO_HASH_PROTO(Gringo::Input::TheoryElement);
GRINGO_HASH_PROTO(Gringo::Input::HdLitTheoryAtom);
GRINGO_HASH_PROTO(Gringo::Input::BdLitTheoryAtom);

#endif
