#pragma once

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "core.hh"
#include "symbol.hh"

namespace Clingo::AST {

namespace py = pybind11;

using Clingo::Symbol::Symbol;

using StringArray = std::vector<std::string>;

template <class T> auto c_cast(std::optional<T> const &opt) -> clingo_ast_t *;

template <class... Ts> auto c_cast(std::variant<Ts...> const &var) -> clingo_ast_t *;

template <class T> auto c_cast(std::vector<T> const &arr) -> std::vector<clingo_ast_t *>;

auto c_cast(StringArray const &arr) -> std::vector<char const *> {
    std::vector<char const *> ret;
    ret.reserve(arr.size());
    for (auto const &str : arr) {
        ret.emplace_back(str.c_str());
    }
    return ret;
}

enum class ProjectionMode {
    Disabled = clingo_projection_mode_disabled,
    Anonymous = clingo_projection_mode_anonymous,
    Pure = clingo_projection_mode_pure,
};

enum class UnaryOperator {
    Minus = 0,
    Negation = 1,
};

enum class BinaryOperator {
    And = 0,
    Division = 1,
    Minus = 2,
    Modulo = 3,
    Multiplication = 4,
    Or = 5,
    Plus = 6,
    Power = 7,
    Xor = 8,
};

enum class Sign {
    NoSign = 0,
    Single = 1,
    Double = 2,
};

enum class Relation {
    Equal = 0,
    NotEqual = 1,
    Less = 2,
    LessEqual = 3,
    Greater = 4,
    GreaterEqual = 5,
};

enum class AggregateFunction {
    Count = 0,
    Sum = 1,
    Sump = 2,
    Min = 3,
    Max = 4,
};

enum class TheoryOperatorType {
    Unary = 0,
    BinaryLeft = 1,
    BinaryRight = 2,
};

enum class TheoryTupleType {
    Tuple = 0,
    Set = 1,
    List = 2,
};

enum class TheoryAtomType {
    Head = 0,
    Body = 1,
    Any = 2,
    Directive = 3,
};

enum class OptimizeType {
    Minimize = 0,
    Maximize = 1,
};

enum class IncludeType {
    System = 0,
    Inbuild = 1,
};

enum class ConstType {
    Default = 0,
    Override = 1,
};

enum class CommentType {
    Line = 0,
    Block = 1,
};

class TermVariable;

class TermSymbolic;

class TermAbsolute;

class TermUnaryOperation;

class TermBinaryOperation;

class TermTuple;

class TermFunction;

using Term = std::variant<TermVariable, TermSymbolic, TermAbsolute, TermUnaryOperation, TermBinaryOperation, TermTuple,
                          TermFunction>;

auto construct_term(clingo_ast_t *ast) -> Term;

using TermArray = std::vector<Term>;

auto construct_term_array(clingo_ast_t **ast, size_t size) -> TermArray;

using OptionalTerm = std::optional<Term>;

class Projection {
  public:
    // Note: for pybind
    Projection() = default;

    Projection(Projection const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    Projection(Projection &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(Projection const &x) -> Projection & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(Projection &&x) noexcept -> Projection & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(Projection const &a, Projection const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(Projection const &a, Projection const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, Projection)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~Projection() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    static auto acquire(clingo_ast_t *ast) -> Projection { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location) -> Projection;

    friend auto c_cast(Projection const &x) -> clingo_ast_t *;

  private:
    Projection(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(Projection const &x) -> clingo_ast_t * { return x.ast_; }

using TermOrProjection = std::variant<Term, Projection>;

auto construct_term_or_projection(clingo_ast_t *ast) -> TermOrProjection;

using TermOrProjectionArray = std::vector<TermOrProjection>;

auto construct_term_or_projection_array(clingo_ast_t **ast, size_t size) -> TermOrProjectionArray;

class ArgumentTuple;

using ArgumentTupleArray = std::vector<ArgumentTuple>;

auto construct_argument_tuple_array(clingo_ast_t **ast, size_t size) -> ArgumentTupleArray;

class ArgumentTuple;

using TermOrArgumentTuple = std::variant<Term, ArgumentTuple>;

auto construct_term_or_argument_tuple(clingo_ast_t *ast) -> TermOrArgumentTuple;

using TermOrArgumentTupleArray = std::vector<TermOrArgumentTuple>;

auto construct_term_or_argument_tuple_array(clingo_ast_t **ast, size_t size) -> TermOrArgumentTupleArray;

class TermVariable {
  public:
    // Note: for pybind
    TermVariable() = default;

    TermVariable(TermVariable const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TermVariable(TermVariable &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TermVariable const &x) -> TermVariable & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TermVariable &&x) noexcept -> TermVariable & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TermVariable const &a, TermVariable const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TermVariable const &a, TermVariable const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TermVariable)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TermVariable() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto anonymous() -> bool;

    static auto acquire(clingo_ast_t *ast) -> TermVariable { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name, bool anonymous)
        -> TermVariable;

    friend auto c_cast(TermVariable const &x) -> clingo_ast_t *;

  private:
    TermVariable(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TermVariable const &x) -> clingo_ast_t * { return x.ast_; }

class TermSymbolic {
  public:
    // Note: for pybind
    TermSymbolic() = default;

    TermSymbolic(TermSymbolic const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TermSymbolic(TermSymbolic &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TermSymbolic const &x) -> TermSymbolic & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TermSymbolic &&x) noexcept -> TermSymbolic & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TermSymbolic const &a, TermSymbolic const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TermSymbolic const &a, TermSymbolic const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TermSymbolic)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TermSymbolic() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto symbol() -> Symbol;

    static auto acquire(clingo_ast_t *ast) -> TermSymbolic { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Symbol const &symbol) -> TermSymbolic;

    friend auto c_cast(TermSymbolic const &x) -> clingo_ast_t *;

  private:
    TermSymbolic(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TermSymbolic const &x) -> clingo_ast_t * { return x.ast_; }

class TermAbsolute {
  public:
    // Note: for pybind
    TermAbsolute() = default;

    TermAbsolute(TermAbsolute const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TermAbsolute(TermAbsolute &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TermAbsolute const &x) -> TermAbsolute & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TermAbsolute &&x) noexcept -> TermAbsolute & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TermAbsolute const &a, TermAbsolute const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TermAbsolute const &a, TermAbsolute const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TermAbsolute)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TermAbsolute() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto pool() -> TermArray;

    static auto acquire(clingo_ast_t *ast) -> TermAbsolute { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, TermArray const &pool) -> TermAbsolute;

    friend auto c_cast(TermAbsolute const &x) -> clingo_ast_t *;

  private:
    TermAbsolute(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TermAbsolute const &x) -> clingo_ast_t * { return x.ast_; }

class TermUnaryOperation {
  public:
    // Note: for pybind
    TermUnaryOperation() = default;

    TermUnaryOperation(TermUnaryOperation const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TermUnaryOperation(TermUnaryOperation &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TermUnaryOperation const &x) -> TermUnaryOperation & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TermUnaryOperation &&x) noexcept -> TermUnaryOperation & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TermUnaryOperation const &a, TermUnaryOperation const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TermUnaryOperation const &a, TermUnaryOperation const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TermUnaryOperation)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TermUnaryOperation() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto operator_type() -> UnaryOperator;

    auto right() -> Term;

    static auto acquire(clingo_ast_t *ast) -> TermUnaryOperation { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, UnaryOperator const &operator_type,
                          Term const &right) -> TermUnaryOperation;

    friend auto c_cast(TermUnaryOperation const &x) -> clingo_ast_t *;

  private:
    TermUnaryOperation(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TermUnaryOperation const &x) -> clingo_ast_t * { return x.ast_; }

class TermBinaryOperation {
  public:
    // Note: for pybind
    TermBinaryOperation() = default;

    TermBinaryOperation(TermBinaryOperation const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TermBinaryOperation(TermBinaryOperation &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TermBinaryOperation const &x) -> TermBinaryOperation & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TermBinaryOperation &&x) noexcept -> TermBinaryOperation & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TermBinaryOperation const &a, TermBinaryOperation const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TermBinaryOperation const &a, TermBinaryOperation const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TermBinaryOperation)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TermBinaryOperation() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto left() -> Term;

    auto operator_type() -> BinaryOperator;

    auto right() -> Term;

    static auto acquire(clingo_ast_t *ast) -> TermBinaryOperation { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Term const &left,
                          BinaryOperator const &operator_type, Term const &right) -> TermBinaryOperation;

    friend auto c_cast(TermBinaryOperation const &x) -> clingo_ast_t *;

  private:
    TermBinaryOperation(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TermBinaryOperation const &x) -> clingo_ast_t * { return x.ast_; }

class TermTuple {
  public:
    // Note: for pybind
    TermTuple() = default;

    TermTuple(TermTuple const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TermTuple(TermTuple &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TermTuple const &x) -> TermTuple & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TermTuple &&x) noexcept -> TermTuple & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TermTuple const &a, TermTuple const &b) -> bool { return clingo_ast_equal(a.ast_, b.ast_); }

    friend auto operator<(TermTuple const &a, TermTuple const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TermTuple)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TermTuple() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto pool() -> TermOrArgumentTupleArray;

    static auto acquire(clingo_ast_t *ast) -> TermTuple { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, TermOrArgumentTupleArray const &pool)
        -> TermTuple;

    friend auto c_cast(TermTuple const &x) -> clingo_ast_t *;

  private:
    TermTuple(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TermTuple const &x) -> clingo_ast_t * { return x.ast_; }

class TermFunction {
  public:
    // Note: for pybind
    TermFunction() = default;

    TermFunction(TermFunction const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TermFunction(TermFunction &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TermFunction const &x) -> TermFunction & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TermFunction &&x) noexcept -> TermFunction & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TermFunction const &a, TermFunction const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TermFunction const &a, TermFunction const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TermFunction)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TermFunction() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto pool() -> ArgumentTupleArray;

    auto external() -> bool;

    static auto acquire(clingo_ast_t *ast) -> TermFunction { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name,
                          ArgumentTupleArray const &pool, bool external) -> TermFunction;

    friend auto c_cast(TermFunction const &x) -> clingo_ast_t *;

  private:
    TermFunction(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TermFunction const &x) -> clingo_ast_t * { return x.ast_; }

class ArgumentTuple {
  public:
    // Note: for pybind
    ArgumentTuple() = default;

    ArgumentTuple(ArgumentTuple const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    ArgumentTuple(ArgumentTuple &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(ArgumentTuple const &x) -> ArgumentTuple & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(ArgumentTuple &&x) noexcept -> ArgumentTuple & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(ArgumentTuple const &a, ArgumentTuple const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(ArgumentTuple const &a, ArgumentTuple const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, ArgumentTuple)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~ArgumentTuple() { clingo_ast_free(ast_); }

    auto arguments() -> TermOrProjectionArray;

    static auto acquire(clingo_ast_t *ast) -> ArgumentTuple { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, TermOrProjectionArray const &arguments) -> ArgumentTuple;

    friend auto c_cast(ArgumentTuple const &x) -> clingo_ast_t *;

  private:
    ArgumentTuple(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(ArgumentTuple const &x) -> clingo_ast_t * { return x.ast_; }

class LiteralBoolean;

class LiteralComparison;

class LiteralSymbolic;

using Literal = std::variant<LiteralBoolean, LiteralComparison, LiteralSymbolic>;

auto construct_literal(clingo_ast_t *ast) -> Literal;

class LeftGuard {
  public:
    // Note: for pybind
    LeftGuard() = default;

    LeftGuard(LeftGuard const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    LeftGuard(LeftGuard &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(LeftGuard const &x) -> LeftGuard & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(LeftGuard &&x) noexcept -> LeftGuard & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(LeftGuard const &a, LeftGuard const &b) -> bool { return clingo_ast_equal(a.ast_, b.ast_); }

    friend auto operator<(LeftGuard const &a, LeftGuard const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, LeftGuard)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~LeftGuard() { clingo_ast_free(ast_); }

    auto term() -> Term;

    auto relation() -> Relation;

    static auto acquire(clingo_ast_t *ast) -> LeftGuard { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, Term const &term, Relation const &relation) -> LeftGuard;

    friend auto c_cast(LeftGuard const &x) -> clingo_ast_t *;

  private:
    LeftGuard(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(LeftGuard const &x) -> clingo_ast_t * { return x.ast_; }

using OptionalLeftGuard = std::optional<LeftGuard>;

class RightGuard {
  public:
    // Note: for pybind
    RightGuard() = default;

    RightGuard(RightGuard const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    RightGuard(RightGuard &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(RightGuard const &x) -> RightGuard & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(RightGuard &&x) noexcept -> RightGuard & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(RightGuard const &a, RightGuard const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(RightGuard const &a, RightGuard const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, RightGuard)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~RightGuard() { clingo_ast_free(ast_); }

    auto relation() -> Relation;

    auto term() -> Term;

    static auto acquire(clingo_ast_t *ast) -> RightGuard { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, Relation const &relation, Term const &term) -> RightGuard;

    friend auto c_cast(RightGuard const &x) -> clingo_ast_t *;

  private:
    RightGuard(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(RightGuard const &x) -> clingo_ast_t * { return x.ast_; }

using OptionalRightGuard = std::optional<RightGuard>;

using RightGuardArray = std::vector<RightGuard>;

auto construct_right_guard_array(clingo_ast_t **ast, size_t size) -> RightGuardArray;

class LiteralBoolean {
  public:
    // Note: for pybind
    LiteralBoolean() = default;

    LiteralBoolean(LiteralBoolean const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    LiteralBoolean(LiteralBoolean &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(LiteralBoolean const &x) -> LiteralBoolean & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(LiteralBoolean &&x) noexcept -> LiteralBoolean & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(LiteralBoolean const &a, LiteralBoolean const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(LiteralBoolean const &a, LiteralBoolean const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, LiteralBoolean)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~LiteralBoolean() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto sign() -> Sign;

    auto value() -> bool;

    static auto acquire(clingo_ast_t *ast) -> LiteralBoolean { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Sign const &sign, bool value)
        -> LiteralBoolean;

    friend auto c_cast(LiteralBoolean const &x) -> clingo_ast_t *;

  private:
    LiteralBoolean(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(LiteralBoolean const &x) -> clingo_ast_t * { return x.ast_; }

class LiteralComparison {
  public:
    // Note: for pybind
    LiteralComparison() = default;

    LiteralComparison(LiteralComparison const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    LiteralComparison(LiteralComparison &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(LiteralComparison const &x) -> LiteralComparison & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(LiteralComparison &&x) noexcept -> LiteralComparison & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(LiteralComparison const &a, LiteralComparison const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(LiteralComparison const &a, LiteralComparison const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, LiteralComparison)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~LiteralComparison() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto sign() -> Sign;

    auto left() -> Term;

    auto right() -> RightGuardArray;

    static auto acquire(clingo_ast_t *ast) -> LiteralComparison { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Sign const &sign, Term const &left,
                          RightGuardArray const &right) -> LiteralComparison;

    friend auto c_cast(LiteralComparison const &x) -> clingo_ast_t *;

  private:
    LiteralComparison(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(LiteralComparison const &x) -> clingo_ast_t * { return x.ast_; }

class LiteralSymbolic {
  public:
    // Note: for pybind
    LiteralSymbolic() = default;

    LiteralSymbolic(LiteralSymbolic const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    LiteralSymbolic(LiteralSymbolic &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(LiteralSymbolic const &x) -> LiteralSymbolic & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(LiteralSymbolic &&x) noexcept -> LiteralSymbolic & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(LiteralSymbolic const &a, LiteralSymbolic const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(LiteralSymbolic const &a, LiteralSymbolic const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, LiteralSymbolic)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~LiteralSymbolic() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto sign() -> Sign;

    auto atom() -> Term;

    static auto acquire(clingo_ast_t *ast) -> LiteralSymbolic { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Sign const &sign, Term const &atom)
        -> LiteralSymbolic;

    friend auto c_cast(LiteralSymbolic const &x) -> clingo_ast_t *;

  private:
    LiteralSymbolic(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(LiteralSymbolic const &x) -> clingo_ast_t * { return x.ast_; }

class TheoryTermVariable;

class TheoryTermSymbolic;

class TheoryTermTuple;

class TheoryTermFunction;

class TheoryTermUnparsed;

using TheoryTerm =
    std::variant<TheoryTermVariable, TheoryTermSymbolic, TheoryTermTuple, TheoryTermFunction, TheoryTermUnparsed>;

auto construct_theory_term(clingo_ast_t *ast) -> TheoryTerm;

using TheoryTermArray = std::vector<TheoryTerm>;

auto construct_theory_term_array(clingo_ast_t **ast, size_t size) -> TheoryTermArray;

class UnparsedElement {
  public:
    // Note: for pybind
    UnparsedElement() = default;

    UnparsedElement(UnparsedElement const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    UnparsedElement(UnparsedElement &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(UnparsedElement const &x) -> UnparsedElement & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(UnparsedElement &&x) noexcept -> UnparsedElement & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(UnparsedElement const &a, UnparsedElement const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(UnparsedElement const &a, UnparsedElement const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, UnparsedElement)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~UnparsedElement() { clingo_ast_free(ast_); }

    auto operators() -> std::vector<char const *>;

    auto term() -> TheoryTerm;

    static auto acquire(clingo_ast_t *ast) -> UnparsedElement { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, StringArray const &operators, TheoryTerm const &term) -> UnparsedElement;

    friend auto c_cast(UnparsedElement const &x) -> clingo_ast_t *;

  private:
    UnparsedElement(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(UnparsedElement const &x) -> clingo_ast_t * { return x.ast_; }

using UnparsedElementArray = std::vector<UnparsedElement>;

auto construct_unparsed_element_array(clingo_ast_t **ast, size_t size) -> UnparsedElementArray;

class TheoryTermVariable {
  public:
    // Note: for pybind
    TheoryTermVariable() = default;

    TheoryTermVariable(TheoryTermVariable const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TheoryTermVariable(TheoryTermVariable &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TheoryTermVariable const &x) -> TheoryTermVariable & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TheoryTermVariable &&x) noexcept -> TheoryTermVariable & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TheoryTermVariable)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TheoryTermVariable() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto anonymous() -> bool;

    static auto acquire(clingo_ast_t *ast) -> TheoryTermVariable { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name, bool anonymous)
        -> TheoryTermVariable;

    friend auto c_cast(TheoryTermVariable const &x) -> clingo_ast_t *;

  private:
    TheoryTermVariable(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TheoryTermVariable const &x) -> clingo_ast_t * { return x.ast_; }

class TheoryTermSymbolic {
  public:
    // Note: for pybind
    TheoryTermSymbolic() = default;

    TheoryTermSymbolic(TheoryTermSymbolic const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TheoryTermSymbolic(TheoryTermSymbolic &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TheoryTermSymbolic const &x) -> TheoryTermSymbolic & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TheoryTermSymbolic &&x) noexcept -> TheoryTermSymbolic & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TheoryTermSymbolic const &a, TheoryTermSymbolic const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TheoryTermSymbolic const &a, TheoryTermSymbolic const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TheoryTermSymbolic)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TheoryTermSymbolic() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto symbol() -> Symbol;

    static auto acquire(clingo_ast_t *ast) -> TheoryTermSymbolic { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Symbol const &symbol) -> TheoryTermSymbolic;

    friend auto c_cast(TheoryTermSymbolic const &x) -> clingo_ast_t *;

  private:
    TheoryTermSymbolic(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TheoryTermSymbolic const &x) -> clingo_ast_t * { return x.ast_; }

class TheoryTermTuple {
  public:
    // Note: for pybind
    TheoryTermTuple() = default;

    TheoryTermTuple(TheoryTermTuple const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TheoryTermTuple(TheoryTermTuple &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TheoryTermTuple const &x) -> TheoryTermTuple & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TheoryTermTuple &&x) noexcept -> TheoryTermTuple & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TheoryTermTuple)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TheoryTermTuple() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto tuple_type() -> TheoryTupleType;

    auto arguments() -> TheoryTermArray;

    static auto acquire(clingo_ast_t *ast) -> TheoryTermTuple { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, TheoryTupleType const &tuple_type,
                          TheoryTermArray const &arguments) -> TheoryTermTuple;

    friend auto c_cast(TheoryTermTuple const &x) -> clingo_ast_t *;

  private:
    TheoryTermTuple(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TheoryTermTuple const &x) -> clingo_ast_t * { return x.ast_; }

class TheoryTermFunction {
  public:
    // Note: for pybind
    TheoryTermFunction() = default;

    TheoryTermFunction(TheoryTermFunction const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TheoryTermFunction(TheoryTermFunction &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TheoryTermFunction const &x) -> TheoryTermFunction & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TheoryTermFunction &&x) noexcept -> TheoryTermFunction & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TheoryTermFunction)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TheoryTermFunction() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto arguments() -> TheoryTermArray;

    static auto acquire(clingo_ast_t *ast) -> TheoryTermFunction { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name,
                          TheoryTermArray const &arguments) -> TheoryTermFunction;

    friend auto c_cast(TheoryTermFunction const &x) -> clingo_ast_t *;

  private:
    TheoryTermFunction(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TheoryTermFunction const &x) -> clingo_ast_t * { return x.ast_; }

class TheoryTermUnparsed {
  public:
    // Note: for pybind
    TheoryTermUnparsed() = default;

    TheoryTermUnparsed(TheoryTermUnparsed const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TheoryTermUnparsed(TheoryTermUnparsed &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TheoryTermUnparsed const &x) -> TheoryTermUnparsed & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TheoryTermUnparsed &&x) noexcept -> TheoryTermUnparsed & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TheoryTermUnparsed)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TheoryTermUnparsed() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto elements() -> UnparsedElementArray;

    static auto acquire(clingo_ast_t *ast) -> TheoryTermUnparsed { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, UnparsedElementArray const &elements)
        -> TheoryTermUnparsed;

    friend auto c_cast(TheoryTermUnparsed const &x) -> clingo_ast_t *;

  private:
    TheoryTermUnparsed(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TheoryTermUnparsed const &x) -> clingo_ast_t * { return x.ast_; }

class TheoryRightGuard {
  public:
    // Note: for pybind
    TheoryRightGuard() = default;

    TheoryRightGuard(TheoryRightGuard const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TheoryRightGuard(TheoryRightGuard &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TheoryRightGuard const &x) -> TheoryRightGuard & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TheoryRightGuard &&x) noexcept -> TheoryRightGuard & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TheoryRightGuard const &a, TheoryRightGuard const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TheoryRightGuard const &a, TheoryRightGuard const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TheoryRightGuard)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TheoryRightGuard() { clingo_ast_free(ast_); }

    auto theory_operator() -> char const *;

    auto term() -> TheoryTerm;

    static auto acquire(clingo_ast_t *ast) -> TheoryRightGuard { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, char const *theory_operator, TheoryTerm const &term) -> TheoryRightGuard;

    friend auto c_cast(TheoryRightGuard const &x) -> clingo_ast_t *;

  private:
    TheoryRightGuard(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TheoryRightGuard const &x) -> clingo_ast_t * { return x.ast_; }

using OptionalTheoryRightGuard = std::optional<TheoryRightGuard>;

using LiteralArray = std::vector<Literal>;

auto construct_literal_array(clingo_ast_t **ast, size_t size) -> LiteralArray;

class SetAggregateElement {
  public:
    // Note: for pybind
    SetAggregateElement() = default;

    SetAggregateElement(SetAggregateElement const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    SetAggregateElement(SetAggregateElement &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(SetAggregateElement const &x) -> SetAggregateElement & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(SetAggregateElement &&x) noexcept -> SetAggregateElement & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(SetAggregateElement const &a, SetAggregateElement const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(SetAggregateElement const &a, SetAggregateElement const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, SetAggregateElement)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~SetAggregateElement() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto literal() -> Literal;

    auto condition() -> LiteralArray;

    static auto acquire(clingo_ast_t *ast) -> SetAggregateElement { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Literal const &literal,
                          LiteralArray const &condition) -> SetAggregateElement;

    friend auto c_cast(SetAggregateElement const &x) -> clingo_ast_t *;

  private:
    SetAggregateElement(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(SetAggregateElement const &x) -> clingo_ast_t * { return x.ast_; }

using SetAggregateElementArray = std::vector<SetAggregateElement>;

auto construct_set_aggregate_element_array(clingo_ast_t **ast, size_t size) -> SetAggregateElementArray;

class BodyAggregateElement {
  public:
    // Note: for pybind
    BodyAggregateElement() = default;

    BodyAggregateElement(BodyAggregateElement const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    BodyAggregateElement(BodyAggregateElement &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(BodyAggregateElement const &x) -> BodyAggregateElement & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(BodyAggregateElement &&x) noexcept -> BodyAggregateElement & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(BodyAggregateElement const &a, BodyAggregateElement const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(BodyAggregateElement const &a, BodyAggregateElement const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, BodyAggregateElement)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~BodyAggregateElement() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto tuple() -> TermArray;

    auto condition() -> LiteralArray;

    static auto acquire(clingo_ast_t *ast) -> BodyAggregateElement { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, TermArray const &tuple,
                          LiteralArray const &condition) -> BodyAggregateElement;

    friend auto c_cast(BodyAggregateElement const &x) -> clingo_ast_t *;

  private:
    BodyAggregateElement(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(BodyAggregateElement const &x) -> clingo_ast_t * { return x.ast_; }

using BodyAggregateElementArray = std::vector<BodyAggregateElement>;

auto construct_body_aggregate_element_array(clingo_ast_t **ast, size_t size) -> BodyAggregateElementArray;

class TheoryAtomElement {
  public:
    // Note: for pybind
    TheoryAtomElement() = default;

    TheoryAtomElement(TheoryAtomElement const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TheoryAtomElement(TheoryAtomElement &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TheoryAtomElement const &x) -> TheoryAtomElement & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TheoryAtomElement &&x) noexcept -> TheoryAtomElement & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TheoryAtomElement const &a, TheoryAtomElement const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TheoryAtomElement const &a, TheoryAtomElement const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TheoryAtomElement)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TheoryAtomElement() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto tuple() -> TheoryTermArray;

    auto condition() -> LiteralArray;

    static auto acquire(clingo_ast_t *ast) -> TheoryAtomElement { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, TheoryTermArray const &tuple,
                          LiteralArray const &condition) -> TheoryAtomElement;

    friend auto c_cast(TheoryAtomElement const &x) -> clingo_ast_t *;

  private:
    TheoryAtomElement(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TheoryAtomElement const &x) -> clingo_ast_t * { return x.ast_; }

using TheoryAtomElementArray = std::vector<TheoryAtomElement>;

auto construct_theory_atom_element_array(clingo_ast_t **ast, size_t size) -> TheoryAtomElementArray;

class BodySimpleLiteral;

class BodyAggregate;

class BodySetAggregate;

class BodyTheoryAtom;

class BodyConditionalLiteral;

using BodyLiteral =
    std::variant<BodySimpleLiteral, BodyAggregate, BodySetAggregate, BodyTheoryAtom, BodyConditionalLiteral>;

auto construct_body_literal(clingo_ast_t *ast) -> BodyLiteral;

using BodyLiteralArray = std::vector<BodyLiteral>;

auto construct_body_literal_array(clingo_ast_t **ast, size_t size) -> BodyLiteralArray;

class BodySimpleLiteral {
  public:
    // Note: for pybind
    BodySimpleLiteral() = default;

    BodySimpleLiteral(BodySimpleLiteral const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    BodySimpleLiteral(BodySimpleLiteral &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(BodySimpleLiteral const &x) -> BodySimpleLiteral & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(BodySimpleLiteral &&x) noexcept -> BodySimpleLiteral & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(BodySimpleLiteral const &a, BodySimpleLiteral const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(BodySimpleLiteral const &a, BodySimpleLiteral const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, BodySimpleLiteral)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~BodySimpleLiteral() { clingo_ast_free(ast_); }

    auto literal() -> Literal;

    static auto acquire(clingo_ast_t *ast) -> BodySimpleLiteral { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, Literal const &literal) -> BodySimpleLiteral;

    friend auto c_cast(BodySimpleLiteral const &x) -> clingo_ast_t *;

  private:
    BodySimpleLiteral(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(BodySimpleLiteral const &x) -> clingo_ast_t * { return x.ast_; }

class BodyAggregate {
  public:
    // Note: for pybind
    BodyAggregate() = default;

    BodyAggregate(BodyAggregate const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    BodyAggregate(BodyAggregate &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(BodyAggregate const &x) -> BodyAggregate & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(BodyAggregate &&x) noexcept -> BodyAggregate & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(BodyAggregate const &a, BodyAggregate const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(BodyAggregate const &a, BodyAggregate const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, BodyAggregate)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~BodyAggregate() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto sign() -> Sign;

    auto left() -> OptionalLeftGuard;

    auto function() -> AggregateFunction;

    auto elements() -> BodyAggregateElementArray;

    auto right() -> OptionalRightGuard;

    static auto acquire(clingo_ast_t *ast) -> BodyAggregate { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Sign const &sign,
                          OptionalLeftGuard const &left, AggregateFunction const &function,
                          BodyAggregateElementArray const &elements, OptionalRightGuard const &right) -> BodyAggregate;

    friend auto c_cast(BodyAggregate const &x) -> clingo_ast_t *;

  private:
    BodyAggregate(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(BodyAggregate const &x) -> clingo_ast_t * { return x.ast_; }

class BodySetAggregate {
  public:
    // Note: for pybind
    BodySetAggregate() = default;

    BodySetAggregate(BodySetAggregate const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    BodySetAggregate(BodySetAggregate &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(BodySetAggregate const &x) -> BodySetAggregate & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(BodySetAggregate &&x) noexcept -> BodySetAggregate & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(BodySetAggregate const &a, BodySetAggregate const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(BodySetAggregate const &a, BodySetAggregate const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, BodySetAggregate)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~BodySetAggregate() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto sign() -> Sign;

    auto left() -> OptionalLeftGuard;

    auto elements() -> SetAggregateElementArray;

    auto right() -> OptionalRightGuard;

    static auto acquire(clingo_ast_t *ast) -> BodySetAggregate { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Sign const &sign,
                          OptionalLeftGuard const &left, SetAggregateElementArray const &elements,
                          OptionalRightGuard const &right) -> BodySetAggregate;

    friend auto c_cast(BodySetAggregate const &x) -> clingo_ast_t *;

  private:
    BodySetAggregate(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(BodySetAggregate const &x) -> clingo_ast_t * { return x.ast_; }

class BodyTheoryAtom {
  public:
    // Note: for pybind
    BodyTheoryAtom() = default;

    BodyTheoryAtom(BodyTheoryAtom const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    BodyTheoryAtom(BodyTheoryAtom &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(BodyTheoryAtom const &x) -> BodyTheoryAtom & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(BodyTheoryAtom &&x) noexcept -> BodyTheoryAtom & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(BodyTheoryAtom const &a, BodyTheoryAtom const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(BodyTheoryAtom const &a, BodyTheoryAtom const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, BodyTheoryAtom)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~BodyTheoryAtom() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto sign() -> Sign;

    auto name() -> Term;

    auto elements() -> TheoryAtomElementArray;

    auto right() -> OptionalTheoryRightGuard;

    static auto acquire(clingo_ast_t *ast) -> BodyTheoryAtom { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Sign const &sign, Term const &name,
                          TheoryAtomElementArray const &elements, OptionalTheoryRightGuard const &right)
        -> BodyTheoryAtom;

    friend auto c_cast(BodyTheoryAtom const &x) -> clingo_ast_t *;

  private:
    BodyTheoryAtom(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(BodyTheoryAtom const &x) -> clingo_ast_t * { return x.ast_; }

class BodyConditionalLiteral {
  public:
    // Note: for pybind
    BodyConditionalLiteral() = default;

    BodyConditionalLiteral(BodyConditionalLiteral const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    BodyConditionalLiteral(BodyConditionalLiteral &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(BodyConditionalLiteral const &x) -> BodyConditionalLiteral & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(BodyConditionalLiteral &&x) noexcept -> BodyConditionalLiteral & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(BodyConditionalLiteral const &a, BodyConditionalLiteral const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(BodyConditionalLiteral const &a, BodyConditionalLiteral const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, BodyConditionalLiteral)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~BodyConditionalLiteral() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto literal() -> Literal;

    auto condition() -> LiteralArray;

    static auto acquire(clingo_ast_t *ast) -> BodyConditionalLiteral { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Literal const &literal,
                          LiteralArray const &condition) -> BodyConditionalLiteral;

    friend auto c_cast(BodyConditionalLiteral const &x) -> clingo_ast_t *;

  private:
    BodyConditionalLiteral(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(BodyConditionalLiteral const &x) -> clingo_ast_t * { return x.ast_; }

class HeadConditionalLiteral {
  public:
    // Note: for pybind
    HeadConditionalLiteral() = default;

    HeadConditionalLiteral(HeadConditionalLiteral const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    HeadConditionalLiteral(HeadConditionalLiteral &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(HeadConditionalLiteral const &x) -> HeadConditionalLiteral & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(HeadConditionalLiteral &&x) noexcept -> HeadConditionalLiteral & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(HeadConditionalLiteral const &a, HeadConditionalLiteral const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(HeadConditionalLiteral const &a, HeadConditionalLiteral const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, HeadConditionalLiteral)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~HeadConditionalLiteral() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto literal() -> Literal;

    auto condition() -> LiteralArray;

    static auto acquire(clingo_ast_t *ast) -> HeadConditionalLiteral { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Literal const &literal,
                          LiteralArray const &condition) -> HeadConditionalLiteral;

    friend auto c_cast(HeadConditionalLiteral const &x) -> clingo_ast_t *;

  private:
    HeadConditionalLiteral(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(HeadConditionalLiteral const &x) -> clingo_ast_t * { return x.ast_; }

using DisjunctionElement = std::variant<Literal, HeadConditionalLiteral>;

auto construct_disjunction_element(clingo_ast_t *ast) -> DisjunctionElement;

using DisjunctionElementArray = std::vector<DisjunctionElement>;

auto construct_disjunction_element_array(clingo_ast_t **ast, size_t size) -> DisjunctionElementArray;

class HeadAggregateElement {
  public:
    // Note: for pybind
    HeadAggregateElement() = default;

    HeadAggregateElement(HeadAggregateElement const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    HeadAggregateElement(HeadAggregateElement &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(HeadAggregateElement const &x) -> HeadAggregateElement & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(HeadAggregateElement &&x) noexcept -> HeadAggregateElement & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(HeadAggregateElement const &a, HeadAggregateElement const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(HeadAggregateElement const &a, HeadAggregateElement const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, HeadAggregateElement)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~HeadAggregateElement() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto tuple() -> TermArray;

    auto literal() -> Literal;

    auto condition() -> LiteralArray;

    static auto acquire(clingo_ast_t *ast) -> HeadAggregateElement { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, TermArray const &tuple,
                          Literal const &literal, LiteralArray const &condition) -> HeadAggregateElement;

    friend auto c_cast(HeadAggregateElement const &x) -> clingo_ast_t *;

  private:
    HeadAggregateElement(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(HeadAggregateElement const &x) -> clingo_ast_t * { return x.ast_; }

using HeadAggregateElementArray = std::vector<HeadAggregateElement>;

auto construct_head_aggregate_element_array(clingo_ast_t **ast, size_t size) -> HeadAggregateElementArray;

class HeadSimpleLiteral;

class HeadAggregate;

class HeadSetAggregate;

class HeadTheoryAtom;

class HeadDisjunction;

using HeadLiteral = std::variant<HeadSimpleLiteral, HeadAggregate, HeadSetAggregate, HeadTheoryAtom, HeadDisjunction>;

auto construct_head_literal(clingo_ast_t *ast) -> HeadLiteral;

class HeadSimpleLiteral {
  public:
    // Note: for pybind
    HeadSimpleLiteral() = default;

    HeadSimpleLiteral(HeadSimpleLiteral const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    HeadSimpleLiteral(HeadSimpleLiteral &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(HeadSimpleLiteral const &x) -> HeadSimpleLiteral & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(HeadSimpleLiteral &&x) noexcept -> HeadSimpleLiteral & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(HeadSimpleLiteral const &a, HeadSimpleLiteral const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(HeadSimpleLiteral const &a, HeadSimpleLiteral const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, HeadSimpleLiteral)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~HeadSimpleLiteral() { clingo_ast_free(ast_); }

    auto literal() -> Literal;

    static auto acquire(clingo_ast_t *ast) -> HeadSimpleLiteral { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, Literal const &literal) -> HeadSimpleLiteral;

    friend auto c_cast(HeadSimpleLiteral const &x) -> clingo_ast_t *;

  private:
    HeadSimpleLiteral(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(HeadSimpleLiteral const &x) -> clingo_ast_t * { return x.ast_; }

class HeadAggregate {
  public:
    // Note: for pybind
    HeadAggregate() = default;

    HeadAggregate(HeadAggregate const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    HeadAggregate(HeadAggregate &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(HeadAggregate const &x) -> HeadAggregate & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(HeadAggregate &&x) noexcept -> HeadAggregate & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(HeadAggregate const &a, HeadAggregate const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(HeadAggregate const &a, HeadAggregate const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, HeadAggregate)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~HeadAggregate() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto left() -> OptionalLeftGuard;

    auto function() -> AggregateFunction;

    auto elements() -> HeadAggregateElementArray;

    auto right() -> OptionalRightGuard;

    static auto acquire(clingo_ast_t *ast) -> HeadAggregate { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, OptionalLeftGuard const &left,
                          AggregateFunction const &function, HeadAggregateElementArray const &elements,
                          OptionalRightGuard const &right) -> HeadAggregate;

    friend auto c_cast(HeadAggregate const &x) -> clingo_ast_t *;

  private:
    HeadAggregate(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(HeadAggregate const &x) -> clingo_ast_t * { return x.ast_; }

class HeadSetAggregate {
  public:
    // Note: for pybind
    HeadSetAggregate() = default;

    HeadSetAggregate(HeadSetAggregate const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    HeadSetAggregate(HeadSetAggregate &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(HeadSetAggregate const &x) -> HeadSetAggregate & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(HeadSetAggregate &&x) noexcept -> HeadSetAggregate & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(HeadSetAggregate const &a, HeadSetAggregate const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(HeadSetAggregate const &a, HeadSetAggregate const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, HeadSetAggregate)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~HeadSetAggregate() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto left() -> OptionalLeftGuard;

    auto elements() -> SetAggregateElementArray;

    auto right() -> OptionalRightGuard;

    static auto acquire(clingo_ast_t *ast) -> HeadSetAggregate { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, OptionalLeftGuard const &left,
                          SetAggregateElementArray const &elements, OptionalRightGuard const &right)
        -> HeadSetAggregate;

    friend auto c_cast(HeadSetAggregate const &x) -> clingo_ast_t *;

  private:
    HeadSetAggregate(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(HeadSetAggregate const &x) -> clingo_ast_t * { return x.ast_; }

class HeadTheoryAtom {
  public:
    // Note: for pybind
    HeadTheoryAtom() = default;

    HeadTheoryAtom(HeadTheoryAtom const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    HeadTheoryAtom(HeadTheoryAtom &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(HeadTheoryAtom const &x) -> HeadTheoryAtom & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(HeadTheoryAtom &&x) noexcept -> HeadTheoryAtom & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(HeadTheoryAtom const &a, HeadTheoryAtom const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(HeadTheoryAtom const &a, HeadTheoryAtom const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, HeadTheoryAtom)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~HeadTheoryAtom() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> Term;

    auto elements() -> TheoryAtomElementArray;

    auto right() -> OptionalTheoryRightGuard;

    static auto acquire(clingo_ast_t *ast) -> HeadTheoryAtom { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Term const &name,
                          TheoryAtomElementArray const &elements, OptionalTheoryRightGuard const &right)
        -> HeadTheoryAtom;

    friend auto c_cast(HeadTheoryAtom const &x) -> clingo_ast_t *;

  private:
    HeadTheoryAtom(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(HeadTheoryAtom const &x) -> clingo_ast_t * { return x.ast_; }

class HeadDisjunction {
  public:
    // Note: for pybind
    HeadDisjunction() = default;

    HeadDisjunction(HeadDisjunction const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    HeadDisjunction(HeadDisjunction &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(HeadDisjunction const &x) -> HeadDisjunction & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(HeadDisjunction &&x) noexcept -> HeadDisjunction & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(HeadDisjunction const &a, HeadDisjunction const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(HeadDisjunction const &a, HeadDisjunction const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, HeadDisjunction)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~HeadDisjunction() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto elements() -> DisjunctionElementArray;

    static auto acquire(clingo_ast_t *ast) -> HeadDisjunction { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, DisjunctionElementArray const &elements)
        -> HeadDisjunction;

    friend auto c_cast(HeadDisjunction const &x) -> clingo_ast_t *;

  private:
    HeadDisjunction(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(HeadDisjunction const &x) -> clingo_ast_t * { return x.ast_; }

class TheoryOperatorDefinition {
  public:
    // Note: for pybind
    TheoryOperatorDefinition() = default;

    TheoryOperatorDefinition(TheoryOperatorDefinition const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TheoryOperatorDefinition(TheoryOperatorDefinition &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TheoryOperatorDefinition const &x) -> TheoryOperatorDefinition & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TheoryOperatorDefinition &&x) noexcept -> TheoryOperatorDefinition & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TheoryOperatorDefinition const &a, TheoryOperatorDefinition const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TheoryOperatorDefinition const &a, TheoryOperatorDefinition const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TheoryOperatorDefinition)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TheoryOperatorDefinition() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto priority() -> int;

    auto operator_type() -> TheoryOperatorType;

    static auto acquire(clingo_ast_t *ast) -> TheoryOperatorDefinition { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name, int priority,
                          TheoryOperatorType const &operator_type) -> TheoryOperatorDefinition;

    friend auto c_cast(TheoryOperatorDefinition const &x) -> clingo_ast_t *;

  private:
    TheoryOperatorDefinition(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TheoryOperatorDefinition const &x) -> clingo_ast_t * { return x.ast_; }

using TheoryOperatorDefinitionArray = std::vector<TheoryOperatorDefinition>;

auto construct_theory_operator_definition_array(clingo_ast_t **ast, size_t size) -> TheoryOperatorDefinitionArray;

class TheoryTermDefinition {
  public:
    // Note: for pybind
    TheoryTermDefinition() = default;

    TheoryTermDefinition(TheoryTermDefinition const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TheoryTermDefinition(TheoryTermDefinition &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TheoryTermDefinition const &x) -> TheoryTermDefinition & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TheoryTermDefinition &&x) noexcept -> TheoryTermDefinition & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TheoryTermDefinition)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TheoryTermDefinition() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto operators() -> TheoryOperatorDefinitionArray;

    static auto acquire(clingo_ast_t *ast) -> TheoryTermDefinition { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name,
                          TheoryOperatorDefinitionArray const &operators) -> TheoryTermDefinition;

    friend auto c_cast(TheoryTermDefinition const &x) -> clingo_ast_t *;

  private:
    TheoryTermDefinition(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TheoryTermDefinition const &x) -> clingo_ast_t * { return x.ast_; }

using TheoryTermDefinitionArray = std::vector<TheoryTermDefinition>;

auto construct_theory_term_definition_array(clingo_ast_t **ast, size_t size) -> TheoryTermDefinitionArray;

class TheoryGuardDefinition {
  public:
    // Note: for pybind
    TheoryGuardDefinition() = default;

    TheoryGuardDefinition(TheoryGuardDefinition const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TheoryGuardDefinition(TheoryGuardDefinition &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TheoryGuardDefinition const &x) -> TheoryGuardDefinition & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TheoryGuardDefinition &&x) noexcept -> TheoryGuardDefinition & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TheoryGuardDefinition const &a, TheoryGuardDefinition const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TheoryGuardDefinition const &a, TheoryGuardDefinition const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TheoryGuardDefinition)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TheoryGuardDefinition() { clingo_ast_free(ast_); }

    auto operators() -> std::vector<char const *>;

    auto term() -> char const *;

    static auto acquire(clingo_ast_t *ast) -> TheoryGuardDefinition { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, StringArray const &operators, char const *term) -> TheoryGuardDefinition;

    friend auto c_cast(TheoryGuardDefinition const &x) -> clingo_ast_t *;

  private:
    TheoryGuardDefinition(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TheoryGuardDefinition const &x) -> clingo_ast_t * { return x.ast_; }

using OptionalTheoryGuardDefinition = std::optional<TheoryGuardDefinition>;

class TheoryAtomDefinition {
  public:
    // Note: for pybind
    TheoryAtomDefinition() = default;

    TheoryAtomDefinition(TheoryAtomDefinition const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    TheoryAtomDefinition(TheoryAtomDefinition &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(TheoryAtomDefinition const &x) -> TheoryAtomDefinition & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(TheoryAtomDefinition &&x) noexcept -> TheoryAtomDefinition & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, TheoryAtomDefinition)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~TheoryAtomDefinition() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto arity() -> int;

    auto term() -> char const *;

    auto guard() -> OptionalTheoryGuardDefinition;

    auto atom_type() -> TheoryAtomType;

    static auto acquire(clingo_ast_t *ast) -> TheoryAtomDefinition { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name, int arity,
                          char const *term, OptionalTheoryGuardDefinition const &guard, TheoryAtomType const &atom_type)
        -> TheoryAtomDefinition;

    friend auto c_cast(TheoryAtomDefinition const &x) -> clingo_ast_t *;

  private:
    TheoryAtomDefinition(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(TheoryAtomDefinition const &x) -> clingo_ast_t * { return x.ast_; }

using TheoryAtomDefinitionArray = std::vector<TheoryAtomDefinition>;

auto construct_theory_atom_definition_array(clingo_ast_t **ast, size_t size) -> TheoryAtomDefinitionArray;

class OptimizeTuple {
  public:
    // Note: for pybind
    OptimizeTuple() = default;

    OptimizeTuple(OptimizeTuple const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    OptimizeTuple(OptimizeTuple &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(OptimizeTuple const &x) -> OptimizeTuple & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(OptimizeTuple &&x) noexcept -> OptimizeTuple & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(OptimizeTuple const &a, OptimizeTuple const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(OptimizeTuple const &a, OptimizeTuple const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, OptimizeTuple)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~OptimizeTuple() { clingo_ast_free(ast_); }

    auto weight() -> Term;

    auto priority() -> OptionalTerm;

    auto terms() -> TermArray;

    static auto acquire(clingo_ast_t *ast) -> OptimizeTuple { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, Term const &weight, OptionalTerm const &priority, TermArray const &terms)
        -> OptimizeTuple;

    friend auto c_cast(OptimizeTuple const &x) -> clingo_ast_t *;

  private:
    OptimizeTuple(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(OptimizeTuple const &x) -> clingo_ast_t * { return x.ast_; }

class OptimizeElement {
  public:
    // Note: for pybind
    OptimizeElement() = default;

    OptimizeElement(OptimizeElement const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    OptimizeElement(OptimizeElement &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(OptimizeElement const &x) -> OptimizeElement & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(OptimizeElement &&x) noexcept -> OptimizeElement & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(OptimizeElement const &a, OptimizeElement const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(OptimizeElement const &a, OptimizeElement const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, OptimizeElement)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~OptimizeElement() { clingo_ast_free(ast_); }

    auto tuple() -> OptimizeTuple;

    auto condition() -> LiteralArray;

    static auto acquire(clingo_ast_t *ast) -> OptimizeElement { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, OptimizeTuple const &tuple, LiteralArray const &condition) -> OptimizeElement;

    friend auto c_cast(OptimizeElement const &x) -> clingo_ast_t *;

  private:
    OptimizeElement(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(OptimizeElement const &x) -> clingo_ast_t * { return x.ast_; }

using OptimizeElementArray = std::vector<OptimizeElement>;

auto construct_optimize_element_array(clingo_ast_t **ast, size_t size) -> OptimizeElementArray;

class Edge {
  public:
    // Note: for pybind
    Edge() = default;

    Edge(Edge const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    Edge(Edge &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(Edge const &x) -> Edge & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(Edge &&x) noexcept -> Edge & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(Edge const &a, Edge const &b) -> bool { return clingo_ast_equal(a.ast_, b.ast_); }

    friend auto operator<(Edge const &a, Edge const &b) -> bool { return clingo_ast_less_than(a.ast_, b.ast_); }

    CLINGO_CPP_TOTAL_ORDER(friend, Edge)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~Edge() { clingo_ast_free(ast_); }

    auto u() -> Term;

    auto v() -> Term;

    static auto acquire(clingo_ast_t *ast) -> Edge { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, Term const &u, Term const &v) -> Edge;

    friend auto c_cast(Edge const &x) -> clingo_ast_t *;

  private:
    Edge(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(Edge const &x) -> clingo_ast_t * { return x.ast_; }

using EdgeArray = std::vector<Edge>;

auto construct_edge_array(clingo_ast_t **ast, size_t size) -> EdgeArray;

class StatementRule;

class StatementTheory;

class StatementOptimize;

class StatementWeakConstraint;

class StatementShow;

class StatementShowSignature;

class StatementProject;

class StatementProjectSignature;

class StatementDefined;

class StatementExternal;

class StatementEdge;

class StatementHeuristic;

class StatementScript;

class StatementInclude;

class StatementProgram;

class StatementConst;

class StatementComment;

using Statement = std::variant<StatementRule, StatementTheory, StatementOptimize, StatementWeakConstraint,
                               StatementShow, StatementShowSignature, StatementProject, StatementProjectSignature,
                               StatementDefined, StatementExternal, StatementEdge, StatementHeuristic, StatementScript,
                               StatementInclude, StatementProgram, StatementConst, StatementComment>;

auto construct_statement(clingo_ast_t *ast) -> Statement;

class StatementRule {
  public:
    // Note: for pybind
    StatementRule() = default;

    StatementRule(StatementRule const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementRule(StatementRule &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementRule const &x) -> StatementRule & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementRule &&x) noexcept -> StatementRule & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementRule const &a, StatementRule const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementRule const &a, StatementRule const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementRule)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementRule() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto head() -> HeadLiteral;

    auto body() -> BodyLiteralArray;

    static auto acquire(clingo_ast_t *ast) -> StatementRule { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, HeadLiteral const &head,
                          BodyLiteralArray const &body) -> StatementRule;

    friend auto c_cast(StatementRule const &x) -> clingo_ast_t *;

  private:
    StatementRule(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementRule const &x) -> clingo_ast_t * { return x.ast_; }

class StatementTheory {
  public:
    // Note: for pybind
    StatementTheory() = default;

    StatementTheory(StatementTheory const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementTheory(StatementTheory &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementTheory const &x) -> StatementTheory & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementTheory &&x) noexcept -> StatementTheory & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementTheory const &a, StatementTheory const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementTheory const &a, StatementTheory const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementTheory)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementTheory() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto terms() -> TheoryTermDefinitionArray;

    auto atoms() -> TheoryAtomDefinitionArray;

    static auto acquire(clingo_ast_t *ast) -> StatementTheory { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name,
                          TheoryTermDefinitionArray const &terms, TheoryAtomDefinitionArray const &atoms)
        -> StatementTheory;

    friend auto c_cast(StatementTheory const &x) -> clingo_ast_t *;

  private:
    StatementTheory(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementTheory const &x) -> clingo_ast_t * { return x.ast_; }

class StatementOptimize {
  public:
    // Note: for pybind
    StatementOptimize() = default;

    StatementOptimize(StatementOptimize const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementOptimize(StatementOptimize &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementOptimize const &x) -> StatementOptimize & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementOptimize &&x) noexcept -> StatementOptimize & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementOptimize const &a, StatementOptimize const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementOptimize const &a, StatementOptimize const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementOptimize)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementOptimize() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto elements() -> OptimizeElementArray;

    auto optimize_type() -> OptimizeType;

    static auto acquire(clingo_ast_t *ast) -> StatementOptimize { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, OptimizeElementArray const &elements,
                          OptimizeType const &optimize_type) -> StatementOptimize;

    friend auto c_cast(StatementOptimize const &x) -> clingo_ast_t *;

  private:
    StatementOptimize(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementOptimize const &x) -> clingo_ast_t * { return x.ast_; }

class StatementWeakConstraint {
  public:
    // Note: for pybind
    StatementWeakConstraint() = default;

    StatementWeakConstraint(StatementWeakConstraint const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementWeakConstraint(StatementWeakConstraint &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementWeakConstraint const &x) -> StatementWeakConstraint & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementWeakConstraint &&x) noexcept -> StatementWeakConstraint & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementWeakConstraint const &a, StatementWeakConstraint const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementWeakConstraint const &a, StatementWeakConstraint const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementWeakConstraint)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementWeakConstraint() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto body() -> BodyLiteralArray;

    auto tuple() -> OptimizeTuple;

    static auto acquire(clingo_ast_t *ast) -> StatementWeakConstraint { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, BodyLiteralArray const &body,
                          OptimizeTuple const &tuple) -> StatementWeakConstraint;

    friend auto c_cast(StatementWeakConstraint const &x) -> clingo_ast_t *;

  private:
    StatementWeakConstraint(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementWeakConstraint const &x) -> clingo_ast_t * { return x.ast_; }

class StatementShow {
  public:
    // Note: for pybind
    StatementShow() = default;

    StatementShow(StatementShow const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementShow(StatementShow &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementShow const &x) -> StatementShow & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementShow &&x) noexcept -> StatementShow & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementShow const &a, StatementShow const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementShow const &a, StatementShow const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementShow)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementShow() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto term() -> Term;

    auto body() -> BodyLiteralArray;

    static auto acquire(clingo_ast_t *ast) -> StatementShow { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Term const &term,
                          BodyLiteralArray const &body) -> StatementShow;

    friend auto c_cast(StatementShow const &x) -> clingo_ast_t *;

  private:
    StatementShow(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementShow const &x) -> clingo_ast_t * { return x.ast_; }

class StatementShowSignature {
  public:
    // Note: for pybind
    StatementShowSignature() = default;

    StatementShowSignature(StatementShowSignature const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementShowSignature(StatementShowSignature &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementShowSignature const &x) -> StatementShowSignature & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementShowSignature &&x) noexcept -> StatementShowSignature & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementShowSignature const &a, StatementShowSignature const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementShowSignature const &a, StatementShowSignature const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementShowSignature)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementShowSignature() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto arity() -> int;

    auto sign() -> bool;

    static auto acquire(clingo_ast_t *ast) -> StatementShowSignature { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name, int arity, bool sign)
        -> StatementShowSignature;

    friend auto c_cast(StatementShowSignature const &x) -> clingo_ast_t *;

  private:
    StatementShowSignature(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementShowSignature const &x) -> clingo_ast_t * { return x.ast_; }

class StatementProject {
  public:
    // Note: for pybind
    StatementProject() = default;

    StatementProject(StatementProject const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementProject(StatementProject &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementProject const &x) -> StatementProject & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementProject &&x) noexcept -> StatementProject & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementProject const &a, StatementProject const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementProject const &a, StatementProject const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementProject)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementProject() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto atom() -> Term;

    auto body() -> BodyLiteralArray;

    static auto acquire(clingo_ast_t *ast) -> StatementProject { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Term const &atom,
                          BodyLiteralArray const &body) -> StatementProject;

    friend auto c_cast(StatementProject const &x) -> clingo_ast_t *;

  private:
    StatementProject(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementProject const &x) -> clingo_ast_t * { return x.ast_; }

class StatementProjectSignature {
  public:
    // Note: for pybind
    StatementProjectSignature() = default;

    StatementProjectSignature(StatementProjectSignature const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementProjectSignature(StatementProjectSignature &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementProjectSignature const &x) -> StatementProjectSignature & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementProjectSignature &&x) noexcept -> StatementProjectSignature & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementProjectSignature const &a, StatementProjectSignature const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementProjectSignature const &a, StatementProjectSignature const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementProjectSignature)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementProjectSignature() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto arity() -> int;

    auto sign() -> bool;

    static auto acquire(clingo_ast_t *ast) -> StatementProjectSignature { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name, int arity, bool sign)
        -> StatementProjectSignature;

    friend auto c_cast(StatementProjectSignature const &x) -> clingo_ast_t *;

  private:
    StatementProjectSignature(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementProjectSignature const &x) -> clingo_ast_t * { return x.ast_; }

class StatementDefined {
  public:
    // Note: for pybind
    StatementDefined() = default;

    StatementDefined(StatementDefined const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementDefined(StatementDefined &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementDefined const &x) -> StatementDefined & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementDefined &&x) noexcept -> StatementDefined & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementDefined const &a, StatementDefined const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementDefined const &a, StatementDefined const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementDefined)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementDefined() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto arity() -> int;

    auto sign() -> bool;

    static auto acquire(clingo_ast_t *ast) -> StatementDefined { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name, int arity, bool sign)
        -> StatementDefined;

    friend auto c_cast(StatementDefined const &x) -> clingo_ast_t *;

  private:
    StatementDefined(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementDefined const &x) -> clingo_ast_t * { return x.ast_; }

class StatementExternal {
  public:
    // Note: for pybind
    StatementExternal() = default;

    StatementExternal(StatementExternal const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementExternal(StatementExternal &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementExternal const &x) -> StatementExternal & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementExternal &&x) noexcept -> StatementExternal & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementExternal const &a, StatementExternal const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementExternal const &a, StatementExternal const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementExternal)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementExternal() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto atom() -> Term;

    auto body() -> BodyLiteralArray;

    auto external_type() -> OptionalTerm;

    static auto acquire(clingo_ast_t *ast) -> StatementExternal { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Term const &atom,
                          BodyLiteralArray const &body, OptionalTerm const &external_type) -> StatementExternal;

    friend auto c_cast(StatementExternal const &x) -> clingo_ast_t *;

  private:
    StatementExternal(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementExternal const &x) -> clingo_ast_t * { return x.ast_; }

class StatementEdge {
  public:
    // Note: for pybind
    StatementEdge() = default;

    StatementEdge(StatementEdge const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementEdge(StatementEdge &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementEdge const &x) -> StatementEdge & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementEdge &&x) noexcept -> StatementEdge & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementEdge const &a, StatementEdge const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementEdge const &a, StatementEdge const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementEdge)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementEdge() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto pool() -> EdgeArray;

    auto body() -> BodyLiteralArray;

    static auto acquire(clingo_ast_t *ast) -> StatementEdge { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, EdgeArray const &pool,
                          BodyLiteralArray const &body) -> StatementEdge;

    friend auto c_cast(StatementEdge const &x) -> clingo_ast_t *;

  private:
    StatementEdge(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementEdge const &x) -> clingo_ast_t * { return x.ast_; }

class StatementHeuristic {
  public:
    // Note: for pybind
    StatementHeuristic() = default;

    StatementHeuristic(StatementHeuristic const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementHeuristic(StatementHeuristic &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementHeuristic const &x) -> StatementHeuristic & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementHeuristic &&x) noexcept -> StatementHeuristic & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementHeuristic const &a, StatementHeuristic const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementHeuristic const &a, StatementHeuristic const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementHeuristic)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementHeuristic() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto atom() -> Term;

    auto body() -> BodyLiteralArray;

    auto weight() -> Term;

    auto modifier() -> Term;

    auto priority() -> OptionalTerm;

    static auto acquire(clingo_ast_t *ast) -> StatementHeuristic { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, Term const &atom,
                          BodyLiteralArray const &body, Term const &weight, Term const &modifier,
                          OptionalTerm const &priority) -> StatementHeuristic;

    friend auto c_cast(StatementHeuristic const &x) -> clingo_ast_t *;

  private:
    StatementHeuristic(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementHeuristic const &x) -> clingo_ast_t * { return x.ast_; }

class StatementScript {
  public:
    // Note: for pybind
    StatementScript() = default;

    StatementScript(StatementScript const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementScript(StatementScript &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementScript const &x) -> StatementScript & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementScript &&x) noexcept -> StatementScript & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementScript const &a, StatementScript const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementScript const &a, StatementScript const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementScript)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementScript() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto value() -> char const *;

    auto script_type() -> char const *;

    static auto acquire(clingo_ast_t *ast) -> StatementScript { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *value, char const *script_type)
        -> StatementScript;

    friend auto c_cast(StatementScript const &x) -> clingo_ast_t *;

  private:
    StatementScript(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementScript const &x) -> clingo_ast_t * { return x.ast_; }

class StatementInclude {
  public:
    // Note: for pybind
    StatementInclude() = default;

    StatementInclude(StatementInclude const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementInclude(StatementInclude &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementInclude const &x) -> StatementInclude & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementInclude &&x) noexcept -> StatementInclude & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementInclude const &a, StatementInclude const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementInclude const &a, StatementInclude const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementInclude)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementInclude() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto value() -> char const *;

    auto include_type() -> IncludeType;

    static auto acquire(clingo_ast_t *ast) -> StatementInclude { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *value,
                          IncludeType const &include_type) -> StatementInclude;

    friend auto c_cast(StatementInclude const &x) -> clingo_ast_t *;

  private:
    StatementInclude(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementInclude const &x) -> clingo_ast_t * { return x.ast_; }

class StatementProgram {
  public:
    // Note: for pybind
    StatementProgram() = default;

    StatementProgram(StatementProgram const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementProgram(StatementProgram &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementProgram const &x) -> StatementProgram & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementProgram &&x) noexcept -> StatementProgram & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementProgram const &a, StatementProgram const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementProgram const &a, StatementProgram const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementProgram)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementProgram() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto arguments() -> std::vector<char const *>;

    static auto acquire(clingo_ast_t *ast) -> StatementProgram { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name,
                          StringArray const &arguments) -> StatementProgram;

    friend auto c_cast(StatementProgram const &x) -> clingo_ast_t *;

  private:
    StatementProgram(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementProgram const &x) -> clingo_ast_t * { return x.ast_; }

class StatementConst {
  public:
    // Note: for pybind
    StatementConst() = default;

    StatementConst(StatementConst const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementConst(StatementConst &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementConst const &x) -> StatementConst & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementConst &&x) noexcept -> StatementConst & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementConst const &a, StatementConst const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementConst const &a, StatementConst const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementConst)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementConst() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto name() -> char const *;

    auto value() -> Term;

    auto const_type() -> ConstType;

    static auto acquire(clingo_ast_t *ast) -> StatementConst { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *name, Term const &value,
                          ConstType const &const_type) -> StatementConst;

    friend auto c_cast(StatementConst const &x) -> clingo_ast_t *;

  private:
    StatementConst(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementConst const &x) -> clingo_ast_t * { return x.ast_; }

class StatementComment {
  public:
    // Note: for pybind
    StatementComment() = default;

    StatementComment(StatementComment const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    StatementComment(StatementComment &&x) noexcept { std::swap(ast_, x.ast_); }

    auto operator=(StatementComment const &x) -> StatementComment & {
        clingo_ast_free(ast_);
        ast_ = nullptr;
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
        return *this;
    }

    auto operator=(StatementComment &&x) noexcept -> StatementComment & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(StatementComment const &a, StatementComment const &b) -> bool {
        return clingo_ast_equal(a.ast_, b.ast_);
    }

    friend auto operator<(StatementComment const &a, StatementComment const &b) -> bool {
        return clingo_ast_less_than(a.ast_, b.ast_);
    }

    CLINGO_CPP_TOTAL_ORDER(friend, StatementComment)

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    ~StatementComment() { clingo_ast_free(ast_); }

    auto location() -> clingo_location_t;

    auto value() -> char const *;

    auto comment_type() -> CommentType;

    static auto acquire(clingo_ast_t *ast) -> StatementComment { return {ast}; }

    void visit(py::handle visitor);

    static auto construct(Library &lib, clingo_location_t const &location, char const *value,
                          CommentType const &comment_type) -> StatementComment;

    friend auto c_cast(StatementComment const &x) -> clingo_ast_t *;

  private:
    StatementComment(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(StatementComment const &x) -> clingo_ast_t * { return x.ast_; }

auto construct_term(clingo_ast_t *ast) -> Term {
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_term_variable: {
            return TermVariable::acquire(ast);
        }
        case clingo_ast_type_term_symbolic: {
            return TermSymbolic::acquire(ast);
        }
        case clingo_ast_type_term_absolute: {
            return TermAbsolute::acquire(ast);
        }
        case clingo_ast_type_term_unary_operation: {
            return TermUnaryOperation::acquire(ast);
        }
        case clingo_ast_type_term_binary_operation: {
            return TermBinaryOperation::acquire(ast);
        }
        case clingo_ast_type_term_tuple: {
            return TermTuple::acquire(ast);
        }
        case clingo_ast_type_term_function: {
            return TermFunction::acquire(ast);
        }
    }
    clingo_ast_free(ast);
    throw std::runtime_error("unexpected ast type");
}

auto construct_term_array(clingo_ast_t **ast, size_t size) -> TermArray {
    TermArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(construct_term(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto Projection::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto Projection::construct(Library &lib, clingo_location_t const &location) -> Projection {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_projection, &res_, &location));
    return Projection::acquire(res_);
}

void Projection::visit(py::handle visitor) {}

auto construct_term_or_projection(clingo_ast_t *ast) -> TermOrProjection {
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_term_variable: {
            return TermVariable::acquire(ast);
        }
        case clingo_ast_type_term_symbolic: {
            return TermSymbolic::acquire(ast);
        }
        case clingo_ast_type_term_absolute: {
            return TermAbsolute::acquire(ast);
        }
        case clingo_ast_type_term_unary_operation: {
            return TermUnaryOperation::acquire(ast);
        }
        case clingo_ast_type_term_binary_operation: {
            return TermBinaryOperation::acquire(ast);
        }
        case clingo_ast_type_term_tuple: {
            return TermTuple::acquire(ast);
        }
        case clingo_ast_type_term_function: {
            return TermFunction::acquire(ast);
        }
        case clingo_ast_type_projection: {
            return Projection::acquire(ast);
        }
    }
    clingo_ast_free(ast);
    throw std::runtime_error("unexpected ast type");
}

auto construct_term_or_projection_array(clingo_ast_t **ast, size_t size) -> TermOrProjectionArray {
    TermOrProjectionArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(construct_term_or_projection(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto construct_argument_tuple_array(clingo_ast_t **ast, size_t size) -> ArgumentTupleArray {
    ArgumentTupleArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(ArgumentTuple::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto construct_term_or_argument_tuple(clingo_ast_t *ast) -> TermOrArgumentTuple {
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_term_variable: {
            return TermVariable::acquire(ast);
        }
        case clingo_ast_type_term_symbolic: {
            return TermSymbolic::acquire(ast);
        }
        case clingo_ast_type_term_absolute: {
            return TermAbsolute::acquire(ast);
        }
        case clingo_ast_type_term_unary_operation: {
            return TermUnaryOperation::acquire(ast);
        }
        case clingo_ast_type_term_binary_operation: {
            return TermBinaryOperation::acquire(ast);
        }
        case clingo_ast_type_term_tuple: {
            return TermTuple::acquire(ast);
        }
        case clingo_ast_type_term_function: {
            return TermFunction::acquire(ast);
        }
        case clingo_ast_type_argument_tuple: {
            return ArgumentTuple::acquire(ast);
        }
    }
    clingo_ast_free(ast);
    throw std::runtime_error("unexpected ast type");
}

auto construct_term_or_argument_tuple_array(clingo_ast_t **ast, size_t size) -> TermOrArgumentTupleArray {
    TermOrArgumentTupleArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(construct_term_or_argument_tuple(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto TermVariable::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TermVariable::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto TermVariable::anonymous() -> bool {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_anonymous, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret != 0;
}

auto TermVariable::construct(Library &lib, clingo_location_t const &location, char const *name, bool anonymous)
    -> TermVariable {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_term_variable, &res_, &location, name,
                                           static_cast<int>(anonymous)));
    return TermVariable::acquire(res_);
}

void TermVariable::visit(py::handle visitor) {}

auto TermSymbolic::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TermSymbolic::symbol() -> Symbol {
    clingo_symbol_t ret;
    if (!clingo_ast_attribute_get_symbol(ast_, clingo_ast_attribute_symbol, &ret)) {
        throw std::runtime_error("could not get symbol attribute");
    }
    return Symbol::acquire(ret);
}

auto TermSymbolic::construct(Library &lib, clingo_location_t const &location, Symbol const &symbol) -> TermSymbolic {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_term_symbolic, &res_, &location, symbol.handle()));
    return TermSymbolic::acquire(res_);
}

void TermSymbolic::visit(py::handle visitor) {}

auto TermAbsolute::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TermAbsolute::pool() -> TermArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_term_array(ast, size);
}

auto TermAbsolute::construct(Library &lib, clingo_location_t const &location, TermArray const &pool) -> TermAbsolute {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_term_absolute, &res_, &location, c_cast(pool).data(),
                                           pool.size()));
    return TermAbsolute::acquire(res_);
}

void TermAbsolute::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_term(cpy));
        });
    }
}

auto TermUnaryOperation::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TermUnaryOperation::operator_type() -> UnaryOperator {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_operator_type, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<UnaryOperator>(ret);
}

auto TermUnaryOperation::right() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto TermUnaryOperation::construct(Library &lib, clingo_location_t const &location, UnaryOperator const &operator_type,
                                   Term const &right) -> TermUnaryOperation {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_term_unary_operation, &res_, &location,
                                           static_cast<int>(operator_type), c_cast(right)));
    return TermUnaryOperation::acquire(res_);
}

void TermUnaryOperation::visit(py::handle visitor) { visitor(right()); }

auto TermBinaryOperation::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TermBinaryOperation::left() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto TermBinaryOperation::operator_type() -> BinaryOperator {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_operator_type, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<BinaryOperator>(ret);
}

auto TermBinaryOperation::right() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto TermBinaryOperation::construct(Library &lib, clingo_location_t const &location, Term const &left,
                                    BinaryOperator const &operator_type, Term const &right) -> TermBinaryOperation {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_term_binary_operation, &res_, &location, c_cast(left),
                                           static_cast<int>(operator_type), c_cast(right)));
    return TermBinaryOperation::acquire(res_);
}

void TermBinaryOperation::visit(py::handle visitor) {
    visitor(left());
    visitor(right());
}

auto TermTuple::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TermTuple::pool() -> TermOrArgumentTupleArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_term_or_argument_tuple_array(ast, size);
}

auto TermTuple::construct(Library &lib, clingo_location_t const &location, TermOrArgumentTupleArray const &pool)
    -> TermTuple {
    clingo_ast_t *res_;
    handle_error(
        lib, clingo_ast_construct(lib, clingo_ast_type_term_tuple, &res_, &location, c_cast(pool).data(), pool.size()));
    return TermTuple::acquire(res_);
}

void TermTuple::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_term_or_argument_tuple(cpy));
        });
    }
}

auto TermFunction::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TermFunction::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto TermFunction::pool() -> ArgumentTupleArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_argument_tuple_array(ast, size);
}

auto TermFunction::external() -> bool {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_external, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret != 0;
}

auto TermFunction::construct(Library &lib, clingo_location_t const &location, char const *name,
                             ArgumentTupleArray const &pool, bool external) -> TermFunction {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_term_function, &res_, &location, name,
                                           c_cast(pool).data(), pool.size(), static_cast<int>(external)));
    return TermFunction::acquire(res_);
}

void TermFunction::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(ArgumentTuple::acquire(cpy));
        });
    }
}

auto ArgumentTuple::arguments() -> TermOrProjectionArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_arguments, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_term_or_projection_array(ast, size);
}

auto ArgumentTuple::construct(Library &lib, TermOrProjectionArray const &arguments) -> ArgumentTuple {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_argument_tuple, &res_, c_cast(arguments).data(),
                                           arguments.size()));
    return ArgumentTuple::acquire(res_);
}

void ArgumentTuple::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_arguments, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_term_or_projection(cpy));
        });
    }
}

auto construct_literal(clingo_ast_t *ast) -> Literal {
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_literal_boolean: {
            return LiteralBoolean::acquire(ast);
        }
        case clingo_ast_type_literal_comparison: {
            return LiteralComparison::acquire(ast);
        }
        case clingo_ast_type_literal_symbolic: {
            return LiteralSymbolic::acquire(ast);
        }
    }
    clingo_ast_free(ast);
    throw std::runtime_error("unexpected ast type");
}

auto LeftGuard::term() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto LeftGuard::relation() -> Relation {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_relation, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<Relation>(ret);
}

auto LeftGuard::construct(Library &lib, Term const &term, Relation const &relation) -> LeftGuard {
    clingo_ast_t *res_;
    handle_error(
        lib, clingo_ast_construct(lib, clingo_ast_type_left_guard, &res_, c_cast(term), static_cast<int>(relation)));
    return LeftGuard::acquire(res_);
}

void LeftGuard::visit(py::handle visitor) { visitor(term()); }

auto RightGuard::relation() -> Relation {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_relation, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<Relation>(ret);
}

auto RightGuard::term() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto RightGuard::construct(Library &lib, Relation const &relation, Term const &term) -> RightGuard {
    clingo_ast_t *res_;
    handle_error(
        lib, clingo_ast_construct(lib, clingo_ast_type_right_guard, &res_, static_cast<int>(relation), c_cast(term)));
    return RightGuard::acquire(res_);
}

void RightGuard::visit(py::handle visitor) { visitor(term()); }

auto construct_right_guard_array(clingo_ast_t **ast, size_t size) -> RightGuardArray {
    RightGuardArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(RightGuard::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto LiteralBoolean::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto LiteralBoolean::sign() -> Sign {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<Sign>(ret);
}

auto LiteralBoolean::value() -> bool {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_value, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret != 0;
}

auto LiteralBoolean::construct(Library &lib, clingo_location_t const &location, Sign const &sign, bool value)
    -> LiteralBoolean {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_literal_boolean, &res_, &location,
                                           static_cast<int>(sign), static_cast<int>(value)));
    return LiteralBoolean::acquire(res_);
}

void LiteralBoolean::visit(py::handle visitor) {}

auto LiteralComparison::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto LiteralComparison::sign() -> Sign {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<Sign>(ret);
}

auto LiteralComparison::left() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto LiteralComparison::right() -> RightGuardArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_right, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_right_guard_array(ast, size);
}

auto LiteralComparison::construct(Library &lib, clingo_location_t const &location, Sign const &sign, Term const &left,
                                  RightGuardArray const &right) -> LiteralComparison {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_literal_comparison, &res_, &location,
                                           static_cast<int>(sign), c_cast(left), c_cast(right).data(), right.size()));
    return LiteralComparison::acquire(res_);
}

void LiteralComparison::visit(py::handle visitor) {
    visitor(left());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_right, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(RightGuard::acquire(cpy));
        });
    }
}

auto LiteralSymbolic::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto LiteralSymbolic::sign() -> Sign {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<Sign>(ret);
}

auto LiteralSymbolic::atom() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_atom, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto LiteralSymbolic::construct(Library &lib, clingo_location_t const &location, Sign const &sign, Term const &atom)
    -> LiteralSymbolic {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_literal_symbolic, &res_, &location,
                                           static_cast<int>(sign), c_cast(atom)));
    return LiteralSymbolic::acquire(res_);
}

void LiteralSymbolic::visit(py::handle visitor) { visitor(atom()); }

auto construct_theory_term(clingo_ast_t *ast) -> TheoryTerm {
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_theory_term_variable: {
            return TheoryTermVariable::acquire(ast);
        }
        case clingo_ast_type_theory_term_symbolic: {
            return TheoryTermSymbolic::acquire(ast);
        }
        case clingo_ast_type_theory_term_tuple: {
            return TheoryTermTuple::acquire(ast);
        }
        case clingo_ast_type_theory_term_function: {
            return TheoryTermFunction::acquire(ast);
        }
        case clingo_ast_type_theory_term_unparsed: {
            return TheoryTermUnparsed::acquire(ast);
        }
    }
    clingo_ast_free(ast);
    throw std::runtime_error("unexpected ast type");
}

auto construct_theory_term_array(clingo_ast_t **ast, size_t size) -> TheoryTermArray {
    TheoryTermArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(construct_theory_term(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto UnparsedElement::operators() -> std::vector<char const *> {
    size_t size;
    if (!clingo_ast_attribute_get_string_array(ast_, clingo_ast_attribute_operators, nullptr, &size)) {
        throw std::runtime_error("could not get string array attribute");
    }
    std::vector<char const *> ret;
    ret.resize(size, nullptr);
    if (!clingo_ast_attribute_get_string_array(ast_, clingo_ast_attribute_operators, ret.data(), &size)) {
        throw std::runtime_error("could not get string array attribute");
    }
    return ret;
}

auto UnparsedElement::term() -> TheoryTerm {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_theory_term(ast);
}

auto UnparsedElement::construct(Library &lib, StringArray const &operators, TheoryTerm const &term) -> UnparsedElement {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_unparsed_element, &res_, c_cast(operators).data(),
                                           operators.size(), c_cast(term)));
    return UnparsedElement::acquire(res_);
}

void UnparsedElement::visit(py::handle visitor) { visitor(term()); }

auto construct_unparsed_element_array(clingo_ast_t **ast, size_t size) -> UnparsedElementArray {
    UnparsedElementArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(UnparsedElement::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto TheoryTermVariable::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TheoryTermVariable::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto TheoryTermVariable::anonymous() -> bool {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_anonymous, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret != 0;
}

auto TheoryTermVariable::construct(Library &lib, clingo_location_t const &location, char const *name, bool anonymous)
    -> TheoryTermVariable {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_theory_term_variable, &res_, &location, name,
                                           static_cast<int>(anonymous)));
    return TheoryTermVariable::acquire(res_);
}

void TheoryTermVariable::visit(py::handle visitor) {}

auto TheoryTermSymbolic::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TheoryTermSymbolic::symbol() -> Symbol {
    clingo_symbol_t ret;
    if (!clingo_ast_attribute_get_symbol(ast_, clingo_ast_attribute_symbol, &ret)) {
        throw std::runtime_error("could not get symbol attribute");
    }
    return Symbol::acquire(ret);
}

auto TheoryTermSymbolic::construct(Library &lib, clingo_location_t const &location, Symbol const &symbol)
    -> TheoryTermSymbolic {
    clingo_ast_t *res_;
    handle_error(lib,
                 clingo_ast_construct(lib, clingo_ast_type_theory_term_symbolic, &res_, &location, symbol.handle()));
    return TheoryTermSymbolic::acquire(res_);
}

void TheoryTermSymbolic::visit(py::handle visitor) {}

auto TheoryTermTuple::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TheoryTermTuple::tuple_type() -> TheoryTupleType {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_tuple_type, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<TheoryTupleType>(ret);
}

auto TheoryTermTuple::arguments() -> TheoryTermArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_arguments, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_theory_term_array(ast, size);
}

auto TheoryTermTuple::construct(Library &lib, clingo_location_t const &location, TheoryTupleType const &tuple_type,
                                TheoryTermArray const &arguments) -> TheoryTermTuple {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_theory_term_tuple, &res_, &location,
                                           static_cast<int>(tuple_type), c_cast(arguments).data(), arguments.size()));
    return TheoryTermTuple::acquire(res_);
}

void TheoryTermTuple::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_arguments, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_theory_term(cpy));
        });
    }
}

auto TheoryTermFunction::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TheoryTermFunction::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto TheoryTermFunction::arguments() -> TheoryTermArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_arguments, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_theory_term_array(ast, size);
}

auto TheoryTermFunction::construct(Library &lib, clingo_location_t const &location, char const *name,
                                   TheoryTermArray const &arguments) -> TheoryTermFunction {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_theory_term_function, &res_, &location, name,
                                           c_cast(arguments).data(), arguments.size()));
    return TheoryTermFunction::acquire(res_);
}

void TheoryTermFunction::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_arguments, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_theory_term(cpy));
        });
    }
}

auto TheoryTermUnparsed::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TheoryTermUnparsed::elements() -> UnparsedElementArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_unparsed_element_array(ast, size);
}

auto TheoryTermUnparsed::construct(Library &lib, clingo_location_t const &location,
                                   UnparsedElementArray const &elements) -> TheoryTermUnparsed {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_theory_term_unparsed, &res_, &location,
                                           c_cast(elements).data(), elements.size()));
    return TheoryTermUnparsed::acquire(res_);
}

void TheoryTermUnparsed::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(UnparsedElement::acquire(cpy));
        });
    }
}

auto TheoryRightGuard::theory_operator() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_theory_operator, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto TheoryRightGuard::term() -> TheoryTerm {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_theory_term(ast);
}

auto TheoryRightGuard::construct(Library &lib, char const *theory_operator, TheoryTerm const &term)
    -> TheoryRightGuard {
    clingo_ast_t *res_;
    handle_error(lib,
                 clingo_ast_construct(lib, clingo_ast_type_theory_right_guard, &res_, theory_operator, c_cast(term)));
    return TheoryRightGuard::acquire(res_);
}

void TheoryRightGuard::visit(py::handle visitor) { visitor(term()); }

auto construct_literal_array(clingo_ast_t **ast, size_t size) -> LiteralArray {
    LiteralArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(construct_literal(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto SetAggregateElement::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto SetAggregateElement::literal() -> Literal {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_literal(ast);
}

auto SetAggregateElement::condition() -> LiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_literal_array(ast, size);
}

auto SetAggregateElement::construct(Library &lib, clingo_location_t const &location, Literal const &literal,
                                    LiteralArray const &condition) -> SetAggregateElement {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_set_aggregate_element, &res_, &location,
                                           c_cast(literal), c_cast(condition).data(), condition.size()));
    return SetAggregateElement::acquire(res_);
}

void SetAggregateElement::visit(py::handle visitor) {
    visitor(literal());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_literal(cpy));
        });
    }
}

auto construct_set_aggregate_element_array(clingo_ast_t **ast, size_t size) -> SetAggregateElementArray {
    SetAggregateElementArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(SetAggregateElement::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto BodyAggregateElement::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto BodyAggregateElement::tuple() -> TermArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_tuple, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_term_array(ast, size);
}

auto BodyAggregateElement::condition() -> LiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_literal_array(ast, size);
}

auto BodyAggregateElement::construct(Library &lib, clingo_location_t const &location, TermArray const &tuple,
                                     LiteralArray const &condition) -> BodyAggregateElement {
    clingo_ast_t *res_;
    handle_error(lib,
                 clingo_ast_construct(lib, clingo_ast_type_body_aggregate_element, &res_, &location,
                                      c_cast(tuple).data(), tuple.size(), c_cast(condition).data(), condition.size()));
    return BodyAggregateElement::acquire(res_);
}

void BodyAggregateElement::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_tuple, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_term(cpy));
        });
    }
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_literal(cpy));
        });
    }
}

auto construct_body_aggregate_element_array(clingo_ast_t **ast, size_t size) -> BodyAggregateElementArray {
    BodyAggregateElementArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(BodyAggregateElement::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto TheoryAtomElement::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TheoryAtomElement::tuple() -> TheoryTermArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_tuple, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_theory_term_array(ast, size);
}

auto TheoryAtomElement::condition() -> LiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_literal_array(ast, size);
}

auto TheoryAtomElement::construct(Library &lib, clingo_location_t const &location, TheoryTermArray const &tuple,
                                  LiteralArray const &condition) -> TheoryAtomElement {
    clingo_ast_t *res_;
    handle_error(lib,
                 clingo_ast_construct(lib, clingo_ast_type_theory_atom_element, &res_, &location, c_cast(tuple).data(),
                                      tuple.size(), c_cast(condition).data(), condition.size()));
    return TheoryAtomElement::acquire(res_);
}

void TheoryAtomElement::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_tuple, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_theory_term(cpy));
        });
    }
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_literal(cpy));
        });
    }
}

auto construct_theory_atom_element_array(clingo_ast_t **ast, size_t size) -> TheoryAtomElementArray {
    TheoryAtomElementArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(TheoryAtomElement::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto construct_body_literal(clingo_ast_t *ast) -> BodyLiteral {
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_body_simple_literal: {
            return BodySimpleLiteral::acquire(ast);
        }
        case clingo_ast_type_body_aggregate: {
            return BodyAggregate::acquire(ast);
        }
        case clingo_ast_type_body_set_aggregate: {
            return BodySetAggregate::acquire(ast);
        }
        case clingo_ast_type_body_theory_atom: {
            return BodyTheoryAtom::acquire(ast);
        }
        case clingo_ast_type_body_conditional_literal: {
            return BodyConditionalLiteral::acquire(ast);
        }
    }
    clingo_ast_free(ast);
    throw std::runtime_error("unexpected ast type");
}

auto construct_body_literal_array(clingo_ast_t **ast, size_t size) -> BodyLiteralArray {
    BodyLiteralArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(construct_body_literal(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto BodySimpleLiteral::literal() -> Literal {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_literal(ast);
}

auto BodySimpleLiteral::construct(Library &lib, Literal const &literal) -> BodySimpleLiteral {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_body_simple_literal, &res_, c_cast(literal)));
    return BodySimpleLiteral::acquire(res_);
}

void BodySimpleLiteral::visit(py::handle visitor) { visitor(literal()); }

auto BodyAggregate::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto BodyAggregate::sign() -> Sign {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<Sign>(ret);
}

auto BodyAggregate::left() -> OptionalLeftGuard {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // left_guard
        return LeftGuard::acquire(ast);
    }
    return std::nullopt;
}

auto BodyAggregate::function() -> AggregateFunction {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_function, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<AggregateFunction>(ret);
}

auto BodyAggregate::elements() -> BodyAggregateElementArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_body_aggregate_element_array(ast, size);
}

auto BodyAggregate::right() -> OptionalRightGuard {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // right_guard
        return RightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto BodyAggregate::construct(Library &lib, clingo_location_t const &location, Sign const &sign,
                              OptionalLeftGuard const &left, AggregateFunction const &function,
                              BodyAggregateElementArray const &elements, OptionalRightGuard const &right)
    -> BodyAggregate {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_body_aggregate, &res_, &location,
                                           static_cast<int>(sign), c_cast(left), static_cast<int>(function),
                                           c_cast(elements).data(), elements.size(), c_cast(right)));
    return BodyAggregate::acquire(res_);
}

void BodyAggregate::visit(py::handle visitor) {
    if (auto opt = left()) {
        visitor(*opt);
    }
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(BodyAggregateElement::acquire(cpy));
        });
    }
    if (auto opt = right()) {
        visitor(*opt);
    }
}

auto BodySetAggregate::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto BodySetAggregate::sign() -> Sign {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<Sign>(ret);
}

auto BodySetAggregate::left() -> OptionalLeftGuard {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // left_guard
        return LeftGuard::acquire(ast);
    }
    return std::nullopt;
}

auto BodySetAggregate::elements() -> SetAggregateElementArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_set_aggregate_element_array(ast, size);
}

auto BodySetAggregate::right() -> OptionalRightGuard {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // right_guard
        return RightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto BodySetAggregate::construct(Library &lib, clingo_location_t const &location, Sign const &sign,
                                 OptionalLeftGuard const &left, SetAggregateElementArray const &elements,
                                 OptionalRightGuard const &right) -> BodySetAggregate {
    clingo_ast_t *res_;
    handle_error(lib,
                 clingo_ast_construct(lib, clingo_ast_type_body_set_aggregate, &res_, &location, static_cast<int>(sign),
                                      c_cast(left), c_cast(elements).data(), elements.size(), c_cast(right)));
    return BodySetAggregate::acquire(res_);
}

void BodySetAggregate::visit(py::handle visitor) {
    if (auto opt = left()) {
        visitor(*opt);
    }
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(SetAggregateElement::acquire(cpy));
        });
    }
    if (auto opt = right()) {
        visitor(*opt);
    }
}

auto BodyTheoryAtom::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto BodyTheoryAtom::sign() -> Sign {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<Sign>(ret);
}

auto BodyTheoryAtom::name() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_name, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto BodyTheoryAtom::elements() -> TheoryAtomElementArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_theory_atom_element_array(ast, size);
}

auto BodyTheoryAtom::right() -> OptionalTheoryRightGuard {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // theory_right_guard
        return TheoryRightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto BodyTheoryAtom::construct(Library &lib, clingo_location_t const &location, Sign const &sign, Term const &name,
                               TheoryAtomElementArray const &elements, OptionalTheoryRightGuard const &right)
    -> BodyTheoryAtom {
    clingo_ast_t *res_;
    handle_error(lib,
                 clingo_ast_construct(lib, clingo_ast_type_body_theory_atom, &res_, &location, static_cast<int>(sign),
                                      c_cast(name), c_cast(elements).data(), elements.size(), c_cast(right)));
    return BodyTheoryAtom::acquire(res_);
}

void BodyTheoryAtom::visit(py::handle visitor) {
    visitor(name());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(TheoryAtomElement::acquire(cpy));
        });
    }
    if (auto opt = right()) {
        visitor(*opt);
    }
}

auto BodyConditionalLiteral::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto BodyConditionalLiteral::literal() -> Literal {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_literal(ast);
}

auto BodyConditionalLiteral::condition() -> LiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_literal_array(ast, size);
}

auto BodyConditionalLiteral::construct(Library &lib, clingo_location_t const &location, Literal const &literal,
                                       LiteralArray const &condition) -> BodyConditionalLiteral {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_body_conditional_literal, &res_, &location,
                                           c_cast(literal), c_cast(condition).data(), condition.size()));
    return BodyConditionalLiteral::acquire(res_);
}

void BodyConditionalLiteral::visit(py::handle visitor) {
    visitor(literal());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_literal(cpy));
        });
    }
}

auto HeadConditionalLiteral::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto HeadConditionalLiteral::literal() -> Literal {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_literal(ast);
}

auto HeadConditionalLiteral::condition() -> LiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_literal_array(ast, size);
}

auto HeadConditionalLiteral::construct(Library &lib, clingo_location_t const &location, Literal const &literal,
                                       LiteralArray const &condition) -> HeadConditionalLiteral {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_head_conditional_literal, &res_, &location,
                                           c_cast(literal), c_cast(condition).data(), condition.size()));
    return HeadConditionalLiteral::acquire(res_);
}

void HeadConditionalLiteral::visit(py::handle visitor) {
    visitor(literal());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_literal(cpy));
        });
    }
}

auto construct_disjunction_element(clingo_ast_t *ast) -> DisjunctionElement {
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_literal_boolean: {
            return LiteralBoolean::acquire(ast);
        }
        case clingo_ast_type_literal_comparison: {
            return LiteralComparison::acquire(ast);
        }
        case clingo_ast_type_literal_symbolic: {
            return LiteralSymbolic::acquire(ast);
        }
        case clingo_ast_type_head_conditional_literal: {
            return HeadConditionalLiteral::acquire(ast);
        }
    }
    clingo_ast_free(ast);
    throw std::runtime_error("unexpected ast type");
}

auto construct_disjunction_element_array(clingo_ast_t **ast, size_t size) -> DisjunctionElementArray {
    DisjunctionElementArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(construct_disjunction_element(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto HeadAggregateElement::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto HeadAggregateElement::tuple() -> TermArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_tuple, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_term_array(ast, size);
}

auto HeadAggregateElement::literal() -> Literal {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_literal(ast);
}

auto HeadAggregateElement::condition() -> LiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_literal_array(ast, size);
}

auto HeadAggregateElement::construct(Library &lib, clingo_location_t const &location, TermArray const &tuple,
                                     Literal const &literal, LiteralArray const &condition) -> HeadAggregateElement {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_head_aggregate_element, &res_, &location,
                                           c_cast(tuple).data(), tuple.size(), c_cast(literal),
                                           c_cast(condition).data(), condition.size()));
    return HeadAggregateElement::acquire(res_);
}

void HeadAggregateElement::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_tuple, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_term(cpy));
        });
    }
    visitor(literal());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_literal(cpy));
        });
    }
}

auto construct_head_aggregate_element_array(clingo_ast_t **ast, size_t size) -> HeadAggregateElementArray {
    HeadAggregateElementArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(HeadAggregateElement::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto construct_head_literal(clingo_ast_t *ast) -> HeadLiteral {
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_head_simple_literal: {
            return HeadSimpleLiteral::acquire(ast);
        }
        case clingo_ast_type_head_aggregate: {
            return HeadAggregate::acquire(ast);
        }
        case clingo_ast_type_head_set_aggregate: {
            return HeadSetAggregate::acquire(ast);
        }
        case clingo_ast_type_head_theory_atom: {
            return HeadTheoryAtom::acquire(ast);
        }
        case clingo_ast_type_head_disjunction: {
            return HeadDisjunction::acquire(ast);
        }
    }
    clingo_ast_free(ast);
    throw std::runtime_error("unexpected ast type");
}

auto HeadSimpleLiteral::literal() -> Literal {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_literal(ast);
}

auto HeadSimpleLiteral::construct(Library &lib, Literal const &literal) -> HeadSimpleLiteral {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_head_simple_literal, &res_, c_cast(literal)));
    return HeadSimpleLiteral::acquire(res_);
}

void HeadSimpleLiteral::visit(py::handle visitor) { visitor(literal()); }

auto HeadAggregate::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto HeadAggregate::left() -> OptionalLeftGuard {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // left_guard
        return LeftGuard::acquire(ast);
    }
    return std::nullopt;
}

auto HeadAggregate::function() -> AggregateFunction {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_function, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<AggregateFunction>(ret);
}

auto HeadAggregate::elements() -> HeadAggregateElementArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_head_aggregate_element_array(ast, size);
}

auto HeadAggregate::right() -> OptionalRightGuard {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // right_guard
        return RightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto HeadAggregate::construct(Library &lib, clingo_location_t const &location, OptionalLeftGuard const &left,
                              AggregateFunction const &function, HeadAggregateElementArray const &elements,
                              OptionalRightGuard const &right) -> HeadAggregate {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_head_aggregate, &res_, &location, c_cast(left),
                                           static_cast<int>(function), c_cast(elements).data(), elements.size(),
                                           c_cast(right)));
    return HeadAggregate::acquire(res_);
}

void HeadAggregate::visit(py::handle visitor) {
    if (auto opt = left()) {
        visitor(*opt);
    }
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(HeadAggregateElement::acquire(cpy));
        });
    }
    if (auto opt = right()) {
        visitor(*opt);
    }
}

auto HeadSetAggregate::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto HeadSetAggregate::left() -> OptionalLeftGuard {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // left_guard
        return LeftGuard::acquire(ast);
    }
    return std::nullopt;
}

auto HeadSetAggregate::elements() -> SetAggregateElementArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_set_aggregate_element_array(ast, size);
}

auto HeadSetAggregate::right() -> OptionalRightGuard {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // right_guard
        return RightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto HeadSetAggregate::construct(Library &lib, clingo_location_t const &location, OptionalLeftGuard const &left,
                                 SetAggregateElementArray const &elements, OptionalRightGuard const &right)
    -> HeadSetAggregate {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_head_set_aggregate, &res_, &location, c_cast(left),
                                           c_cast(elements).data(), elements.size(), c_cast(right)));
    return HeadSetAggregate::acquire(res_);
}

void HeadSetAggregate::visit(py::handle visitor) {
    if (auto opt = left()) {
        visitor(*opt);
    }
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(SetAggregateElement::acquire(cpy));
        });
    }
    if (auto opt = right()) {
        visitor(*opt);
    }
}

auto HeadTheoryAtom::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto HeadTheoryAtom::name() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_name, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto HeadTheoryAtom::elements() -> TheoryAtomElementArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_theory_atom_element_array(ast, size);
}

auto HeadTheoryAtom::right() -> OptionalTheoryRightGuard {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // theory_right_guard
        return TheoryRightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto HeadTheoryAtom::construct(Library &lib, clingo_location_t const &location, Term const &name,
                               TheoryAtomElementArray const &elements, OptionalTheoryRightGuard const &right)
    -> HeadTheoryAtom {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_head_theory_atom, &res_, &location, c_cast(name),
                                           c_cast(elements).data(), elements.size(), c_cast(right)));
    return HeadTheoryAtom::acquire(res_);
}

void HeadTheoryAtom::visit(py::handle visitor) {
    visitor(name());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(TheoryAtomElement::acquire(cpy));
        });
    }
    if (auto opt = right()) {
        visitor(*opt);
    }
}

auto HeadDisjunction::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto HeadDisjunction::elements() -> DisjunctionElementArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_disjunction_element_array(ast, size);
}

auto HeadDisjunction::construct(Library &lib, clingo_location_t const &location,
                                DisjunctionElementArray const &elements) -> HeadDisjunction {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_head_disjunction, &res_, &location,
                                           c_cast(elements).data(), elements.size()));
    return HeadDisjunction::acquire(res_);
}

void HeadDisjunction::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_disjunction_element(cpy));
        });
    }
}

auto TheoryOperatorDefinition::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TheoryOperatorDefinition::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto TheoryOperatorDefinition::priority() -> int {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_priority, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret;
}

auto TheoryOperatorDefinition::operator_type() -> TheoryOperatorType {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_operator_type, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<TheoryOperatorType>(ret);
}

auto TheoryOperatorDefinition::construct(Library &lib, clingo_location_t const &location, char const *name,
                                         int priority, TheoryOperatorType const &operator_type)
    -> TheoryOperatorDefinition {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_theory_operator_definition, &res_, &location, name,
                                           priority, static_cast<int>(operator_type)));
    return TheoryOperatorDefinition::acquire(res_);
}

void TheoryOperatorDefinition::visit(py::handle visitor) {}

auto construct_theory_operator_definition_array(clingo_ast_t **ast, size_t size) -> TheoryOperatorDefinitionArray {
    TheoryOperatorDefinitionArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(TheoryOperatorDefinition::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto TheoryTermDefinition::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TheoryTermDefinition::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto TheoryTermDefinition::operators() -> TheoryOperatorDefinitionArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_operators, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_theory_operator_definition_array(ast, size);
}

auto TheoryTermDefinition::construct(Library &lib, clingo_location_t const &location, char const *name,
                                     TheoryOperatorDefinitionArray const &operators) -> TheoryTermDefinition {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_theory_term_definition, &res_, &location, name,
                                           c_cast(operators).data(), operators.size()));
    return TheoryTermDefinition::acquire(res_);
}

void TheoryTermDefinition::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_operators, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(TheoryOperatorDefinition::acquire(cpy));
        });
    }
}

auto construct_theory_term_definition_array(clingo_ast_t **ast, size_t size) -> TheoryTermDefinitionArray {
    TheoryTermDefinitionArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(TheoryTermDefinition::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto TheoryGuardDefinition::operators() -> std::vector<char const *> {
    size_t size;
    if (!clingo_ast_attribute_get_string_array(ast_, clingo_ast_attribute_operators, nullptr, &size)) {
        throw std::runtime_error("could not get string array attribute");
    }
    std::vector<char const *> ret;
    ret.resize(size, nullptr);
    if (!clingo_ast_attribute_get_string_array(ast_, clingo_ast_attribute_operators, ret.data(), &size)) {
        throw std::runtime_error("could not get string array attribute");
    }
    return ret;
}

auto TheoryGuardDefinition::term() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_term, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto TheoryGuardDefinition::construct(Library &lib, StringArray const &operators, char const *term)
    -> TheoryGuardDefinition {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_theory_guard_definition, &res_,
                                           c_cast(operators).data(), operators.size(), term));
    return TheoryGuardDefinition::acquire(res_);
}

void TheoryGuardDefinition::visit(py::handle visitor) {}

auto TheoryAtomDefinition::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto TheoryAtomDefinition::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto TheoryAtomDefinition::arity() -> int {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_arity, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret;
}

auto TheoryAtomDefinition::term() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_term, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto TheoryAtomDefinition::guard() -> OptionalTheoryGuardDefinition {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_guard, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // theory_guard_definition
        return TheoryGuardDefinition::acquire(ast);
    }
    return std::nullopt;
}

auto TheoryAtomDefinition::atom_type() -> TheoryAtomType {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_atom_type, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<TheoryAtomType>(ret);
}

auto TheoryAtomDefinition::construct(Library &lib, clingo_location_t const &location, char const *name, int arity,
                                     char const *term, OptionalTheoryGuardDefinition const &guard,
                                     TheoryAtomType const &atom_type) -> TheoryAtomDefinition {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_theory_atom_definition, &res_, &location, name, arity,
                                           term, c_cast(guard), static_cast<int>(atom_type)));
    return TheoryAtomDefinition::acquire(res_);
}

void TheoryAtomDefinition::visit(py::handle visitor) {
    if (auto opt = guard()) {
        visitor(*opt);
    }
}

auto construct_theory_atom_definition_array(clingo_ast_t **ast, size_t size) -> TheoryAtomDefinitionArray {
    TheoryAtomDefinitionArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(TheoryAtomDefinition::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto OptimizeTuple::weight() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_weight, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto OptimizeTuple::priority() -> OptionalTerm {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_priority, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // term
        return construct_term(ast);
    }
    return std::nullopt;
}

auto OptimizeTuple::terms() -> TermArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_terms, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_term_array(ast, size);
}

auto OptimizeTuple::construct(Library &lib, Term const &weight, OptionalTerm const &priority, TermArray const &terms)
    -> OptimizeTuple {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_optimize_tuple, &res_, c_cast(weight), c_cast(priority),
                                           c_cast(terms).data(), terms.size()));
    return OptimizeTuple::acquire(res_);
}

void OptimizeTuple::visit(py::handle visitor) {
    visitor(weight());
    if (auto opt = priority()) {
        visitor(*opt);
    }
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_terms, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_term(cpy));
        });
    }
}

auto OptimizeElement::tuple() -> OptimizeTuple {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_tuple, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return OptimizeTuple::acquire(ast);
}

auto OptimizeElement::condition() -> LiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_literal_array(ast, size);
}

auto OptimizeElement::construct(Library &lib, OptimizeTuple const &tuple, LiteralArray const &condition)
    -> OptimizeElement {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_optimize_element, &res_, c_cast(tuple),
                                           c_cast(condition).data(), condition.size()));
    return OptimizeElement::acquire(res_);
}

void OptimizeElement::visit(py::handle visitor) {
    visitor(tuple());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_literal(cpy));
        });
    }
}

auto construct_optimize_element_array(clingo_ast_t **ast, size_t size) -> OptimizeElementArray {
    OptimizeElementArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(OptimizeElement::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto Edge::u() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_u, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto Edge::v() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_v, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto Edge::construct(Library &lib, Term const &u, Term const &v) -> Edge {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_edge, &res_, c_cast(u), c_cast(v)));
    return Edge::acquire(res_);
}

void Edge::visit(py::handle visitor) {
    visitor(u());
    visitor(v());
}

auto construct_edge_array(clingo_ast_t **ast, size_t size) -> EdgeArray {
    EdgeArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(Edge::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto construct_statement(clingo_ast_t *ast) -> Statement {
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_statement_rule: {
            return StatementRule::acquire(ast);
        }
        case clingo_ast_type_statement_theory: {
            return StatementTheory::acquire(ast);
        }
        case clingo_ast_type_statement_optimize: {
            return StatementOptimize::acquire(ast);
        }
        case clingo_ast_type_statement_weak_constraint: {
            return StatementWeakConstraint::acquire(ast);
        }
        case clingo_ast_type_statement_show: {
            return StatementShow::acquire(ast);
        }
        case clingo_ast_type_statement_show_signature: {
            return StatementShowSignature::acquire(ast);
        }
        case clingo_ast_type_statement_project: {
            return StatementProject::acquire(ast);
        }
        case clingo_ast_type_statement_project_signature: {
            return StatementProjectSignature::acquire(ast);
        }
        case clingo_ast_type_statement_defined: {
            return StatementDefined::acquire(ast);
        }
        case clingo_ast_type_statement_external: {
            return StatementExternal::acquire(ast);
        }
        case clingo_ast_type_statement_edge: {
            return StatementEdge::acquire(ast);
        }
        case clingo_ast_type_statement_heuristic: {
            return StatementHeuristic::acquire(ast);
        }
        case clingo_ast_type_statement_script: {
            return StatementScript::acquire(ast);
        }
        case clingo_ast_type_statement_include: {
            return StatementInclude::acquire(ast);
        }
        case clingo_ast_type_statement_program: {
            return StatementProgram::acquire(ast);
        }
        case clingo_ast_type_statement_const: {
            return StatementConst::acquire(ast);
        }
        case clingo_ast_type_statement_comment: {
            return StatementComment::acquire(ast);
        }
    }
    clingo_ast_free(ast);
    throw std::runtime_error("unexpected ast type");
}

auto StatementRule::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementRule::head() -> HeadLiteral {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_head, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_head_literal(ast);
}

auto StatementRule::body() -> BodyLiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_body_literal_array(ast, size);
}

auto StatementRule::construct(Library &lib, clingo_location_t const &location, HeadLiteral const &head,
                              BodyLiteralArray const &body) -> StatementRule {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_rule, &res_, &location, c_cast(head),
                                           c_cast(body).data(), body.size()));
    return StatementRule::acquire(res_);
}

void StatementRule::visit(py::handle visitor) {
    visitor(head());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_body_literal(cpy));
        });
    }
}

auto StatementTheory::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementTheory::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto StatementTheory::terms() -> TheoryTermDefinitionArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_terms, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_theory_term_definition_array(ast, size);
}

auto StatementTheory::atoms() -> TheoryAtomDefinitionArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_atoms, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_theory_atom_definition_array(ast, size);
}

auto StatementTheory::construct(Library &lib, clingo_location_t const &location, char const *name,
                                TheoryTermDefinitionArray const &terms, TheoryAtomDefinitionArray const &atoms)
    -> StatementTheory {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_theory, &res_, &location, name,
                                           c_cast(terms).data(), terms.size(), c_cast(atoms).data(), atoms.size()));
    return StatementTheory::acquire(res_);
}

void StatementTheory::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_terms, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(TheoryTermDefinition::acquire(cpy));
        });
    }
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_atoms, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(TheoryAtomDefinition::acquire(cpy));
        });
    }
}

auto StatementOptimize::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementOptimize::elements() -> OptimizeElementArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_optimize_element_array(ast, size);
}

auto StatementOptimize::optimize_type() -> OptimizeType {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_optimize_type, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<OptimizeType>(ret);
}

auto StatementOptimize::construct(Library &lib, clingo_location_t const &location, OptimizeElementArray const &elements,
                                  OptimizeType const &optimize_type) -> StatementOptimize {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_optimize, &res_, &location,
                                           c_cast(elements).data(), elements.size(), static_cast<int>(optimize_type)));
    return StatementOptimize::acquire(res_);
}

void StatementOptimize::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(OptimizeElement::acquire(cpy));
        });
    }
}

auto StatementWeakConstraint::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementWeakConstraint::body() -> BodyLiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_body_literal_array(ast, size);
}

auto StatementWeakConstraint::tuple() -> OptimizeTuple {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_tuple, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return OptimizeTuple::acquire(ast);
}

auto StatementWeakConstraint::construct(Library &lib, clingo_location_t const &location, BodyLiteralArray const &body,
                                        OptimizeTuple const &tuple) -> StatementWeakConstraint {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_weak_constraint, &res_, &location,
                                           c_cast(body).data(), body.size(), c_cast(tuple)));
    return StatementWeakConstraint::acquire(res_);
}

void StatementWeakConstraint::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_body_literal(cpy));
        });
    }
    visitor(tuple());
}

auto StatementShow::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementShow::term() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto StatementShow::body() -> BodyLiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_body_literal_array(ast, size);
}

auto StatementShow::construct(Library &lib, clingo_location_t const &location, Term const &term,
                              BodyLiteralArray const &body) -> StatementShow {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_show, &res_, &location, c_cast(term),
                                           c_cast(body).data(), body.size()));
    return StatementShow::acquire(res_);
}

void StatementShow::visit(py::handle visitor) {
    visitor(term());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_body_literal(cpy));
        });
    }
}

auto StatementShowSignature::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementShowSignature::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto StatementShowSignature::arity() -> int {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_arity, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret;
}

auto StatementShowSignature::sign() -> bool {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret != 0;
}

auto StatementShowSignature::construct(Library &lib, clingo_location_t const &location, char const *name, int arity,
                                       bool sign) -> StatementShowSignature {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_show_signature, &res_, &location, name, arity,
                                           static_cast<int>(sign)));
    return StatementShowSignature::acquire(res_);
}

void StatementShowSignature::visit(py::handle visitor) {}

auto StatementProject::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementProject::atom() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_atom, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto StatementProject::body() -> BodyLiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_body_literal_array(ast, size);
}

auto StatementProject::construct(Library &lib, clingo_location_t const &location, Term const &atom,
                                 BodyLiteralArray const &body) -> StatementProject {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_project, &res_, &location, c_cast(atom),
                                           c_cast(body).data(), body.size()));
    return StatementProject::acquire(res_);
}

void StatementProject::visit(py::handle visitor) {
    visitor(atom());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_body_literal(cpy));
        });
    }
}

auto StatementProjectSignature::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementProjectSignature::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto StatementProjectSignature::arity() -> int {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_arity, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret;
}

auto StatementProjectSignature::sign() -> bool {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret != 0;
}

auto StatementProjectSignature::construct(Library &lib, clingo_location_t const &location, char const *name, int arity,
                                          bool sign) -> StatementProjectSignature {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_project_signature, &res_, &location, name,
                                           arity, static_cast<int>(sign)));
    return StatementProjectSignature::acquire(res_);
}

void StatementProjectSignature::visit(py::handle visitor) {}

auto StatementDefined::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementDefined::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto StatementDefined::arity() -> int {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_arity, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret;
}

auto StatementDefined::sign() -> bool {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret != 0;
}

auto StatementDefined::construct(Library &lib, clingo_location_t const &location, char const *name, int arity,
                                 bool sign) -> StatementDefined {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_defined, &res_, &location, name, arity,
                                           static_cast<int>(sign)));
    return StatementDefined::acquire(res_);
}

void StatementDefined::visit(py::handle visitor) {}

auto StatementExternal::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementExternal::atom() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_atom, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto StatementExternal::body() -> BodyLiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_body_literal_array(ast, size);
}

auto StatementExternal::external_type() -> OptionalTerm {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_external_type, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // term
        return construct_term(ast);
    }
    return std::nullopt;
}

auto StatementExternal::construct(Library &lib, clingo_location_t const &location, Term const &atom,
                                  BodyLiteralArray const &body, OptionalTerm const &external_type)
    -> StatementExternal {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_external, &res_, &location, c_cast(atom),
                                           c_cast(body).data(), body.size(), c_cast(external_type)));
    return StatementExternal::acquire(res_);
}

void StatementExternal::visit(py::handle visitor) {
    visitor(atom());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_body_literal(cpy));
        });
    }
    if (auto opt = external_type()) {
        visitor(*opt);
    }
}

auto StatementEdge::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementEdge::pool() -> EdgeArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_edge_array(ast, size);
}

auto StatementEdge::body() -> BodyLiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_body_literal_array(ast, size);
}

auto StatementEdge::construct(Library &lib, clingo_location_t const &location, EdgeArray const &pool,
                              BodyLiteralArray const &body) -> StatementEdge {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_edge, &res_, &location, c_cast(pool).data(),
                                           pool.size(), c_cast(body).data(), body.size()));
    return StatementEdge::acquire(res_);
}

void StatementEdge::visit(py::handle visitor) {
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(Edge::acquire(cpy));
        });
    }
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_body_literal(cpy));
        });
    }
}

auto StatementHeuristic::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementHeuristic::atom() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_atom, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto StatementHeuristic::body() -> BodyLiteralArray {
    clingo_ast_t **ast;
    size_t size;
    if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size)) {
        throw std::runtime_error("could not get ast array attribute");
    }
    return construct_body_literal_array(ast, size);
}

auto StatementHeuristic::weight() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_weight, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto StatementHeuristic::modifier() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_modifier, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto StatementHeuristic::priority() -> OptionalTerm {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_priority, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    if (ast != nullptr) {
        // term
        return construct_term(ast);
    }
    return std::nullopt;
}

auto StatementHeuristic::construct(Library &lib, clingo_location_t const &location, Term const &atom,
                                   BodyLiteralArray const &body, Term const &weight, Term const &modifier,
                                   OptionalTerm const &priority) -> StatementHeuristic {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_heuristic, &res_, &location, c_cast(atom),
                                           c_cast(body).data(), body.size(), c_cast(weight), c_cast(modifier),
                                           c_cast(priority)));
    return StatementHeuristic::acquire(res_);
}

void StatementHeuristic::visit(py::handle visitor) {
    visitor(atom());
    {
        struct Array {
            ~Array() { clingo_ast_array_free(begin, size); }
            clingo_ast_t **begin = nullptr;
            size_t size = 0;
        };
        auto array = Array{};
        if (!clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &array.begin, &array.size)) {
            throw std::runtime_error("could not get ast array attribute");
        }
        std::for_each_n(array.begin, array.size, [&visitor](auto *&ast) {
            auto *cpy = ast;
            ast = nullptr;
            visitor(construct_body_literal(cpy));
        });
    }
    visitor(weight());
    visitor(modifier());
    if (auto opt = priority()) {
        visitor(*opt);
    }
}

auto StatementScript::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementScript::value() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_value, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto StatementScript::script_type() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_script_type, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto StatementScript::construct(Library &lib, clingo_location_t const &location, char const *value,
                                char const *script_type) -> StatementScript {
    clingo_ast_t *res_;
    handle_error(lib,
                 clingo_ast_construct(lib, clingo_ast_type_statement_script, &res_, &location, value, script_type));
    return StatementScript::acquire(res_);
}

void StatementScript::visit(py::handle visitor) {}

auto StatementInclude::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementInclude::value() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_value, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto StatementInclude::include_type() -> IncludeType {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_include_type, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<IncludeType>(ret);
}

auto StatementInclude::construct(Library &lib, clingo_location_t const &location, char const *value,
                                 IncludeType const &include_type) -> StatementInclude {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_include, &res_, &location, value,
                                           static_cast<int>(include_type)));
    return StatementInclude::acquire(res_);
}

void StatementInclude::visit(py::handle visitor) {}

auto StatementProgram::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementProgram::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto StatementProgram::arguments() -> std::vector<char const *> {
    size_t size;
    if (!clingo_ast_attribute_get_string_array(ast_, clingo_ast_attribute_arguments, nullptr, &size)) {
        throw std::runtime_error("could not get string array attribute");
    }
    std::vector<char const *> ret;
    ret.resize(size, nullptr);
    if (!clingo_ast_attribute_get_string_array(ast_, clingo_ast_attribute_arguments, ret.data(), &size)) {
        throw std::runtime_error("could not get string array attribute");
    }
    return ret;
}

auto StatementProgram::construct(Library &lib, clingo_location_t const &location, char const *name,
                                 StringArray const &arguments) -> StatementProgram {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_program, &res_, &location, name,
                                           c_cast(arguments).data(), arguments.size()));
    return StatementProgram::acquire(res_);
}

void StatementProgram::visit(py::handle visitor) {}

auto StatementConst::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementConst::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto StatementConst::value() -> Term {
    clingo_ast_t *ast;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_value, &ast)) {
        throw std::runtime_error("could not get ast attribute");
    }
    return construct_term(ast);
}

auto StatementConst::const_type() -> ConstType {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_const_type, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<ConstType>(ret);
}

auto StatementConst::construct(Library &lib, clingo_location_t const &location, char const *name, Term const &value,
                               ConstType const &const_type) -> StatementConst {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_const, &res_, &location, name, c_cast(value),
                                           static_cast<int>(const_type)));
    return StatementConst::acquire(res_);
}

void StatementConst::visit(py::handle visitor) { visitor(value()); }

auto StatementComment::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}

auto StatementComment::value() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_value, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}

auto StatementComment::comment_type() -> CommentType {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_comment_type, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<CommentType>(ret);
}

auto StatementComment::construct(Library &lib, clingo_location_t const &location, char const *value,
                                 CommentType const &comment_type) -> StatementComment {
    clingo_ast_t *res_;
    handle_error(lib, clingo_ast_construct(lib, clingo_ast_type_statement_comment, &res_, &location, value,
                                           static_cast<int>(comment_type)));
    return StatementComment::acquire(res_);
}

void StatementComment::visit(py::handle visitor) {}

template <class T> auto c_cast(std::optional<T> const &opt) -> clingo_ast_t * {
    if (opt) {
        return c_cast(*opt);
    }
    return nullptr;
}

template <class... Ts> auto c_cast(std::variant<Ts...> const &var) -> clingo_ast_t * {
    return std::visit([](auto const &x) { return c_cast(x); }, var);
}

template <class T> auto c_cast(std::vector<T> const &arr) -> std::vector<clingo_ast_t *> {
    std::vector<clingo_ast_t *> ret;
    ret.reserve(arr.size());
    for (auto const &x : arr) {
        ret.emplace_back(c_cast(x));
    }
    return ret;
}

auto parse_term(Library &lib, char const *string) -> Term {
    clingo_ast_t *ast;
    handle_error(lib, clingo_ast_parse_expression(lib, clingo_ast_parse_type_term, string, &ast));
    return construct_term(ast);
}

auto parse_theory_term(Library &lib, char const *string) -> TheoryTerm {
    clingo_ast_t *ast;
    handle_error(lib, clingo_ast_parse_expression(lib, clingo_ast_parse_type_theory_term, string, &ast));
    return construct_theory_term(ast);
}

auto parse_literal(Library &lib, char const *string) -> Literal {
    clingo_ast_t *ast;
    handle_error(lib, clingo_ast_parse_expression(lib, clingo_ast_parse_type_literal, string, &ast));
    return construct_literal(ast);
}

auto parse_head_literal(Library &lib, char const *string) -> HeadLiteral {
    clingo_ast_t *ast;
    handle_error(lib, clingo_ast_parse_expression(lib, clingo_ast_parse_type_head_literal, string, &ast));
    return construct_head_literal(ast);
}

auto parse_body_literal(Library &lib, char const *string) -> BodyLiteral {
    clingo_ast_t *ast;
    handle_error(lib, clingo_ast_parse_expression(lib, clingo_ast_parse_type_body_literal, string, &ast));
    return construct_body_literal(ast);
}

auto parse_statement(Library &lib, char const *string) -> Statement {
    clingo_ast_t *ast;
    handle_error(lib, clingo_ast_parse_expression(lib, clingo_ast_parse_type_statement, string, &ast));
    return construct_statement(ast);
}

class Scanner {
  public:
    class Iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Statement;
        using reference = Statement &;
        using pointer = Statement *;

        Iterator(Scanner *scanner = nullptr) : scanner_{scanner} {}

        auto operator*() -> value_type { return *std::move(scanner_->value_); }

        auto operator->() -> pointer { return &*scanner_->value_; }

        auto operator++() -> Iterator & {
            scanner_->next();
            return *this;
        }

        void operator++(int) { scanner_->next(); }

        friend auto operator==(Iterator const &a, Iterator const &b) -> bool {
            if (a.scanner_ != nullptr && b.scanner_ == nullptr) {
                return !a.scanner_->value_.has_value();
            }
            if (a.scanner_ == nullptr && b.scanner_ != nullptr) {
                return !b.scanner_->value_.has_value();
            }
            return a.scanner_ == b.scanner_;
        }

        friend auto operator!=(Iterator const &a, Iterator const &b) -> bool { return !(a == b); }

      private:
        Scanner *scanner_;
    };
    friend auto operator==(Iterator const &a, Iterator const &b) -> bool;
    friend auto operator!=(Iterator const &a, Iterator const &b) -> bool;

    Scanner(Library &lib, char const *program) : lib_{lib} {
        handle_error(lib, clingo_ast_scan_string(lib, program, &scanner_));
    }

    Scanner(Library &lib, std::vector<std::string> files) : lib_{lib} {
        std::vector<char const *> cfiles;
        cfiles.reserve(cfiles.size());
        std::transform(files.begin(), files.end(), std::back_inserter(cfiles),
                       [](auto const &file) { return file.c_str(); });
        handle_error(lib, clingo_ast_scan_files(lib, cfiles.data(), cfiles.size(), &scanner_));
    }

    Scanner(Scanner const &other) = delete;
    auto operator=(Scanner const &other) -> Scanner & = delete;

    Scanner(Scanner &&other) noexcept { *this = std::move(other); }
    auto operator=(Scanner &&other) noexcept -> Scanner & {
        std::swap(lib_, other.lib_);
        std::swap(scanner_, other.scanner_);
        std::swap(value_, other.value_);
        return *this;
    }

    auto iter() -> py::iterator {
        next();
        return py::make_iterator(Iterator{this}, Iterator{});
    }
    void next() {
        clingo_ast_t *ast;
        handle_error(lib_, clingo_ast_scanner_next(scanner_, &ast));
        if (ast != nullptr) {
            value_ = construct_statement(ast);
        } else {
            value_.reset();
        }
    }

    void close() {
        if (scanner_ != nullptr) {
            clingo_ast_scanner_close(scanner_);
            scanner_ = nullptr;
        }
    }
    ~Scanner() { close(); }

  private:
    clingo_lib_t *lib_ = nullptr;
    clingo_ast_scanner_t *scanner_ = nullptr;
    std::optional<Statement> value_;
};

auto rewrite_statement(Library &lib, Statement &stm, std::vector<std::string> parameters, ProjectionMode mode,
                       bool project_anonymous) -> std::vector<Statement> {
    std::vector<char const *> params;
    params.reserve(parameters.size());
    std::transform(parameters.begin(), parameters.end(), std::back_inserter(params),
                   [](auto const &str) { return str.c_str(); });
    struct Array {
        ~Array() { clingo_ast_array_free(result, result_size); }
        clingo_ast_t **result = nullptr;
        size_t result_size = 0;
    };
    auto arr = Array{};
    clingo_ast_rewrite_options_t options{static_cast<clingo_projection_mode_e>(mode), project_anonymous};
    handle_error(lib, clingo_ast_rewrite(lib, c_cast(stm), &options, params.data(), params.size(), &arr.result,
                                         &arr.result_size));
    std::vector<Statement> res;
    res.reserve(arr.result_size);
    std::for_each_n(arr.result, arr.result_size, [&res](clingo_ast *&ast) {
        auto *cpy = ast;
        ast = nullptr;
        res.emplace_back(construct_statement(cpy));
    });
    return res;
}

void register_module(pybind11::module &m) {
    auto ast = m.def_submodule("ast", doc(R"(
This module provides functions to work with Abstract Syntax Trees of logic programs.
)"));

    ast.def("_type_info_yaml", &clingo_ast_type_info_yaml, doc(R"(
Return a yaml description of the AST.

This can be used to auto-generate most of the binding.)"));

    auto py_projection_mode =
        py::enum_<ProjectionMode>(ast, "ProjectionMode", R"doc(Available projection modes.)doc")
            .value("Disabled", ProjectionMode::Disabled, R"doc(Do not project.)doc")
            .value("Anonymous", ProjectionMode::Anonymous, R"doc(Only project anonymous variables.)doc")
            .value("Pure", ProjectionMode::Pure, R"doc(Project pure variables.)doc");

    auto py_unary_operator = py::enum_<UnaryOperator>(ast, "UnaryOperator", R"doc(Available unary operators.)doc");

    auto py_binary_operator = py::enum_<BinaryOperator>(ast, "BinaryOperator", R"doc(Available binary operators.)doc");

    auto py_sign = py::enum_<Sign>(ast, "Sign", R"doc(The available signs.)doc");

    auto py_relation = py::enum_<Relation>(ast, "Relation", R"doc(Available relation symbols.)doc");

    auto py_aggregate_function =
        py::enum_<AggregateFunction>(ast, "AggregateFunction", R"doc(Enumeration of aggregate functions.)doc");

    auto py_theory_operator_type =
        py::enum_<TheoryOperatorType>(ast, "TheoryOperatorType", R"doc(Enumeration of theory operators.)doc");

    auto py_theory_tuple_type =
        py::enum_<TheoryTupleType>(ast, "TheoryTupleType", R"doc(Enumeration of theory tuple types.)doc");

    auto py_theory_atom_type =
        py::enum_<TheoryAtomType>(ast, "TheoryAtomType", R"doc(Enumeration of the theory atom types.)doc");

    auto py_optimize_type = py::enum_<OptimizeType>(ast, "OptimizeType", R"doc(Enumeration of optimization types.)doc");

    auto py_include_type = py::enum_<IncludeType>(ast, "IncludeType", R"doc(Enumeration of include types.)doc");

    auto py_const_type = py::enum_<ConstType>(ast, "ConstType", R"doc(Enumeration of const types.)doc");

    auto py_comment_type = py::enum_<CommentType>(ast, "CommentType", R"doc(Enumeration of comment types.)doc");

    auto py_projection =
        py::class_<Projection>(ast, "Projection", R"doc(A placeholder for an argument to project.)doc");

    auto py_term_variable = py::class_<TermVariable>(ast, "TermVariable", R"doc(A term representing a variable.)doc");

    auto py_term_symbolic = py::class_<TermSymbolic>(ast, "TermSymbolic", R"doc(A term representing a symbol.)doc");

    auto py_term_absolute =
        py::class_<TermAbsolute>(ast, "TermAbsolute", R"doc(A term representing the absolute operation.)doc");

    auto py_term_unary_operation =
        py::class_<TermUnaryOperation>(ast, "TermUnaryOperation", R"doc(A term representing a unary operation.)doc");

    auto py_term_binary_operation =
        py::class_<TermBinaryOperation>(ast, "TermBinaryOperation", R"doc(A term representing a binary operation.)doc");

    auto py_term_tuple = py::class_<TermTuple>(ast, "TermTuple", R"doc(A term representing a tuple.)doc");

    auto py_term_function = py::class_<TermFunction>(ast, "TermFunction", R"doc(A term representing a function.)doc");

    auto py_argument_tuple =
        py::class_<ArgumentTuple>(ast, "ArgumentTuple", R"doc(A list of arguments for a function or tuple.)doc");

    auto py_left_guard = py::class_<LeftGuard>(ast, "LeftGuard",
                                               R"doc(A right hand side guard consisting of a term and a relation.)doc");

    auto py_right_guard = py::class_<RightGuard>(ast, "RightGuard",
                                                 R"doc(A right hand side guard consisting of a relation and term.)doc");

    auto py_literal_boolean =
        py::class_<LiteralBoolean>(ast, "LiteralBoolean", R"doc(A literal representing a Boolean constant.)doc");

    auto py_literal_comparison = py::class_<LiteralComparison>(
        ast, "LiteralComparison", R"doc(A literal representing a (chain of) comparison(s).)doc");

    auto py_literal_symbolic =
        py::class_<LiteralSymbolic>(ast, "LiteralSymbolic", R"doc(A literal representing a symbolic literal.)doc");

    auto py_unparsed_element =
        py::class_<UnparsedElement>(ast, "UnparsedElement", R"doc(A list of unparsed theory terms and operators.)doc");

    auto py_theory_term_variable =
        py::class_<TheoryTermVariable>(ast, "TheoryTermVariable", R"doc(A theory term representing a variable.)doc");

    auto py_theory_term_symbolic =
        py::class_<TheoryTermSymbolic>(ast, "TheoryTermSymbolic", R"doc(A theory term representing a symbol.)doc");

    auto py_theory_term_tuple =
        py::class_<TheoryTermTuple>(ast, "TheoryTermTuple", R"doc(A theory term representing a tuple.)doc");

    auto py_theory_term_function =
        py::class_<TheoryTermFunction>(ast, "TheoryTermFunction", R"doc(A theory term representing a function.)doc");

    auto py_theory_term_unparsed = py::class_<TheoryTermUnparsed>(
        ast, "TheoryTermUnparsed", R"doc(A theory term representing an unparsed theory term.)doc");

    auto py_theory_right_guard = py::class_<TheoryRightGuard>(
        ast, "TheoryRightGuard", R"doc(A right hand side guard consisting of a theory operator and theory
term.)doc");

    auto py_set_aggregate_element =
        py::class_<SetAggregateElement>(ast, "SetAggregateElement", R"doc(An element of a set aggregate.)doc");

    auto py_body_aggregate_element =
        py::class_<BodyAggregateElement>(ast, "BodyAggregateElement", R"doc(An element of a body aggregate.)doc");

    auto py_theory_atom_element =
        py::class_<TheoryAtomElement>(ast, "TheoryAtomElement", R"doc(An element of a theory atom elements.)doc");

    auto py_body_simple_literal =
        py::class_<BodySimpleLiteral>(ast, "BodySimpleLiteral", R"doc(A literal in a rule body.)doc");

    auto py_body_aggregate = py::class_<BodyAggregate>(ast, "BodyAggregate", R"doc(An aggregate in a rule body.)doc");

    auto py_body_set_aggregate = py::class_<BodySetAggregate>(ast, "BodySetAggregate", R"doc(A set aggregate.)doc");

    auto py_body_theory_atom = py::class_<BodyTheoryAtom>(ast, "BodyTheoryAtom", R"doc(A theory atom.)doc");

    auto py_body_conditional_literal =
        py::class_<BodyConditionalLiteral>(ast, "BodyConditionalLiteral", R"doc(A conditional_literal.)doc");

    auto py_head_conditional_literal =
        py::class_<HeadConditionalLiteral>(ast, "HeadConditionalLiteral", R"doc(A conditional_literal.)doc");

    auto py_head_aggregate_element =
        py::class_<HeadAggregateElement>(ast, "HeadAggregateElement", R"doc(An element of a head aggregate.)doc");

    auto py_head_simple_literal =
        py::class_<HeadSimpleLiteral>(ast, "HeadSimpleLiteral", R"doc(A literal in a rule head.)doc");

    auto py_head_aggregate = py::class_<HeadAggregate>(ast, "HeadAggregate", R"doc(An aggregate in a rule head.)doc");

    auto py_head_set_aggregate = py::class_<HeadSetAggregate>(ast, "HeadSetAggregate", R"doc(A set aggregate.)doc");

    auto py_head_theory_atom = py::class_<HeadTheoryAtom>(ast, "HeadTheoryAtom", R"doc(A theory atom.)doc");

    auto py_head_disjunction = py::class_<HeadDisjunction>(ast, "HeadDisjunction", R"doc(A disjunction.)doc");

    auto py_theory_operator_definition =
        py::class_<TheoryOperatorDefinition>(ast, "TheoryOperatorDefinition", R"doc(A theory operator definition.)doc");

    auto py_theory_term_definition =
        py::class_<TheoryTermDefinition>(ast, "TheoryTermDefinition", R"doc(A theory term definition.)doc");

    auto py_theory_guard_definition =
        py::class_<TheoryGuardDefinition>(ast, "TheoryGuardDefinition", R"doc(A definition of a theory guard.)doc");

    auto py_theory_atom_definition =
        py::class_<TheoryAtomDefinition>(ast, "TheoryAtomDefinition", R"doc(A theory atom definition.)doc");

    auto py_optimize_tuple =
        py::class_<OptimizeTuple>(ast, "OptimizeTuple", R"doc(A tuple of an optimizization statement.)doc");

    auto py_optimize_element =
        py::class_<OptimizeElement>(ast, "OptimizeElement", R"doc(An element of an optimization statement.)doc");

    auto py_edge = py::class_<Edge>(ast, "Edge", R"doc(An edge of an edge statement.)doc");

    auto py_statement_rule = py::class_<StatementRule>(ast, "StatementRule", R"doc(A rule.)doc");

    auto py_statement_theory = py::class_<StatementTheory>(ast, "StatementTheory", R"doc(A theory definition.)doc");

    auto py_statement_optimize =
        py::class_<StatementOptimize>(ast, "StatementOptimize", R"doc(An optimization statement.)doc");

    auto py_statement_weak_constraint =
        py::class_<StatementWeakConstraint>(ast, "StatementWeakConstraint", R"doc(A weak constraint.)doc");

    auto py_statement_show = py::class_<StatementShow>(ast, "StatementShow", R"doc(A show statement.)doc");

    auto py_statement_show_signature =
        py::class_<StatementShowSignature>(ast, "StatementShowSignature", R"doc(A show signature statement.)doc");

    auto py_statement_project = py::class_<StatementProject>(ast, "StatementProject", R"doc(A project statement.)doc");

    auto py_statement_project_signature = py::class_<StatementProjectSignature>(
        ast, "StatementProjectSignature", R"doc(A project signature statement.)doc");

    auto py_statement_defined = py::class_<StatementDefined>(ast, "StatementDefined", R"doc(A defined statement.)doc");

    auto py_statement_external =
        py::class_<StatementExternal>(ast, "StatementExternal", R"doc(An external statement.)doc");

    auto py_statement_edge = py::class_<StatementEdge>(ast, "StatementEdge", R"doc(An edge statement.)doc");

    auto py_statement_heuristic =
        py::class_<StatementHeuristic>(ast, "StatementHeuristic", R"doc(A heuristic statement.)doc");

    auto py_statement_script = py::class_<StatementScript>(ast, "StatementScript", R"doc(A script statement.)doc");

    auto py_statement_include = py::class_<StatementInclude>(ast, "StatementInclude", R"doc(An include statement.)doc");

    auto py_statement_program = py::class_<StatementProgram>(ast, "StatementProgram", R"doc(An program statement.)doc");

    auto py_statement_const = py::class_<StatementConst>(ast, "StatementConst", R"doc(A const statement.)doc");

    auto py_statement_comment = py::class_<StatementComment>(ast, "StatementComment", R"doc(A comment.)doc");

    py_unary_operator.value("Minus", UnaryOperator::Minus, R"doc(Operator `-`.)doc")
        .value("Negation", UnaryOperator::Negation, R"doc(Operator `~`.)doc");

    py_binary_operator.value("And", BinaryOperator::And, R"doc(Operator `&`.)doc")
        .value("Division", BinaryOperator::Division, R"doc(Operator `/`.)doc")
        .value("Minus", BinaryOperator::Minus, R"doc(Operator `-`.)doc")
        .value("Modulo", BinaryOperator::Modulo, R"doc(Operator `%`.)doc")
        .value("Multiplication", BinaryOperator::Multiplication, R"doc(Operator `*`.)doc")
        .value("Or", BinaryOperator::Or, R"doc(Operator `|`.)doc")
        .value("Plus", BinaryOperator::Plus, R"doc(Operator `+`.)doc")
        .value("Power", BinaryOperator::Power, R"doc(Operator `**`.)doc")
        .value("Xor", BinaryOperator::Xor, R"doc(Operator `^`.)doc");

    py_sign.value("NoSign", Sign::NoSign, R"doc(No sign.)doc")
        .value("Single", Sign::Single, R"doc(One sign.)doc")
        .value("Double", Sign::Double, R"doc(Two signs.)doc");

    py_relation.value("Equal", Relation::Equal, R"doc(The equal to relation.)doc")
        .value("NotEqual", Relation::NotEqual, R"doc(The not equal to relation.)doc")
        .value("Less", Relation::Less, R"doc(The less than relation.)doc")
        .value("LessEqual", Relation::LessEqual, R"doc(The less than or equal to relation.)doc")
        .value("Greater", Relation::Greater, R"doc(The greater than relation.)doc")
        .value("GreaterEqual", Relation::GreaterEqual, R"doc(The greater than or equal to relation.)doc");

    py_aggregate_function.value("Count", AggregateFunction::Count, R"doc(Operator "^".)doc")
        .value("Sum", AggregateFunction::Sum, R"doc(Operator "?".)doc")
        .value("Sump", AggregateFunction::Sump, R"doc(Operator "&".)doc")
        .value("Min", AggregateFunction::Min, R"doc(Operator "+".)doc")
        .value("Max", AggregateFunction::Max, R"doc(Operator "-".)doc");

    py_theory_operator_type.value("Unary", TheoryOperatorType::Unary, R"doc(An unary theory operator.)doc")
        .value("BinaryLeft", TheoryOperatorType::BinaryLeft, R"doc(A left associative binary operator.)doc")
        .value("BinaryRight", TheoryOperatorType::BinaryRight, R"doc(A right associative binary operator.)doc");

    py_theory_tuple_type.value("Tuple", TheoryTupleType::Tuple, R"doc(Theory tuples "(t1,...,tn)".)doc")
        .value("Set", TheoryTupleType::Set, R"doc(Theory sets "{t1,...,tn}".)doc")
        .value("List", TheoryTupleType::List, R"doc(Theory lists "[t1,...,tn]".)doc");

    py_theory_atom_type.value("Head", TheoryAtomType::Head, R"doc(For theory atoms that can appear in the head.)doc")
        .value("Body", TheoryAtomType::Body, R"doc(For theory atoms that can appear in the body.)doc")
        .value("Any", TheoryAtomType::Any, R"doc(For theory atoms that can appear in both head and body.)doc")
        .value("Directive", TheoryAtomType::Directive, R"doc(For theory atoms that must not have a body.)doc");

    py_optimize_type.value("Minimize", OptimizeType::Minimize, R"doc(For `#minimize` statements.)doc")
        .value("Maximize", OptimizeType::Maximize, R"doc(For `#maximize` statements.)doc");

    py_include_type.value("System", IncludeType::System, R"doc(For file includes.)doc")
        .value("Inbuild", IncludeType::Inbuild, R"doc(For inbuild includes.)doc");

    py_const_type.value("Default", ConstType::Default, R"doc(For default const statements.)doc")
        .value("Override", ConstType::Override, R"doc(For overriding const statements.)doc");

    py_comment_type.value("Line", CommentType::Line, R"doc(For line comments.)doc")
        .value("Block", CommentType::Block, R"doc(For block comments.)doc");

    py_projection
        .def(py::init(&Projection::construct), py::arg("lib"), py::arg("location"), R"doc(Construct a Projection object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the placeholder.)doc")
        .def("__str__", &Projection::to_string)
        .def("__hash__", &Projection::hash)
        .def_property_readonly("location", &Projection::location, R"doc(The location of the placeholder.)doc")
        .def("visit", &Projection::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_term_variable
        .def(py::init(&TermVariable::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("anonymous") = false, R"doc(Construct a TermVariable object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the variable.
name
    The name of the variable.
anonymous
    Whether the variable is anonymous.

    Anonymous variables receive a unique name during preprocessing.)doc")
        .def("__str__", &TermVariable::to_string)
        .def("__hash__", &TermVariable::hash)
        .def_property_readonly("location", &TermVariable::location, R"doc(The location of the variable.)doc")
        .def_property_readonly("name", &TermVariable::name, R"doc(The name of the variable.)doc")
        .def_property_readonly("anonymous", &TermVariable::anonymous, R"doc(Whether the variable is anonymous.
Anonymous variables receive a unique name during preprocessing.)doc")
        .def("visit", &TermVariable::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_term_symbolic
        .def(py::init(&TermSymbolic::construct), py::arg("lib"), py::arg("location"), py::arg("symbol"),
             R"doc(Construct a TermSymbolic object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the symbol.
symbol
    The symbol.)doc")
        .def("__str__", &TermSymbolic::to_string)
        .def("__hash__", &TermSymbolic::hash)
        .def_property_readonly("location", &TermSymbolic::location, R"doc(The location of the symbol.)doc")
        .def_property_readonly("symbol", &TermSymbolic::symbol, R"doc(The symbol.)doc")
        .def("visit", &TermSymbolic::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_term_absolute
        .def(py::init(&TermAbsolute::construct), py::arg("lib"), py::arg("location"), py::arg("pool"),
             R"doc(Construct a TermAbsolute object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the operation.
pool
    The argument pool.

    If there is more than one argument in the pool, the term is
    unpooled during preprocessing.)doc")
        .def("__str__", &TermAbsolute::to_string)
        .def("__hash__", &TermAbsolute::hash)
        .def_property_readonly("location", &TermAbsolute::location, R"doc(The location of the operation.)doc")
        .def_property_readonly("pool", &TermAbsolute::pool, R"doc(The argument pool.
If there is more than one argument in the pool, the term is unpooled during preprocessing.)doc")
        .def("visit", &TermAbsolute::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_term_unary_operation
        .def(py::init(&TermUnaryOperation::construct), py::arg("lib"), py::arg("location"), py::arg("operator_type"),
             py::arg("right"), R"doc(Construct a TermUnaryOperation object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the operation.
operator_type
    The type of the operation.
right
    The argument of the operation.)doc")
        .def("__str__", &TermUnaryOperation::to_string)
        .def("__hash__", &TermUnaryOperation::hash)
        .def_property_readonly("location", &TermUnaryOperation::location, R"doc(The location of the operation.)doc")
        .def_property_readonly("operator_type", &TermUnaryOperation::operator_type,
                               R"doc(The type of the operation.)doc")
        .def_property_readonly("right", &TermUnaryOperation::right, R"doc(The argument of the operation.)doc")
        .def("visit", &TermUnaryOperation::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_term_binary_operation
        .def(py::init(&TermBinaryOperation::construct), py::arg("lib"), py::arg("location"), py::arg("left"),
             py::arg("operator_type"), py::arg("right"), R"doc(Construct a TermBinaryOperation object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the operation.
left
    The left argument of the operation.
operator_type
    The type of the operation.
right
    The right argument of the operation.)doc")
        .def("__str__", &TermBinaryOperation::to_string)
        .def("__hash__", &TermBinaryOperation::hash)
        .def_property_readonly("location", &TermBinaryOperation::location, R"doc(The location of the operation.)doc")
        .def_property_readonly("left", &TermBinaryOperation::left, R"doc(The left argument of the operation.)doc")
        .def_property_readonly("operator_type", &TermBinaryOperation::operator_type,
                               R"doc(The type of the operation.)doc")
        .def_property_readonly("right", &TermBinaryOperation::right, R"doc(The right argument of the operation.)doc")
        .def("visit", &TermBinaryOperation::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_term_tuple
        .def(py::init(&TermTuple::construct), py::arg("lib"), py::arg("location"), py::arg("pool"),
             R"doc(Construct a TermTuple object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the tuple.
pool
    The argument pool of the tuple.

    If there is more than one element in the pool, the term is
    unpooled during preprocessing.)doc")
        .def("__str__", &TermTuple::to_string)
        .def("__hash__", &TermTuple::hash)
        .def_property_readonly("location", &TermTuple::location, R"doc(The location of the tuple.)doc")
        .def_property_readonly("pool", &TermTuple::pool, R"doc(The argument pool of the tuple.
If there is more than one element in the pool, the term is unpooled during preprocessing.)doc")
        .def("visit", &TermTuple::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_term_function
        .def(py::init(&TermFunction::construct), py::arg("lib"), py::arg("location"), py::arg("name"), py::arg("pool"),
             py::arg("external") = false, R"doc(Construct a TermFunction object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the function.
name
    The name of the function.
pool
    The argument pool of the function.

    If there is more than one element in the pool, the term is
    unpooled during preprocessing.
external
    Whether the function is external.)doc")
        .def("__str__", &TermFunction::to_string)
        .def("__hash__", &TermFunction::hash)
        .def_property_readonly("location", &TermFunction::location, R"doc(The location of the function.)doc")
        .def_property_readonly("name", &TermFunction::name, R"doc(The name of the function.)doc")
        .def_property_readonly("pool", &TermFunction::pool, R"doc(The argument pool of the function.
If there is more than one element in the pool, the term is unpooled during preprocessing.)doc")
        .def_property_readonly("external", &TermFunction::external, R"doc(Whether the function is external.)doc")
        .def("visit", &TermFunction::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_argument_tuple
        .def(py::init(&ArgumentTuple::construct), py::arg("lib"), py::arg("arguments") = TermOrProjectionArray{},
             R"doc(Construct a ArgumentTuple object.

Parameters
----------
lib
    The library object for storing symbols.
arguments
    The arguments of the tuple.)doc")
        .def("__str__", &ArgumentTuple::to_string)
        .def("__hash__", &ArgumentTuple::hash)
        .def_property_readonly("arguments", &ArgumentTuple::arguments, R"doc(The arguments of the tuple.)doc")
        .def("visit", &ArgumentTuple::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_left_guard
        .def(py::init(&LeftGuard::construct), py::arg("lib"), py::arg("term"), py::arg("relation"),
             R"doc(Construct a LeftGuard object.

Parameters
----------
lib
    The library object for storing symbols.
term
    The term of the guard.
relation
    The relation of the guard.)doc")
        .def("__str__", &LeftGuard::to_string)
        .def("__hash__", &LeftGuard::hash)
        .def_property_readonly("term", &LeftGuard::term, R"doc(The term of the guard.)doc")
        .def_property_readonly("relation", &LeftGuard::relation, R"doc(The relation of the guard.)doc")
        .def("visit", &LeftGuard::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_right_guard
        .def(py::init(&RightGuard::construct), py::arg("lib"), py::arg("relation"), py::arg("term"),
             R"doc(Construct a RightGuard object.

Parameters
----------
lib
    The library object for storing symbols.
relation
    The relation of the guard.
term
    The term of the guard.)doc")
        .def("__str__", &RightGuard::to_string)
        .def("__hash__", &RightGuard::hash)
        .def_property_readonly("relation", &RightGuard::relation, R"doc(The relation of the guard.)doc")
        .def_property_readonly("term", &RightGuard::term, R"doc(The term of the guard.)doc")
        .def("visit", &RightGuard::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_literal_boolean
        .def(py::init(&LiteralBoolean::construct), py::arg("lib"), py::arg("location"), py::arg("sign"),
             py::arg("value"), R"doc(Construct a LiteralBoolean object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the symbol.
sign
    The sign of the literal.
value
    The fixed value of the literal.)doc")
        .def("__str__", &LiteralBoolean::to_string)
        .def("__hash__", &LiteralBoolean::hash)
        .def_property_readonly("location", &LiteralBoolean::location, R"doc(The location of the symbol.)doc")
        .def_property_readonly("sign", &LiteralBoolean::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("value", &LiteralBoolean::value, R"doc(The fixed value of the literal.)doc")
        .def("visit", &LiteralBoolean::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_literal_comparison
        .def(py::init(&LiteralComparison::construct), py::arg("lib"), py::arg("location"), py::arg("sign"),
             py::arg("left"), py::arg("right"), R"doc(Construct a LiteralComparison object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the symbol.
sign
    The sign of the literal.
left
    The first term of the comparison.
right
    The chain of comparisons.

    Note that the chain must have at least length one.)doc")
        .def("__str__", &LiteralComparison::to_string)
        .def("__hash__", &LiteralComparison::hash)
        .def_property_readonly("location", &LiteralComparison::location, R"doc(The location of the symbol.)doc")
        .def_property_readonly("sign", &LiteralComparison::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("left", &LiteralComparison::left, R"doc(The first term of the comparison.)doc")
        .def_property_readonly("right", &LiteralComparison::right, R"doc(The chain of comparisons.
Note that the chain must have at least length one.)doc")
        .def("visit", &LiteralComparison::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_literal_symbolic
        .def(py::init(&LiteralSymbolic::construct), py::arg("lib"), py::arg("location"), py::arg("sign"),
             py::arg("atom"), R"doc(Construct a LiteralSymbolic object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the symbol.
sign
    The sign of the literal.
atom
    The term representing the atom.)doc")
        .def("__str__", &LiteralSymbolic::to_string)
        .def("__hash__", &LiteralSymbolic::hash)
        .def_property_readonly("location", &LiteralSymbolic::location, R"doc(The location of the symbol.)doc")
        .def_property_readonly("sign", &LiteralSymbolic::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("atom", &LiteralSymbolic::atom, R"doc(The term representing the atom.)doc")
        .def("visit", &LiteralSymbolic::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_unparsed_element
        .def(py::init(&UnparsedElement::construct), py::arg("lib"), py::arg("operators"), py::arg("term"),
             R"doc(Construct a UnparsedElement object.

Parameters
----------
lib
    The library object for storing symbols.
operators
    The list of theory operators.
term
    The theory term.)doc")
        .def("__str__", &UnparsedElement::to_string)
        .def("__hash__", &UnparsedElement::hash)
        .def_property_readonly("operators", &UnparsedElement::operators, R"doc(The list of theory operators.)doc")
        .def_property_readonly("term", &UnparsedElement::term, R"doc(The theory term.)doc")
        .def("visit", &UnparsedElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_theory_term_variable
        .def(py::init(&TheoryTermVariable::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("anonymous") = false, R"doc(Construct a TheoryTermVariable object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the variable.
name
    The name of the variable.
anonymous
    Whether the variable is anonymous.

    Anonymous variables receive a unique name during preprocessing.)doc")
        .def("__str__", &TheoryTermVariable::to_string)
        .def("__hash__", &TheoryTermVariable::hash)
        .def_property_readonly("location", &TheoryTermVariable::location, R"doc(The location of the variable.)doc")
        .def_property_readonly("name", &TheoryTermVariable::name, R"doc(The name of the variable.)doc")
        .def_property_readonly("anonymous", &TheoryTermVariable::anonymous, R"doc(Whether the variable is anonymous.
Anonymous variables receive a unique name during preprocessing.)doc")
        .def("visit", &TheoryTermVariable::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_theory_term_symbolic
        .def(py::init(&TheoryTermSymbolic::construct), py::arg("lib"), py::arg("location"), py::arg("symbol"),
             R"doc(Construct a TheoryTermSymbolic object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the symbol.
symbol
    The symbol.)doc")
        .def("__str__", &TheoryTermSymbolic::to_string)
        .def("__hash__", &TheoryTermSymbolic::hash)
        .def_property_readonly("location", &TheoryTermSymbolic::location, R"doc(The location of the symbol.)doc")
        .def_property_readonly("symbol", &TheoryTermSymbolic::symbol, R"doc(The symbol.)doc")
        .def("visit", &TheoryTermSymbolic::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_theory_term_tuple
        .def(py::init(&TheoryTermTuple::construct), py::arg("lib"), py::arg("location"), py::arg("tuple_type"),
             py::arg("arguments"), R"doc(Construct a TheoryTermTuple object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the tuple.
tuple_type
    The type of the tuple.
arguments
    The arguments of the tuple.)doc")
        .def("__str__", &TheoryTermTuple::to_string)
        .def("__hash__", &TheoryTermTuple::hash)
        .def_property_readonly("location", &TheoryTermTuple::location, R"doc(The location of the tuple.)doc")
        .def_property_readonly("tuple_type", &TheoryTermTuple::tuple_type, R"doc(The type of the tuple.)doc")
        .def_property_readonly("arguments", &TheoryTermTuple::arguments, R"doc(The arguments of the tuple.)doc")
        .def("visit", &TheoryTermTuple::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_theory_term_function
        .def(py::init(&TheoryTermFunction::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arguments"), R"doc(Construct a TheoryTermFunction object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the function.
name
    The name of the function.
arguments
    The arguments of the function.)doc")
        .def("__str__", &TheoryTermFunction::to_string)
        .def("__hash__", &TheoryTermFunction::hash)
        .def_property_readonly("location", &TheoryTermFunction::location, R"doc(The location of the function.)doc")
        .def_property_readonly("name", &TheoryTermFunction::name, R"doc(The name of the function.)doc")
        .def_property_readonly("arguments", &TheoryTermFunction::arguments, R"doc(The arguments of the function.)doc")
        .def("visit", &TheoryTermFunction::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_theory_term_unparsed
        .def(py::init(&TheoryTermUnparsed::construct), py::arg("lib"), py::arg("location"), py::arg("elements"),
             R"doc(Construct a TheoryTermUnparsed object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the theory term.
elements
    The unparsed theory elements.)doc")
        .def("__str__", &TheoryTermUnparsed::to_string)
        .def("__hash__", &TheoryTermUnparsed::hash)
        .def_property_readonly("location", &TheoryTermUnparsed::location, R"doc(The location of the theory term.)doc")
        .def_property_readonly("elements", &TheoryTermUnparsed::elements, R"doc(The unparsed theory elements.)doc")
        .def("visit", &TheoryTermUnparsed::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_theory_right_guard
        .def(py::init(&TheoryRightGuard::construct), py::arg("lib"), py::arg("theory_operator"), py::arg("term"),
             R"doc(Construct a TheoryRightGuard object.

Parameters
----------
lib
    The library object for storing symbols.
theory_operator
    The operator of the guard.
term
    The theory term of the guard.)doc")
        .def("__str__", &TheoryRightGuard::to_string)
        .def("__hash__", &TheoryRightGuard::hash)
        .def_property_readonly("theory_operator", &TheoryRightGuard::theory_operator,
                               R"doc(The operator of the guard.)doc")
        .def_property_readonly("term", &TheoryRightGuard::term, R"doc(The theory term of the guard.)doc")
        .def("visit", &TheoryRightGuard::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_set_aggregate_element
        .def(py::init(&SetAggregateElement::construct), py::arg("lib"), py::arg("location"), py::arg("literal"),
             py::arg("condition"), R"doc(Construct a SetAggregateElement object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
literal
    The literal of the element.
condition
    The condition of the element.)doc")
        .def("__str__", &SetAggregateElement::to_string)
        .def("__hash__", &SetAggregateElement::hash)
        .def_property_readonly("location", &SetAggregateElement::location, R"doc(The location of the element.)doc")
        .def_property_readonly("literal", &SetAggregateElement::literal, R"doc(The literal of the element.)doc")
        .def_property_readonly("condition", &SetAggregateElement::condition, R"doc(The condition of the element.)doc")
        .def("visit", &SetAggregateElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_body_aggregate_element
        .def(py::init(&BodyAggregateElement::construct), py::arg("lib"), py::arg("location"), py::arg("tuple"),
             py::arg("condition"), R"doc(Construct a BodyAggregateElement object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
tuple
    The term tuple of the element.
condition
    The condition of the element.)doc")
        .def("__str__", &BodyAggregateElement::to_string)
        .def("__hash__", &BodyAggregateElement::hash)
        .def_property_readonly("location", &BodyAggregateElement::location, R"doc(The location of the element.)doc")
        .def_property_readonly("tuple", &BodyAggregateElement::tuple, R"doc(The term tuple of the element.)doc")
        .def_property_readonly("condition", &BodyAggregateElement::condition, R"doc(The condition of the element.)doc")
        .def("visit", &BodyAggregateElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_theory_atom_element
        .def(py::init(&TheoryAtomElement::construct), py::arg("lib"), py::arg("location"), py::arg("tuple"),
             py::arg("condition"), R"doc(Construct a TheoryAtomElement object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
tuple
    The theory term tuple of the element.
condition
    The condition of the element.)doc")
        .def("__str__", &TheoryAtomElement::to_string)
        .def("__hash__", &TheoryAtomElement::hash)
        .def_property_readonly("location", &TheoryAtomElement::location, R"doc(The location of the element.)doc")
        .def_property_readonly("tuple", &TheoryAtomElement::tuple, R"doc(The theory term tuple of the element.)doc")
        .def_property_readonly("condition", &TheoryAtomElement::condition, R"doc(The condition of the element.)doc")
        .def("visit", &TheoryAtomElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_body_simple_literal
        .def(py::init(&BodySimpleLiteral::construct), py::arg("lib"), py::arg("literal"),
             R"doc(Construct a BodySimpleLiteral object.

Parameters
----------
lib
    The library object for storing symbols.
literal
    The literal.)doc")
        .def("__str__", &BodySimpleLiteral::to_string)
        .def("__hash__", &BodySimpleLiteral::hash)
        .def_property_readonly("literal", &BodySimpleLiteral::literal, R"doc(The literal.)doc")
        .def("visit", &BodySimpleLiteral::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_body_aggregate
        .def(py::init(&BodyAggregate::construct), py::arg("lib"), py::arg("location"), py::arg("sign"), py::arg("left"),
             py::arg("function"), py::arg("elements"), py::arg("right"), R"doc(Construct a BodyAggregate object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
sign
    The sign of the literal.
left
    The left guard of the aggregate.
function
    The aggregate function.
elements
    The aggregate elements.
right
    The right guard of the aggregate.)doc")
        .def("__str__", &BodyAggregate::to_string)
        .def("__hash__", &BodyAggregate::hash)
        .def_property_readonly("location", &BodyAggregate::location, R"doc(The location of the element.)doc")
        .def_property_readonly("sign", &BodyAggregate::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("left", &BodyAggregate::left, R"doc(The left guard of the aggregate.)doc")
        .def_property_readonly("function", &BodyAggregate::function, R"doc(The aggregate function.)doc")
        .def_property_readonly("elements", &BodyAggregate::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &BodyAggregate::right, R"doc(The right guard of the aggregate.)doc")
        .def("visit", &BodyAggregate::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_body_set_aggregate
        .def(py::init(&BodySetAggregate::construct), py::arg("lib"), py::arg("location"), py::arg("sign"),
             py::arg("left"), py::arg("elements"), py::arg("right"), R"doc(Construct a BodySetAggregate object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
sign
    The sign of the literal.
left
    The left guard of the aggregate.
elements
    The aggregate elements.
right
    The right guard of the aggregate.)doc")
        .def("__str__", &BodySetAggregate::to_string)
        .def("__hash__", &BodySetAggregate::hash)
        .def_property_readonly("location", &BodySetAggregate::location, R"doc(The location of the element.)doc")
        .def_property_readonly("sign", &BodySetAggregate::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("left", &BodySetAggregate::left, R"doc(The left guard of the aggregate.)doc")
        .def_property_readonly("elements", &BodySetAggregate::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &BodySetAggregate::right, R"doc(The right guard of the aggregate.)doc")
        .def("visit", &BodySetAggregate::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_body_theory_atom
        .def(py::init(&BodyTheoryAtom::construct), py::arg("lib"), py::arg("location"), py::arg("sign"),
             py::arg("name"), py::arg("elements"), py::arg("right"), R"doc(Construct a BodyTheoryAtom object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
sign
    The sign of the literal.
name
    The name of the theory atom.
elements
    The aggregate elements.
right
    The right guard of the theory atom.)doc")
        .def("__str__", &BodyTheoryAtom::to_string)
        .def("__hash__", &BodyTheoryAtom::hash)
        .def_property_readonly("location", &BodyTheoryAtom::location, R"doc(The location of the element.)doc")
        .def_property_readonly("sign", &BodyTheoryAtom::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("name", &BodyTheoryAtom::name, R"doc(The name of the theory atom.)doc")
        .def_property_readonly("elements", &BodyTheoryAtom::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &BodyTheoryAtom::right, R"doc(The right guard of the theory atom.)doc")
        .def("visit", &BodyTheoryAtom::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_body_conditional_literal
        .def(py::init(&BodyConditionalLiteral::construct), py::arg("lib"), py::arg("location"), py::arg("literal"),
             py::arg("condition"), R"doc(Construct a BodyConditionalLiteral object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
literal
    The literal of the element.
condition
    The condition of the element.)doc")
        .def("__str__", &BodyConditionalLiteral::to_string)
        .def("__hash__", &BodyConditionalLiteral::hash)
        .def_property_readonly("location", &BodyConditionalLiteral::location, R"doc(The location of the element.)doc")
        .def_property_readonly("literal", &BodyConditionalLiteral::literal, R"doc(The literal of the element.)doc")
        .def_property_readonly("condition", &BodyConditionalLiteral::condition,
                               R"doc(The condition of the element.)doc")
        .def("visit", &BodyConditionalLiteral::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_head_conditional_literal
        .def(py::init(&HeadConditionalLiteral::construct), py::arg("lib"), py::arg("location"), py::arg("literal"),
             py::arg("condition"), R"doc(Construct a HeadConditionalLiteral object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
literal
    The literal of the element.
condition
    The condition of the element.)doc")
        .def("__str__", &HeadConditionalLiteral::to_string)
        .def("__hash__", &HeadConditionalLiteral::hash)
        .def_property_readonly("location", &HeadConditionalLiteral::location, R"doc(The location of the element.)doc")
        .def_property_readonly("literal", &HeadConditionalLiteral::literal, R"doc(The literal of the element.)doc")
        .def_property_readonly("condition", &HeadConditionalLiteral::condition,
                               R"doc(The condition of the element.)doc")
        .def("visit", &HeadConditionalLiteral::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_head_aggregate_element
        .def(py::init(&HeadAggregateElement::construct), py::arg("lib"), py::arg("location"), py::arg("tuple"),
             py::arg("literal"), py::arg("condition"), R"doc(Construct a HeadAggregateElement object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
tuple
    The term tuple of the element.
literal
    The literal of the element.
condition
    The condition of the element.)doc")
        .def("__str__", &HeadAggregateElement::to_string)
        .def("__hash__", &HeadAggregateElement::hash)
        .def_property_readonly("location", &HeadAggregateElement::location, R"doc(The location of the element.)doc")
        .def_property_readonly("tuple", &HeadAggregateElement::tuple, R"doc(The term tuple of the element.)doc")
        .def_property_readonly("literal", &HeadAggregateElement::literal, R"doc(The literal of the element.)doc")
        .def_property_readonly("condition", &HeadAggregateElement::condition, R"doc(The condition of the element.)doc")
        .def("visit", &HeadAggregateElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_head_simple_literal
        .def(py::init(&HeadSimpleLiteral::construct), py::arg("lib"), py::arg("literal"),
             R"doc(Construct a HeadSimpleLiteral object.

Parameters
----------
lib
    The library object for storing symbols.
literal
    The literal.)doc")
        .def("__str__", &HeadSimpleLiteral::to_string)
        .def("__hash__", &HeadSimpleLiteral::hash)
        .def_property_readonly("literal", &HeadSimpleLiteral::literal, R"doc(The literal.)doc")
        .def("visit", &HeadSimpleLiteral::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_head_aggregate
        .def(py::init(&HeadAggregate::construct), py::arg("lib"), py::arg("location"), py::arg("left"),
             py::arg("function"), py::arg("elements"), py::arg("right"), R"doc(Construct a HeadAggregate object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
left
    The left guard of the aggregate.
function
    The aggregate function.
elements
    The aggregate elements.
right
    The right guard of the aggregate.)doc")
        .def("__str__", &HeadAggregate::to_string)
        .def("__hash__", &HeadAggregate::hash)
        .def_property_readonly("location", &HeadAggregate::location, R"doc(The location of the element.)doc")
        .def_property_readonly("left", &HeadAggregate::left, R"doc(The left guard of the aggregate.)doc")
        .def_property_readonly("function", &HeadAggregate::function, R"doc(The aggregate function.)doc")
        .def_property_readonly("elements", &HeadAggregate::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &HeadAggregate::right, R"doc(The right guard of the aggregate.)doc")
        .def("visit", &HeadAggregate::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_head_set_aggregate
        .def(py::init(&HeadSetAggregate::construct), py::arg("lib"), py::arg("location"), py::arg("left"),
             py::arg("elements"), py::arg("right"), R"doc(Construct a HeadSetAggregate object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
left
    The left guard of the aggregate.
elements
    The aggregate elements.
right
    The right guard of the aggregate.)doc")
        .def("__str__", &HeadSetAggregate::to_string)
        .def("__hash__", &HeadSetAggregate::hash)
        .def_property_readonly("location", &HeadSetAggregate::location, R"doc(The location of the element.)doc")
        .def_property_readonly("left", &HeadSetAggregate::left, R"doc(The left guard of the aggregate.)doc")
        .def_property_readonly("elements", &HeadSetAggregate::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &HeadSetAggregate::right, R"doc(The right guard of the aggregate.)doc")
        .def("visit", &HeadSetAggregate::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_head_theory_atom
        .def(py::init(&HeadTheoryAtom::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("elements"), py::arg("right"), R"doc(Construct a HeadTheoryAtom object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
name
    The name of the theory atom.
elements
    The aggregate elements.
right
    The right guard of the theory atom.)doc")
        .def("__str__", &HeadTheoryAtom::to_string)
        .def("__hash__", &HeadTheoryAtom::hash)
        .def_property_readonly("location", &HeadTheoryAtom::location, R"doc(The location of the element.)doc")
        .def_property_readonly("name", &HeadTheoryAtom::name, R"doc(The name of the theory atom.)doc")
        .def_property_readonly("elements", &HeadTheoryAtom::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &HeadTheoryAtom::right, R"doc(The right guard of the theory atom.)doc")
        .def("visit", &HeadTheoryAtom::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_head_disjunction
        .def(py::init(&HeadDisjunction::construct), py::arg("lib"), py::arg("location"), py::arg("elements"),
             R"doc(Construct a HeadDisjunction object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the element.
elements
    The elements of the disjunction.)doc")
        .def("__str__", &HeadDisjunction::to_string)
        .def("__hash__", &HeadDisjunction::hash)
        .def_property_readonly("location", &HeadDisjunction::location, R"doc(The location of the element.)doc")
        .def_property_readonly("elements", &HeadDisjunction::elements, R"doc(The elements of the disjunction.)doc")
        .def("visit", &HeadDisjunction::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_theory_operator_definition
        .def(py::init(&TheoryOperatorDefinition::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("priority"), py::arg("operator_type"), R"doc(Construct a TheoryOperatorDefinition object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the definition.
name
    The name of the definition.
priority
    The priority of the operator.
operator_type
    The type of the operator.)doc")
        .def("__str__", &TheoryOperatorDefinition::to_string)
        .def("__hash__", &TheoryOperatorDefinition::hash)
        .def_property_readonly("location", &TheoryOperatorDefinition::location,
                               R"doc(The location of the definition.)doc")
        .def_property_readonly("name", &TheoryOperatorDefinition::name, R"doc(The name of the definition.)doc")
        .def_property_readonly("priority", &TheoryOperatorDefinition::priority,
                               R"doc(The priority of the operator.)doc")
        .def_property_readonly("operator_type", &TheoryOperatorDefinition::operator_type,
                               R"doc(The type of the operator.)doc")
        .def("visit", &TheoryOperatorDefinition::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_theory_term_definition
        .def(py::init(&TheoryTermDefinition::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("operators"), R"doc(Construct a TheoryTermDefinition object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the definition.
name
    The name of the definition.
operators
    The operator definitions to construct terms.)doc")
        .def("__str__", &TheoryTermDefinition::to_string)
        .def("__hash__", &TheoryTermDefinition::hash)
        .def_property_readonly("location", &TheoryTermDefinition::location, R"doc(The location of the definition.)doc")
        .def_property_readonly("name", &TheoryTermDefinition::name, R"doc(The name of the definition.)doc")
        .def_property_readonly("operators", &TheoryTermDefinition::operators,
                               R"doc(The operator definitions to construct terms.)doc")
        .def("visit", &TheoryTermDefinition::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_theory_guard_definition
        .def(py::init(&TheoryGuardDefinition::construct), py::arg("lib"), py::arg("operators"), py::arg("term"),
             R"doc(Construct a TheoryGuardDefinition object.

Parameters
----------
lib
    The library object for storing symbols.
operators
    A list of operator definition names.
term
    The name of a term definition.)doc")
        .def("__str__", &TheoryGuardDefinition::to_string)
        .def("__hash__", &TheoryGuardDefinition::hash)
        .def_property_readonly("operators", &TheoryGuardDefinition::operators,
                               R"doc(A list of operator definition names.)doc")
        .def_property_readonly("term", &TheoryGuardDefinition::term, R"doc(The name of a term definition.)doc")
        .def("visit", &TheoryGuardDefinition::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_theory_atom_definition
        .def(py::init(&TheoryAtomDefinition::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arity"), py::arg("term"), py::arg("guard"), py::arg("atom_type"),
             R"doc(Construct a TheoryAtomDefinition object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the definition.
name
    The name of the atom.
arity
    The arity of the atom.
term
    The name of a term definition.
guard
    An optional guard definition.
atom_type
    The type of the atom definition.)doc")
        .def("__str__", &TheoryAtomDefinition::to_string)
        .def("__hash__", &TheoryAtomDefinition::hash)
        .def_property_readonly("location", &TheoryAtomDefinition::location, R"doc(The location of the definition.)doc")
        .def_property_readonly("name", &TheoryAtomDefinition::name, R"doc(The name of the atom.)doc")
        .def_property_readonly("arity", &TheoryAtomDefinition::arity, R"doc(The arity of the atom.)doc")
        .def_property_readonly("term", &TheoryAtomDefinition::term, R"doc(The name of a term definition.)doc")
        .def_property_readonly("guard", &TheoryAtomDefinition::guard, R"doc(An optional guard definition.)doc")
        .def_property_readonly("atom_type", &TheoryAtomDefinition::atom_type,
                               R"doc(The type of the atom definition.)doc")
        .def("visit", &TheoryAtomDefinition::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_optimize_tuple
        .def(py::init(&OptimizeTuple::construct), py::arg("lib"), py::arg("weight"), py::arg("priority"),
             py::arg("terms"), R"doc(Construct a OptimizeTuple object.

Parameters
----------
lib
    The library object for storing symbols.
weight
    The weight of the tuple.
priority
    An optional priority.
terms
    The remaining terms in the tuple.)doc")
        .def("__str__", &OptimizeTuple::to_string)
        .def("__hash__", &OptimizeTuple::hash)
        .def_property_readonly("weight", &OptimizeTuple::weight, R"doc(The weight of the tuple.)doc")
        .def_property_readonly("priority", &OptimizeTuple::priority, R"doc(An optional priority.)doc")
        .def_property_readonly("terms", &OptimizeTuple::terms, R"doc(The remaining terms in the tuple.)doc")
        .def("visit", &OptimizeTuple::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_optimize_element
        .def(py::init(&OptimizeElement::construct), py::arg("lib"), py::arg("tuple"), py::arg("condition"),
             R"doc(Construct a OptimizeElement object.

Parameters
----------
lib
    The library object for storing symbols.
tuple
    The tuple of the element.
condition
    The condition of the element.)doc")
        .def("__str__", &OptimizeElement::to_string)
        .def("__hash__", &OptimizeElement::hash)
        .def_property_readonly("tuple", &OptimizeElement::tuple, R"doc(The tuple of the element.)doc")
        .def_property_readonly("condition", &OptimizeElement::condition, R"doc(The condition of the element.)doc")
        .def("visit", &OptimizeElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_edge
        .def(py::init(&Edge::construct), py::arg("lib"), py::arg("u"), py::arg("v"), R"doc(Construct a Edge object.

Parameters
----------
lib
    The library object for storing symbols.
u
    The start vertex.
v
    The end vertex.)doc")
        .def("__str__", &Edge::to_string)
        .def("__hash__", &Edge::hash)
        .def_property_readonly("u", &Edge::u, R"doc(The start vertex.)doc")
        .def_property_readonly("v", &Edge::v, R"doc(The end vertex.)doc")
        .def("visit", &Edge::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_rule
        .def(py::init(&StatementRule::construct), py::arg("lib"), py::arg("location"), py::arg("head"), py::arg("body"),
             R"doc(Construct a StatementRule object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
head
    The head literal.
body
    The body of the statement.)doc")
        .def("__str__", &StatementRule::to_string)
        .def("__hash__", &StatementRule::hash)
        .def_property_readonly("location", &StatementRule::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("head", &StatementRule::head, R"doc(The head literal.)doc")
        .def_property_readonly("body", &StatementRule::body, R"doc(The body of the statement.)doc")
        .def("visit", &StatementRule::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_theory
        .def(py::init(&StatementTheory::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("terms"), py::arg("atoms"), R"doc(Construct a StatementTheory object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
name
    The name of the theory.
terms
    A list of term definitions.
atoms
    A list of atom definitions.)doc")
        .def("__str__", &StatementTheory::to_string)
        .def("__hash__", &StatementTheory::hash)
        .def_property_readonly("location", &StatementTheory::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementTheory::name, R"doc(The name of the theory.)doc")
        .def_property_readonly("terms", &StatementTheory::terms, R"doc(A list of term definitions.)doc")
        .def_property_readonly("atoms", &StatementTheory::atoms, R"doc(A list of atom definitions.)doc")
        .def("visit", &StatementTheory::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_optimize
        .def(py::init(&StatementOptimize::construct), py::arg("lib"), py::arg("location"), py::arg("elements"),
             py::arg("optimize_type"), R"doc(Construct a StatementOptimize object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
elements
    The elements of the statement.
optimize_type
    The type of the statement.)doc")
        .def("__str__", &StatementOptimize::to_string)
        .def("__hash__", &StatementOptimize::hash)
        .def_property_readonly("location", &StatementOptimize::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("elements", &StatementOptimize::elements, R"doc(The elements of the statement.)doc")
        .def_property_readonly("optimize_type", &StatementOptimize::optimize_type,
                               R"doc(The type of the statement.)doc")
        .def("visit", &StatementOptimize::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_weak_constraint
        .def(py::init(&StatementWeakConstraint::construct), py::arg("lib"), py::arg("location"), py::arg("body"),
             py::arg("tuple"), R"doc(Construct a StatementWeakConstraint object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
body
    The body of the statement.
tuple
    The tuple of the statement.)doc")
        .def("__str__", &StatementWeakConstraint::to_string)
        .def("__hash__", &StatementWeakConstraint::hash)
        .def_property_readonly("location", &StatementWeakConstraint::location,
                               R"doc(The location of the statement.)doc")
        .def_property_readonly("body", &StatementWeakConstraint::body, R"doc(The body of the statement.)doc")
        .def_property_readonly("tuple", &StatementWeakConstraint::tuple, R"doc(The tuple of the statement.)doc")
        .def("visit", &StatementWeakConstraint::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_show
        .def(py::init(&StatementShow::construct), py::arg("lib"), py::arg("location"), py::arg("term"), py::arg("body"),
             R"doc(Construct a StatementShow object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
term
    The term to show.
body
    The body of the statement.)doc")
        .def("__str__", &StatementShow::to_string)
        .def("__hash__", &StatementShow::hash)
        .def_property_readonly("location", &StatementShow::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("term", &StatementShow::term, R"doc(The term to show.)doc")
        .def_property_readonly("body", &StatementShow::body, R"doc(The body of the statement.)doc")
        .def("visit", &StatementShow::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_show_signature
        .def(py::init(&StatementShowSignature::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arity"), py::arg("sign") = false, R"doc(Construct a StatementShowSignature object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
name
    The name of the atom to show.
arity
    The arity of the atom to show.
sign
    The classical sign of the atom.)doc")
        .def("__str__", &StatementShowSignature::to_string)
        .def("__hash__", &StatementShowSignature::hash)
        .def_property_readonly("location", &StatementShowSignature::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementShowSignature::name, R"doc(The name of the atom to show.)doc")
        .def_property_readonly("arity", &StatementShowSignature::arity, R"doc(The arity of the atom to show.)doc")
        .def_property_readonly("sign", &StatementShowSignature::sign, R"doc(The classical sign of the atom.)doc")
        .def("visit", &StatementShowSignature::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_project
        .def(py::init(&StatementProject::construct), py::arg("lib"), py::arg("location"), py::arg("atom"),
             py::arg("body"), R"doc(Construct a StatementProject object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
atom
    The atom to project.
body
    The body of the statement.)doc")
        .def("__str__", &StatementProject::to_string)
        .def("__hash__", &StatementProject::hash)
        .def_property_readonly("location", &StatementProject::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("atom", &StatementProject::atom, R"doc(The atom to project.)doc")
        .def_property_readonly("body", &StatementProject::body, R"doc(The body of the statement.)doc")
        .def("visit", &StatementProject::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_project_signature
        .def(py::init(&StatementProjectSignature::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arity"), py::arg("sign") = false, R"doc(Construct a StatementProjectSignature object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
name
    The name of the atom to project.
arity
    The arity of the atom to project.
sign
    The classical sign of the atom.)doc")
        .def("__str__", &StatementProjectSignature::to_string)
        .def("__hash__", &StatementProjectSignature::hash)
        .def_property_readonly("location", &StatementProjectSignature::location,
                               R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementProjectSignature::name, R"doc(The name of the atom to project.)doc")
        .def_property_readonly("arity", &StatementProjectSignature::arity, R"doc(The arity of the atom to project.)doc")
        .def_property_readonly("sign", &StatementProjectSignature::sign, R"doc(The classical sign of the atom.)doc")
        .def("visit", &StatementProjectSignature::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_defined
        .def(py::init(&StatementDefined::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arity"), py::arg("sign") = false, R"doc(Construct a StatementDefined object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
name
    The name of the atom to project.
arity
    The arity of the atom to project.
sign
    The classical sign of the atom.)doc")
        .def("__str__", &StatementDefined::to_string)
        .def("__hash__", &StatementDefined::hash)
        .def_property_readonly("location", &StatementDefined::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementDefined::name, R"doc(The name of the atom to project.)doc")
        .def_property_readonly("arity", &StatementDefined::arity, R"doc(The arity of the atom to project.)doc")
        .def_property_readonly("sign", &StatementDefined::sign, R"doc(The classical sign of the atom.)doc")
        .def("visit", &StatementDefined::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_external
        .def(py::init(&StatementExternal::construct), py::arg("lib"), py::arg("location"), py::arg("atom"),
             py::arg("body"), py::arg("external_type") = std::nullopt, R"doc(Construct a StatementExternal object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
atom
    The atom to project.
body
    The body of the statement.
external_type
    The type of the external.)doc")
        .def("__str__", &StatementExternal::to_string)
        .def("__hash__", &StatementExternal::hash)
        .def_property_readonly("location", &StatementExternal::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("atom", &StatementExternal::atom, R"doc(The atom to project.)doc")
        .def_property_readonly("body", &StatementExternal::body, R"doc(The body of the statement.)doc")
        .def_property_readonly("external_type", &StatementExternal::external_type, R"doc(The type of the external.)doc")
        .def("visit", &StatementExternal::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_edge
        .def(py::init(&StatementEdge::construct), py::arg("lib"), py::arg("location"), py::arg("pool"), py::arg("body"),
             R"doc(Construct a StatementEdge object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
pool
    The edge pool of the statement.
body
    The body of the statement.)doc")
        .def("__str__", &StatementEdge::to_string)
        .def("__hash__", &StatementEdge::hash)
        .def_property_readonly("location", &StatementEdge::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("pool", &StatementEdge::pool, R"doc(The edge pool of the statement.)doc")
        .def_property_readonly("body", &StatementEdge::body, R"doc(The body of the statement.)doc")
        .def("visit", &StatementEdge::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_heuristic
        .def(py::init(&StatementHeuristic::construct), py::arg("lib"), py::arg("location"), py::arg("atom"),
             py::arg("body"), py::arg("weight"), py::arg("modifier"), py::arg("priority") = std::nullopt,
             R"doc(Construct a StatementHeuristic object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
atom
    The atom to heuristically modify.
body
    The body of the statement.
weight
    The weight of the heuristic modification.
modifier
    The heuristic modifier.
priority
    An optional priority.)doc")
        .def("__str__", &StatementHeuristic::to_string)
        .def("__hash__", &StatementHeuristic::hash)
        .def_property_readonly("location", &StatementHeuristic::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("atom", &StatementHeuristic::atom, R"doc(The atom to heuristically modify.)doc")
        .def_property_readonly("body", &StatementHeuristic::body, R"doc(The body of the statement.)doc")
        .def_property_readonly("weight", &StatementHeuristic::weight,
                               R"doc(The weight of the heuristic modification.)doc")
        .def_property_readonly("modifier", &StatementHeuristic::modifier, R"doc(The heuristic modifier.)doc")
        .def_property_readonly("priority", &StatementHeuristic::priority, R"doc(An optional priority.)doc")
        .def("visit", &StatementHeuristic::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_script
        .def(py::init(&StatementScript::construct), py::arg("lib"), py::arg("location"), py::arg("value"),
             py::arg("script_type"), R"doc(Construct a StatementScript object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
value
    The content of the script.
script_type
    The type of the script.)doc")
        .def("__str__", &StatementScript::to_string)
        .def("__hash__", &StatementScript::hash)
        .def_property_readonly("location", &StatementScript::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("value", &StatementScript::value, R"doc(The content of the script.)doc")
        .def_property_readonly("script_type", &StatementScript::script_type, R"doc(The type of the script.)doc")
        .def("visit", &StatementScript::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_include
        .def(py::init(&StatementInclude::construct), py::arg("lib"), py::arg("location"), py::arg("value"),
             py::arg("include_type"), R"doc(Construct a StatementInclude object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
value
    The path of the statement.
include_type
    The type of the include.)doc")
        .def("__str__", &StatementInclude::to_string)
        .def("__hash__", &StatementInclude::hash)
        .def_property_readonly("location", &StatementInclude::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("value", &StatementInclude::value, R"doc(The path of the statement.)doc")
        .def_property_readonly("include_type", &StatementInclude::include_type, R"doc(The type of the include.)doc")
        .def("visit", &StatementInclude::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_program
        .def(py::init(&StatementProgram::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arguments"), R"doc(Construct a StatementProgram object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
name
    The name of the program.
arguments
    The arguments of the program.)doc")
        .def("__str__", &StatementProgram::to_string)
        .def("__hash__", &StatementProgram::hash)
        .def_property_readonly("location", &StatementProgram::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementProgram::name, R"doc(The name of the program.)doc")
        .def_property_readonly("arguments", &StatementProgram::arguments, R"doc(The arguments of the program.)doc")
        .def("visit", &StatementProgram::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_const
        .def(py::init(&StatementConst::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("value"), py::arg("const_type"), R"doc(Construct a StatementConst object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the statement.
name
    The name of the statement.
value
    The term of the statement.
const_type
    The type of the statement.)doc")
        .def("__str__", &StatementConst::to_string)
        .def("__hash__", &StatementConst::hash)
        .def_property_readonly("location", &StatementConst::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementConst::name, R"doc(The name of the statement.)doc")
        .def_property_readonly("value", &StatementConst::value, R"doc(The term of the statement.)doc")
        .def_property_readonly("const_type", &StatementConst::const_type, R"doc(The type of the statement.)doc")
        .def("visit", &StatementConst::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py_statement_comment
        .def(py::init(&StatementComment::construct), py::arg("lib"), py::arg("location"), py::arg("value"),
             py::arg("comment_type"), R"doc(Construct a StatementComment object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the comment.
value
    The value of the comment.
comment_type
    The type of the comment.)doc")
        .def("__str__", &StatementComment::to_string)
        .def("__hash__", &StatementComment::hash)
        .def_property_readonly("location", &StatementComment::location, R"doc(The location of the comment.)doc")
        .def_property_readonly("value", &StatementComment::value, R"doc(The value of the comment.)doc")
        .def_property_readonly("comment_type", &StatementComment::comment_type, R"doc(The type of the comment.)doc")
        .def("visit", &StatementComment::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py::class_<Scanner>(ast, "Scanner", R"doc( Scanner to parse statements.)doc")
        .def(py::init<Library &, char const *>(), py::arg("lib"), py::arg("program"),
             R"doc(Create a scanner to parse from the given string.

Parameters
----------
lib
    A library object to store symbols.
program
    The program to parse.
)doc")
        .def(py::init<Library &, std::vector<std::string>>(), py::arg("lib"), py::arg("files"), R"(
Create a scanner to parse from the given files.

The scanner follows clingo's handling of files on the command line. Filename
"-" is treated as "STDIN" and if an empty list is given, then the parser will
read from "STDIN".

Parameters
----------
lib
    A library object to store symbols.
files
    A list of files to parse.
)")
        .def(
            "__enter__", [](Scanner &scanner) -> Scanner & { return scanner; }, R"(Return self.)")
        .def("__iter__", &Scanner::iter, R"(Return an iterator over parsed statements.)")
        .def(
            "__exit__",
            [](Scanner &scanner, py::object, py::object, py::object) -> bool {
                scanner.close();
                return false;
            },
            R"doc(Close the scanner object.)doc");
    ast.def("parse_term", &parse_term, py::arg("lib"), py::arg("string"), R"doc(Parse a term.

Parameters
----------
lib
    The library object for storing symbols.
string
    The string to parse.

Returns
-------
The parsed Term object.)doc");
    ast.def("parse_theory_term", &parse_theory_term, py::arg("lib"), py::arg("string"), R"doc(Parse a theory term.

Parameters
----------
lib
    The library object for storing symbols.
string
    The string to parse.

Returns
-------
The parsed TheoryTerm object.)doc");
    ast.def("parse_literal", &parse_literal, py::arg("lib"), py::arg("string"), R"doc(Parse a literal.

Parameters
----------
lib
    The library object for storing symbols.
string
    The string to parse.

Returns
-------
The parsed Literal object.)doc");
    ast.def("parse_head_literal", &parse_head_literal, py::arg("lib"), py::arg("string"), R"doc(Parse a head literal.

Parameters
----------
lib
    The library object for storing symbols.
string
    The string to parse.

Returns
-------
The parsed HeadLiteral object.)doc");
    ast.def("parse_body_literal", &parse_body_literal, py::arg("lib"), py::arg("string"), R"doc(Parse a body literal.

Parameters
----------
lib
    The library object for storing symbols.
string
    The string to parse.

Returns
-------
The parsed BodyLiteral object.)doc");
    ast.def("parse_statement", &parse_statement, py::arg("lib"), py::arg("string"), R"doc(Parse a statement.

Parameters
----------
lib
    The library object for storing symbols.
string
    The string to parse.

Returns
-------
The parsed Statement object.)doc");
    ast.def("rewrite_statement", &rewrite_statement, py::arg("lib"), py::arg("statement"),
            py::arg("parameters") = std::vector<std::string>{}, py::arg("project_mode") = ProjectionMode::Pure,
            py::arg("project_anonymous") = false,
            R"doc(Simplify the given statement.

Parameters
----------
lib
    The library object for storing symbols.
statement
    The statement to rewrite.
parameters
    The parameters to exempt from simplification.
project_mode
    Which variables to project.
project_anonymous
    Whether to project anonymous variables in negative literals.

Returns
-------
A list of rewritten statements.)doc");
}

} // namespace Clingo::AST
