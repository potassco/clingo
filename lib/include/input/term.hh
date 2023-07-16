#pragma once

//! @file
//! This file contains the term interface and derived terms.

#include <optional>
#include <unordered_set>
#include <variant>
#include <vector>

#include <util/algorithm.hh>
#include <util/hash.hh>
#include <util/shared_ptr.hh>

#include <symbol.hh>

namespace Gringo::Input {

//! Variable selection modes for select_variables().
enum VariableSelectMode {
    add, //!< Add variables to the set.
    del, //!< Remove variables from the set.
};

//! Variable selection scopes.
//!
//! @see Statement::visit_variables()
enum class VariableContext {
    global, //!< Visit variables occurring in global scope.
    all,    //!< Visit all variable occurrences.
};

//! A set of variable names.
using VariableSet = std::unordered_set<std::string>;
//! A vector of variable names.
using VariableVec = std::vector<std::string>;
//! A function to visit variable occurrences.
using VarVisitFun = std::function<void(std::string const &var)>;

//! Add/remove variables to/from a set occuring in the given expression.
template <class E> void select_variables(E &expr, VariableSet &vars, VariableSelectMode mode) {
    if (mode == VariableSelectMode::add) {
        expr.visit_variables([&vars](std::string const &var) { vars.emplace(var); });
    } else {
        expr.visit_variables([&vars](std::string const &var) { vars.erase(var); });
    }
}

//! Convenience method for @ref select_variables(E, VariableSet &, VariableSelectMode) returning a set.
template <class E> auto select_variables(E &expr, VariableSelectMode mode) -> VariableSet {
    VariableSet vars;
    select_variables(expr, vars, mode);
    return vars;
}

//! Enumeration to select variables to project.
//!
//! @see Projection
enum class ProjectionMode {
    disabled = 0,  //!< Disable projection.
    anonymous = 1, //!< Only project anonymous variables.
    pure = 2,      //!< Project pure variables.
};

//! Helper to gather projection related arguments.
class Projection {
  public:
    //! Constructor taking the mode which variables to project and a map with counts of variables.
    explicit Projection(ProjectionMode mode, std::unordered_map<std::string, size_t> const &counts)
        : counts_{counts}, mode_{mode} {};
    //! Return whether a the given variable should be projected.
    //!
    //! Only variables with a count of exactly one can be projected while the mode adds further restrictions.
    [[nodiscard]] auto projectable(std::string const &var, bool anonymous) const -> bool;
    //! Return the variable counts.
    [[nodiscard]] auto counts() const -> std::unordered_map<std::string, size_t> const &;
    //! Return the mode.
    [[nodiscard]] auto mode() const -> ProjectionMode;

  private:
    //! The variable counts.
    std::unordered_map<std::string, size_t> const &counts_;
    //! The projection mode.
    ProjectionMode mode_;
};

// TODO: until here

struct TermVariable;
struct TermSymbol;
struct TermTuple;
struct TermFunction;
struct TermAbs;
struct TermUnary;
struct TermBinary;
struct RecTerm;
// TODO: I think that the code would be nicer moving the shared pointers into
// the struct. The small size overhead should not matter because all structs
// are relatively small.
//! Variant holding the different term types.
using Term = std::variant<TermVariable, TermSymbol, TermTuple, TermFunction, TermAbs, TermUnary, TermBinary>;

//! A vector of terms.
using TermVec = std::vector<Term>;
//! A vector of vecors of terms.
using TermVecVec = std::vector<TermVec>;

//! A variant capturing either a term or a position that is to be projected.
using TupleElem = std::variant<std::monostate, Term>;
//! A tuple of terms or positions to project.
using TupleVec = std::vector<TupleElem>;
//! A vector of tuples used as function or predicate arguments.
using PoolVec = std::vector<TupleVec>;

//! Term representing a variable.
//!
//! For example <tt>X</tt>.
struct TermVariable {
    //! Construct a variable.
    explicit TermVariable(std::string name, bool is_anonymous = false)
        : name{std::move(name)}, is_anonymous{is_anonymous} {}

    //! The name of the variable.
    std::string name;
    //! Whether the variable is anonymous.
    bool is_anonymous;
};

//! Compare two variables.
auto operator==(TermVariable const &a, TermVariable const &b) -> bool;

//! Term representing a symbol.
//!
//! For example <tt>1</tt>.
struct TermSymbol {
    //! Construct term with the given symbol.
    explicit TermSymbol(Symbol value) : value{std::move(value)} {}

    //! The associated symbol.
    Symbol value;
};

//! Compare two symbols.
auto operator==(TermSymbol const &a, TermSymbol const &b) -> bool;

//! Term representing a tuple.
//!
//! For example <tt>(a,b;c)</tt>.
struct TermTuple {
    using Element = std::variant<TupleVec, Term>;
    using ElementVec = std::vector<Element>;

    //! Construct a  tuple.
    explicit TermTuple(ElementVec args);

    //! The argument pool of the tuple.
    ElementVec pool;
};

//! Compare two tuple terms.
auto operator==(TermTuple const &a, TermTuple const &b) -> bool;

//! Term representing a symbolic or external function.
//!
//! For example <tt>f(a,b;c)</tt>.
struct TermFunction {
    //! Construct a symbolic function.
    //!
    //! The function takes a pool of term tuples, which will be reduced to a single element after calling
    //! Term::unpool().
    explicit TermFunction(std::string name, PoolVec args, bool external);

    //! The name of the function.
    std::string name;
    //! The argument pool of the function.
    PoolVec pool;
    //! Whether this is an external function.
    bool external;
};

//! Compare two function terms.
auto operator==(TermFunction const &a, TermFunction const &b) -> bool;

//! Term representing the absolute function.
//!
//! For example <tt>|-X|</tt>.
struct TermAbs {
    //! Construct an absolute term.
    //!
    //! The term has a pool of arguments, which will be reduced to a single element after calling Term::unpool().
    explicit TermAbs(TermVec pool);

    //! The argument pool of the absolute term.
    TermVec pool;
};

//! Compare two absolute terms.
auto operator==(TermAbs const &a, TermAbs const &b) -> bool;

//! Enumeration of available unary operators.
enum class UnaryOperator : int {
    negate, //!< The unary minus sign (-).
    invert, //!< The unary negation sign (~).
};

//! Term representing an unary operation.
//!
//! For example <tt>-X</tt>.
struct TermUnary {
    //! Contruct a term for an unary operation.
    explicit TermUnary(UnaryOperator op, Term rhs);
    //! Contruct a term for an unary operation.
    explicit TermUnary(UnaryOperator op, Util::shared_ptr<Term> rhs);

    //! The operation.
    UnaryOperator op;
    //! The right-hand-side.
    Util::shared_ptr<Term> rhs;
};

//! Compare two unary terms.
auto operator==(TermUnary const &a, TermUnary const &b) -> bool;

//! Enumaration of available binary operators.
enum class BinaryOperator : int {
    dots,  //!< The interval operator.
    xor_,  //!< The XOR bit operation.
    or_,   //!< The OR bit operation.
    and_,  //!< The AND bit operation.
    plus,  //!< The plus arithmetic operation.
    minus, //!< The minus arithmetic operation.
    times, //!< The multiply arithmetic operation.
    div,   //!< The (integer) divide arithmetic operation.
    mod,   //!< The modulo arithmetic operation.
    pow,   //!< The exponentiation arithmetic operation.
};

//! Term representing a binary operation.
//!
//! For example <tt>X-Y</tt>.
struct TermBinary {
    //! Contruct a term for an binary operation.
    explicit TermBinary(Term lhs, BinaryOperator op, Term rhs);
    //! Contruct a term for an binary operation.
    explicit TermBinary(Util::shared_ptr<Term> lhs, BinaryOperator op, Util::shared_ptr<Term> rhs);

    //! The operation.
    BinaryOperator op;
    //! The left-hand-side.
    Util::shared_ptr<Term> lhs;
    //! The right-hand-side.
    Util::shared_ptr<Term> rhs;
};

//! Compare two binary terms.
auto operator==(TermBinary const &a, TermBinary const &b) -> bool;

inline TermAbs::TermAbs(TermVec pool) : pool{std::move(pool)} {}
inline TermUnary::TermUnary(UnaryOperator op, Term rhs) : op{op}, rhs{Util::construct_shared<Term>(std::move(rhs))} {}
inline TermUnary::TermUnary(UnaryOperator op, Util::shared_ptr<Term> rhs) : op{op}, rhs{std::move(rhs)} {}
inline TermBinary::TermBinary(Term lhs, BinaryOperator op, Term rhs)
    : op{op}, lhs{Util::construct_shared<Term>(std::move(lhs))}, rhs{Util::construct_shared<Term>(std::move(rhs))} {}
inline TermBinary::TermBinary(Util::shared_ptr<Term> lhs, BinaryOperator op, Util::shared_ptr<Term> rhs)
    : op{op}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}
inline TermFunction::TermFunction(std::string name, PoolVec args, bool external)
    : name(std::move(name)), pool{std::move(args)}, external{external} {}
inline TermTuple::TermTuple(ElementVec args) : pool{std::move(args)} {}

} // namespace Gringo::Input

HASH_PROTO(Gringo::Input::TermVariable);
HASH_PROTO(Gringo::Input::TermSymbol);
HASH_PROTO(Gringo::Input::TermFunction);
HASH_PROTO(Gringo::Input::TermTuple);
HASH_PROTO(Gringo::Input::TermAbs);
HASH_PROTO(Gringo::Input::TermUnary);
HASH_PROTO(Gringo::Input::TermBinary);
