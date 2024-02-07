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
using TheoryTermVec = Util::immutable_array<TheoryTerm>;

//! A symbolic theory term.
//!
//! For example: <tt>1</tt>.
class TheoryTermSymbol {
  public:
    //! Construct a symbolic theory term.
    explicit TheoryTermSymbol(Location loc, Symbol value) : loc_{std::move(loc)}, value_{std::move(value)} {}

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The symbol.
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

  private:
    Location loc_;

  public:
    //! The name of the variable.
    String name_;
    //! Whether the variable is anonymous.
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
    explicit TheoryTermTuple(Location loc, TheoryTermTupleType type, TheoryTermVec elems);

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The type of the term.
    TheoryTermTupleType type_;
    //! The elements of the tuple.
    TheoryTermVec elems_;
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
    explicit TheoryTermFunction(Location loc, String name, TheoryTermVec args);

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The name of the function.
    String name_;
    //! The arguments of the function.
    TheoryTermVec args_;
};

//! Compare two theory terms.
//!
//! @related TheoryTermFunction
auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool;

//! Compare two theory terms.
//!
//! @related TheoryTermFunction
auto operator<(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool;

//! An unparsed theory term.
//!
//! The priorities and associativities of the operators have not yet been applied.
//! They are simply stored as a list.
//!
//! For example: <tt>- X ++ Y << Z</tt>.
class TheoryTermUnparsed {
  public:
    //! A vector of operators.
    using OpVec = Util::immutable_array<String>;
    //! An element having the form of a right guard.
    using Element = std::pair<OpVec, TheoryTerm>;
    //! A vector of elements.
    //!
    //! In this context, it has to have at least length one.
    //! Furthermore, all but the first element must have at least one operator.
    using ElementVec = Util::immutable_array<Element>;

    //! Construct an unparsed theory term.
    explicit TheoryTermUnparsed(Location loc, ElementVec elems);

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The vector of elements.
    ElementVec elems_;
};

//! Compare two theory terms.
//!
//! @related TheoryTermUnparsed
auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool;

//! Compare two theory terms.
//!
//! @related TheoryTermUnparsed
auto operator<(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool;

inline TheoryTermTuple::TheoryTermTuple(Location loc, TheoryTermTupleType type, TheoryTermVec elems)
    : loc_{std::move(loc)}, type_(type), elems_{std::move(elems)} {}
inline TheoryTermFunction::TheoryTermFunction(Location loc, String name)
    : TheoryTermFunction{std::move(loc), name, {}} {}
inline TheoryTermFunction::TheoryTermFunction(Location loc, String name, TheoryTermVec args)
    : loc_{std::move(loc)}, name_(name), args_{std::move(args)} {}
inline TheoryTermUnparsed::TheoryTermUnparsed(Location loc, ElementVec elems)
    : loc_{std::move(loc)}, elems_{std::move(elems)} {}

//! The optional right guard of the theory atom.
using TheoryRGuard = std::optional<std::pair<String, TheoryTerm>>;

//! An element of the theory atom.
class TheoryElement {
  public:
    TheoryElement(Location loc, TheoryTermVec tuple, LiteralVec cond)
        : loc_{loc}, tuple_{std::move(tuple)}, cond_{std::move(cond)} {}
    //! The location of the theory element.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The tuple of the theory element.
    TheoryTermVec tuple_;
    //! The condition of the theory element.
    LiteralVec cond_;
};
//! A vector of theory atom elements.
using TheoryElementVec = Util::immutable_array<TheoryElement>;

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
    explicit TheoryAtom(Location loc, Term name, TheoryElementVec elems, TheoryRGuard rhs)
        : loc_{std::move(loc)}, name_{std::move(name)}, elems_{std::move(elems)}, rhs_{std::move(rhs)} {
        static_assert(!HasSign);
    }

    //! Construct a theory atom.
    explicit TheoryAtom(Location loc, Sign sign, Term name, TheoryElementVec elems, TheoryRGuard rhs)
        : Signed{sign}, loc_{std::move(loc)}, name_{std::move(name)}, elems_{std::move(elems)}, rhs_{std::move(rhs)} {
        static_assert(HasSign);
    }

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The name of the atom.
    Term name_;
    //! The elements of the atom.
    TheoryElementVec elems_;
    //! The optional right guard of the atom.
    TheoryRGuard rhs_;
};

//! A head theory atom.
using HeadTheoryAtom = TheoryAtom<false>;

//! A body theory atom.
using BodyTheoryAtom = TheoryAtom<true>;

//! Compare two theory atoms.
//!
//! @related TheoryAtom
template <bool HasSign> auto operator==(TheoryAtom<HasSign> const &a, TheoryAtom<HasSign> const &b) -> bool;

//! Compare two theory atoms.
//!
//! @related TheoryAtom
template <bool HasSign> auto operator<(TheoryAtom<HasSign> const &a, TheoryAtom<HasSign> const &b) -> bool;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::TheoryTermVariable);
GRINGO_HASH_PROTO(Gringo::Input::TheoryTermSymbol);
GRINGO_HASH_PROTO(Gringo::Input::TheoryTermFunction);
GRINGO_HASH_PROTO(Gringo::Input::TheoryTermTuple);
GRINGO_HASH_PROTO(Gringo::Input::TheoryTermUnparsed);
GRINGO_HASH_PROTO(Gringo::Input::TheoryElement);
GRINGO_HASH_PROTO(Gringo::Input::HeadTheoryAtom);
GRINGO_HASH_PROTO(Gringo::Input::BodyTheoryAtom);

#endif
