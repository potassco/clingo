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

  private:
    friend auto operator==(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> bool;
    friend auto operator<(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> bool;
    friend struct Util::value_hasher<TheoryTermSymbol>;

    Location loc_;
    Symbol value_;
};

//! Compare two theory terms.
//!
//! @related TheoryTermSymbol
auto operator==(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> bool;

//! Compare two theory terms.
//!
//! @related TheoryTermSymbol
auto operator<(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> bool;

//! A variable theory term.
//!
//! For example: <tt>X</tt>.
class TheoryTermVariable {
  public:
    //! Construct a variable theory term.
    explicit TheoryTermVariable(Location loc, String name, bool is_anonymous = false)
        : loc_{std::move(loc)}, name_{name}, is_anonymous_{is_anonymous} {}

    //! The location of the variable.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the variable.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! Whether the variable is anonymous.
    [[nodiscard]] auto is_anonymous() const -> bool { return is_anonymous_; }

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_name, a_anonymous}, Types{args...});
        return TheoryTermVariable{select<Opt>(a_loc, loc_, args...), select<Opt>(a_name, name_, args...),
                                  select<Opt>(a_anonymous, is_anonymous_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool;
    friend auto operator<(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool;
    friend struct Util::value_hasher<TheoryTermVariable>;

    Location loc_;
    String name_;
    bool is_anonymous_;
};

//! Compare two theory terms.
//!
//! @related TheoryTermVariable
auto operator==(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool;

//! Compare two theory terms.
//!
//! @related TheoryTermVariable
auto operator<(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool;

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
    [[nodiscard]] auto elems() const -> TheoryTermArray const &;

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

  private:
    friend auto operator==(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool;
    friend auto operator<(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool;
    friend struct Util::value_hasher<TheoryTermTuple>;

    Location loc_;
    TheoryTermTupleType type_;
    TheoryTermArray elems_;
};

//! Compare two theory terms.
//!
//! @related TheoryTermTuple
auto operator==(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool;

//! Compare two theory terms.
//!
//! @related TheoryTermTuple
auto operator<(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool;

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
    [[nodiscard]] auto args() const -> TheoryTermArray const &;

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

  private:
    friend auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool;
    friend auto operator<(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool;
    friend struct Util::value_hasher<TheoryTermFunction>;

    Location loc_;
    String name_;
    TheoryTermArray args_;
};

//! Compare two theory terms.
//!
//! @related TheoryTermFunction
auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool;

//! Compare two theory terms.
//!
//! @related TheoryTermFunction
auto operator<(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool;

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
    [[nodiscard]] auto elems() const -> UnparsedElementArray const &;

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

  private:
    friend auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool;
    friend auto operator<(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool;
    friend struct Util::value_hasher<TheoryTermUnparsed>;

    Location loc_;
    UnparsedElementArray elems_;
};

//! Compare two theory terms.
//!
//! @related TheoryTermUnparsed
auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool;

//! Compare two theory terms.
//!
//! @related TheoryTermUnparsed
auto operator<(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool;

inline TheoryTermTuple::TheoryTermTuple(Location loc, TheoryTermTupleType type, TheoryTermArray elems)
    : loc_{std::move(loc)}, type_(type), elems_{std::move(elems)} {}
inline TheoryTermFunction::TheoryTermFunction(Location loc, String name)
    : TheoryTermFunction{std::move(loc), name, {}} {}
inline TheoryTermFunction::TheoryTermFunction(Location loc, String name, TheoryTermArray args)
    : loc_{std::move(loc)}, name_(name), args_{std::move(args)} {}
inline TheoryTermUnparsed::TheoryTermUnparsed(Location loc, UnparsedElementArray elems)
    : loc_{std::move(loc)}, elems_{std::move(elems)} {}

inline auto TheoryTermTuple::elems() const -> TheoryTermArray const & { return elems_; }
inline auto TheoryTermFunction::args() const -> TheoryTermArray const & { return args_; }
inline auto TheoryTermUnparsed::elems() const -> UnparsedElementArray const & { return elems_; }

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

  private:
    friend auto operator==(TheoryElement const &a, TheoryElement const &b) -> bool;
    friend auto operator<(TheoryElement const &a, TheoryElement const &b) -> bool;
    friend struct Util::value_hasher<TheoryElement>;

    Location loc_;
    TheoryTermArray tuple_;
    LitArray cond_;
};
//! A vector of theory atom elements.
using TheoryElementArray = Util::immutable_array<TheoryElement>;

//! Compare two theory elements.
//!
//! @related TheoryElement
auto operator==(TheoryElement const &a, TheoryElement const &b) -> bool;

//! Compare two theory elements.
//!
//! @related TheoryElement
auto operator<(TheoryElement const &a, TheoryElement const &b) -> bool;

//! A theory atom.
//!
//! For example: <tt>&sum { X+Y: p(X), q(Y) } >= 0</tt>.
template <bool HasSign> class TheoryAtom : public std::conditional_t<HasSign, Signed, Unsigned> {
  public:
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

  private:
    friend auto operator==(TheoryAtom<HasSign> const &a, TheoryAtom<HasSign> const &b) -> bool;
    friend auto operator<(TheoryAtom<HasSign> const &a, TheoryAtom<HasSign> const &b) -> bool;
    friend struct Util::value_hasher<TheoryAtom<HasSign>>;

    Location loc_;
    Term name_;
    TheoryElementArray elems_;
    TheoryRGuard rhs_;
};

//! A head theory atom.
using HdLitTheoryAtom = TheoryAtom<false>;

//! A body theory atom.
using BdLitTheoryAtom = TheoryAtom<true>;

//! Compare two theory atoms.
//!
//! @related TheoryAtom
auto operator==(TheoryAtom<true> const &a, TheoryAtom<true> const &b) -> bool;

//! Compare two theory atoms.
//!
//! @related TheoryAtom
auto operator==(TheoryAtom<false> const &a, TheoryAtom<false> const &b) -> bool;

//! Compare two theory atoms.
//!
//! @related TheoryAtom
auto operator<(TheoryAtom<true> const &a, TheoryAtom<true> const &b) -> bool;

//! Compare two theory atoms.
//!
//! @related TheoryAtom
auto operator<(TheoryAtom<false> const &a, TheoryAtom<false> const &b) -> bool;

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
