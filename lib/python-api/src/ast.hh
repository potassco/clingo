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

auto parse_term(Library &lib, char const *string) {
    clingo_ast_t *ast;
    handle_error(lib, clingo_ast_parse_term(lib, string, &ast));
    return construct_term(ast);
}

void register_module(pybind11::module &m) {
    auto ast = m.def_submodule("ast", doc(R"(
This module provides functions to work with Abstract Syntax Trees of logic programs.
)"));

    ast.def("_type_info_yaml", &clingo_ast_type_info_yaml, doc(R"(
Return a yaml description of the AST.

This can be used to auto-generate most of the binding.)"));

    py::enum_<UnaryOperator>(ast, "UnaryOperator", R"(Available unary operators.)")
        .value("Minus", UnaryOperator::Minus, R"(Operator `-`.)")
        .value("Negation", UnaryOperator::Negation, R"(Operator `~`.)");

    py::enum_<BinaryOperator>(ast, "BinaryOperator", R"(Available binary operators.)")
        .value("And", BinaryOperator::And, R"(Operator `&`.)")
        .value("Division", BinaryOperator::Division, R"(Operator `/`.)")
        .value("Minus", BinaryOperator::Minus, R"(Operator `-`.)")
        .value("Modulo", BinaryOperator::Modulo, R"(Operator `%`.)")
        .value("Multiplication", BinaryOperator::Multiplication, R"(Operator `*`.)")
        .value("Or", BinaryOperator::Or, R"(Operator `|`.)")
        .value("Plus", BinaryOperator::Plus, R"(Operator `+`.)")
        .value("Power", BinaryOperator::Power, R"(Operator `**`.)")
        .value("Xor", BinaryOperator::Xor, R"(Operator `^`.)");

    py::class_<Projection>(ast, "Projection", R"doc(A placeholder for an argument to project.)doc")
        .def(py::init(&Projection::construct), py::arg("lib"), py::arg("location"), R"doc(Construct a Projection object.

Parameters
----------
location
    The location of the placeholder.)doc")
        .def("__str__", &Projection::to_string)
        .def("__hash__", &Projection::hash)
        .def_property_readonly("location", &Projection::location)
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py::class_<TermVariable>(ast, "TermVariable", R"doc(A term representing a variable.)doc")
        .def(py::init(&TermVariable::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("anonymous") = false, R"doc(Construct a TermVariable object.

Parameters
----------
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

    py::class_<TermSymbolic>(ast, "TermSymbolic", R"doc(A term representing a symbol.)doc")
        .def(py::init(&TermSymbolic::construct), py::arg("lib"), py::arg("location"), py::arg("symbol"),
             R"doc(Construct a TermSymbolic object.

Parameters
----------
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

    py::class_<TermAbsolute>(ast, "TermAbsolute", R"doc(A term representing the absolute operation.)doc")
        .def(py::init(&TermAbsolute::construct), py::arg("lib"), py::arg("location"), py::arg("pool"),
             R"doc(Construct a TermAbsolute object.

Parameters
----------
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

    py::class_<TermUnaryOperation>(ast, "TermUnaryOperation", R"doc(A term representing a unary operation.)doc")
        .def(py::init(&TermUnaryOperation::construct), py::arg("lib"), py::arg("location"), py::arg("operator_type"),
             py::arg("right"), R"doc(Construct a TermUnaryOperation object.

Parameters
----------
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

    py::class_<TermBinaryOperation>(ast, "TermBinaryOperation", R"doc(A term representing a binary operation.)doc")
        .def(py::init(&TermBinaryOperation::construct), py::arg("lib"), py::arg("location"), py::arg("left"),
             py::arg("operator_type"), py::arg("right"), R"doc(Construct a TermBinaryOperation object.

Parameters
----------
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

    py::class_<TermTuple>(ast, "TermTuple", R"doc(A term representing a tuple.)doc")
        .def(py::init(&TermTuple::construct), py::arg("lib"), py::arg("location"), py::arg("pool"),
             R"doc(Construct a TermTuple object.

Parameters
----------
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

    py::class_<TermFunction>(ast, "TermFunction", R"doc(A term representing a function.)doc")
        .def(py::init(&TermFunction::construct), py::arg("lib"), py::arg("location"), py::arg("name"), py::arg("pool"),
             py::arg("external") = false, R"doc(Construct a TermFunction object.

Parameters
----------
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

    py::class_<ArgumentTuple>(ast, "ArgumentTuple", R"doc(A list of arguments for a function or tuple.)doc")
        .def(py::init(&ArgumentTuple::construct), py::arg("lib"), py::arg("arguments") = TermOrProjectionArray{},
             R"doc(Construct a ArgumentTuple object.

Parameters
----------
arguments
    The arguments of the tuple.)doc")
        .def("__str__", &ArgumentTuple::to_string)
        .def("__hash__", &ArgumentTuple::hash)
        .def_property_readonly("arguments", &ArgumentTuple::arguments)
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
}

} // namespace Clingo::AST
