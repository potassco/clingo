#pragma once

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "core.hh"
#include "symbol.hh"

namespace Clingo::AST {

namespace py = pybind11;

using Clingo::Symbol::Symbol;

template <class... Ts> auto c_cast(std::variant<Ts...> const &var) -> clingo_ast_t *;

template <class T> auto c_cast(std::vector<T> const &arr) -> std::vector<clingo_ast_t *>;

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

enum class TheoryOperator {
    Unary = 0,
    BinaryLeft = 1,
    BinaryRight = 2,
};

enum class TheorySequenceType {
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

class TermVariable;

class TermSymbolic;

class TermAbsolute;

class TermUnaryOperation;

class TermBinaryOperation;

class TermTuple;

class TermFunction;

class TermVariable;

using Term = std::variant<TermVariable, TermSymbolic, TermAbsolute, TermUnaryOperation, TermBinaryOperation, TermTuple,
                          TermFunction>;

auto construct_term(clingo_ast_t *ast) -> Term;

using TermArray = std::vector<Term>;

auto construct_term_array(clingo_ast_t **ast, size_t size) -> TermArray;

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

    static auto construct(Library &lib, Term const &term, Relation const &relation) -> LeftGuard;

    friend auto c_cast(LeftGuard const &x) -> clingo_ast_t *;

  private:
    LeftGuard(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(LeftGuard const &x) -> clingo_ast_t * { return x.ast_; }

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

    static auto construct(Library &lib, Relation const &relation, Term const &term) -> RightGuard;

    friend auto c_cast(RightGuard const &x) -> clingo_ast_t *;

  private:
    RightGuard(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(RightGuard const &x) -> clingo_ast_t * { return x.ast_; }

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

    static auto construct(Library &lib, clingo_location_t const &location, Sign const &sign, Term const &atom)
        -> LiteralSymbolic;

    friend auto c_cast(LiteralSymbolic const &x) -> clingo_ast_t *;

  private:
    LiteralSymbolic(clingo_ast_t *ast) : ast_{ast} {}

    clingo_ast_t *ast_ = nullptr;
};

inline auto c_cast(LiteralSymbolic const &x) -> clingo_ast_t * { return x.ast_; }

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

auto parse_literal(Library &lib, char const *string) -> Literal {
    clingo_ast_t *ast;
    handle_error(lib, clingo_ast_parse_expression(lib, clingo_ast_parse_type_literal, string, &ast));
    return construct_literal(ast);
}

void register_module(pybind11::module &m) {
    auto ast = m.def_submodule("ast", doc(R"(
This module provides functions to work with Abstract Syntax Trees of logic programs.
)"));

    ast.def("_type_info_yaml", &clingo_ast_type_info_yaml, doc(R"(
Return a yaml description of the AST.

This can be used to auto-generate most of the binding.)"));

    auto py_unary_operator = py::enum_<UnaryOperator>(ast, "UnaryOperator", R"doc(Available unary operators.)doc");

    auto py_binary_operator = py::enum_<BinaryOperator>(ast, "BinaryOperator", R"doc(Available binary operators.)doc");

    auto py_sign = py::enum_<Sign>(ast, "Sign", R"doc(The available signs.)doc");

    auto py_relation = py::enum_<Relation>(ast, "Relation", R"doc(Available relation symbols.)doc");

    auto py_aggregate_function =
        py::enum_<AggregateFunction>(ast, "AggregateFunction", R"doc(Enumeration of aggregate functions.)doc");

    auto py_theory_operator =
        py::enum_<TheoryOperator>(ast, "TheoryOperator", R"doc(Enumeration of theory operators.)doc");

    auto py_theory_sequence_type =
        py::enum_<TheorySequenceType>(ast, "TheorySequenceType", R"doc(Enumeration of theory sequence types.)doc");

    auto py_theory_atom_type =
        py::enum_<TheoryAtomType>(ast, "TheoryAtomType", R"doc(Enumeration of the theory atom types.)doc");

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

    py_theory_operator.value("Unary", TheoryOperator::Unary, R"doc(An unary theory operator.)doc")
        .value("BinaryLeft", TheoryOperator::BinaryLeft, R"doc(A left associative binary operator.)doc")
        .value("BinaryRight", TheoryOperator::BinaryRight, R"doc(A right associative binary operator.)doc");

    py_theory_sequence_type.value("Tuple", TheorySequenceType::Tuple, R"doc(Theory tuples "(t1,...,tn)".)doc")
        .value("Set", TheorySequenceType::Set, R"doc(Theory sets "{t1,...,tn}".)doc")
        .value("List", TheorySequenceType::List, R"doc(Theory lists "[t1,...,tn]".)doc");

    py_theory_atom_type.value("Head", TheoryAtomType::Head, R"doc(For theory atoms that can appear in the head.)doc")
        .value("Body", TheoryAtomType::Body, R"doc(For theory atoms that can appear in the body.)doc")
        .value("Any", TheoryAtomType::Any, R"doc(For theory atoms that can appear in both head and body.)doc")
        .value("Directive", TheoryAtomType::Directive, R"doc(For theory atoms that must not have a body.)doc");

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
        .def_property_readonly("location", &Projection::location)
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
        .def_property_readonly("location", &TermVariable::location)
        .def_property_readonly("name", &TermVariable::name)
        .def_property_readonly("anonymous", &TermVariable::anonymous)
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
        .def_property_readonly("location", &TermSymbolic::location)
        .def_property_readonly("symbol", &TermSymbolic::symbol)
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
        .def_property_readonly("location", &TermAbsolute::location)
        .def_property_readonly("pool", &TermAbsolute::pool)
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
        .def_property_readonly("location", &TermUnaryOperation::location)
        .def_property_readonly("operator_type", &TermUnaryOperation::operator_type)
        .def_property_readonly("right", &TermUnaryOperation::right)
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
        .def_property_readonly("location", &TermBinaryOperation::location)
        .def_property_readonly("left", &TermBinaryOperation::left)
        .def_property_readonly("operator_type", &TermBinaryOperation::operator_type)
        .def_property_readonly("right", &TermBinaryOperation::right)
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
        .def_property_readonly("location", &TermTuple::location)
        .def_property_readonly("pool", &TermTuple::pool)
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
    The argument pool of the tuple.

    If there is more than one element in the pool, the term is
    unpooled during preprocessing.
external
    Whether the function is external.)doc")
        .def("__str__", &TermFunction::to_string)
        .def("__hash__", &TermFunction::hash)
        .def_property_readonly("location", &TermFunction::location)
        .def_property_readonly("name", &TermFunction::name)
        .def_property_readonly("pool", &TermFunction::pool)
        .def_property_readonly("external", &TermFunction::external)
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
        .def_property_readonly("arguments", &ArgumentTuple::arguments)
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
        .def_property_readonly("term", &LeftGuard::term)
        .def_property_readonly("relation", &LeftGuard::relation)
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
        .def_property_readonly("relation", &RightGuard::relation)
        .def_property_readonly("term", &RightGuard::term)
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
        .def_property_readonly("location", &LiteralBoolean::location)
        .def_property_readonly("sign", &LiteralBoolean::sign)
        .def_property_readonly("value", &LiteralBoolean::value)
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
        .def_property_readonly("location", &LiteralComparison::location)
        .def_property_readonly("sign", &LiteralComparison::sign)
        .def_property_readonly("left", &LiteralComparison::left)
        .def_property_readonly("right", &LiteralComparison::right)
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
        .def_property_readonly("location", &LiteralSymbolic::location)
        .def_property_readonly("sign", &LiteralSymbolic::sign)
        .def_property_readonly("atom", &LiteralSymbolic::atom)
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

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
}

} // namespace Clingo::AST
