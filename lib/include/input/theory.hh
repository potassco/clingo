#pragma once

#include <input/literal.hh>
#include <input/term.hh>

namespace Gringo::Input {

//! @defgroup input_theory Theory Terms and Atoms
//! @ingroup input_language
//!
//! Data structures and functions to represent theory terms and atoms.
//!
//! @{

struct TheoryTermSymbol;
struct TheoryTermVariable;
struct TheoryTermTuple;
struct TheoryTermFunction;
struct TheoryTermUnparsed;

//! A variant for the different theory terms.
using TheoryTerm =
    std::variant<TheoryTermSymbol, TheoryTermVariable, TheoryTermTuple, TheoryTermFunction, TheoryTermUnparsed>;
//! A vector of theory terms.
using TheoryTermVec = std::vector<TheoryTerm>;

//! A symbolic theory term.
//!
//! For example: <tt>1</tt>.
struct TheoryTermSymbol {
    //! Construct a symbolic theory term.
    explicit TheoryTermSymbol(Location loc, Symbol value) : loc{std::move(loc)}, value{std::move(value)} {}

    //! The location of the symbol.
    Location loc;
    //! The symbol.
    Symbol value;
};

//! A variable theory term.
//!
//! For example: <tt>X</tt>.
struct TheoryTermVariable {
    //! Construct a variable theory term.
    explicit TheoryTermVariable(Location loc, std::string name, bool is_anonymous = false)
        : loc{std::move(loc)}, name{std::move(name)}, is_anonymous{is_anonymous} {}

    //! The location of the variable.
    Location loc;
    //! The name of the variable.
    std::string name;
    //! Whether the variable is anonymous.
    bool is_anonymous;
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
struct TheoryTermTuple {
    //! Construct a tuple theory term.
    explicit TheoryTermTuple(Location loc, TheoryTermTupleType type, TheoryTermVec elems);

    //! The location of the symbol.
    Location loc;
    //! The type of the term.
    TheoryTermTupleType type;
    //! The elements of the tuple.
    TheoryTermVec elems;
};

//! A tuple (set or list) theory term.
//!
//! For example: <tt>{f(X,y), Z}</tt>.
struct TheoryTermFunction {
    //! Construct a function theory term.
    explicit TheoryTermFunction(Location loc, std::string name);
    //! Construct a function theory term.
    explicit TheoryTermFunction(Location loc, std::string name, TheoryTermVec args);

    //! The location of the symbol.
    Location loc;
    //! The name of the function.
    std::string name;
    //! The arguments of the function.
    TheoryTermVec args;
};

//! An unparsed theory term.
//!
//! The priorities and associativities of the operators have not yet been applied.
//! They are simply stored as a list.
//!
//! For example: <tt>- X ++ Y << Z</tt>.
struct TheoryTermUnparsed {
    //! A vector of operators.
    using OpVec = std::vector<std::string>;
    //! An element having the form of a right guard.
    using Element = std::pair<OpVec, TheoryTerm>;
    //! A vector of elements.
    //!
    //! In this context, it has to have at least length one.
    //! Furthermore, all but the first element must have at least one operator.
    using ElementVec = std::vector<Element>;

    //! Construct an unparsed theory term.
    explicit TheoryTermUnparsed(Location loc, ElementVec elems);

    //! The location of the symbol.
    Location loc;
    //! The vector of elements.
    ElementVec elems;
};

inline TheoryTermTuple::TheoryTermTuple(Location loc, TheoryTermTupleType type, TheoryTermVec elems)
    : loc{std::move(loc)}, type(type), elems{std::move(elems)} {}
inline TheoryTermFunction::TheoryTermFunction(Location loc, std::string name)
    : TheoryTermFunction{std::move(loc), std::move(name), {}} {}
inline TheoryTermFunction::TheoryTermFunction(Location loc, std::string name, TheoryTermVec args)
    : loc{std::move(loc)}, name(std::move(name)), args{std::move(args)} {}
inline TheoryTermUnparsed::TheoryTermUnparsed(Location loc, ElementVec elems)
    : loc{std::move(loc)}, elems{std::move(elems)} {}

//! A theory atom.
//!
//! For example: <tt>&sum { X+Y: p(X), q(Y) } >= 0</tt>.
struct TheoryAtom {
    //! The optional right guard of the theory atom.
    using RGuard = std::optional<std::pair<std::string, TheoryTerm>>;
    //! An element of the theory atom.
    using Element = std::pair<TheoryTermVec, LiteralVec>;
    //! A vector of theory atom elements.
    using ElementVec = std::vector<Element>;

    //! Construct a theory atom.
    explicit TheoryAtom(Location loc, Term name, ElementVec elems, RGuard rhs)
        : loc{std::move(loc)}, name{std::move(name)}, elems{std::move(elems)}, rhs{std::move(rhs)} {}

    //! The location of the symbol.
    Location loc;
    //! The name of the atom.
    Term name;
    //! The elements of the atom.
    ElementVec elems;
    //! The optional right guard of the atom.
    RGuard rhs;
};

//! @}

} // namespace Gringo::Input
