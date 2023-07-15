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

//! Enumeration for Term::check_type().
enum class TermCheckType : int {
    atom,              //!< Check if term is an atom.
    sig,               //!< Check if term is a signature.
    identifier,        //!< Check if term is an identifier.
    signed_identifier, //!< Check if term is a signed identifier.
    pos_number         //!< Check if term is a positive number.
};

//! Extract additional information while checking the type of a term.
//!
//! @see Term::check_type()
struct CheckTypeResult {
    //! Wheather the term is signed.
    bool has_sign = false;
    //! The number represented by the term.
    int pos_number = 0;
    //! The identifier represented by the term.
    std::string identifier;
};

struct TermVariable;
struct TermSymbol;
struct TermTuple;
struct TermFunction;
struct TermAbs;
struct TermUnary;
struct TermBinary;
using TermV2 = std::variant<TermVariable, TermSymbol, Util::shared_ptr<TermTuple>, Util::shared_ptr<TermFunction>,
                            Util::shared_ptr<TermAbs>, Util::shared_ptr<TermUnary>, Util::shared_ptr<TermBinary>>;

//! A vector of terms.
using TermVec = std::vector<TermV2>;
//! A vector of vecors of terms.
using TermVecVec = std::vector<TermVec>;

//! A variant capturing either a term or a position that is to be projected.
using TupleElemV2 = std::variant<std::monostate, TermV2>;
//! A tuple of terms or positions to project.
using TupleVecV2 = std::vector<TupleElemV2>;
//! A vector of tuples used as function or predicate arguments.
using PoolVecV2 = std::vector<TupleVecV2>;

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

//! Term representing a symbol.
//!
//! For example <tt>1</tt>.
struct TermSymbol {
    //! Construct term with the given symbol.
    explicit TermSymbol(Symbol value) : value{std::move(value)} {}

    //! The associated symbol.
    Symbol value;
};

//! Term representing a tuple.
//!
//! For example <tt>(a,b;c)</tt>.
struct TermTuple : Util::enable_shared {
    using Element = std::variant<TupleVecV2, TermV2>;
    using ElementVec = std::vector<Element>;

    //! Construct a  tuple.
    explicit TermTuple(ElementVec args) : pool{std::move(args)} {}

    //! The argument pool of the tuple.
    ElementVec pool;
};

//! Term representing a symbolic or external function.
//!
//! For example <tt>f(a,b;c)</tt>.
struct TermFunction : Util::enable_shared {
    //! Construct a symbolic function.
    //!
    //! The function takes a pool of term tuples, which will be reduced to a single element after calling
    //! Term::unpool().
    explicit TermFunction(std::string name, PoolVecV2 args, bool external)
        : name(std::move(name)), pool{std::move(args)}, external{external} {}

    //! The name of the function.
    std::string name;
    //! The argument pool of the function.
    PoolVecV2 pool;
    //! Whether this is an external function.
    bool external;
};

//! Term representing the absolute function.
//!
//! For example <tt>|-X|</tt>.
struct TermAbs : Util::enable_shared {
    //! Construct an absolute term.
    //!
    //! The term has a pool of arguments, which will be reduced to a single element after calling Term::unpool().
    explicit TermAbs(TermVec pool) : pool{std::move(pool)} {}

    //! The argument pool of the absolute term.
    TermVec pool;
};

//! Enumeration of available unary operators.
enum class UnaryOperator : int {
    negate, //!< The unary minus sign (-).
    invert, //!< The unary negation sign (~).
};

//! Term representing an unary operation.
//!
//! For example <tt>-X</tt>.
struct TermUnary : Util::enable_shared {
    //! Contruct a term for an unary operation.
    explicit TermUnary(UnaryOperator op, TermV2 rhs) : op{op}, rhs{std::move(rhs)} {}

    //! The operation.
    UnaryOperator op;
    //! The right-hand-side.
    TermV2 rhs;
};

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
struct TermBinary : Util::enable_shared {
    //! Contruct a term for an binary operation.
    explicit TermBinary(TermV2 lhs, BinaryOperator op, TermV2 rhs) : op{op}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}

    //! The operation.
    BinaryOperator op;
    //! The left-hand-side.
    TermV2 lhs;
    //! The right-hand-side.
    TermV2 rhs;
};

class Term;
//! A shared pointer to a term.
using STerm = Util::shared_ptr<Term>;
//! A vector of shared term pointers.
using STermVec = std::vector<STerm>;
//! A vector of vecors of shared term pointers.
using STermVecVec = std::vector<STermVec>;

//! A variant capturing either a term or a position that is to be projected.
using TupleElem = std::variant<std::monostate, STerm>;
//! A tuple of terms or positions to project.
using TupleVec = std::vector<TupleElem>;
//! A vector of tuples used as function or predicate arguments.
using PoolVec = std::vector<TupleVec>;

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

class TermVisitor;

//! The term interface to be removed.
class Term : public Util::enable_shared {
  public:
    Term(TermV2 term) : term{std::move(term)} {}
    //! Check if the term has the given type optionally adding context information.
    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res = nullptr) const -> bool;
    //! Remove all pooled arguments from the term.
    [[nodiscard]] auto unpool() const -> std::optional<STermVec>;
    //! Equality compare two terms.
    [[nodiscard]] auto is_equal(Term const &other) const -> bool;
    //! Equality compare two terms.
    [[nodiscard]] friend auto operator==(Term const &a, Term const &b) { return a.is_equal(b); }
    //! Compute a hash for the term.
    [[nodiscard]] auto hash() const -> size_t;
    //! Visit variables with the given function.
    void visit_variables(VarVisitFun const &fun) const;
    //! Project variables according to given projection mode.
    [[nodiscard]] auto project(Projection project) const -> std::optional<STerm>;
    //! Unconditionally project anonymous variables.
    //!
    //! This is a deprecated feature to support old programs.
    //! The projection star should be used instead.
    [[nodiscard]] auto project_anonymous() const -> std::optional<STerm>;

    //! Visit terms with the given visitor.
    void accept(TermVisitor const &visitor) const;

    TermV2 term;
};

//! A visitor for available term types.
class TermVisitor {
  public:
    //! Virtual destructor.
    virtual ~TermVisitor() = default;

    //! Visit a symbolic term.
    virtual void visit(TermSymbol const &term) const = 0;
    //! Visit a variable term.
    virtual void visit(TermVariable const &term) const = 0;
    //! Visit a function term.
    virtual void visit(TermFunction const &term) const = 0;
    //! Visit a tuple term.
    virtual void visit(TermTuple const &term) const = 0;
    //! Visit an absolute term.
    virtual void visit(TermAbs const &term) const = 0;
    //! Visit an unary term.
    virtual void visit(TermUnary const &term) const = 0;
    //! Visit a binary term.
    virtual void visit(TermBinary const &term) const = 0;
};

} // namespace Gringo::Input

HASH(Gringo::Input::Term)
