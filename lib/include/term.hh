#pragma once

#include <optional>
#include <ostream>
#include <unordered_set>
#include <variant>
#include <vector>

#include <util/hash.hh>
#include <util/shared_ptr.hh>

#include <symbol.hh>

template <class T> class Pool;
template <class T, class C> class PoolParent;

enum class TermCheckType : int { atom, sig, identifier, signed_identifier, pos_number };

struct CheckTypeResult {
    bool has_sign = false;
    int pos_number = 0;
    std::string identifier;
};

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

class Term;
using STerm = shared_ptr<Term>;
using STermVec = std::vector<STerm>;
using STermVecVec = std::vector<STermVec>;
using PoolTerm = Pool<STerm>;

enum VariableSelectMode {
    add,
    del,
};

using VariableSet = std::unordered_set<std::string>;
using VariableVec = std::vector<std::string>;

class Term {
  public:
    virtual ~Term() = default;

    virtual void print(std::ostream &out) const = 0;
    friend auto operator<<(std::ostream &out, Term const &ast) -> std::ostream &;
    [[nodiscard]] auto to_string() const -> std::string;
    [[nodiscard]] virtual auto check_type(TermCheckType type, CheckTypeResult *res = nullptr) const -> bool;
    [[nodiscard]] auto unpool() -> STermVec;
    virtual void unpool(PoolTerm &pool) = 0;
    [[nodiscard]] virtual auto is_equal(Term const &other) const -> bool = 0;
    [[nodiscard]] friend auto operator==(Term const &a, Term const &b) { return a.is_equal(b); }
    [[nodiscard]] virtual auto hash() const -> size_t = 0;
    virtual void variables(VariableSet &vars, VariableSelectMode mode) const = 0;

    // AST interface
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

    size_t refs = 0;
};

class TermSymbol : public Term {
  public:
    explicit TermSymbol(Symbol value) : value_{std::move(value)} {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;
    void unpool(PoolTerm &pool) override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void variables(VariableSet &vars, VariableSelectMode mode) const override;

    // AST interface
    [[nodiscard]] auto type() const -> TermType override;
    [[nodiscard]] auto get_int(Attribute attr) -> int & override;

  private:
    Symbol value_;
};

class TermTuple : public Term {
  public:
    using Element = std::variant<STermVec, STerm>;
    using ElementVec = std::vector<Element>;

    explicit TermTuple(ElementVec args) : pool_{std::move(args)} {}

    void print(std::ostream &out) const override;
    void unpool(PoolTerm &pool) override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void variables(VariableSet &vars, VariableSelectMode mode) const override;

    // AST interface
    [[nodiscard]] auto type() const -> TermType override;

  private:
    ElementVec pool_;
};

class TermVariable : public Term {
  public:
    explicit TermVariable(std::string name) : name_{std::move(name)} {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> TermType override;
    void unpool(PoolTerm &pool) override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void variables(VariableSet &vars, VariableSelectMode mode) const override;

  private:
    std::string name_;
};

class TermAbs : public Term {
  public:
    explicit TermAbs(STermVec pool) : pool_{std::move(pool)} {}

    void print(std::ostream &out) const override;
    void unpool(PoolTerm &pool) override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void variables(VariableSet &vars, VariableSelectMode mode) const override;

    // AST interface
    [[nodiscard]] auto type() const -> TermType override;

  private:
    STermVec pool_;
};

class TermFunction : public Term {
  public:
    explicit TermFunction(std::string name, STermVecVec args, bool external)
        : name_(std::move(name)), pool_{std::move(args)}, external_{external} {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;
    void unpool(PoolTerm &pool) override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void variables(VariableSet &vars, VariableSelectMode mode) const override;

    // AST interface
    [[nodiscard]] auto type() const -> TermType override;

  private:
    std::string name_;
    STermVecVec pool_;
    bool external_;
};

enum class UnaryOperator : int {
    negate,
    invert,
};

auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream &;

class TermUnary : public Term {
  public:
    explicit TermUnary(UnaryOperator op, STerm e) : op_{op}, rhs_{std::move(e)} {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;
    void unpool(PoolTerm &pool) override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void variables(VariableSet &vars, VariableSelectMode mode) const override;

    // AST interface
    [[nodiscard]] auto type() const -> TermType override;
    [[nodiscard]] auto get_int(Attribute attr) -> int & override;
    [[nodiscard]] auto get_ast(Attribute attr) -> STerm & override;

  private:
    UnaryOperator op_;
    STerm rhs_;
};

enum class BinaryOperator : int {
    dots,
    xor_,
    or_,
    and_,
    plus,
    minus,
    times,
    div,
    mod,
    pow,
};

auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream &;

class TermBinary : public Term {
  public:
    explicit TermBinary(STerm lhs, BinaryOperator op, STerm rhs)
        : op_{op}, lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;
    void unpool(PoolTerm &pool) override;
    [[nodiscard]] auto is_equal(Term const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void variables(VariableSet &vars, VariableSelectMode mode) const override;

    // AST interface
    [[nodiscard]] auto type() const -> TermType override;
    [[nodiscard]] auto get_int(Attribute attr) -> int & override;
    [[nodiscard]] auto get_ast(Attribute attr) -> STerm & override;

  private:
    BinaryOperator op_;
    STerm lhs_;
    STerm rhs_;
};

HASH(Term)
