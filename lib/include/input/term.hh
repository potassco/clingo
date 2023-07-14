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

/*
enum class TermType : int {
    TermSymbol,
    TermTuple,
    TermVariable,
    TermAbs,
    TermFunction,
    TermUnary,
    TermBinary,
};

enum class Attribute : int {
    Value,
    Name,
    Pool,
    Arguments,
    Left,
    Right,
    Operator,
};

auto operator<<(std::ostream &out, TermType type) -> std::ostream &;
auto operator<<(std::ostream &out, Attribute attr) -> std::ostream &;
*/

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

//! Generator for auxiliary variables.
class NameGen {
  public:
    //! Constructor taking a set of variables names.
    //!
    //! The generator ensures that there are no collisions with these names.
    NameGen(VariableSet vars) : vars_{std::move(vars)} {}
    //! Generate a unique variable name.
    [[nodiscard]] auto new_name() -> std::string;

  private:
    //! Taken variable names.
    VariableSet vars_;
    //! Running number used to generate variable names.
    size_t num_ = 0;
};

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

//! The term interface.
class Term {
  public:
    //! Virtual destructor.
    virtual ~Term() = default;

    //! Convert the term to string.
    [[nodiscard]] auto to_string() const -> std::string;
    //! Check if the term has the given type optionally adding context information.
    [[nodiscard]] virtual auto check_type(TermCheckType type, CheckTypeResult *res = nullptr) const -> bool;
    //! Remove all pooled arguments from the term.
    [[nodiscard]] virtual auto unpool() const -> std::optional<STermVec> = 0;
    //! Equality compare two terms.
    [[nodiscard]] virtual auto is_equal(Term const &other) const -> bool = 0;
    //! Equality compare two terms.
    [[nodiscard]] friend auto operator==(Term const &a, Term const &b) { return a.is_equal(b); }
    //! Compute a hash for the term.
    [[nodiscard]] virtual auto hash() const -> size_t = 0;
    //! Visit variables with the given function.
    virtual void visit_variables(VarVisitFun const &fun) const = 0;
    //! Project variables according to given projection mode.
    [[nodiscard]] virtual auto project(Projection project) const -> std::optional<STerm> = 0;
    //! Unconditionally project anonymous variables.
    //!
    //! This is a deprecated feature to support old programs.
    //! The projection star should be used instead.
    [[nodiscard]] virtual auto project_anonymous() const -> std::optional<STerm> = 0;
    // TODO: remove
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<STerm>;

    //! Visit terms with the given visitor.
    virtual void accept(TermVisitor const &visitor) const = 0;

    // AST interface
    /*
    [[nodiscard]] virtual auto type() const -> TermType = 0;
    [[nodiscard]] virtual auto get_int(Attribute attr) -> int &;
    [[nodiscard]] virtual auto get_ast(Attribute attr) -> STerm &;
    [[nodiscard]] virtual auto get_ast_vec(Attribute attr) -> STermVec &;
    [[nodiscard]] virtual auto get_ast_vec_vec(Attribute attr) -> STermVecVec &;
    template <class T> auto get(Attribute attr) -> T & {
        // Note that getters and setters for SASTs will work fine. However,
        // getters and setters for vectors won't work without some special treatment because vectors of shared pointers
        // cannot be upcasted. One alternative could be to use ASTs or at least types storing ASTs:
        //   ASTRef<Term>:
        //     // the ref can make sure that we always have a term
        //     // and fail if we do not have a term
        //     Term *operator-> ()
        //     Term &operator* ()
        //     SAST value;
        //   ASTVec<Term>
        //     // the vec can make sure that all SASTs in values are terms.
        //     emplace_back(ASTRef<Term>)
        //     emplace_back(SAST)
        //     SASTVec values;
        //   Advantages:
        //     no unnecessary allocations
        //     we can pass around references
        //     less type safe
        // Otherwise, it is also possible to implement a view on an AST holding a vector:
        //   ASTVec:
        //     vector methods
        //     get/set/length for attribute
        //     shared pointer to AST
        //   Advantages:
        //     view on the actual type safe datastructure
        //   Disadvantages:
        //     additional allocations for construction
        //     indirection for get/set of vectors
        // Maybe, it would also be a good idea to represent at least the base classes in the AST:
        //   Classes:
        //     ASTTerm
        //     ASTLiteral
        //     ASTHeadLiteral
        //     ASTBodyLiteral
        //     ASTStatement
        //   Advantages:
        //     asts and vectors of asts can be passed directly
        //     vectors of the respective types can be construrted right away!
        //     the huge enums in the clingo API will become more specific
        //     by providing enough meta info, the python interface can still be generated
        //   Disadvantages:
        //     a bit more boiler plate
        //   Implementation:
        //     each base class provides an interface similar to what we have for term now
        //     the current ast and term will be merged
        //   I think, I'll go for this variant!
        //
        if constexpr (std::is_same_v<T, STerm>) {
            return get_ast(attr);
        } else if constexpr (std::is_same_v<T, STermVec>) {
            return get_ast_vec(attr);
        } else if constexpr (std::is_same_v<T, STermVecVec>) {
            return get_ast_vec_vec(attr);
        } else if constexpr (std::is_same_v<T, STerm>) {
            return get_ast(attr);
        } else if constexpr (std::is_same_v<T, int>) {
            return get_int(attr);
        } else {
            static_assert(sizeof(T *) == 0, "unsupported type in AST::get");
        }
    };
    */
  private:
    //! Increment reference count of the term.
    friend void inc_ref_count(Term &term) { ++term.refs_; }
    //! Decrement reference count of the term.
    friend void dec_ref_count(Term &term) { ++term.refs_; }
    //! Get reference count of the term.
    [[nodiscard]] friend auto get_ref_count(Term const &term) -> size_t { return term.refs_; }

    //! The reference count of the term.
    size_t refs_ = 0;
};

//! Term representing a symbol.
//!
//! For example <tt>1</tt>.
class TermSymbol : public Term {
  public:
    //! Construct term with the given symbol.
    explicit TermSymbol(Symbol value) : value_{std::move(value)} {}

    [[nodiscard]] auto symbol() const -> Symbol const & { return value_; }

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;
    [[nodiscard]] auto unpool() const -> std::optional<STermVec> override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<STerm> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<STerm> override;

    void accept(TermVisitor const &visitor) const override;
    // AST interface
    /*
    [[nodiscard]] auto type() const -> TermType override;
    [[nodiscard]] auto get_int(Attribute attr) -> int & override;
    */

  private:
    Symbol value_;
};

//! Term representing a tuple.
//!
//! For example <tt>(a,b;c)</tt>.
class TermTuple : public Term {
  public:
    //! A tuple element is either a term tuple or an individual term.
    using Element = std::variant<TupleVec, STerm>;
    //! A pool of elements.
    //!
    //! The pool will be reduced to a single element after calling Term::unpool().
    using ElementVec = std::vector<Element>;

    //! Construct a  tuple.
    explicit TermTuple(ElementVec args) : pool_{std::move(args)} {}
    //! Get the argument pool of the term tuple.
    [[nodiscard]] auto pool() const -> ElementVec const & { return pool_; }

    [[nodiscard]] auto unpool() const -> std::optional<STermVec> override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<STerm> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<STerm> override;

    void accept(TermVisitor const &visitor) const override;
    // AST interface
    /*
    [[nodiscard]] auto type() const -> TermType override;
    */

  private:
    ElementVec pool_;
};

//! Term representing a variable.
//!
//! For example <tt>X</tt>.
class TermVariable : public Term {
  public:
    //! Construct a variable.
    //!
    //! Anonymous variables should set parameter is_anonymous to true.
    //! Such variables receive a unique name after calling rewrite_anonymous().
    explicit TermVariable(std::string name, bool is_anonymous = false)
        : name_{std::move(name)}, is_anonymous_{is_anonymous} {}

    //! Get the name of the variable.
    [[nodiscard]] auto name() const -> std::string const &;
    //! Check if the variable is an anonymous variable.
    [[nodiscard]] auto is_anonymous() const -> bool;

    [[nodiscard]] auto unpool() const -> std::optional<STermVec> override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<STerm> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<STerm> override;

    void accept(TermVisitor const &visitor) const override;
    /*
    [[nodiscard]] auto type() const -> TermType override;
    */

  private:
    std::string name_;
    bool is_anonymous_;
};

//! Term representing the absolute function.
//!
//! For example <tt>|-X|</tt>.
class TermAbs : public Term {
  public:
    //! Construct an absolute term.
    //!
    //! The term has a pool of arguments, which will be reduced to a single element after calling Term::unpool().
    explicit TermAbs(STermVec pool) : pool_{std::move(pool)} {}

    //! Return the argument pool of the absolute term.
    [[nodiscard]] auto pool() const -> STermVec const & { return pool_; }

    [[nodiscard]] auto unpool() const -> std::optional<STermVec> override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<STerm> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<STerm> override;

    void accept(TermVisitor const &visitor) const override;
    // AST interface
    /*
    [[nodiscard]] auto type() const -> TermType override;
    */

  private:
    STermVec pool_;
};

//! Term representing a symbolic or external function.
//!
//! For example <tt>f(a,b;c)</tt>.
class TermFunction : public Term {
  public:
    //! Construct a symbolic function.
    //!
    //! The function takes a pool of term tuples, which will be reduced to a single element after calling
    //! Term::unpool().
    explicit TermFunction(std::string name, PoolVec args, bool external)
        : name_(std::move(name)), pool_{std::move(args)}, external_{external} {}

    //! Check whether the function is external.
    [[nodiscard]] auto is_external() const -> bool { return external_; }
    //! Get the name of the function.
    [[nodiscard]] auto name() const -> std::string const & { return name_; }
    //! Get the argument pool of the function.
    [[nodiscard]] auto pool() const -> PoolVec const & { return pool_; }

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;
    [[nodiscard]] auto unpool() const -> std::optional<STermVec> override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<STerm> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<STerm> override;

    void accept(TermVisitor const &visitor) const override;
    // AST interface
    /*
    [[nodiscard]] auto type() const -> TermType override;
    */

  private:
    std::string name_;
    PoolVec pool_;
    bool external_;
};

//! Enumeration of available unary operators.
enum class UnaryOperator : int {
    negate, //!< The unary minus sign (-).
    invert, //!< The unary negation sign (~).
};

//! Term representing an unary operation.
//!
//! For example <tt>-X</tt>.
class TermUnary : public Term {
  public:
    //! Contruct a term for an unary operation.
    explicit TermUnary(UnaryOperator op, STerm e) : op_{op}, rhs_{std::move(e)} {}

    //! Get the operator of the unary term.
    [[nodiscard]] auto unary_operator() const -> UnaryOperator { return op_; }
    //! Get the right-hand-side of the unary term.
    [[nodiscard]] auto rhs() const -> STerm const & { return rhs_; }

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;
    [[nodiscard]] auto unpool() const -> std::optional<STermVec> override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<STerm> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<STerm> override;

    void accept(TermVisitor const &visitor) const override;
    // AST interface
    /*
    [[nodiscard]] auto type() const -> TermType override;
    [[nodiscard]] auto get_int(Attribute attr) -> int & override;
    [[nodiscard]] auto get_ast(Attribute attr) -> STerm & override;
    */

  private:
    UnaryOperator op_;
    STerm rhs_;
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
class TermBinary : public Term {
  public:
    //! Contruct a term for an binary operation.
    explicit TermBinary(STerm lhs, BinaryOperator op, STerm rhs)
        : op_{op}, lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {}

    //! Get the operator of the unary term.
    [[nodiscard]] auto binary_operator() const -> BinaryOperator { return op_; }
    //! Get the left-hand-side of the unary term.
    [[nodiscard]] auto lhs() const -> STerm const & { return lhs_; }
    //! Get the right-hand-side of the unary term.
    [[nodiscard]] auto rhs() const -> STerm const & { return rhs_; }

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;
    [[nodiscard]] auto unpool() const -> std::optional<STermVec> override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<STerm> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<STerm> override;

    void accept(TermVisitor const &visitor) const override;
    // AST interface
    /*
    [[nodiscard]] auto type() const -> TermType override;
    [[nodiscard]] auto get_int(Attribute attr) -> int & override;
    [[nodiscard]] auto get_ast(Attribute attr) -> STerm & override;
    */

  private:
    BinaryOperator op_;
    STerm lhs_;
    STerm rhs_;
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
