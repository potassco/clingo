#include "ast.hh"
#include "control.hh"
#include "core.hh"
#include "iterable.hh"
#include "symbol.hh"
#include "util.hh"

#include <pybind11/functional.h>
#include <pybind11/native_enum.h>
#include <pybind11/operators.h>

#include <algorithm>

// NOLINTBEGIN(readability-convert-member-functions-to-static,performance-enum-size)

namespace PyClingo::AST {

namespace py = pybind11;

using StringArray = std::vector<std::string>;
using StringIterable = Iterable<std::string>;

using SymbolArray = std::vector<Symbol>;
using SymbolIterable = Iterable<Symbol>;

auto to_string_array(std::vector<std::string_view> arr) -> StringArray {
    auto res = StringArray();
    res.insert(res.end(), arr.begin(), arr.end());
    return res;
}

template <class T> auto c_cast(std::optional<T> const &opt) -> clingo_ast_t *;

template <class... Ts> auto c_cast(std::variant<Ts...> const &var) -> clingo_ast_t *;

template <class T> auto c_cast(std::vector<T> const &arr) -> std::vector<clingo_ast_t *>;

auto c_cast(StringArray const &arr) -> std::vector<clingo_string_t> {
    std::vector<clingo_string_t> ret;
    ret.reserve(arr.size());
    for (auto const &str : arr) {
        ret.emplace_back(str.data(), str.size());
    }
    return ret;
}

auto c_cast(SymbolArray const &arr) -> std::vector<clingo_symbol_t> {
    std::vector<clingo_symbol_t> ret;
    ret.reserve(arr.size());
    for (auto const &sym : arr) {
        ret.emplace_back(sym.handle());
    }
    return ret;
}

template <class T> auto c_cast(Iterable<T> const &it) {
    return c_cast(it.vector());
}

template <class Cons>
void visit_array(clingo_ast_t *ast, clingo_ast_attribute_t attr, py::handle visitor, py::args const &args,
                 py::kwargs const &kwargs, Cons cons);

template <class Array>
auto transform_array(Array arr, py::handle transform, py::args const &args, py::kwargs const &kwargs)
    -> std::pair<Array, bool>;

template <class Value>
auto transform_value(Value val, py::handle transform, py::args const &args, py::kwargs const &kwargs)
    -> std::pair<Value, bool>;

template <class Value>
auto transform_opt_value(Value opt, py::handle transform, py::args const &args, py::kwargs const &kwargs)
    -> std::pair<Value, bool>;

class ASTBase {
  public:
    // Note: for pybind
    ASTBase() = default;

    ASTBase(ASTBase const &x) { handle_error(clingo_ast_copy(x.ast_, &ast_)); }

    ASTBase(ASTBase &&x) noexcept { std::swap(ast_, x.ast_); }

    ~ASTBase() noexcept { clingo_ast_free(ast_); }

    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    auto operator=(ASTBase const &x) -> ASTBase & {
        if (ast_ != x.ast_) {
            clingo_ast_free(ast_);
            ast_ = nullptr;
            handle_error(clingo_ast_copy(x.ast_, &ast_));
        }
        return *this;
    }

    auto operator=(ASTBase &&x) noexcept -> ASTBase & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    auto to_string() -> std::string_view {
        auto *bld = string_builder();
        handle_error(clingo_ast_to_string(ast_, bld));
        clingo_string_t str;
        handle_error(clingo_string_builder_string(bld, &str));
        return {str.data, str.size};
    }

    friend auto operator==(ASTBase const &a, ASTBase const &b) -> bool { return clingo_ast_equal(a.ast_, b.ast_); }

    friend auto operator<=>(ASTBase const &a, ASTBase const &b) -> std::strong_ordering {
        return clingo_ast_compare(a.ast_, b.ast_) <=> 0;
    }

    friend auto c_cast(ASTBase const &x) -> clingo_ast_t *;
    friend auto c_cast(ASTBase const *x) -> clingo_ast_t *;

  protected:
    ASTBase(clingo_ast_t *ast) : ast_{ast} {}

    // NOLINTNEXTLINE
    clingo_ast_t *ast_ = nullptr;
};

auto c_cast(ASTBase const &x) -> clingo_ast_t * {
    return x.ast_;
}

auto c_cast(ASTBase const *x) -> clingo_ast_t * {
    return x->ast_;
}

struct CString {
    CString(std::string_view str) : str_{str} {}
    CString(std::string &&str) : str_{std::move(str)} {}
    operator std::string_view() const {
        return std::visit(
            []<class T>(T const &x) {
                if constexpr (std::is_same_v<T, std::string>) {
                    return std::string_view{x};
                } else {
                    return x;
                }
            },
            str_);
    }
    std::variant<std::string, std::string_view> str_;
};

template <class T> using update_result_t = std::conditional_t<std::is_same_v<T, std::string_view>, CString, T>;

template <class T, class F, class M>
auto update_value(F *self, M fun, py::kwargs const &kwargs, char const *attr) -> update_result_t<T>;

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

enum class Precedence {
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

class TermFormatString;

using Term = std::variant<TermVariable, TermSymbolic, TermAbsolute, TermUnaryOperation, TermBinaryOperation, TermTuple,
                          TermFunction, TermFormatString>;

auto construct_term(clingo_ast_t *ast) -> Term;

using TermArray = std::vector<Term>;
using TermIterable = Iterable<Term>;

auto construct_term_array(clingo_ast_t **ast, size_t size) -> TermArray;

using OptionalTerm = std::optional<Term>;

class FormatFieldLiteral;

class FormatFieldExpression;

using FormatField = std::variant<FormatFieldLiteral, FormatFieldExpression>;

auto construct_format_field(clingo_ast_t *ast) -> FormatField;

using FormatFieldArray = std::vector<FormatField>;
using FormatFieldIterable = Iterable<FormatField>;

auto construct_format_field_array(clingo_ast_t **ast, size_t size) -> FormatFieldArray;

class FormatFieldLiteral : public ASTBase {
  public:
    FormatFieldLiteral() = default;
    FormatFieldLiteral(FormatFieldLiteral const &x) = default;
    FormatFieldLiteral(FormatFieldLiteral &&x) noexcept = default;
    auto operator=(FormatFieldLiteral const &x) -> FormatFieldLiteral & = default;
    auto operator=(FormatFieldLiteral &&x) noexcept -> FormatFieldLiteral & = default;
    ~FormatFieldLiteral() noexcept = default;

    auto location() -> Location;
    auto value() -> std::string_view;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<FormatFieldLiteral>;
    auto update(Library &lib, py::kwargs const &kwargs) -> FormatFieldLiteral;

    static auto construct(Library &lib, Location const &location, std::string_view value) -> FormatFieldLiteral;
    static auto acquire(clingo_ast_t *ast) -> FormatFieldLiteral { return {ast}; }

    friend auto operator==(FormatFieldLiteral const &a, FormatFieldLiteral const &b) -> bool = default;
    friend auto operator<=>(FormatFieldLiteral const &a, FormatFieldLiteral const &b) -> std::strong_ordering = default;

  private:
    FormatFieldLiteral(clingo_ast_t *ast) : ASTBase{ast} {}
};

class FormatFieldExpression : public ASTBase {
  public:
    FormatFieldExpression() = default;
    FormatFieldExpression(FormatFieldExpression const &x) = default;
    FormatFieldExpression(FormatFieldExpression &&x) noexcept = default;
    auto operator=(FormatFieldExpression const &x) -> FormatFieldExpression & = default;
    auto operator=(FormatFieldExpression &&x) noexcept -> FormatFieldExpression & = default;
    ~FormatFieldExpression() noexcept = default;

    auto location() -> Location;
    auto left() -> Term;
    auto right() -> std::string_view;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<FormatFieldExpression>;
    auto update(Library &lib, py::kwargs const &kwargs) -> FormatFieldExpression;

    static auto construct(Library &lib, Location const &location, Term const &left, std::string_view right)
        -> FormatFieldExpression;
    static auto acquire(clingo_ast_t *ast) -> FormatFieldExpression { return {ast}; }

    friend auto operator==(FormatFieldExpression const &a, FormatFieldExpression const &b) -> bool = default;
    friend auto operator<=>(FormatFieldExpression const &a, FormatFieldExpression const &b)
        -> std::strong_ordering = default;

  private:
    FormatFieldExpression(clingo_ast_t *ast) : ASTBase{ast} {}
};

class Projection : public ASTBase {
  public:
    Projection() = default;
    Projection(Projection const &x) = default;
    Projection(Projection &&x) noexcept = default;
    auto operator=(Projection const &x) -> Projection & = default;
    auto operator=(Projection &&x) noexcept -> Projection & = default;
    ~Projection() noexcept = default;

    auto location() -> Location;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<Projection>;
    auto update(Library &lib, py::kwargs const &kwargs) -> Projection;

    static auto construct(Library &lib, Location const &location) -> Projection;
    static auto acquire(clingo_ast_t *ast) -> Projection { return {ast}; }

    friend auto operator==(Projection const &a, Projection const &b) -> bool = default;
    friend auto operator<=>(Projection const &a, Projection const &b) -> std::strong_ordering = default;

  private:
    Projection(clingo_ast_t *ast) : ASTBase{ast} {}
};

using TermOrProjection = std::variant<Term, Projection>;

auto construct_term_or_projection(clingo_ast_t *ast) -> TermOrProjection;

using TermOrProjectionArray = std::vector<TermOrProjection>;
using TermOrProjectionIterable = Iterable<TermOrProjection>;

auto construct_term_or_projection_array(clingo_ast_t **ast, size_t size) -> TermOrProjectionArray;

class ArgumentTuple;

using ArgumentTupleArray = std::vector<ArgumentTuple>;
using ArgumentTupleIterable = Iterable<ArgumentTuple>;

auto construct_argument_tuple_array(clingo_ast_t **ast, size_t size) -> ArgumentTupleArray;

class ArgumentTuple;

using TermOrArgumentTuple = std::variant<Term, ArgumentTuple>;

auto construct_term_or_argument_tuple(clingo_ast_t *ast) -> TermOrArgumentTuple;

using TermOrArgumentTupleArray = std::vector<TermOrArgumentTuple>;
using TermOrArgumentTupleIterable = Iterable<TermOrArgumentTuple>;

auto construct_term_or_argument_tuple_array(clingo_ast_t **ast, size_t size) -> TermOrArgumentTupleArray;

class TermFormatString : public ASTBase {
  public:
    TermFormatString() = default;
    TermFormatString(TermFormatString const &x) = default;
    TermFormatString(TermFormatString &&x) noexcept = default;
    auto operator=(TermFormatString const &x) -> TermFormatString & = default;
    auto operator=(TermFormatString &&x) noexcept -> TermFormatString & = default;
    ~TermFormatString() noexcept = default;

    auto location() -> Location;
    auto elements() -> FormatFieldArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TermFormatString>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TermFormatString;

    static auto construct(Library &lib, Location const &location, FormatFieldIterable const &elements)
        -> TermFormatString;
    static auto acquire(clingo_ast_t *ast) -> TermFormatString { return {ast}; }

    friend auto operator==(TermFormatString const &a, TermFormatString const &b) -> bool = default;
    friend auto operator<=>(TermFormatString const &a, TermFormatString const &b) -> std::strong_ordering = default;

  private:
    TermFormatString(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TermVariable : public ASTBase {
  public:
    TermVariable() = default;
    TermVariable(TermVariable const &x) = default;
    TermVariable(TermVariable &&x) noexcept = default;
    auto operator=(TermVariable const &x) -> TermVariable & = default;
    auto operator=(TermVariable &&x) noexcept -> TermVariable & = default;
    ~TermVariable() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto anonymous() -> bool;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TermVariable>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TermVariable;

    static auto construct(Library &lib, Location const &location, std::string_view name, bool anonymous)
        -> TermVariable;
    static auto acquire(clingo_ast_t *ast) -> TermVariable { return {ast}; }

    friend auto operator==(TermVariable const &a, TermVariable const &b) -> bool = default;
    friend auto operator<=>(TermVariable const &a, TermVariable const &b) -> std::strong_ordering = default;

  private:
    TermVariable(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TermSymbolic : public ASTBase {
  public:
    TermSymbolic() = default;
    TermSymbolic(TermSymbolic const &x) = default;
    TermSymbolic(TermSymbolic &&x) noexcept = default;
    auto operator=(TermSymbolic const &x) -> TermSymbolic & = default;
    auto operator=(TermSymbolic &&x) noexcept -> TermSymbolic & = default;
    ~TermSymbolic() noexcept = default;

    auto location() -> Location;
    auto symbol() -> Symbol;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TermSymbolic>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TermSymbolic;

    static auto construct(Library &lib, Location const &location, Symbol const &symbol) -> TermSymbolic;
    static auto acquire(clingo_ast_t *ast) -> TermSymbolic { return {ast}; }

    friend auto operator==(TermSymbolic const &a, TermSymbolic const &b) -> bool = default;
    friend auto operator<=>(TermSymbolic const &a, TermSymbolic const &b) -> std::strong_ordering = default;

  private:
    TermSymbolic(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TermAbsolute : public ASTBase {
  public:
    TermAbsolute() = default;
    TermAbsolute(TermAbsolute const &x) = default;
    TermAbsolute(TermAbsolute &&x) noexcept = default;
    auto operator=(TermAbsolute const &x) -> TermAbsolute & = default;
    auto operator=(TermAbsolute &&x) noexcept -> TermAbsolute & = default;
    ~TermAbsolute() noexcept = default;

    auto location() -> Location;
    auto pool() -> TermArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TermAbsolute>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TermAbsolute;

    static auto construct(Library &lib, Location const &location, TermIterable const &pool) -> TermAbsolute;
    static auto acquire(clingo_ast_t *ast) -> TermAbsolute { return {ast}; }

    friend auto operator==(TermAbsolute const &a, TermAbsolute const &b) -> bool = default;
    friend auto operator<=>(TermAbsolute const &a, TermAbsolute const &b) -> std::strong_ordering = default;

  private:
    TermAbsolute(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TermUnaryOperation : public ASTBase {
  public:
    TermUnaryOperation() = default;
    TermUnaryOperation(TermUnaryOperation const &x) = default;
    TermUnaryOperation(TermUnaryOperation &&x) noexcept = default;
    auto operator=(TermUnaryOperation const &x) -> TermUnaryOperation & = default;
    auto operator=(TermUnaryOperation &&x) noexcept -> TermUnaryOperation & = default;
    ~TermUnaryOperation() noexcept = default;

    auto location() -> Location;
    auto operator_type() -> UnaryOperator;
    auto right() -> Term;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TermUnaryOperation>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TermUnaryOperation;

    static auto construct(Library &lib, Location const &location, UnaryOperator const &operator_type, Term const &right)
        -> TermUnaryOperation;
    static auto acquire(clingo_ast_t *ast) -> TermUnaryOperation { return {ast}; }

    friend auto operator==(TermUnaryOperation const &a, TermUnaryOperation const &b) -> bool = default;
    friend auto operator<=>(TermUnaryOperation const &a, TermUnaryOperation const &b) -> std::strong_ordering = default;

  private:
    TermUnaryOperation(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TermBinaryOperation : public ASTBase {
  public:
    TermBinaryOperation() = default;
    TermBinaryOperation(TermBinaryOperation const &x) = default;
    TermBinaryOperation(TermBinaryOperation &&x) noexcept = default;
    auto operator=(TermBinaryOperation const &x) -> TermBinaryOperation & = default;
    auto operator=(TermBinaryOperation &&x) noexcept -> TermBinaryOperation & = default;
    ~TermBinaryOperation() noexcept = default;

    auto location() -> Location;
    auto left() -> Term;
    auto operator_type() -> BinaryOperator;
    auto right() -> Term;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TermBinaryOperation>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TermBinaryOperation;

    static auto construct(Library &lib, Location const &location, Term const &left, BinaryOperator const &operator_type,
                          Term const &right) -> TermBinaryOperation;
    static auto acquire(clingo_ast_t *ast) -> TermBinaryOperation { return {ast}; }

    friend auto operator==(TermBinaryOperation const &a, TermBinaryOperation const &b) -> bool = default;
    friend auto operator<=>(TermBinaryOperation const &a, TermBinaryOperation const &b)
        -> std::strong_ordering = default;

  private:
    TermBinaryOperation(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TermTuple : public ASTBase {
  public:
    TermTuple() = default;
    TermTuple(TermTuple const &x) = default;
    TermTuple(TermTuple &&x) noexcept = default;
    auto operator=(TermTuple const &x) -> TermTuple & = default;
    auto operator=(TermTuple &&x) noexcept -> TermTuple & = default;
    ~TermTuple() noexcept = default;

    auto location() -> Location;
    auto pool() -> TermOrArgumentTupleArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TermTuple>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TermTuple;

    static auto construct(Library &lib, Location const &location, TermOrArgumentTupleIterable const &pool) -> TermTuple;
    static auto acquire(clingo_ast_t *ast) -> TermTuple { return {ast}; }

    friend auto operator==(TermTuple const &a, TermTuple const &b) -> bool = default;
    friend auto operator<=>(TermTuple const &a, TermTuple const &b) -> std::strong_ordering = default;

  private:
    TermTuple(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TermFunction : public ASTBase {
  public:
    TermFunction() = default;
    TermFunction(TermFunction const &x) = default;
    TermFunction(TermFunction &&x) noexcept = default;
    auto operator=(TermFunction const &x) -> TermFunction & = default;
    auto operator=(TermFunction &&x) noexcept -> TermFunction & = default;
    ~TermFunction() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto pool() -> ArgumentTupleArray;
    auto external() -> bool;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TermFunction>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TermFunction;

    static auto construct(Library &lib, Location const &location, std::string_view name,
                          ArgumentTupleIterable const &pool, bool external) -> TermFunction;
    static auto acquire(clingo_ast_t *ast) -> TermFunction { return {ast}; }

    friend auto operator==(TermFunction const &a, TermFunction const &b) -> bool = default;
    friend auto operator<=>(TermFunction const &a, TermFunction const &b) -> std::strong_ordering = default;

  private:
    TermFunction(clingo_ast_t *ast) : ASTBase{ast} {}
};

class ArgumentTuple : public ASTBase {
  public:
    ArgumentTuple() = default;
    ArgumentTuple(ArgumentTuple const &x) = default;
    ArgumentTuple(ArgumentTuple &&x) noexcept = default;
    auto operator=(ArgumentTuple const &x) -> ArgumentTuple & = default;
    auto operator=(ArgumentTuple &&x) noexcept -> ArgumentTuple & = default;
    ~ArgumentTuple() noexcept = default;

    auto arguments() -> TermOrProjectionArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<ArgumentTuple>;
    auto update(Library &lib, py::kwargs const &kwargs) -> ArgumentTuple;

    static auto construct(Library &lib, TermOrProjectionIterable const &arguments) -> ArgumentTuple;
    static auto acquire(clingo_ast_t *ast) -> ArgumentTuple { return {ast}; }

    friend auto operator==(ArgumentTuple const &a, ArgumentTuple const &b) -> bool = default;
    friend auto operator<=>(ArgumentTuple const &a, ArgumentTuple const &b) -> std::strong_ordering = default;

  private:
    ArgumentTuple(clingo_ast_t *ast) : ASTBase{ast} {}
};

class LiteralBoolean;

class LiteralComparison;

class LiteralSymbolic;

using Literal = std::variant<LiteralBoolean, LiteralComparison, LiteralSymbolic>;

auto construct_literal(clingo_ast_t *ast) -> Literal;

class LeftGuard : public ASTBase {
  public:
    LeftGuard() = default;
    LeftGuard(LeftGuard const &x) = default;
    LeftGuard(LeftGuard &&x) noexcept = default;
    auto operator=(LeftGuard const &x) -> LeftGuard & = default;
    auto operator=(LeftGuard &&x) noexcept -> LeftGuard & = default;
    ~LeftGuard() noexcept = default;

    auto term() -> Term;
    auto relation() -> Relation;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<LeftGuard>;
    auto update(Library &lib, py::kwargs const &kwargs) -> LeftGuard;

    static auto construct(Library &lib, Term const &term, Relation const &relation) -> LeftGuard;
    static auto acquire(clingo_ast_t *ast) -> LeftGuard { return {ast}; }

    friend auto operator==(LeftGuard const &a, LeftGuard const &b) -> bool = default;
    friend auto operator<=>(LeftGuard const &a, LeftGuard const &b) -> std::strong_ordering = default;

  private:
    LeftGuard(clingo_ast_t *ast) : ASTBase{ast} {}
};

using OptionalLeftGuard = std::optional<LeftGuard>;

class RightGuard : public ASTBase {
  public:
    RightGuard() = default;
    RightGuard(RightGuard const &x) = default;
    RightGuard(RightGuard &&x) noexcept = default;
    auto operator=(RightGuard const &x) -> RightGuard & = default;
    auto operator=(RightGuard &&x) noexcept -> RightGuard & = default;
    ~RightGuard() noexcept = default;

    auto relation() -> Relation;
    auto term() -> Term;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<RightGuard>;
    auto update(Library &lib, py::kwargs const &kwargs) -> RightGuard;

    static auto construct(Library &lib, Relation const &relation, Term const &term) -> RightGuard;
    static auto acquire(clingo_ast_t *ast) -> RightGuard { return {ast}; }

    friend auto operator==(RightGuard const &a, RightGuard const &b) -> bool = default;
    friend auto operator<=>(RightGuard const &a, RightGuard const &b) -> std::strong_ordering = default;

  private:
    RightGuard(clingo_ast_t *ast) : ASTBase{ast} {}
};

using OptionalRightGuard = std::optional<RightGuard>;

using RightGuardArray = std::vector<RightGuard>;
using RightGuardIterable = Iterable<RightGuard>;

auto construct_right_guard_array(clingo_ast_t **ast, size_t size) -> RightGuardArray;

class LiteralBoolean : public ASTBase {
  public:
    LiteralBoolean() = default;
    LiteralBoolean(LiteralBoolean const &x) = default;
    LiteralBoolean(LiteralBoolean &&x) noexcept = default;
    auto operator=(LiteralBoolean const &x) -> LiteralBoolean & = default;
    auto operator=(LiteralBoolean &&x) noexcept -> LiteralBoolean & = default;
    ~LiteralBoolean() noexcept = default;

    auto location() -> Location;
    auto sign() -> Sign;
    auto value() -> bool;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<LiteralBoolean>;
    auto update(Library &lib, py::kwargs const &kwargs) -> LiteralBoolean;

    static auto construct(Library &lib, Location const &location, Sign const &sign, bool value) -> LiteralBoolean;
    static auto acquire(clingo_ast_t *ast) -> LiteralBoolean { return {ast}; }

    friend auto operator==(LiteralBoolean const &a, LiteralBoolean const &b) -> bool = default;
    friend auto operator<=>(LiteralBoolean const &a, LiteralBoolean const &b) -> std::strong_ordering = default;

  private:
    LiteralBoolean(clingo_ast_t *ast) : ASTBase{ast} {}
};

class LiteralComparison : public ASTBase {
  public:
    LiteralComparison() = default;
    LiteralComparison(LiteralComparison const &x) = default;
    LiteralComparison(LiteralComparison &&x) noexcept = default;
    auto operator=(LiteralComparison const &x) -> LiteralComparison & = default;
    auto operator=(LiteralComparison &&x) noexcept -> LiteralComparison & = default;
    ~LiteralComparison() noexcept = default;

    auto location() -> Location;
    auto sign() -> Sign;
    auto left() -> Term;
    auto right() -> RightGuardArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<LiteralComparison>;
    auto update(Library &lib, py::kwargs const &kwargs) -> LiteralComparison;

    static auto construct(Library &lib, Location const &location, Sign const &sign, Term const &left,
                          RightGuardIterable const &right) -> LiteralComparison;
    static auto acquire(clingo_ast_t *ast) -> LiteralComparison { return {ast}; }

    friend auto operator==(LiteralComparison const &a, LiteralComparison const &b) -> bool = default;
    friend auto operator<=>(LiteralComparison const &a, LiteralComparison const &b) -> std::strong_ordering = default;

  private:
    LiteralComparison(clingo_ast_t *ast) : ASTBase{ast} {}
};

class LiteralSymbolic : public ASTBase {
  public:
    LiteralSymbolic() = default;
    LiteralSymbolic(LiteralSymbolic const &x) = default;
    LiteralSymbolic(LiteralSymbolic &&x) noexcept = default;
    auto operator=(LiteralSymbolic const &x) -> LiteralSymbolic & = default;
    auto operator=(LiteralSymbolic &&x) noexcept -> LiteralSymbolic & = default;
    ~LiteralSymbolic() noexcept = default;

    auto location() -> Location;
    auto sign() -> Sign;
    auto atom() -> Term;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<LiteralSymbolic>;
    auto update(Library &lib, py::kwargs const &kwargs) -> LiteralSymbolic;

    static auto construct(Library &lib, Location const &location, Sign const &sign, Term const &atom)
        -> LiteralSymbolic;
    static auto acquire(clingo_ast_t *ast) -> LiteralSymbolic { return {ast}; }

    friend auto operator==(LiteralSymbolic const &a, LiteralSymbolic const &b) -> bool = default;
    friend auto operator<=>(LiteralSymbolic const &a, LiteralSymbolic const &b) -> std::strong_ordering = default;

  private:
    LiteralSymbolic(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TheoryTermVariable;

class TheoryTermSymbolic;

class TheoryTermTuple;

class TheoryTermFunction;

class TheoryTermUnparsed;

using TheoryTerm =
    std::variant<TheoryTermVariable, TheoryTermSymbolic, TheoryTermTuple, TheoryTermFunction, TheoryTermUnparsed>;

auto construct_theory_term(clingo_ast_t *ast) -> TheoryTerm;

using TheoryTermArray = std::vector<TheoryTerm>;
using TheoryTermIterable = Iterable<TheoryTerm>;

auto construct_theory_term_array(clingo_ast_t **ast, size_t size) -> TheoryTermArray;

class UnparsedElement : public ASTBase {
  public:
    UnparsedElement() = default;
    UnparsedElement(UnparsedElement const &x) = default;
    UnparsedElement(UnparsedElement &&x) noexcept = default;
    auto operator=(UnparsedElement const &x) -> UnparsedElement & = default;
    auto operator=(UnparsedElement &&x) noexcept -> UnparsedElement & = default;
    ~UnparsedElement() noexcept = default;

    auto operators() -> std::vector<std::string_view>;
    auto term() -> TheoryTerm;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<UnparsedElement>;
    auto update(Library &lib, py::kwargs const &kwargs) -> UnparsedElement;

    static auto construct(Library &lib, StringIterable const &operators, TheoryTerm const &term) -> UnparsedElement;
    static auto acquire(clingo_ast_t *ast) -> UnparsedElement { return {ast}; }

    friend auto operator==(UnparsedElement const &a, UnparsedElement const &b) -> bool = default;
    friend auto operator<=>(UnparsedElement const &a, UnparsedElement const &b) -> std::strong_ordering = default;

  private:
    UnparsedElement(clingo_ast_t *ast) : ASTBase{ast} {}
};

using UnparsedElementArray = std::vector<UnparsedElement>;
using UnparsedElementIterable = Iterable<UnparsedElement>;

auto construct_unparsed_element_array(clingo_ast_t **ast, size_t size) -> UnparsedElementArray;

class TheoryTermVariable : public ASTBase {
  public:
    TheoryTermVariable() = default;
    TheoryTermVariable(TheoryTermVariable const &x) = default;
    TheoryTermVariable(TheoryTermVariable &&x) noexcept = default;
    auto operator=(TheoryTermVariable const &x) -> TheoryTermVariable & = default;
    auto operator=(TheoryTermVariable &&x) noexcept -> TheoryTermVariable & = default;
    ~TheoryTermVariable() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto anonymous() -> bool;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TheoryTermVariable>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TheoryTermVariable;

    static auto construct(Library &lib, Location const &location, std::string_view name, bool anonymous)
        -> TheoryTermVariable;
    static auto acquire(clingo_ast_t *ast) -> TheoryTermVariable { return {ast}; }

    friend auto operator==(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool = default;
    friend auto operator<=>(TheoryTermVariable const &a, TheoryTermVariable const &b) -> std::strong_ordering = default;

  private:
    TheoryTermVariable(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TheoryTermSymbolic : public ASTBase {
  public:
    TheoryTermSymbolic() = default;
    TheoryTermSymbolic(TheoryTermSymbolic const &x) = default;
    TheoryTermSymbolic(TheoryTermSymbolic &&x) noexcept = default;
    auto operator=(TheoryTermSymbolic const &x) -> TheoryTermSymbolic & = default;
    auto operator=(TheoryTermSymbolic &&x) noexcept -> TheoryTermSymbolic & = default;
    ~TheoryTermSymbolic() noexcept = default;

    auto location() -> Location;
    auto symbol() -> Symbol;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TheoryTermSymbolic>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TheoryTermSymbolic;

    static auto construct(Library &lib, Location const &location, Symbol const &symbol) -> TheoryTermSymbolic;
    static auto acquire(clingo_ast_t *ast) -> TheoryTermSymbolic { return {ast}; }

    friend auto operator==(TheoryTermSymbolic const &a, TheoryTermSymbolic const &b) -> bool = default;
    friend auto operator<=>(TheoryTermSymbolic const &a, TheoryTermSymbolic const &b) -> std::strong_ordering = default;

  private:
    TheoryTermSymbolic(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TheoryTermTuple : public ASTBase {
  public:
    TheoryTermTuple() = default;
    TheoryTermTuple(TheoryTermTuple const &x) = default;
    TheoryTermTuple(TheoryTermTuple &&x) noexcept = default;
    auto operator=(TheoryTermTuple const &x) -> TheoryTermTuple & = default;
    auto operator=(TheoryTermTuple &&x) noexcept -> TheoryTermTuple & = default;
    ~TheoryTermTuple() noexcept = default;

    auto location() -> Location;
    auto tuple_type() -> TheoryTupleType;
    auto arguments() -> TheoryTermArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TheoryTermTuple>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TheoryTermTuple;

    static auto construct(Library &lib, Location const &location, TheoryTupleType const &tuple_type,
                          TheoryTermIterable const &arguments) -> TheoryTermTuple;
    static auto acquire(clingo_ast_t *ast) -> TheoryTermTuple { return {ast}; }

    friend auto operator==(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool = default;
    friend auto operator<=>(TheoryTermTuple const &a, TheoryTermTuple const &b) -> std::strong_ordering = default;

  private:
    TheoryTermTuple(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TheoryTermFunction : public ASTBase {
  public:
    TheoryTermFunction() = default;
    TheoryTermFunction(TheoryTermFunction const &x) = default;
    TheoryTermFunction(TheoryTermFunction &&x) noexcept = default;
    auto operator=(TheoryTermFunction const &x) -> TheoryTermFunction & = default;
    auto operator=(TheoryTermFunction &&x) noexcept -> TheoryTermFunction & = default;
    ~TheoryTermFunction() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto arguments() -> TheoryTermArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TheoryTermFunction>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TheoryTermFunction;

    static auto construct(Library &lib, Location const &location, std::string_view name,
                          TheoryTermIterable const &arguments) -> TheoryTermFunction;
    static auto acquire(clingo_ast_t *ast) -> TheoryTermFunction { return {ast}; }

    friend auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool = default;
    friend auto operator<=>(TheoryTermFunction const &a, TheoryTermFunction const &b) -> std::strong_ordering = default;

  private:
    TheoryTermFunction(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TheoryTermUnparsed : public ASTBase {
  public:
    TheoryTermUnparsed() = default;
    TheoryTermUnparsed(TheoryTermUnparsed const &x) = default;
    TheoryTermUnparsed(TheoryTermUnparsed &&x) noexcept = default;
    auto operator=(TheoryTermUnparsed const &x) -> TheoryTermUnparsed & = default;
    auto operator=(TheoryTermUnparsed &&x) noexcept -> TheoryTermUnparsed & = default;
    ~TheoryTermUnparsed() noexcept = default;

    auto location() -> Location;
    auto elements() -> UnparsedElementArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TheoryTermUnparsed>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TheoryTermUnparsed;

    static auto construct(Library &lib, Location const &location, UnparsedElementIterable const &elements)
        -> TheoryTermUnparsed;
    static auto acquire(clingo_ast_t *ast) -> TheoryTermUnparsed { return {ast}; }

    friend auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool = default;
    friend auto operator<=>(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> std::strong_ordering = default;

  private:
    TheoryTermUnparsed(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TheoryRightGuard : public ASTBase {
  public:
    TheoryRightGuard() = default;
    TheoryRightGuard(TheoryRightGuard const &x) = default;
    TheoryRightGuard(TheoryRightGuard &&x) noexcept = default;
    auto operator=(TheoryRightGuard const &x) -> TheoryRightGuard & = default;
    auto operator=(TheoryRightGuard &&x) noexcept -> TheoryRightGuard & = default;
    ~TheoryRightGuard() noexcept = default;

    auto theory_operator() -> std::string_view;
    auto term() -> TheoryTerm;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TheoryRightGuard>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TheoryRightGuard;

    static auto construct(Library &lib, std::string_view theory_operator, TheoryTerm const &term) -> TheoryRightGuard;
    static auto acquire(clingo_ast_t *ast) -> TheoryRightGuard { return {ast}; }

    friend auto operator==(TheoryRightGuard const &a, TheoryRightGuard const &b) -> bool = default;
    friend auto operator<=>(TheoryRightGuard const &a, TheoryRightGuard const &b) -> std::strong_ordering = default;

  private:
    TheoryRightGuard(clingo_ast_t *ast) : ASTBase{ast} {}
};

using OptionalTheoryRightGuard = std::optional<TheoryRightGuard>;

using LiteralArray = std::vector<Literal>;
using LiteralIterable = Iterable<Literal>;

auto construct_literal_array(clingo_ast_t **ast, size_t size) -> LiteralArray;

class SetAggregateElement : public ASTBase {
  public:
    SetAggregateElement() = default;
    SetAggregateElement(SetAggregateElement const &x) = default;
    SetAggregateElement(SetAggregateElement &&x) noexcept = default;
    auto operator=(SetAggregateElement const &x) -> SetAggregateElement & = default;
    auto operator=(SetAggregateElement &&x) noexcept -> SetAggregateElement & = default;
    ~SetAggregateElement() noexcept = default;

    auto location() -> Location;
    auto literal() -> Literal;
    auto condition() -> LiteralArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<SetAggregateElement>;
    auto update(Library &lib, py::kwargs const &kwargs) -> SetAggregateElement;

    static auto construct(Library &lib, Location const &location, Literal const &literal,
                          LiteralIterable const &condition) -> SetAggregateElement;
    static auto acquire(clingo_ast_t *ast) -> SetAggregateElement { return {ast}; }

    friend auto operator==(SetAggregateElement const &a, SetAggregateElement const &b) -> bool = default;
    friend auto operator<=>(SetAggregateElement const &a, SetAggregateElement const &b)
        -> std::strong_ordering = default;

  private:
    SetAggregateElement(clingo_ast_t *ast) : ASTBase{ast} {}
};

using SetAggregateElementArray = std::vector<SetAggregateElement>;
using SetAggregateElementIterable = Iterable<SetAggregateElement>;

auto construct_set_aggregate_element_array(clingo_ast_t **ast, size_t size) -> SetAggregateElementArray;

class BodyAggregateElement : public ASTBase {
  public:
    BodyAggregateElement() = default;
    BodyAggregateElement(BodyAggregateElement const &x) = default;
    BodyAggregateElement(BodyAggregateElement &&x) noexcept = default;
    auto operator=(BodyAggregateElement const &x) -> BodyAggregateElement & = default;
    auto operator=(BodyAggregateElement &&x) noexcept -> BodyAggregateElement & = default;
    ~BodyAggregateElement() noexcept = default;

    auto location() -> Location;
    auto tuple() -> TermArray;
    auto condition() -> LiteralArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<BodyAggregateElement>;
    auto update(Library &lib, py::kwargs const &kwargs) -> BodyAggregateElement;

    static auto construct(Library &lib, Location const &location, TermIterable const &tuple,
                          LiteralIterable const &condition) -> BodyAggregateElement;
    static auto acquire(clingo_ast_t *ast) -> BodyAggregateElement { return {ast}; }

    friend auto operator==(BodyAggregateElement const &a, BodyAggregateElement const &b) -> bool = default;
    friend auto operator<=>(BodyAggregateElement const &a, BodyAggregateElement const &b)
        -> std::strong_ordering = default;

  private:
    BodyAggregateElement(clingo_ast_t *ast) : ASTBase{ast} {}
};

using BodyAggregateElementArray = std::vector<BodyAggregateElement>;
using BodyAggregateElementIterable = Iterable<BodyAggregateElement>;

auto construct_body_aggregate_element_array(clingo_ast_t **ast, size_t size) -> BodyAggregateElementArray;

class TheoryAtomElement : public ASTBase {
  public:
    TheoryAtomElement() = default;
    TheoryAtomElement(TheoryAtomElement const &x) = default;
    TheoryAtomElement(TheoryAtomElement &&x) noexcept = default;
    auto operator=(TheoryAtomElement const &x) -> TheoryAtomElement & = default;
    auto operator=(TheoryAtomElement &&x) noexcept -> TheoryAtomElement & = default;
    ~TheoryAtomElement() noexcept = default;

    auto location() -> Location;
    auto tuple() -> TheoryTermArray;
    auto condition() -> LiteralArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TheoryAtomElement>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TheoryAtomElement;

    static auto construct(Library &lib, Location const &location, TheoryTermIterable const &tuple,
                          LiteralIterable const &condition) -> TheoryAtomElement;
    static auto acquire(clingo_ast_t *ast) -> TheoryAtomElement { return {ast}; }

    friend auto operator==(TheoryAtomElement const &a, TheoryAtomElement const &b) -> bool = default;
    friend auto operator<=>(TheoryAtomElement const &a, TheoryAtomElement const &b) -> std::strong_ordering = default;

  private:
    TheoryAtomElement(clingo_ast_t *ast) : ASTBase{ast} {}
};

using TheoryAtomElementArray = std::vector<TheoryAtomElement>;
using TheoryAtomElementIterable = Iterable<TheoryAtomElement>;

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
using BodyLiteralIterable = Iterable<BodyLiteral>;

auto construct_body_literal_array(clingo_ast_t **ast, size_t size) -> BodyLiteralArray;

class BodySimpleLiteral : public ASTBase {
  public:
    BodySimpleLiteral() = default;
    BodySimpleLiteral(BodySimpleLiteral const &x) = default;
    BodySimpleLiteral(BodySimpleLiteral &&x) noexcept = default;
    auto operator=(BodySimpleLiteral const &x) -> BodySimpleLiteral & = default;
    auto operator=(BodySimpleLiteral &&x) noexcept -> BodySimpleLiteral & = default;
    ~BodySimpleLiteral() noexcept = default;

    auto literal() -> Literal;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<BodySimpleLiteral>;
    auto update(Library &lib, py::kwargs const &kwargs) -> BodySimpleLiteral;

    static auto construct(Library &lib, Literal const &literal) -> BodySimpleLiteral;
    static auto acquire(clingo_ast_t *ast) -> BodySimpleLiteral { return {ast}; }

    friend auto operator==(BodySimpleLiteral const &a, BodySimpleLiteral const &b) -> bool = default;
    friend auto operator<=>(BodySimpleLiteral const &a, BodySimpleLiteral const &b) -> std::strong_ordering = default;

  private:
    BodySimpleLiteral(clingo_ast_t *ast) : ASTBase{ast} {}
};

class BodyAggregate : public ASTBase {
  public:
    BodyAggregate() = default;
    BodyAggregate(BodyAggregate const &x) = default;
    BodyAggregate(BodyAggregate &&x) noexcept = default;
    auto operator=(BodyAggregate const &x) -> BodyAggregate & = default;
    auto operator=(BodyAggregate &&x) noexcept -> BodyAggregate & = default;
    ~BodyAggregate() noexcept = default;

    auto location() -> Location;
    auto sign() -> Sign;
    auto left() -> OptionalLeftGuard;
    auto function() -> AggregateFunction;
    auto elements() -> BodyAggregateElementArray;
    auto right() -> OptionalRightGuard;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<BodyAggregate>;
    auto update(Library &lib, py::kwargs const &kwargs) -> BodyAggregate;

    static auto construct(Library &lib, Location const &location, Sign const &sign, OptionalLeftGuard const &left,
                          AggregateFunction const &function, BodyAggregateElementIterable const &elements,
                          OptionalRightGuard const &right) -> BodyAggregate;
    static auto acquire(clingo_ast_t *ast) -> BodyAggregate { return {ast}; }

    friend auto operator==(BodyAggregate const &a, BodyAggregate const &b) -> bool = default;
    friend auto operator<=>(BodyAggregate const &a, BodyAggregate const &b) -> std::strong_ordering = default;

  private:
    BodyAggregate(clingo_ast_t *ast) : ASTBase{ast} {}
};

class BodySetAggregate : public ASTBase {
  public:
    BodySetAggregate() = default;
    BodySetAggregate(BodySetAggregate const &x) = default;
    BodySetAggregate(BodySetAggregate &&x) noexcept = default;
    auto operator=(BodySetAggregate const &x) -> BodySetAggregate & = default;
    auto operator=(BodySetAggregate &&x) noexcept -> BodySetAggregate & = default;
    ~BodySetAggregate() noexcept = default;

    auto location() -> Location;
    auto sign() -> Sign;
    auto left() -> OptionalLeftGuard;
    auto elements() -> SetAggregateElementArray;
    auto right() -> OptionalRightGuard;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<BodySetAggregate>;
    auto update(Library &lib, py::kwargs const &kwargs) -> BodySetAggregate;

    static auto construct(Library &lib, Location const &location, Sign const &sign, OptionalLeftGuard const &left,
                          SetAggregateElementIterable const &elements, OptionalRightGuard const &right)
        -> BodySetAggregate;
    static auto acquire(clingo_ast_t *ast) -> BodySetAggregate { return {ast}; }

    friend auto operator==(BodySetAggregate const &a, BodySetAggregate const &b) -> bool = default;
    friend auto operator<=>(BodySetAggregate const &a, BodySetAggregate const &b) -> std::strong_ordering = default;

  private:
    BodySetAggregate(clingo_ast_t *ast) : ASTBase{ast} {}
};

class BodyTheoryAtom : public ASTBase {
  public:
    BodyTheoryAtom() = default;
    BodyTheoryAtom(BodyTheoryAtom const &x) = default;
    BodyTheoryAtom(BodyTheoryAtom &&x) noexcept = default;
    auto operator=(BodyTheoryAtom const &x) -> BodyTheoryAtom & = default;
    auto operator=(BodyTheoryAtom &&x) noexcept -> BodyTheoryAtom & = default;
    ~BodyTheoryAtom() noexcept = default;

    auto location() -> Location;
    auto sign() -> Sign;
    auto name() -> Term;
    auto elements() -> TheoryAtomElementArray;
    auto right() -> OptionalTheoryRightGuard;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<BodyTheoryAtom>;
    auto update(Library &lib, py::kwargs const &kwargs) -> BodyTheoryAtom;

    static auto construct(Library &lib, Location const &location, Sign const &sign, Term const &name,
                          TheoryAtomElementIterable const &elements, OptionalTheoryRightGuard const &right)
        -> BodyTheoryAtom;
    static auto acquire(clingo_ast_t *ast) -> BodyTheoryAtom { return {ast}; }

    friend auto operator==(BodyTheoryAtom const &a, BodyTheoryAtom const &b) -> bool = default;
    friend auto operator<=>(BodyTheoryAtom const &a, BodyTheoryAtom const &b) -> std::strong_ordering = default;

  private:
    BodyTheoryAtom(clingo_ast_t *ast) : ASTBase{ast} {}
};

class BodyConditionalLiteral : public ASTBase {
  public:
    BodyConditionalLiteral() = default;
    BodyConditionalLiteral(BodyConditionalLiteral const &x) = default;
    BodyConditionalLiteral(BodyConditionalLiteral &&x) noexcept = default;
    auto operator=(BodyConditionalLiteral const &x) -> BodyConditionalLiteral & = default;
    auto operator=(BodyConditionalLiteral &&x) noexcept -> BodyConditionalLiteral & = default;
    ~BodyConditionalLiteral() noexcept = default;

    auto location() -> Location;
    auto literal() -> Literal;
    auto condition() -> LiteralArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<BodyConditionalLiteral>;
    auto update(Library &lib, py::kwargs const &kwargs) -> BodyConditionalLiteral;

    static auto construct(Library &lib, Location const &location, Literal const &literal,
                          LiteralIterable const &condition) -> BodyConditionalLiteral;
    static auto acquire(clingo_ast_t *ast) -> BodyConditionalLiteral { return {ast}; }

    friend auto operator==(BodyConditionalLiteral const &a, BodyConditionalLiteral const &b) -> bool = default;
    friend auto operator<=>(BodyConditionalLiteral const &a, BodyConditionalLiteral const &b)
        -> std::strong_ordering = default;

  private:
    BodyConditionalLiteral(clingo_ast_t *ast) : ASTBase{ast} {}
};

class HeadConditionalLiteral : public ASTBase {
  public:
    HeadConditionalLiteral() = default;
    HeadConditionalLiteral(HeadConditionalLiteral const &x) = default;
    HeadConditionalLiteral(HeadConditionalLiteral &&x) noexcept = default;
    auto operator=(HeadConditionalLiteral const &x) -> HeadConditionalLiteral & = default;
    auto operator=(HeadConditionalLiteral &&x) noexcept -> HeadConditionalLiteral & = default;
    ~HeadConditionalLiteral() noexcept = default;

    auto location() -> Location;
    auto literal() -> Literal;
    auto condition() -> LiteralArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<HeadConditionalLiteral>;
    auto update(Library &lib, py::kwargs const &kwargs) -> HeadConditionalLiteral;

    static auto construct(Library &lib, Location const &location, Literal const &literal,
                          LiteralIterable const &condition) -> HeadConditionalLiteral;
    static auto acquire(clingo_ast_t *ast) -> HeadConditionalLiteral { return {ast}; }

    friend auto operator==(HeadConditionalLiteral const &a, HeadConditionalLiteral const &b) -> bool = default;
    friend auto operator<=>(HeadConditionalLiteral const &a, HeadConditionalLiteral const &b)
        -> std::strong_ordering = default;

  private:
    HeadConditionalLiteral(clingo_ast_t *ast) : ASTBase{ast} {}
};

using DisjunctionElement = std::variant<Literal, HeadConditionalLiteral>;

auto construct_disjunction_element(clingo_ast_t *ast) -> DisjunctionElement;

using DisjunctionElementArray = std::vector<DisjunctionElement>;
using DisjunctionElementIterable = Iterable<DisjunctionElement>;

auto construct_disjunction_element_array(clingo_ast_t **ast, size_t size) -> DisjunctionElementArray;

class HeadAggregateElement : public ASTBase {
  public:
    HeadAggregateElement() = default;
    HeadAggregateElement(HeadAggregateElement const &x) = default;
    HeadAggregateElement(HeadAggregateElement &&x) noexcept = default;
    auto operator=(HeadAggregateElement const &x) -> HeadAggregateElement & = default;
    auto operator=(HeadAggregateElement &&x) noexcept -> HeadAggregateElement & = default;
    ~HeadAggregateElement() noexcept = default;

    auto location() -> Location;
    auto tuple() -> TermArray;
    auto literal() -> Literal;
    auto condition() -> LiteralArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<HeadAggregateElement>;
    auto update(Library &lib, py::kwargs const &kwargs) -> HeadAggregateElement;

    static auto construct(Library &lib, Location const &location, TermIterable const &tuple, Literal const &literal,
                          LiteralIterable const &condition) -> HeadAggregateElement;
    static auto acquire(clingo_ast_t *ast) -> HeadAggregateElement { return {ast}; }

    friend auto operator==(HeadAggregateElement const &a, HeadAggregateElement const &b) -> bool = default;
    friend auto operator<=>(HeadAggregateElement const &a, HeadAggregateElement const &b)
        -> std::strong_ordering = default;

  private:
    HeadAggregateElement(clingo_ast_t *ast) : ASTBase{ast} {}
};

using HeadAggregateElementArray = std::vector<HeadAggregateElement>;
using HeadAggregateElementIterable = Iterable<HeadAggregateElement>;

auto construct_head_aggregate_element_array(clingo_ast_t **ast, size_t size) -> HeadAggregateElementArray;

class HeadSimpleLiteral;

class HeadAggregate;

class HeadSetAggregate;

class HeadTheoryAtom;

class HeadDisjunction;

using HeadLiteral = std::variant<HeadSimpleLiteral, HeadAggregate, HeadSetAggregate, HeadTheoryAtom, HeadDisjunction>;

auto construct_head_literal(clingo_ast_t *ast) -> HeadLiteral;

class HeadSimpleLiteral : public ASTBase {
  public:
    HeadSimpleLiteral() = default;
    HeadSimpleLiteral(HeadSimpleLiteral const &x) = default;
    HeadSimpleLiteral(HeadSimpleLiteral &&x) noexcept = default;
    auto operator=(HeadSimpleLiteral const &x) -> HeadSimpleLiteral & = default;
    auto operator=(HeadSimpleLiteral &&x) noexcept -> HeadSimpleLiteral & = default;
    ~HeadSimpleLiteral() noexcept = default;

    auto literal() -> Literal;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<HeadSimpleLiteral>;
    auto update(Library &lib, py::kwargs const &kwargs) -> HeadSimpleLiteral;

    static auto construct(Library &lib, Literal const &literal) -> HeadSimpleLiteral;
    static auto acquire(clingo_ast_t *ast) -> HeadSimpleLiteral { return {ast}; }

    friend auto operator==(HeadSimpleLiteral const &a, HeadSimpleLiteral const &b) -> bool = default;
    friend auto operator<=>(HeadSimpleLiteral const &a, HeadSimpleLiteral const &b) -> std::strong_ordering = default;

  private:
    HeadSimpleLiteral(clingo_ast_t *ast) : ASTBase{ast} {}
};

class HeadAggregate : public ASTBase {
  public:
    HeadAggregate() = default;
    HeadAggregate(HeadAggregate const &x) = default;
    HeadAggregate(HeadAggregate &&x) noexcept = default;
    auto operator=(HeadAggregate const &x) -> HeadAggregate & = default;
    auto operator=(HeadAggregate &&x) noexcept -> HeadAggregate & = default;
    ~HeadAggregate() noexcept = default;

    auto location() -> Location;
    auto left() -> OptionalLeftGuard;
    auto function() -> AggregateFunction;
    auto elements() -> HeadAggregateElementArray;
    auto right() -> OptionalRightGuard;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<HeadAggregate>;
    auto update(Library &lib, py::kwargs const &kwargs) -> HeadAggregate;

    static auto construct(Library &lib, Location const &location, OptionalLeftGuard const &left,
                          AggregateFunction const &function, HeadAggregateElementIterable const &elements,
                          OptionalRightGuard const &right) -> HeadAggregate;
    static auto acquire(clingo_ast_t *ast) -> HeadAggregate { return {ast}; }

    friend auto operator==(HeadAggregate const &a, HeadAggregate const &b) -> bool = default;
    friend auto operator<=>(HeadAggregate const &a, HeadAggregate const &b) -> std::strong_ordering = default;

  private:
    HeadAggregate(clingo_ast_t *ast) : ASTBase{ast} {}
};

class HeadSetAggregate : public ASTBase {
  public:
    HeadSetAggregate() = default;
    HeadSetAggregate(HeadSetAggregate const &x) = default;
    HeadSetAggregate(HeadSetAggregate &&x) noexcept = default;
    auto operator=(HeadSetAggregate const &x) -> HeadSetAggregate & = default;
    auto operator=(HeadSetAggregate &&x) noexcept -> HeadSetAggregate & = default;
    ~HeadSetAggregate() noexcept = default;

    auto location() -> Location;
    auto left() -> OptionalLeftGuard;
    auto elements() -> SetAggregateElementArray;
    auto right() -> OptionalRightGuard;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<HeadSetAggregate>;
    auto update(Library &lib, py::kwargs const &kwargs) -> HeadSetAggregate;

    static auto construct(Library &lib, Location const &location, OptionalLeftGuard const &left,
                          SetAggregateElementIterable const &elements, OptionalRightGuard const &right)
        -> HeadSetAggregate;
    static auto acquire(clingo_ast_t *ast) -> HeadSetAggregate { return {ast}; }

    friend auto operator==(HeadSetAggregate const &a, HeadSetAggregate const &b) -> bool = default;
    friend auto operator<=>(HeadSetAggregate const &a, HeadSetAggregate const &b) -> std::strong_ordering = default;

  private:
    HeadSetAggregate(clingo_ast_t *ast) : ASTBase{ast} {}
};

class HeadTheoryAtom : public ASTBase {
  public:
    HeadTheoryAtom() = default;
    HeadTheoryAtom(HeadTheoryAtom const &x) = default;
    HeadTheoryAtom(HeadTheoryAtom &&x) noexcept = default;
    auto operator=(HeadTheoryAtom const &x) -> HeadTheoryAtom & = default;
    auto operator=(HeadTheoryAtom &&x) noexcept -> HeadTheoryAtom & = default;
    ~HeadTheoryAtom() noexcept = default;

    auto location() -> Location;
    auto name() -> Term;
    auto elements() -> TheoryAtomElementArray;
    auto right() -> OptionalTheoryRightGuard;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<HeadTheoryAtom>;
    auto update(Library &lib, py::kwargs const &kwargs) -> HeadTheoryAtom;

    static auto construct(Library &lib, Location const &location, Term const &name,
                          TheoryAtomElementIterable const &elements, OptionalTheoryRightGuard const &right)
        -> HeadTheoryAtom;
    static auto acquire(clingo_ast_t *ast) -> HeadTheoryAtom { return {ast}; }

    friend auto operator==(HeadTheoryAtom const &a, HeadTheoryAtom const &b) -> bool = default;
    friend auto operator<=>(HeadTheoryAtom const &a, HeadTheoryAtom const &b) -> std::strong_ordering = default;

  private:
    HeadTheoryAtom(clingo_ast_t *ast) : ASTBase{ast} {}
};

class HeadDisjunction : public ASTBase {
  public:
    HeadDisjunction() = default;
    HeadDisjunction(HeadDisjunction const &x) = default;
    HeadDisjunction(HeadDisjunction &&x) noexcept = default;
    auto operator=(HeadDisjunction const &x) -> HeadDisjunction & = default;
    auto operator=(HeadDisjunction &&x) noexcept -> HeadDisjunction & = default;
    ~HeadDisjunction() noexcept = default;

    auto location() -> Location;
    auto elements() -> DisjunctionElementArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<HeadDisjunction>;
    auto update(Library &lib, py::kwargs const &kwargs) -> HeadDisjunction;

    static auto construct(Library &lib, Location const &location, DisjunctionElementIterable const &elements)
        -> HeadDisjunction;
    static auto acquire(clingo_ast_t *ast) -> HeadDisjunction { return {ast}; }

    friend auto operator==(HeadDisjunction const &a, HeadDisjunction const &b) -> bool = default;
    friend auto operator<=>(HeadDisjunction const &a, HeadDisjunction const &b) -> std::strong_ordering = default;

  private:
    HeadDisjunction(clingo_ast_t *ast) : ASTBase{ast} {}
};

class TheoryOperatorDefinition : public ASTBase {
  public:
    TheoryOperatorDefinition() = default;
    TheoryOperatorDefinition(TheoryOperatorDefinition const &x) = default;
    TheoryOperatorDefinition(TheoryOperatorDefinition &&x) noexcept = default;
    auto operator=(TheoryOperatorDefinition const &x) -> TheoryOperatorDefinition & = default;
    auto operator=(TheoryOperatorDefinition &&x) noexcept -> TheoryOperatorDefinition & = default;
    ~TheoryOperatorDefinition() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto priority() -> int;
    auto operator_type() -> TheoryOperatorType;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TheoryOperatorDefinition>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TheoryOperatorDefinition;

    static auto construct(Library &lib, Location const &location, std::string_view name, int priority,
                          TheoryOperatorType const &operator_type) -> TheoryOperatorDefinition;
    static auto acquire(clingo_ast_t *ast) -> TheoryOperatorDefinition { return {ast}; }

    friend auto operator==(TheoryOperatorDefinition const &a, TheoryOperatorDefinition const &b) -> bool = default;
    friend auto operator<=>(TheoryOperatorDefinition const &a, TheoryOperatorDefinition const &b)
        -> std::strong_ordering = default;

  private:
    TheoryOperatorDefinition(clingo_ast_t *ast) : ASTBase{ast} {}
};

using TheoryOperatorDefinitionArray = std::vector<TheoryOperatorDefinition>;
using TheoryOperatorDefinitionIterable = Iterable<TheoryOperatorDefinition>;

auto construct_theory_operator_definition_array(clingo_ast_t **ast, size_t size) -> TheoryOperatorDefinitionArray;

class TheoryTermDefinition : public ASTBase {
  public:
    TheoryTermDefinition() = default;
    TheoryTermDefinition(TheoryTermDefinition const &x) = default;
    TheoryTermDefinition(TheoryTermDefinition &&x) noexcept = default;
    auto operator=(TheoryTermDefinition const &x) -> TheoryTermDefinition & = default;
    auto operator=(TheoryTermDefinition &&x) noexcept -> TheoryTermDefinition & = default;
    ~TheoryTermDefinition() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto operators() -> TheoryOperatorDefinitionArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TheoryTermDefinition>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TheoryTermDefinition;

    static auto construct(Library &lib, Location const &location, std::string_view name,
                          TheoryOperatorDefinitionIterable const &operators) -> TheoryTermDefinition;
    static auto acquire(clingo_ast_t *ast) -> TheoryTermDefinition { return {ast}; }

    friend auto operator==(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool = default;
    friend auto operator<=>(TheoryTermDefinition const &a, TheoryTermDefinition const &b)
        -> std::strong_ordering = default;

  private:
    TheoryTermDefinition(clingo_ast_t *ast) : ASTBase{ast} {}
};

using TheoryTermDefinitionArray = std::vector<TheoryTermDefinition>;
using TheoryTermDefinitionIterable = Iterable<TheoryTermDefinition>;

auto construct_theory_term_definition_array(clingo_ast_t **ast, size_t size) -> TheoryTermDefinitionArray;

class TheoryGuardDefinition : public ASTBase {
  public:
    TheoryGuardDefinition() = default;
    TheoryGuardDefinition(TheoryGuardDefinition const &x) = default;
    TheoryGuardDefinition(TheoryGuardDefinition &&x) noexcept = default;
    auto operator=(TheoryGuardDefinition const &x) -> TheoryGuardDefinition & = default;
    auto operator=(TheoryGuardDefinition &&x) noexcept -> TheoryGuardDefinition & = default;
    ~TheoryGuardDefinition() noexcept = default;

    auto operators() -> std::vector<std::string_view>;
    auto term() -> std::string_view;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TheoryGuardDefinition>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TheoryGuardDefinition;

    static auto construct(Library &lib, StringIterable const &operators, std::string_view term)
        -> TheoryGuardDefinition;
    static auto acquire(clingo_ast_t *ast) -> TheoryGuardDefinition { return {ast}; }

    friend auto operator==(TheoryGuardDefinition const &a, TheoryGuardDefinition const &b) -> bool = default;
    friend auto operator<=>(TheoryGuardDefinition const &a, TheoryGuardDefinition const &b)
        -> std::strong_ordering = default;

  private:
    TheoryGuardDefinition(clingo_ast_t *ast) : ASTBase{ast} {}
};

using OptionalTheoryGuardDefinition = std::optional<TheoryGuardDefinition>;

class TheoryAtomDefinition : public ASTBase {
  public:
    TheoryAtomDefinition() = default;
    TheoryAtomDefinition(TheoryAtomDefinition const &x) = default;
    TheoryAtomDefinition(TheoryAtomDefinition &&x) noexcept = default;
    auto operator=(TheoryAtomDefinition const &x) -> TheoryAtomDefinition & = default;
    auto operator=(TheoryAtomDefinition &&x) noexcept -> TheoryAtomDefinition & = default;
    ~TheoryAtomDefinition() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto arity() -> int;
    auto term() -> std::string_view;
    auto guard() -> OptionalTheoryGuardDefinition;
    auto atom_type() -> TheoryAtomType;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<TheoryAtomDefinition>;
    auto update(Library &lib, py::kwargs const &kwargs) -> TheoryAtomDefinition;

    static auto construct(Library &lib, Location const &location, std::string_view name, int arity,
                          std::string_view term, OptionalTheoryGuardDefinition const &guard,
                          TheoryAtomType const &atom_type) -> TheoryAtomDefinition;
    static auto acquire(clingo_ast_t *ast) -> TheoryAtomDefinition { return {ast}; }

    friend auto operator==(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool = default;
    friend auto operator<=>(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b)
        -> std::strong_ordering = default;

  private:
    TheoryAtomDefinition(clingo_ast_t *ast) : ASTBase{ast} {}
};

using TheoryAtomDefinitionArray = std::vector<TheoryAtomDefinition>;
using TheoryAtomDefinitionIterable = Iterable<TheoryAtomDefinition>;

auto construct_theory_atom_definition_array(clingo_ast_t **ast, size_t size) -> TheoryAtomDefinitionArray;

class OptimizeTuple : public ASTBase {
  public:
    OptimizeTuple() = default;
    OptimizeTuple(OptimizeTuple const &x) = default;
    OptimizeTuple(OptimizeTuple &&x) noexcept = default;
    auto operator=(OptimizeTuple const &x) -> OptimizeTuple & = default;
    auto operator=(OptimizeTuple &&x) noexcept -> OptimizeTuple & = default;
    ~OptimizeTuple() noexcept = default;

    auto weight() -> Term;
    auto priority() -> OptionalTerm;
    auto terms() -> TermArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<OptimizeTuple>;
    auto update(Library &lib, py::kwargs const &kwargs) -> OptimizeTuple;

    static auto construct(Library &lib, Term const &weight, OptionalTerm const &priority, TermIterable const &terms)
        -> OptimizeTuple;
    static auto acquire(clingo_ast_t *ast) -> OptimizeTuple { return {ast}; }

    friend auto operator==(OptimizeTuple const &a, OptimizeTuple const &b) -> bool = default;
    friend auto operator<=>(OptimizeTuple const &a, OptimizeTuple const &b) -> std::strong_ordering = default;

  private:
    OptimizeTuple(clingo_ast_t *ast) : ASTBase{ast} {}
};

class OptimizeElement : public ASTBase {
  public:
    OptimizeElement() = default;
    OptimizeElement(OptimizeElement const &x) = default;
    OptimizeElement(OptimizeElement &&x) noexcept = default;
    auto operator=(OptimizeElement const &x) -> OptimizeElement & = default;
    auto operator=(OptimizeElement &&x) noexcept -> OptimizeElement & = default;
    ~OptimizeElement() noexcept = default;

    auto tuple() -> OptimizeTuple;
    auto condition() -> LiteralArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<OptimizeElement>;
    auto update(Library &lib, py::kwargs const &kwargs) -> OptimizeElement;

    static auto construct(Library &lib, OptimizeTuple const &tuple, LiteralIterable const &condition)
        -> OptimizeElement;
    static auto acquire(clingo_ast_t *ast) -> OptimizeElement { return {ast}; }

    friend auto operator==(OptimizeElement const &a, OptimizeElement const &b) -> bool = default;
    friend auto operator<=>(OptimizeElement const &a, OptimizeElement const &b) -> std::strong_ordering = default;

  private:
    OptimizeElement(clingo_ast_t *ast) : ASTBase{ast} {}
};

using OptimizeElementArray = std::vector<OptimizeElement>;
using OptimizeElementIterable = Iterable<OptimizeElement>;

auto construct_optimize_element_array(clingo_ast_t **ast, size_t size) -> OptimizeElementArray;

class Edge : public ASTBase {
  public:
    Edge() = default;
    Edge(Edge const &x) = default;
    Edge(Edge &&x) noexcept = default;
    auto operator=(Edge const &x) -> Edge & = default;
    auto operator=(Edge &&x) noexcept -> Edge & = default;
    ~Edge() noexcept = default;

    auto u() -> Term;
    auto v() -> Term;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<Edge>;
    auto update(Library &lib, py::kwargs const &kwargs) -> Edge;

    static auto construct(Library &lib, Term const &u, Term const &v) -> Edge;
    static auto acquire(clingo_ast_t *ast) -> Edge { return {ast}; }

    friend auto operator==(Edge const &a, Edge const &b) -> bool = default;
    friend auto operator<=>(Edge const &a, Edge const &b) -> std::strong_ordering = default;

  private:
    Edge(clingo_ast_t *ast) : ASTBase{ast} {}
};

using EdgeArray = std::vector<Edge>;
using EdgeIterable = Iterable<Edge>;

auto construct_edge_array(clingo_ast_t **ast, size_t size) -> EdgeArray;

class ProgramPart : public ASTBase {
  public:
    ProgramPart() = default;
    ProgramPart(ProgramPart const &x) = default;
    ProgramPart(ProgramPart &&x) noexcept = default;
    auto operator=(ProgramPart const &x) -> ProgramPart & = default;
    auto operator=(ProgramPart &&x) noexcept -> ProgramPart & = default;
    ~ProgramPart() noexcept = default;

    auto name() -> std::string_view;
    auto arguments() -> SymbolArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<ProgramPart>;
    auto update(Library &lib, py::kwargs const &kwargs) -> ProgramPart;

    static auto construct(Library &lib, std::string_view name, SymbolIterable const &arguments) -> ProgramPart;
    static auto acquire(clingo_ast_t *ast) -> ProgramPart { return {ast}; }

    friend auto operator==(ProgramPart const &a, ProgramPart const &b) -> bool = default;
    friend auto operator<=>(ProgramPart const &a, ProgramPart const &b) -> std::strong_ordering = default;

  private:
    ProgramPart(clingo_ast_t *ast) : ASTBase{ast} {}
};

using ProgramPartArray = std::vector<ProgramPart>;
using ProgramPartIterable = Iterable<ProgramPart>;

auto construct_program_part_array(clingo_ast_t **ast, size_t size) -> ProgramPartArray;

class StatementRule;

class StatementTheory;

class StatementOptimize;

class StatementWeakConstraint;

class StatementShow;

class StatementShowNothing;

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

class StatementParts;

class StatementConst;

class StatementComment;

using Statement =
    std::variant<StatementRule, StatementTheory, StatementOptimize, StatementWeakConstraint, StatementShow,
                 StatementShowNothing, StatementShowSignature, StatementProject, StatementProjectSignature,
                 StatementDefined, StatementExternal, StatementEdge, StatementHeuristic, StatementScript,
                 StatementInclude, StatementProgram, StatementParts, StatementConst, StatementComment>;

auto construct_statement(clingo_ast_t *ast) -> Statement;

class StatementRule : public ASTBase {
  public:
    StatementRule() = default;
    StatementRule(StatementRule const &x) = default;
    StatementRule(StatementRule &&x) noexcept = default;
    auto operator=(StatementRule const &x) -> StatementRule & = default;
    auto operator=(StatementRule &&x) noexcept -> StatementRule & = default;
    ~StatementRule() noexcept = default;

    auto location() -> Location;
    auto head() -> HeadLiteral;
    auto body() -> BodyLiteralArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementRule>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementRule;

    static auto construct(Library &lib, Location const &location, HeadLiteral const &head,
                          BodyLiteralIterable const &body) -> StatementRule;
    static auto acquire(clingo_ast_t *ast) -> StatementRule { return {ast}; }

    friend auto operator==(StatementRule const &a, StatementRule const &b) -> bool = default;
    friend auto operator<=>(StatementRule const &a, StatementRule const &b) -> std::strong_ordering = default;

  private:
    StatementRule(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementTheory : public ASTBase {
  public:
    StatementTheory() = default;
    StatementTheory(StatementTheory const &x) = default;
    StatementTheory(StatementTheory &&x) noexcept = default;
    auto operator=(StatementTheory const &x) -> StatementTheory & = default;
    auto operator=(StatementTheory &&x) noexcept -> StatementTheory & = default;
    ~StatementTheory() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto terms() -> TheoryTermDefinitionArray;
    auto atoms() -> TheoryAtomDefinitionArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementTheory>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementTheory;

    static auto construct(Library &lib, Location const &location, std::string_view name,
                          TheoryTermDefinitionIterable const &terms, TheoryAtomDefinitionIterable const &atoms)
        -> StatementTheory;
    static auto acquire(clingo_ast_t *ast) -> StatementTheory { return {ast}; }

    friend auto operator==(StatementTheory const &a, StatementTheory const &b) -> bool = default;
    friend auto operator<=>(StatementTheory const &a, StatementTheory const &b) -> std::strong_ordering = default;

  private:
    StatementTheory(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementOptimize : public ASTBase {
  public:
    StatementOptimize() = default;
    StatementOptimize(StatementOptimize const &x) = default;
    StatementOptimize(StatementOptimize &&x) noexcept = default;
    auto operator=(StatementOptimize const &x) -> StatementOptimize & = default;
    auto operator=(StatementOptimize &&x) noexcept -> StatementOptimize & = default;
    ~StatementOptimize() noexcept = default;

    auto location() -> Location;
    auto elements() -> OptimizeElementArray;
    auto optimize_type() -> OptimizeType;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementOptimize>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementOptimize;

    static auto construct(Library &lib, Location const &location, OptimizeElementIterable const &elements,
                          OptimizeType const &optimize_type) -> StatementOptimize;
    static auto acquire(clingo_ast_t *ast) -> StatementOptimize { return {ast}; }

    friend auto operator==(StatementOptimize const &a, StatementOptimize const &b) -> bool = default;
    friend auto operator<=>(StatementOptimize const &a, StatementOptimize const &b) -> std::strong_ordering = default;

  private:
    StatementOptimize(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementWeakConstraint : public ASTBase {
  public:
    StatementWeakConstraint() = default;
    StatementWeakConstraint(StatementWeakConstraint const &x) = default;
    StatementWeakConstraint(StatementWeakConstraint &&x) noexcept = default;
    auto operator=(StatementWeakConstraint const &x) -> StatementWeakConstraint & = default;
    auto operator=(StatementWeakConstraint &&x) noexcept -> StatementWeakConstraint & = default;
    ~StatementWeakConstraint() noexcept = default;

    auto location() -> Location;
    auto body() -> BodyLiteralArray;
    auto tuple() -> OptimizeTuple;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementWeakConstraint>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementWeakConstraint;

    static auto construct(Library &lib, Location const &location, BodyLiteralIterable const &body,
                          OptimizeTuple const &tuple) -> StatementWeakConstraint;
    static auto acquire(clingo_ast_t *ast) -> StatementWeakConstraint { return {ast}; }

    friend auto operator==(StatementWeakConstraint const &a, StatementWeakConstraint const &b) -> bool = default;
    friend auto operator<=>(StatementWeakConstraint const &a, StatementWeakConstraint const &b)
        -> std::strong_ordering = default;

  private:
    StatementWeakConstraint(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementShow : public ASTBase {
  public:
    StatementShow() = default;
    StatementShow(StatementShow const &x) = default;
    StatementShow(StatementShow &&x) noexcept = default;
    auto operator=(StatementShow const &x) -> StatementShow & = default;
    auto operator=(StatementShow &&x) noexcept -> StatementShow & = default;
    ~StatementShow() noexcept = default;

    auto location() -> Location;
    auto term() -> Term;
    auto body() -> BodyLiteralArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementShow>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementShow;

    static auto construct(Library &lib, Location const &location, Term const &term, BodyLiteralIterable const &body)
        -> StatementShow;
    static auto acquire(clingo_ast_t *ast) -> StatementShow { return {ast}; }

    friend auto operator==(StatementShow const &a, StatementShow const &b) -> bool = default;
    friend auto operator<=>(StatementShow const &a, StatementShow const &b) -> std::strong_ordering = default;

  private:
    StatementShow(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementShowNothing : public ASTBase {
  public:
    StatementShowNothing() = default;
    StatementShowNothing(StatementShowNothing const &x) = default;
    StatementShowNothing(StatementShowNothing &&x) noexcept = default;
    auto operator=(StatementShowNothing const &x) -> StatementShowNothing & = default;
    auto operator=(StatementShowNothing &&x) noexcept -> StatementShowNothing & = default;
    ~StatementShowNothing() noexcept = default;

    auto location() -> Location;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementShowNothing>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementShowNothing;

    static auto construct(Library &lib, Location const &location) -> StatementShowNothing;
    static auto acquire(clingo_ast_t *ast) -> StatementShowNothing { return {ast}; }

    friend auto operator==(StatementShowNothing const &a, StatementShowNothing const &b) -> bool = default;
    friend auto operator<=>(StatementShowNothing const &a, StatementShowNothing const &b)
        -> std::strong_ordering = default;

  private:
    StatementShowNothing(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementShowSignature : public ASTBase {
  public:
    StatementShowSignature() = default;
    StatementShowSignature(StatementShowSignature const &x) = default;
    StatementShowSignature(StatementShowSignature &&x) noexcept = default;
    auto operator=(StatementShowSignature const &x) -> StatementShowSignature & = default;
    auto operator=(StatementShowSignature &&x) noexcept -> StatementShowSignature & = default;
    ~StatementShowSignature() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto arity() -> int;
    auto sign() -> bool;
    auto value() -> bool;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementShowSignature>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementShowSignature;

    static auto construct(Library &lib, Location const &location, std::string_view name, int arity, bool sign,
                          bool value) -> StatementShowSignature;
    static auto acquire(clingo_ast_t *ast) -> StatementShowSignature { return {ast}; }

    friend auto operator==(StatementShowSignature const &a, StatementShowSignature const &b) -> bool = default;
    friend auto operator<=>(StatementShowSignature const &a, StatementShowSignature const &b)
        -> std::strong_ordering = default;

  private:
    StatementShowSignature(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementProject : public ASTBase {
  public:
    StatementProject() = default;
    StatementProject(StatementProject const &x) = default;
    StatementProject(StatementProject &&x) noexcept = default;
    auto operator=(StatementProject const &x) -> StatementProject & = default;
    auto operator=(StatementProject &&x) noexcept -> StatementProject & = default;
    ~StatementProject() noexcept = default;

    auto location() -> Location;
    auto atom() -> Term;
    auto body() -> BodyLiteralArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementProject>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementProject;

    static auto construct(Library &lib, Location const &location, Term const &atom, BodyLiteralIterable const &body)
        -> StatementProject;
    static auto acquire(clingo_ast_t *ast) -> StatementProject { return {ast}; }

    friend auto operator==(StatementProject const &a, StatementProject const &b) -> bool = default;
    friend auto operator<=>(StatementProject const &a, StatementProject const &b) -> std::strong_ordering = default;

  private:
    StatementProject(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementProjectSignature : public ASTBase {
  public:
    StatementProjectSignature() = default;
    StatementProjectSignature(StatementProjectSignature const &x) = default;
    StatementProjectSignature(StatementProjectSignature &&x) noexcept = default;
    auto operator=(StatementProjectSignature const &x) -> StatementProjectSignature & = default;
    auto operator=(StatementProjectSignature &&x) noexcept -> StatementProjectSignature & = default;
    ~StatementProjectSignature() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto arity() -> int;
    auto sign() -> bool;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementProjectSignature>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementProjectSignature;

    static auto construct(Library &lib, Location const &location, std::string_view name, int arity, bool sign)
        -> StatementProjectSignature;
    static auto acquire(clingo_ast_t *ast) -> StatementProjectSignature { return {ast}; }

    friend auto operator==(StatementProjectSignature const &a, StatementProjectSignature const &b) -> bool = default;
    friend auto operator<=>(StatementProjectSignature const &a, StatementProjectSignature const &b)
        -> std::strong_ordering = default;

  private:
    StatementProjectSignature(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementDefined : public ASTBase {
  public:
    StatementDefined() = default;
    StatementDefined(StatementDefined const &x) = default;
    StatementDefined(StatementDefined &&x) noexcept = default;
    auto operator=(StatementDefined const &x) -> StatementDefined & = default;
    auto operator=(StatementDefined &&x) noexcept -> StatementDefined & = default;
    ~StatementDefined() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto arity() -> int;
    auto sign() -> bool;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementDefined>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementDefined;

    static auto construct(Library &lib, Location const &location, std::string_view name, int arity, bool sign)
        -> StatementDefined;
    static auto acquire(clingo_ast_t *ast) -> StatementDefined { return {ast}; }

    friend auto operator==(StatementDefined const &a, StatementDefined const &b) -> bool = default;
    friend auto operator<=>(StatementDefined const &a, StatementDefined const &b) -> std::strong_ordering = default;

  private:
    StatementDefined(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementExternal : public ASTBase {
  public:
    StatementExternal() = default;
    StatementExternal(StatementExternal const &x) = default;
    StatementExternal(StatementExternal &&x) noexcept = default;
    auto operator=(StatementExternal const &x) -> StatementExternal & = default;
    auto operator=(StatementExternal &&x) noexcept -> StatementExternal & = default;
    ~StatementExternal() noexcept = default;

    auto location() -> Location;
    auto atom() -> Term;
    auto body() -> BodyLiteralArray;
    auto external_type() -> OptionalTerm;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementExternal>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementExternal;

    static auto construct(Library &lib, Location const &location, Term const &atom, BodyLiteralIterable const &body,
                          OptionalTerm const &external_type) -> StatementExternal;
    static auto acquire(clingo_ast_t *ast) -> StatementExternal { return {ast}; }

    friend auto operator==(StatementExternal const &a, StatementExternal const &b) -> bool = default;
    friend auto operator<=>(StatementExternal const &a, StatementExternal const &b) -> std::strong_ordering = default;

  private:
    StatementExternal(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementEdge : public ASTBase {
  public:
    StatementEdge() = default;
    StatementEdge(StatementEdge const &x) = default;
    StatementEdge(StatementEdge &&x) noexcept = default;
    auto operator=(StatementEdge const &x) -> StatementEdge & = default;
    auto operator=(StatementEdge &&x) noexcept -> StatementEdge & = default;
    ~StatementEdge() noexcept = default;

    auto location() -> Location;
    auto pool() -> EdgeArray;
    auto body() -> BodyLiteralArray;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementEdge>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementEdge;

    static auto construct(Library &lib, Location const &location, EdgeIterable const &pool,
                          BodyLiteralIterable const &body) -> StatementEdge;
    static auto acquire(clingo_ast_t *ast) -> StatementEdge { return {ast}; }

    friend auto operator==(StatementEdge const &a, StatementEdge const &b) -> bool = default;
    friend auto operator<=>(StatementEdge const &a, StatementEdge const &b) -> std::strong_ordering = default;

  private:
    StatementEdge(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementHeuristic : public ASTBase {
  public:
    StatementHeuristic() = default;
    StatementHeuristic(StatementHeuristic const &x) = default;
    StatementHeuristic(StatementHeuristic &&x) noexcept = default;
    auto operator=(StatementHeuristic const &x) -> StatementHeuristic & = default;
    auto operator=(StatementHeuristic &&x) noexcept -> StatementHeuristic & = default;
    ~StatementHeuristic() noexcept = default;

    auto location() -> Location;
    auto atom() -> Term;
    auto body() -> BodyLiteralArray;
    auto weight() -> Term;
    auto modifier() -> Term;
    auto priority() -> OptionalTerm;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementHeuristic>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementHeuristic;

    static auto construct(Library &lib, Location const &location, Term const &atom, BodyLiteralIterable const &body,
                          Term const &weight, Term const &modifier, OptionalTerm const &priority) -> StatementHeuristic;
    static auto acquire(clingo_ast_t *ast) -> StatementHeuristic { return {ast}; }

    friend auto operator==(StatementHeuristic const &a, StatementHeuristic const &b) -> bool = default;
    friend auto operator<=>(StatementHeuristic const &a, StatementHeuristic const &b) -> std::strong_ordering = default;

  private:
    StatementHeuristic(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementScript : public ASTBase {
  public:
    StatementScript() = default;
    StatementScript(StatementScript const &x) = default;
    StatementScript(StatementScript &&x) noexcept = default;
    auto operator=(StatementScript const &x) -> StatementScript & = default;
    auto operator=(StatementScript &&x) noexcept -> StatementScript & = default;
    ~StatementScript() noexcept = default;

    auto location() -> Location;
    auto value() -> std::string_view;
    auto script_type() -> std::string_view;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementScript>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementScript;

    static auto construct(Library &lib, Location const &location, std::string_view value, std::string_view script_type)
        -> StatementScript;
    static auto acquire(clingo_ast_t *ast) -> StatementScript { return {ast}; }

    friend auto operator==(StatementScript const &a, StatementScript const &b) -> bool = default;
    friend auto operator<=>(StatementScript const &a, StatementScript const &b) -> std::strong_ordering = default;

  private:
    StatementScript(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementInclude : public ASTBase {
  public:
    StatementInclude() = default;
    StatementInclude(StatementInclude const &x) = default;
    StatementInclude(StatementInclude &&x) noexcept = default;
    auto operator=(StatementInclude const &x) -> StatementInclude & = default;
    auto operator=(StatementInclude &&x) noexcept -> StatementInclude & = default;
    ~StatementInclude() noexcept = default;

    auto location() -> Location;
    auto value() -> std::string_view;
    auto include_type() -> IncludeType;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementInclude>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementInclude;

    static auto construct(Library &lib, Location const &location, std::string_view value,
                          IncludeType const &include_type) -> StatementInclude;
    static auto acquire(clingo_ast_t *ast) -> StatementInclude { return {ast}; }

    friend auto operator==(StatementInclude const &a, StatementInclude const &b) -> bool = default;
    friend auto operator<=>(StatementInclude const &a, StatementInclude const &b) -> std::strong_ordering = default;

  private:
    StatementInclude(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementProgram : public ASTBase {
  public:
    StatementProgram() = default;
    StatementProgram(StatementProgram const &x) = default;
    StatementProgram(StatementProgram &&x) noexcept = default;
    auto operator=(StatementProgram const &x) -> StatementProgram & = default;
    auto operator=(StatementProgram &&x) noexcept -> StatementProgram & = default;
    ~StatementProgram() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto arguments() -> std::vector<std::string_view>;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementProgram>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementProgram;

    static auto construct(Library &lib, Location const &location, std::string_view name,
                          StringIterable const &arguments) -> StatementProgram;
    static auto acquire(clingo_ast_t *ast) -> StatementProgram { return {ast}; }

    friend auto operator==(StatementProgram const &a, StatementProgram const &b) -> bool = default;
    friend auto operator<=>(StatementProgram const &a, StatementProgram const &b) -> std::strong_ordering = default;

  private:
    StatementProgram(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementParts : public ASTBase {
  public:
    StatementParts() = default;
    StatementParts(StatementParts const &x) = default;
    StatementParts(StatementParts &&x) noexcept = default;
    auto operator=(StatementParts const &x) -> StatementParts & = default;
    auto operator=(StatementParts &&x) noexcept -> StatementParts & = default;
    ~StatementParts() noexcept = default;

    auto location() -> Location;
    auto elements() -> ProgramPartArray;
    auto precedence() -> Precedence;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementParts>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementParts;

    static auto construct(Library &lib, Location const &location, ProgramPartIterable const &elements,
                          Precedence const &precedence) -> StatementParts;
    static auto acquire(clingo_ast_t *ast) -> StatementParts { return {ast}; }

    friend auto operator==(StatementParts const &a, StatementParts const &b) -> bool = default;
    friend auto operator<=>(StatementParts const &a, StatementParts const &b) -> std::strong_ordering = default;

  private:
    StatementParts(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementConst : public ASTBase {
  public:
    StatementConst() = default;
    StatementConst(StatementConst const &x) = default;
    StatementConst(StatementConst &&x) noexcept = default;
    auto operator=(StatementConst const &x) -> StatementConst & = default;
    auto operator=(StatementConst &&x) noexcept -> StatementConst & = default;
    ~StatementConst() noexcept = default;

    auto location() -> Location;
    auto name() -> std::string_view;
    auto value() -> Term;
    auto precedence() -> Precedence;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementConst>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementConst;

    static auto construct(Library &lib, Location const &location, std::string_view name, Term const &value,
                          Precedence const &precedence) -> StatementConst;
    static auto acquire(clingo_ast_t *ast) -> StatementConst { return {ast}; }

    friend auto operator==(StatementConst const &a, StatementConst const &b) -> bool = default;
    friend auto operator<=>(StatementConst const &a, StatementConst const &b) -> std::strong_ordering = default;

  private:
    StatementConst(clingo_ast_t *ast) : ASTBase{ast} {}
};

class StatementComment : public ASTBase {
  public:
    StatementComment() = default;
    StatementComment(StatementComment const &x) = default;
    StatementComment(StatementComment &&x) noexcept = default;
    auto operator=(StatementComment const &x) -> StatementComment & = default;
    auto operator=(StatementComment &&x) noexcept -> StatementComment & = default;
    ~StatementComment() noexcept = default;

    auto location() -> Location;
    auto value() -> std::string_view;
    auto comment_type() -> CommentType;

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);
    auto transform(Library &lib, py::handle transform, py::args const &args, py::kwargs const &kwargs)
        -> std::optional<StatementComment>;
    auto update(Library &lib, py::kwargs const &kwargs) -> StatementComment;

    static auto construct(Library &lib, Location const &location, std::string_view value,
                          CommentType const &comment_type) -> StatementComment;
    static auto acquire(clingo_ast_t *ast) -> StatementComment { return {ast}; }

    friend auto operator==(StatementComment const &a, StatementComment const &b) -> bool = default;
    friend auto operator<=>(StatementComment const &a, StatementComment const &b) -> std::strong_ordering = default;

  private:
    StatementComment(clingo_ast_t *ast) : ASTBase{ast} {}
};

auto construct_term(clingo_ast_t *ast) -> Term {
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        raise_error();
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
        case clingo_ast_type_term_format_string: {
            return TermFormatString::acquire(ast);
        }
        default: {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    }
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

auto construct_format_field(clingo_ast_t *ast) -> FormatField {
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        raise_error();
    }
    switch (type) {
        case clingo_ast_type_format_field_literal: {
            return FormatFieldLiteral::acquire(ast);
        }
        case clingo_ast_type_format_field_expression: {
            return FormatFieldExpression::acquire(ast);
        }
        default: {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    }
}

auto construct_format_field_array(clingo_ast_t **ast, size_t size) -> FormatFieldArray {
    FormatFieldArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(construct_format_field(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto FormatFieldLiteral::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto FormatFieldLiteral::value() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_value, &ret));
    return {ret.data, ret.size};
}

auto FormatFieldLiteral::construct(Library &lib, Location const &location, std::string_view value)
    -> FormatFieldLiteral {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_format_field_literal, &res_,
                                      static_cast<clingo_location_t const *>(location), value.data(), value.size()));
    return FormatFieldLiteral::acquire(res_);
}

void FormatFieldLiteral::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                               [[maybe_unused]] py::kwargs const &kwargs) {
}

auto FormatFieldLiteral::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                   [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<FormatFieldLiteral> {
    return std::nullopt;
}

auto FormatFieldLiteral::update(Library &lib, py::kwargs const &kwargs) -> FormatFieldLiteral {
    return FormatFieldLiteral::construct(
        lib, update_value<Location>(this, &FormatFieldLiteral::location, kwargs, "location"),
        update_value<std::string_view>(this, &FormatFieldLiteral::value, kwargs, "value"));
}

auto FormatFieldExpression::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto FormatFieldExpression::left() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast));
    return construct_term(ast);
}

auto FormatFieldExpression::right() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_right, &ret));
    return {ret.data, ret.size};
}

auto FormatFieldExpression::construct(Library &lib, Location const &location, Term const &left, std::string_view right)
    -> FormatFieldExpression {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_format_field_expression, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(left), right.data(),
                                      right.size()));
    return FormatFieldExpression::acquire(res_);
}

void FormatFieldExpression::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                  [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(left(), *args, **kwargs);
}

auto FormatFieldExpression::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                      [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<FormatFieldExpression> {
    auto [left_value, left_changed] = transform_value(left(), transform, args, kwargs);
    if (left_changed) {
        return FormatFieldExpression::construct(lib, location(), left_value, right());
    }
    return std::nullopt;
}

auto FormatFieldExpression::update(Library &lib, py::kwargs const &kwargs) -> FormatFieldExpression {
    return FormatFieldExpression::construct(
        lib, update_value<Location>(this, &FormatFieldExpression::location, kwargs, "location"),
        update_value<Term>(this, &FormatFieldExpression::left, kwargs, "left"),
        update_value<std::string_view>(this, &FormatFieldExpression::right, kwargs, "right"));
}

auto Projection::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto Projection::construct(Library &lib, Location const &location) -> Projection {
    clingo_ast_t *res_ = nullptr;
    handle_error(
        clingo_ast_construct(lib, clingo_ast_type_projection, &res_, static_cast<clingo_location_t const *>(location)));
    return Projection::acquire(res_);
}

void Projection::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                       [[maybe_unused]] py::kwargs const &kwargs) {
}

auto Projection::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                           [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<Projection> {
    return std::nullopt;
}

auto Projection::update(Library &lib, py::kwargs const &kwargs) -> Projection {
    return Projection::construct(lib, update_value<Location>(this, &Projection::location, kwargs, "location"));
}

auto construct_term_or_projection(clingo_ast_t *ast) -> TermOrProjection {
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        raise_error();
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
        case clingo_ast_type_term_format_string: {
            return TermFormatString::acquire(ast);
        }
        case clingo_ast_type_projection: {
            return Projection::acquire(ast);
        }
        default: {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    }
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
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        raise_error();
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
        case clingo_ast_type_term_format_string: {
            return TermFormatString::acquire(ast);
        }
        case clingo_ast_type_argument_tuple: {
            return ArgumentTuple::acquire(ast);
        }
        default: {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    }
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

auto TermFormatString::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TermFormatString::elements() -> FormatFieldArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size));
    return construct_format_field_array(ast, size);
}

auto TermFormatString::construct(Library &lib, Location const &location, FormatFieldIterable const &elements)
    -> TermFormatString {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_term_format_string, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(elements).data(),
                                      elements.size()));
    return TermFormatString::acquire(res_);
}

void TermFormatString::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                             [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_elements, visitor, args, kwargs, construct_format_field);
}

auto TermFormatString::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                 [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TermFormatString> {
    auto [elements_value, elements_changed] = transform_array(elements(), transform, args, kwargs);
    if (elements_changed) {
        return TermFormatString::construct(lib, location(), elements_value);
    }
    return std::nullopt;
}

auto TermFormatString::update(Library &lib, py::kwargs const &kwargs) -> TermFormatString {
    return TermFormatString::construct(
        lib, update_value<Location>(this, &TermFormatString::location, kwargs, "location"),
        update_value<FormatFieldArray>(this, &TermFormatString::elements, kwargs, "elements"));
}

auto TermVariable::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TermVariable::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto TermVariable::anonymous() -> bool {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_anonymous, &ret));
    return ret != 0;
}

auto TermVariable::construct(Library &lib, Location const &location, std::string_view name, bool anonymous)
    -> TermVariable {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_term_variable, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(),
                                      static_cast<int>(anonymous)));
    return TermVariable::acquire(res_);
}

void TermVariable::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                         [[maybe_unused]] py::kwargs const &kwargs) {
}

auto TermVariable::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                             [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TermVariable> {
    return std::nullopt;
}

auto TermVariable::update(Library &lib, py::kwargs const &kwargs) -> TermVariable {
    return TermVariable::construct(lib, update_value<Location>(this, &TermVariable::location, kwargs, "location"),
                                   update_value<std::string_view>(this, &TermVariable::name, kwargs, "name"),
                                   update_value<bool>(this, &TermVariable::anonymous, kwargs, "anonymous"));
}

auto TermSymbolic::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TermSymbolic::symbol() -> Symbol {
    clingo_symbol_t ret = 0;
    handle_error(clingo_ast_attribute_get_symbol(ast_, clingo_ast_attribute_symbol, &ret));
    return Symbol{ret, true};
}

auto TermSymbolic::construct(Library &lib, Location const &location, Symbol const &symbol) -> TermSymbolic {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_term_symbolic, &res_,
                                      static_cast<clingo_location_t const *>(location), symbol.handle()));
    return TermSymbolic::acquire(res_);
}

void TermSymbolic::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                         [[maybe_unused]] py::kwargs const &kwargs) {
}

auto TermSymbolic::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                             [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TermSymbolic> {
    return std::nullopt;
}

auto TermSymbolic::update(Library &lib, py::kwargs const &kwargs) -> TermSymbolic {
    return TermSymbolic::construct(lib, update_value<Location>(this, &TermSymbolic::location, kwargs, "location"),
                                   update_value<Symbol>(this, &TermSymbolic::symbol, kwargs, "symbol"));
}

auto TermAbsolute::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TermAbsolute::pool() -> TermArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &ast, &size));
    return construct_term_array(ast, size);
}

auto TermAbsolute::construct(Library &lib, Location const &location, TermIterable const &pool) -> TermAbsolute {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_term_absolute, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(pool).data(),
                                      pool.size()));
    return TermAbsolute::acquire(res_);
}

void TermAbsolute::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                         [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_pool, visitor, args, kwargs, construct_term);
}

auto TermAbsolute::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                             [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TermAbsolute> {
    auto [pool_value, pool_changed] = transform_array(pool(), transform, args, kwargs);
    if (pool_changed) {
        return TermAbsolute::construct(lib, location(), pool_value);
    }
    return std::nullopt;
}

auto TermAbsolute::update(Library &lib, py::kwargs const &kwargs) -> TermAbsolute {
    return TermAbsolute::construct(lib, update_value<Location>(this, &TermAbsolute::location, kwargs, "location"),
                                   update_value<TermArray>(this, &TermAbsolute::pool, kwargs, "pool"));
}

auto TermUnaryOperation::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TermUnaryOperation::operator_type() -> UnaryOperator {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_operator_type, &ret));
    return static_cast<UnaryOperator>(ret);
}

auto TermUnaryOperation::right() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast));
    return construct_term(ast);
}

auto TermUnaryOperation::construct(Library &lib, Location const &location, UnaryOperator const &operator_type,
                                   Term const &right) -> TermUnaryOperation {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_term_unary_operation, &res_,
                                      static_cast<clingo_location_t const *>(location), static_cast<int>(operator_type),
                                      c_cast(right)));
    return TermUnaryOperation::acquire(res_);
}

void TermUnaryOperation::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                               [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(right(), *args, **kwargs);
}

auto TermUnaryOperation::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                   [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TermUnaryOperation> {
    auto [right_value, right_changed] = transform_value(right(), transform, args, kwargs);
    if (right_changed) {
        return TermUnaryOperation::construct(lib, location(), operator_type(), right_value);
    }
    return std::nullopt;
}

auto TermUnaryOperation::update(Library &lib, py::kwargs const &kwargs) -> TermUnaryOperation {
    return TermUnaryOperation::construct(
        lib, update_value<Location>(this, &TermUnaryOperation::location, kwargs, "location"),
        update_value<UnaryOperator>(this, &TermUnaryOperation::operator_type, kwargs, "operator_type"),
        update_value<Term>(this, &TermUnaryOperation::right, kwargs, "right"));
}

auto TermBinaryOperation::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TermBinaryOperation::left() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast));
    return construct_term(ast);
}

auto TermBinaryOperation::operator_type() -> BinaryOperator {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_operator_type, &ret));
    return static_cast<BinaryOperator>(ret);
}

auto TermBinaryOperation::right() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast));
    return construct_term(ast);
}

auto TermBinaryOperation::construct(Library &lib, Location const &location, Term const &left,
                                    BinaryOperator const &operator_type, Term const &right) -> TermBinaryOperation {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_term_binary_operation, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(left),
                                      static_cast<int>(operator_type), c_cast(right)));
    return TermBinaryOperation::acquire(res_);
}

void TermBinaryOperation::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(left(), *args, **kwargs);
    visitor(right(), *args, **kwargs);
}

auto TermBinaryOperation::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                    [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TermBinaryOperation> {
    auto [left_value, left_changed] = transform_value(left(), transform, args, kwargs);
    auto [right_value, right_changed] = transform_value(right(), transform, args, kwargs);
    if (left_changed || right_changed) {
        return TermBinaryOperation::construct(lib, location(), left_value, operator_type(), right_value);
    }
    return std::nullopt;
}

auto TermBinaryOperation::update(Library &lib, py::kwargs const &kwargs) -> TermBinaryOperation {
    return TermBinaryOperation::construct(
        lib, update_value<Location>(this, &TermBinaryOperation::location, kwargs, "location"),
        update_value<Term>(this, &TermBinaryOperation::left, kwargs, "left"),
        update_value<BinaryOperator>(this, &TermBinaryOperation::operator_type, kwargs, "operator_type"),
        update_value<Term>(this, &TermBinaryOperation::right, kwargs, "right"));
}

auto TermTuple::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TermTuple::pool() -> TermOrArgumentTupleArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &ast, &size));
    return construct_term_or_argument_tuple_array(ast, size);
}

auto TermTuple::construct(Library &lib, Location const &location, TermOrArgumentTupleIterable const &pool)
    -> TermTuple {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_term_tuple, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(pool).data(),
                                      pool.size()));
    return TermTuple::acquire(res_);
}

void TermTuple::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                      [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_pool, visitor, args, kwargs, construct_term_or_argument_tuple);
}

auto TermTuple::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                          [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TermTuple> {
    auto [pool_value, pool_changed] = transform_array(pool(), transform, args, kwargs);
    if (pool_changed) {
        return TermTuple::construct(lib, location(), pool_value);
    }
    return std::nullopt;
}

auto TermTuple::update(Library &lib, py::kwargs const &kwargs) -> TermTuple {
    return TermTuple::construct(lib, update_value<Location>(this, &TermTuple::location, kwargs, "location"),
                                update_value<TermOrArgumentTupleArray>(this, &TermTuple::pool, kwargs, "pool"));
}

auto TermFunction::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TermFunction::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto TermFunction::pool() -> ArgumentTupleArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &ast, &size));
    return construct_argument_tuple_array(ast, size);
}

auto TermFunction::external() -> bool {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_external, &ret));
    return ret != 0;
}

auto TermFunction::construct(Library &lib, Location const &location, std::string_view name,
                             ArgumentTupleIterable const &pool, bool external) -> TermFunction {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_term_function, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(),
                                      c_cast(pool).data(), pool.size(), static_cast<int>(external)));
    return TermFunction::acquire(res_);
}

void TermFunction::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                         [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_pool, visitor, args, kwargs, ArgumentTuple::acquire);
}

auto TermFunction::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                             [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TermFunction> {
    auto [pool_value, pool_changed] = transform_array(pool(), transform, args, kwargs);
    if (pool_changed) {
        return TermFunction::construct(lib, location(), name(), pool_value, external());
    }
    return std::nullopt;
}

auto TermFunction::update(Library &lib, py::kwargs const &kwargs) -> TermFunction {
    return TermFunction::construct(lib, update_value<Location>(this, &TermFunction::location, kwargs, "location"),
                                   update_value<std::string_view>(this, &TermFunction::name, kwargs, "name"),
                                   update_value<ArgumentTupleArray>(this, &TermFunction::pool, kwargs, "pool"),
                                   update_value<bool>(this, &TermFunction::external, kwargs, "external"));
}

auto ArgumentTuple::arguments() -> TermOrProjectionArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_arguments, &ast, &size));
    return construct_term_or_projection_array(ast, size);
}

auto ArgumentTuple::construct(Library &lib, TermOrProjectionIterable const &arguments) -> ArgumentTuple {
    clingo_ast_t *res_ = nullptr;
    handle_error(
        clingo_ast_construct(lib, clingo_ast_type_argument_tuple, &res_, c_cast(arguments).data(), arguments.size()));
    return ArgumentTuple::acquire(res_);
}

void ArgumentTuple::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                          [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_arguments, visitor, args, kwargs, construct_term_or_projection);
}

auto ArgumentTuple::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                              [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<ArgumentTuple> {
    auto [arguments_value, arguments_changed] = transform_array(arguments(), transform, args, kwargs);
    if (arguments_changed) {
        return ArgumentTuple::construct(lib, arguments_value);
    }
    return std::nullopt;
}

auto ArgumentTuple::update(Library &lib, py::kwargs const &kwargs) -> ArgumentTuple {
    return ArgumentTuple::construct(
        lib, update_value<TermOrProjectionArray>(this, &ArgumentTuple::arguments, kwargs, "arguments"));
}

auto construct_literal(clingo_ast_t *ast) -> Literal {
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        raise_error();
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
        default: {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    }
}

auto LeftGuard::term() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast));
    return construct_term(ast);
}

auto LeftGuard::relation() -> Relation {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_relation, &ret));
    return static_cast<Relation>(ret);
}

auto LeftGuard::construct(Library &lib, Term const &term, Relation const &relation) -> LeftGuard {
    clingo_ast_t *res_ = nullptr;
    handle_error(
        clingo_ast_construct(lib, clingo_ast_type_left_guard, &res_, c_cast(term), static_cast<int>(relation)));
    return LeftGuard::acquire(res_);
}

void LeftGuard::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                      [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(term(), *args, **kwargs);
}

auto LeftGuard::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                          [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<LeftGuard> {
    auto [term_value, term_changed] = transform_value(term(), transform, args, kwargs);
    if (term_changed) {
        return LeftGuard::construct(lib, term_value, relation());
    }
    return std::nullopt;
}

auto LeftGuard::update(Library &lib, py::kwargs const &kwargs) -> LeftGuard {
    return LeftGuard::construct(lib, update_value<Term>(this, &LeftGuard::term, kwargs, "term"),
                                update_value<Relation>(this, &LeftGuard::relation, kwargs, "relation"));
}

auto RightGuard::relation() -> Relation {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_relation, &ret));
    return static_cast<Relation>(ret);
}

auto RightGuard::term() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast));
    return construct_term(ast);
}

auto RightGuard::construct(Library &lib, Relation const &relation, Term const &term) -> RightGuard {
    clingo_ast_t *res_ = nullptr;
    handle_error(
        clingo_ast_construct(lib, clingo_ast_type_right_guard, &res_, static_cast<int>(relation), c_cast(term)));
    return RightGuard::acquire(res_);
}

void RightGuard::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                       [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(term(), *args, **kwargs);
}

auto RightGuard::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                           [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<RightGuard> {
    auto [term_value, term_changed] = transform_value(term(), transform, args, kwargs);
    if (term_changed) {
        return RightGuard::construct(lib, relation(), term_value);
    }
    return std::nullopt;
}

auto RightGuard::update(Library &lib, py::kwargs const &kwargs) -> RightGuard {
    return RightGuard::construct(lib, update_value<Relation>(this, &RightGuard::relation, kwargs, "relation"),
                                 update_value<Term>(this, &RightGuard::term, kwargs, "term"));
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

auto LiteralBoolean::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto LiteralBoolean::sign() -> Sign {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret));
    return static_cast<Sign>(ret);
}

auto LiteralBoolean::value() -> bool {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_value, &ret));
    return ret != 0;
}

auto LiteralBoolean::construct(Library &lib, Location const &location, Sign const &sign, bool value) -> LiteralBoolean {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_literal_boolean, &res_,
                                      static_cast<clingo_location_t const *>(location), static_cast<int>(sign),
                                      static_cast<int>(value)));
    return LiteralBoolean::acquire(res_);
}

void LiteralBoolean::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                           [[maybe_unused]] py::kwargs const &kwargs) {
}

auto LiteralBoolean::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                               [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<LiteralBoolean> {
    return std::nullopt;
}

auto LiteralBoolean::update(Library &lib, py::kwargs const &kwargs) -> LiteralBoolean {
    return LiteralBoolean::construct(lib, update_value<Location>(this, &LiteralBoolean::location, kwargs, "location"),
                                     update_value<Sign>(this, &LiteralBoolean::sign, kwargs, "sign"),
                                     update_value<bool>(this, &LiteralBoolean::value, kwargs, "value"));
}

auto LiteralComparison::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto LiteralComparison::sign() -> Sign {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret));
    return static_cast<Sign>(ret);
}

auto LiteralComparison::left() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast));
    return construct_term(ast);
}

auto LiteralComparison::right() -> RightGuardArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_right, &ast, &size));
    return construct_right_guard_array(ast, size);
}

auto LiteralComparison::construct(Library &lib, Location const &location, Sign const &sign, Term const &left,
                                  RightGuardIterable const &right) -> LiteralComparison {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_literal_comparison, &res_,
                                      static_cast<clingo_location_t const *>(location), static_cast<int>(sign),
                                      c_cast(left), c_cast(right).data(), right.size()));
    return LiteralComparison::acquire(res_);
}

void LiteralComparison::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                              [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(left(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_right, visitor, args, kwargs, RightGuard::acquire);
}

auto LiteralComparison::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                  [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<LiteralComparison> {
    auto [left_value, left_changed] = transform_value(left(), transform, args, kwargs);
    auto [right_value, right_changed] = transform_array(right(), transform, args, kwargs);
    if (left_changed || right_changed) {
        return LiteralComparison::construct(lib, location(), sign(), left_value, right_value);
    }
    return std::nullopt;
}

auto LiteralComparison::update(Library &lib, py::kwargs const &kwargs) -> LiteralComparison {
    return LiteralComparison::construct(
        lib, update_value<Location>(this, &LiteralComparison::location, kwargs, "location"),
        update_value<Sign>(this, &LiteralComparison::sign, kwargs, "sign"),
        update_value<Term>(this, &LiteralComparison::left, kwargs, "left"),
        update_value<RightGuardArray>(this, &LiteralComparison::right, kwargs, "right"));
}

auto LiteralSymbolic::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto LiteralSymbolic::sign() -> Sign {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret));
    return static_cast<Sign>(ret);
}

auto LiteralSymbolic::atom() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_atom, &ast));
    return construct_term(ast);
}

auto LiteralSymbolic::construct(Library &lib, Location const &location, Sign const &sign, Term const &atom)
    -> LiteralSymbolic {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_literal_symbolic, &res_,
                                      static_cast<clingo_location_t const *>(location), static_cast<int>(sign),
                                      c_cast(atom)));
    return LiteralSymbolic::acquire(res_);
}

void LiteralSymbolic::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                            [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(atom(), *args, **kwargs);
}

auto LiteralSymbolic::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<LiteralSymbolic> {
    auto [atom_value, atom_changed] = transform_value(atom(), transform, args, kwargs);
    if (atom_changed) {
        return LiteralSymbolic::construct(lib, location(), sign(), atom_value);
    }
    return std::nullopt;
}

auto LiteralSymbolic::update(Library &lib, py::kwargs const &kwargs) -> LiteralSymbolic {
    return LiteralSymbolic::construct(lib, update_value<Location>(this, &LiteralSymbolic::location, kwargs, "location"),
                                      update_value<Sign>(this, &LiteralSymbolic::sign, kwargs, "sign"),
                                      update_value<Term>(this, &LiteralSymbolic::atom, kwargs, "atom"));
}

auto construct_theory_term(clingo_ast_t *ast) -> TheoryTerm {
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        raise_error();
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
        default: {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    }
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

auto UnparsedElement::operators() -> std::vector<std::string_view> {
    clingo_string_t const *value = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_string_array(ast_, clingo_ast_attribute_operators, &value, &size));
    std::vector<std::string_view> ret;
    ret.reserve(size);
    for (auto const &x : std::span{value, size}) {
        ret.emplace_back(x.data, x.size);
    }
    return ret;
}

auto UnparsedElement::term() -> TheoryTerm {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast));
    return construct_theory_term(ast);
}

auto UnparsedElement::construct(Library &lib, StringIterable const &operators, TheoryTerm const &term)
    -> UnparsedElement {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_unparsed_element, &res_, c_cast(operators).data(),
                                      operators.size(), c_cast(term)));
    return UnparsedElement::acquire(res_);
}

void UnparsedElement::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                            [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(term(), *args, **kwargs);
}

auto UnparsedElement::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<UnparsedElement> {
    auto [term_value, term_changed] = transform_value(term(), transform, args, kwargs);
    if (term_changed) {
        return UnparsedElement::construct(lib, to_string_array(operators()), term_value);
    }
    return std::nullopt;
}

auto UnparsedElement::update(Library &lib, py::kwargs const &kwargs) -> UnparsedElement {
    return UnparsedElement::construct(lib,
                                      update_value<StringArray>(this, &UnparsedElement::operators, kwargs, "operators"),
                                      update_value<TheoryTerm>(this, &UnparsedElement::term, kwargs, "term"));
}

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

auto TheoryTermVariable::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TheoryTermVariable::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto TheoryTermVariable::anonymous() -> bool {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_anonymous, &ret));
    return ret != 0;
}

auto TheoryTermVariable::construct(Library &lib, Location const &location, std::string_view name, bool anonymous)
    -> TheoryTermVariable {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_theory_term_variable, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(),
                                      static_cast<int>(anonymous)));
    return TheoryTermVariable::acquire(res_);
}

void TheoryTermVariable::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                               [[maybe_unused]] py::kwargs const &kwargs) {
}

auto TheoryTermVariable::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                   [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TheoryTermVariable> {
    return std::nullopt;
}

auto TheoryTermVariable::update(Library &lib, py::kwargs const &kwargs) -> TheoryTermVariable {
    return TheoryTermVariable::construct(
        lib, update_value<Location>(this, &TheoryTermVariable::location, kwargs, "location"),
        update_value<std::string_view>(this, &TheoryTermVariable::name, kwargs, "name"),
        update_value<bool>(this, &TheoryTermVariable::anonymous, kwargs, "anonymous"));
}

auto TheoryTermSymbolic::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TheoryTermSymbolic::symbol() -> Symbol {
    clingo_symbol_t ret = 0;
    handle_error(clingo_ast_attribute_get_symbol(ast_, clingo_ast_attribute_symbol, &ret));
    return Symbol{ret, true};
}

auto TheoryTermSymbolic::construct(Library &lib, Location const &location, Symbol const &symbol) -> TheoryTermSymbolic {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_theory_term_symbolic, &res_,
                                      static_cast<clingo_location_t const *>(location), symbol.handle()));
    return TheoryTermSymbolic::acquire(res_);
}

void TheoryTermSymbolic::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                               [[maybe_unused]] py::kwargs const &kwargs) {
}

auto TheoryTermSymbolic::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                   [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TheoryTermSymbolic> {
    return std::nullopt;
}

auto TheoryTermSymbolic::update(Library &lib, py::kwargs const &kwargs) -> TheoryTermSymbolic {
    return TheoryTermSymbolic::construct(
        lib, update_value<Location>(this, &TheoryTermSymbolic::location, kwargs, "location"),
        update_value<Symbol>(this, &TheoryTermSymbolic::symbol, kwargs, "symbol"));
}

auto TheoryTermTuple::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TheoryTermTuple::tuple_type() -> TheoryTupleType {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_tuple_type, &ret));
    return static_cast<TheoryTupleType>(ret);
}

auto TheoryTermTuple::arguments() -> TheoryTermArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_arguments, &ast, &size));
    return construct_theory_term_array(ast, size);
}

auto TheoryTermTuple::construct(Library &lib, Location const &location, TheoryTupleType const &tuple_type,
                                TheoryTermIterable const &arguments) -> TheoryTermTuple {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_theory_term_tuple, &res_,
                                      static_cast<clingo_location_t const *>(location), static_cast<int>(tuple_type),
                                      c_cast(arguments).data(), arguments.size()));
    return TheoryTermTuple::acquire(res_);
}

void TheoryTermTuple::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                            [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_arguments, visitor, args, kwargs, construct_theory_term);
}

auto TheoryTermTuple::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TheoryTermTuple> {
    auto [arguments_value, arguments_changed] = transform_array(arguments(), transform, args, kwargs);
    if (arguments_changed) {
        return TheoryTermTuple::construct(lib, location(), tuple_type(), arguments_value);
    }
    return std::nullopt;
}

auto TheoryTermTuple::update(Library &lib, py::kwargs const &kwargs) -> TheoryTermTuple {
    return TheoryTermTuple::construct(
        lib, update_value<Location>(this, &TheoryTermTuple::location, kwargs, "location"),
        update_value<TheoryTupleType>(this, &TheoryTermTuple::tuple_type, kwargs, "tuple_type"),
        update_value<TheoryTermArray>(this, &TheoryTermTuple::arguments, kwargs, "arguments"));
}

auto TheoryTermFunction::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TheoryTermFunction::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto TheoryTermFunction::arguments() -> TheoryTermArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_arguments, &ast, &size));
    return construct_theory_term_array(ast, size);
}

auto TheoryTermFunction::construct(Library &lib, Location const &location, std::string_view name,
                                   TheoryTermIterable const &arguments) -> TheoryTermFunction {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_theory_term_function, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(),
                                      c_cast(arguments).data(), arguments.size()));
    return TheoryTermFunction::acquire(res_);
}

void TheoryTermFunction::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                               [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_arguments, visitor, args, kwargs, construct_theory_term);
}

auto TheoryTermFunction::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                   [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TheoryTermFunction> {
    auto [arguments_value, arguments_changed] = transform_array(arguments(), transform, args, kwargs);
    if (arguments_changed) {
        return TheoryTermFunction::construct(lib, location(), name(), arguments_value);
    }
    return std::nullopt;
}

auto TheoryTermFunction::update(Library &lib, py::kwargs const &kwargs) -> TheoryTermFunction {
    return TheoryTermFunction::construct(
        lib, update_value<Location>(this, &TheoryTermFunction::location, kwargs, "location"),
        update_value<std::string_view>(this, &TheoryTermFunction::name, kwargs, "name"),
        update_value<TheoryTermArray>(this, &TheoryTermFunction::arguments, kwargs, "arguments"));
}

auto TheoryTermUnparsed::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TheoryTermUnparsed::elements() -> UnparsedElementArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size));
    return construct_unparsed_element_array(ast, size);
}

auto TheoryTermUnparsed::construct(Library &lib, Location const &location, UnparsedElementIterable const &elements)
    -> TheoryTermUnparsed {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_theory_term_unparsed, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(elements).data(),
                                      elements.size()));
    return TheoryTermUnparsed::acquire(res_);
}

void TheoryTermUnparsed::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                               [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_elements, visitor, args, kwargs, UnparsedElement::acquire);
}

auto TheoryTermUnparsed::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                   [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TheoryTermUnparsed> {
    auto [elements_value, elements_changed] = transform_array(elements(), transform, args, kwargs);
    if (elements_changed) {
        return TheoryTermUnparsed::construct(lib, location(), elements_value);
    }
    return std::nullopt;
}

auto TheoryTermUnparsed::update(Library &lib, py::kwargs const &kwargs) -> TheoryTermUnparsed {
    return TheoryTermUnparsed::construct(
        lib, update_value<Location>(this, &TheoryTermUnparsed::location, kwargs, "location"),
        update_value<UnparsedElementArray>(this, &TheoryTermUnparsed::elements, kwargs, "elements"));
}

auto TheoryRightGuard::theory_operator() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_theory_operator, &ret));
    return {ret.data, ret.size};
}

auto TheoryRightGuard::term() -> TheoryTerm {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast));
    return construct_theory_term(ast);
}

auto TheoryRightGuard::construct(Library &lib, std::string_view theory_operator, TheoryTerm const &term)
    -> TheoryRightGuard {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_theory_right_guard, &res_, theory_operator.data(),
                                      theory_operator.size(), c_cast(term)));
    return TheoryRightGuard::acquire(res_);
}

void TheoryRightGuard::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                             [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(term(), *args, **kwargs);
}

auto TheoryRightGuard::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                 [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TheoryRightGuard> {
    auto [term_value, term_changed] = transform_value(term(), transform, args, kwargs);
    if (term_changed) {
        return TheoryRightGuard::construct(lib, theory_operator(), term_value);
    }
    return std::nullopt;
}

auto TheoryRightGuard::update(Library &lib, py::kwargs const &kwargs) -> TheoryRightGuard {
    return TheoryRightGuard::construct(
        lib, update_value<std::string_view>(this, &TheoryRightGuard::theory_operator, kwargs, "theory_operator"),
        update_value<TheoryTerm>(this, &TheoryRightGuard::term, kwargs, "term"));
}

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

auto SetAggregateElement::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto SetAggregateElement::literal() -> Literal {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast));
    return construct_literal(ast);
}

auto SetAggregateElement::condition() -> LiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size));
    return construct_literal_array(ast, size);
}

auto SetAggregateElement::construct(Library &lib, Location const &location, Literal const &literal,
                                    LiteralIterable const &condition) -> SetAggregateElement {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_set_aggregate_element, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(literal),
                                      c_cast(condition).data(), condition.size()));
    return SetAggregateElement::acquire(res_);
}

void SetAggregateElement::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(literal(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_condition, visitor, args, kwargs, construct_literal);
}

auto SetAggregateElement::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                    [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<SetAggregateElement> {
    auto [literal_value, literal_changed] = transform_value(literal(), transform, args, kwargs);
    auto [condition_value, condition_changed] = transform_array(condition(), transform, args, kwargs);
    if (literal_changed || condition_changed) {
        return SetAggregateElement::construct(lib, location(), literal_value, condition_value);
    }
    return std::nullopt;
}

auto SetAggregateElement::update(Library &lib, py::kwargs const &kwargs) -> SetAggregateElement {
    return SetAggregateElement::construct(
        lib, update_value<Location>(this, &SetAggregateElement::location, kwargs, "location"),
        update_value<Literal>(this, &SetAggregateElement::literal, kwargs, "literal"),
        update_value<LiteralArray>(this, &SetAggregateElement::condition, kwargs, "condition"));
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

auto BodyAggregateElement::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto BodyAggregateElement::tuple() -> TermArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_tuple, &ast, &size));
    return construct_term_array(ast, size);
}

auto BodyAggregateElement::condition() -> LiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size));
    return construct_literal_array(ast, size);
}

auto BodyAggregateElement::construct(Library &lib, Location const &location, TermIterable const &tuple,
                                     LiteralIterable const &condition) -> BodyAggregateElement {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_body_aggregate_element, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(tuple).data(),
                                      tuple.size(), c_cast(condition).data(), condition.size()));
    return BodyAggregateElement::acquire(res_);
}

void BodyAggregateElement::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                 [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_tuple, visitor, args, kwargs, construct_term);
    visit_array(ast_, clingo_ast_attribute_condition, visitor, args, kwargs, construct_literal);
}

auto BodyAggregateElement::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                     [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<BodyAggregateElement> {
    auto [tuple_value, tuple_changed] = transform_array(tuple(), transform, args, kwargs);
    auto [condition_value, condition_changed] = transform_array(condition(), transform, args, kwargs);
    if (tuple_changed || condition_changed) {
        return BodyAggregateElement::construct(lib, location(), tuple_value, condition_value);
    }
    return std::nullopt;
}

auto BodyAggregateElement::update(Library &lib, py::kwargs const &kwargs) -> BodyAggregateElement {
    return BodyAggregateElement::construct(
        lib, update_value<Location>(this, &BodyAggregateElement::location, kwargs, "location"),
        update_value<TermArray>(this, &BodyAggregateElement::tuple, kwargs, "tuple"),
        update_value<LiteralArray>(this, &BodyAggregateElement::condition, kwargs, "condition"));
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

auto TheoryAtomElement::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TheoryAtomElement::tuple() -> TheoryTermArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_tuple, &ast, &size));
    return construct_theory_term_array(ast, size);
}

auto TheoryAtomElement::condition() -> LiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size));
    return construct_literal_array(ast, size);
}

auto TheoryAtomElement::construct(Library &lib, Location const &location, TheoryTermIterable const &tuple,
                                  LiteralIterable const &condition) -> TheoryAtomElement {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_theory_atom_element, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(tuple).data(),
                                      tuple.size(), c_cast(condition).data(), condition.size()));
    return TheoryAtomElement::acquire(res_);
}

void TheoryAtomElement::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                              [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_tuple, visitor, args, kwargs, construct_theory_term);
    visit_array(ast_, clingo_ast_attribute_condition, visitor, args, kwargs, construct_literal);
}

auto TheoryAtomElement::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                  [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TheoryAtomElement> {
    auto [tuple_value, tuple_changed] = transform_array(tuple(), transform, args, kwargs);
    auto [condition_value, condition_changed] = transform_array(condition(), transform, args, kwargs);
    if (tuple_changed || condition_changed) {
        return TheoryAtomElement::construct(lib, location(), tuple_value, condition_value);
    }
    return std::nullopt;
}

auto TheoryAtomElement::update(Library &lib, py::kwargs const &kwargs) -> TheoryAtomElement {
    return TheoryAtomElement::construct(
        lib, update_value<Location>(this, &TheoryAtomElement::location, kwargs, "location"),
        update_value<TheoryTermArray>(this, &TheoryAtomElement::tuple, kwargs, "tuple"),
        update_value<LiteralArray>(this, &TheoryAtomElement::condition, kwargs, "condition"));
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
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        raise_error();
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
        default: {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    }
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
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast));
    return construct_literal(ast);
}

auto BodySimpleLiteral::construct(Library &lib, Literal const &literal) -> BodySimpleLiteral {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_body_simple_literal, &res_, c_cast(literal)));
    return BodySimpleLiteral::acquire(res_);
}

void BodySimpleLiteral::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                              [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(literal(), *args, **kwargs);
}

auto BodySimpleLiteral::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                  [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<BodySimpleLiteral> {
    auto [literal_value, literal_changed] = transform_value(literal(), transform, args, kwargs);
    if (literal_changed) {
        return BodySimpleLiteral::construct(lib, literal_value);
    }
    return std::nullopt;
}

auto BodySimpleLiteral::update(Library &lib, py::kwargs const &kwargs) -> BodySimpleLiteral {
    return BodySimpleLiteral::construct(lib,
                                        update_value<Literal>(this, &BodySimpleLiteral::literal, kwargs, "literal"));
}

auto BodyAggregate::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto BodyAggregate::sign() -> Sign {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret));
    return static_cast<Sign>(ret);
}

auto BodyAggregate::left() -> OptionalLeftGuard {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast));
    if (ast != nullptr) {
        // left_guard
        return LeftGuard::acquire(ast);
    }
    return std::nullopt;
}

auto BodyAggregate::function() -> AggregateFunction {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_function, &ret));
    return static_cast<AggregateFunction>(ret);
}

auto BodyAggregate::elements() -> BodyAggregateElementArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size));
    return construct_body_aggregate_element_array(ast, size);
}

auto BodyAggregate::right() -> OptionalRightGuard {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast));
    if (ast != nullptr) {
        // right_guard
        return RightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto BodyAggregate::construct(Library &lib, Location const &location, Sign const &sign, OptionalLeftGuard const &left,
                              AggregateFunction const &function, BodyAggregateElementIterable const &elements,
                              OptionalRightGuard const &right) -> BodyAggregate {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_body_aggregate, &res_,
                                      static_cast<clingo_location_t const *>(location), static_cast<int>(sign),
                                      c_cast(left), static_cast<int>(function), c_cast(elements).data(),
                                      elements.size(), c_cast(right)));
    return BodyAggregate::acquire(res_);
}

void BodyAggregate::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                          [[maybe_unused]] py::kwargs const &kwargs) {
    if (auto opt = left()) {
        visitor(*opt, *args, **kwargs);
    }
    visit_array(ast_, clingo_ast_attribute_elements, visitor, args, kwargs, BodyAggregateElement::acquire);
    if (auto opt = right()) {
        visitor(*opt, *args, **kwargs);
    }
}

auto BodyAggregate::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                              [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<BodyAggregate> {
    auto [left_value, left_changed] = transform_opt_value(left(), transform, args, kwargs);
    auto [elements_value, elements_changed] = transform_array(elements(), transform, args, kwargs);
    auto [right_value, right_changed] = transform_opt_value(right(), transform, args, kwargs);
    if (left_changed || elements_changed || right_changed) {
        return BodyAggregate::construct(lib, location(), sign(), left_value, function(), elements_value, right_value);
    }
    return std::nullopt;
}

auto BodyAggregate::update(Library &lib, py::kwargs const &kwargs) -> BodyAggregate {
    return BodyAggregate::construct(
        lib, update_value<Location>(this, &BodyAggregate::location, kwargs, "location"),
        update_value<Sign>(this, &BodyAggregate::sign, kwargs, "sign"),
        update_value<OptionalLeftGuard>(this, &BodyAggregate::left, kwargs, "left"),
        update_value<AggregateFunction>(this, &BodyAggregate::function, kwargs, "function"),
        update_value<BodyAggregateElementArray>(this, &BodyAggregate::elements, kwargs, "elements"),
        update_value<OptionalRightGuard>(this, &BodyAggregate::right, kwargs, "right"));
}

auto BodySetAggregate::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto BodySetAggregate::sign() -> Sign {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret));
    return static_cast<Sign>(ret);
}

auto BodySetAggregate::left() -> OptionalLeftGuard {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast));
    if (ast != nullptr) {
        // left_guard
        return LeftGuard::acquire(ast);
    }
    return std::nullopt;
}

auto BodySetAggregate::elements() -> SetAggregateElementArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size));
    return construct_set_aggregate_element_array(ast, size);
}

auto BodySetAggregate::right() -> OptionalRightGuard {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast));
    if (ast != nullptr) {
        // right_guard
        return RightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto BodySetAggregate::construct(Library &lib, Location const &location, Sign const &sign,
                                 OptionalLeftGuard const &left, SetAggregateElementIterable const &elements,
                                 OptionalRightGuard const &right) -> BodySetAggregate {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_body_set_aggregate, &res_,
                                      static_cast<clingo_location_t const *>(location), static_cast<int>(sign),
                                      c_cast(left), c_cast(elements).data(), elements.size(), c_cast(right)));
    return BodySetAggregate::acquire(res_);
}

void BodySetAggregate::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                             [[maybe_unused]] py::kwargs const &kwargs) {
    if (auto opt = left()) {
        visitor(*opt, *args, **kwargs);
    }
    visit_array(ast_, clingo_ast_attribute_elements, visitor, args, kwargs, SetAggregateElement::acquire);
    if (auto opt = right()) {
        visitor(*opt, *args, **kwargs);
    }
}

auto BodySetAggregate::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                 [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<BodySetAggregate> {
    auto [left_value, left_changed] = transform_opt_value(left(), transform, args, kwargs);
    auto [elements_value, elements_changed] = transform_array(elements(), transform, args, kwargs);
    auto [right_value, right_changed] = transform_opt_value(right(), transform, args, kwargs);
    if (left_changed || elements_changed || right_changed) {
        return BodySetAggregate::construct(lib, location(), sign(), left_value, elements_value, right_value);
    }
    return std::nullopt;
}

auto BodySetAggregate::update(Library &lib, py::kwargs const &kwargs) -> BodySetAggregate {
    return BodySetAggregate::construct(
        lib, update_value<Location>(this, &BodySetAggregate::location, kwargs, "location"),
        update_value<Sign>(this, &BodySetAggregate::sign, kwargs, "sign"),
        update_value<OptionalLeftGuard>(this, &BodySetAggregate::left, kwargs, "left"),
        update_value<SetAggregateElementArray>(this, &BodySetAggregate::elements, kwargs, "elements"),
        update_value<OptionalRightGuard>(this, &BodySetAggregate::right, kwargs, "right"));
}

auto BodyTheoryAtom::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto BodyTheoryAtom::sign() -> Sign {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret));
    return static_cast<Sign>(ret);
}

auto BodyTheoryAtom::name() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_name, &ast));
    return construct_term(ast);
}

auto BodyTheoryAtom::elements() -> TheoryAtomElementArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size));
    return construct_theory_atom_element_array(ast, size);
}

auto BodyTheoryAtom::right() -> OptionalTheoryRightGuard {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast));
    if (ast != nullptr) {
        // theory_right_guard
        return TheoryRightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto BodyTheoryAtom::construct(Library &lib, Location const &location, Sign const &sign, Term const &name,
                               TheoryAtomElementIterable const &elements, OptionalTheoryRightGuard const &right)
    -> BodyTheoryAtom {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_body_theory_atom, &res_,
                                      static_cast<clingo_location_t const *>(location), static_cast<int>(sign),
                                      c_cast(name), c_cast(elements).data(), elements.size(), c_cast(right)));
    return BodyTheoryAtom::acquire(res_);
}

void BodyTheoryAtom::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                           [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(name(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_elements, visitor, args, kwargs, TheoryAtomElement::acquire);
    if (auto opt = right()) {
        visitor(*opt, *args, **kwargs);
    }
}

auto BodyTheoryAtom::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                               [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<BodyTheoryAtom> {
    auto [name_value, name_changed] = transform_value(name(), transform, args, kwargs);
    auto [elements_value, elements_changed] = transform_array(elements(), transform, args, kwargs);
    auto [right_value, right_changed] = transform_opt_value(right(), transform, args, kwargs);
    if (name_changed || elements_changed || right_changed) {
        return BodyTheoryAtom::construct(lib, location(), sign(), name_value, elements_value, right_value);
    }
    return std::nullopt;
}

auto BodyTheoryAtom::update(Library &lib, py::kwargs const &kwargs) -> BodyTheoryAtom {
    return BodyTheoryAtom::construct(
        lib, update_value<Location>(this, &BodyTheoryAtom::location, kwargs, "location"),
        update_value<Sign>(this, &BodyTheoryAtom::sign, kwargs, "sign"),
        update_value<Term>(this, &BodyTheoryAtom::name, kwargs, "name"),
        update_value<TheoryAtomElementArray>(this, &BodyTheoryAtom::elements, kwargs, "elements"),
        update_value<OptionalTheoryRightGuard>(this, &BodyTheoryAtom::right, kwargs, "right"));
}

auto BodyConditionalLiteral::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto BodyConditionalLiteral::literal() -> Literal {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast));
    return construct_literal(ast);
}

auto BodyConditionalLiteral::condition() -> LiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size));
    return construct_literal_array(ast, size);
}

auto BodyConditionalLiteral::construct(Library &lib, Location const &location, Literal const &literal,
                                       LiteralIterable const &condition) -> BodyConditionalLiteral {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_body_conditional_literal, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(literal),
                                      c_cast(condition).data(), condition.size()));
    return BodyConditionalLiteral::acquire(res_);
}

void BodyConditionalLiteral::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                   [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(literal(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_condition, visitor, args, kwargs, construct_literal);
}

auto BodyConditionalLiteral::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                       [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<BodyConditionalLiteral> {
    auto [literal_value, literal_changed] = transform_value(literal(), transform, args, kwargs);
    auto [condition_value, condition_changed] = transform_array(condition(), transform, args, kwargs);
    if (literal_changed || condition_changed) {
        return BodyConditionalLiteral::construct(lib, location(), literal_value, condition_value);
    }
    return std::nullopt;
}

auto BodyConditionalLiteral::update(Library &lib, py::kwargs const &kwargs) -> BodyConditionalLiteral {
    return BodyConditionalLiteral::construct(
        lib, update_value<Location>(this, &BodyConditionalLiteral::location, kwargs, "location"),
        update_value<Literal>(this, &BodyConditionalLiteral::literal, kwargs, "literal"),
        update_value<LiteralArray>(this, &BodyConditionalLiteral::condition, kwargs, "condition"));
}

auto HeadConditionalLiteral::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto HeadConditionalLiteral::literal() -> Literal {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast));
    return construct_literal(ast);
}

auto HeadConditionalLiteral::condition() -> LiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size));
    return construct_literal_array(ast, size);
}

auto HeadConditionalLiteral::construct(Library &lib, Location const &location, Literal const &literal,
                                       LiteralIterable const &condition) -> HeadConditionalLiteral {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_head_conditional_literal, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(literal),
                                      c_cast(condition).data(), condition.size()));
    return HeadConditionalLiteral::acquire(res_);
}

void HeadConditionalLiteral::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                   [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(literal(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_condition, visitor, args, kwargs, construct_literal);
}

auto HeadConditionalLiteral::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                       [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<HeadConditionalLiteral> {
    auto [literal_value, literal_changed] = transform_value(literal(), transform, args, kwargs);
    auto [condition_value, condition_changed] = transform_array(condition(), transform, args, kwargs);
    if (literal_changed || condition_changed) {
        return HeadConditionalLiteral::construct(lib, location(), literal_value, condition_value);
    }
    return std::nullopt;
}

auto HeadConditionalLiteral::update(Library &lib, py::kwargs const &kwargs) -> HeadConditionalLiteral {
    return HeadConditionalLiteral::construct(
        lib, update_value<Location>(this, &HeadConditionalLiteral::location, kwargs, "location"),
        update_value<Literal>(this, &HeadConditionalLiteral::literal, kwargs, "literal"),
        update_value<LiteralArray>(this, &HeadConditionalLiteral::condition, kwargs, "condition"));
}

auto construct_disjunction_element(clingo_ast_t *ast) -> DisjunctionElement {
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        raise_error();
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
        default: {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    }
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

auto HeadAggregateElement::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto HeadAggregateElement::tuple() -> TermArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_tuple, &ast, &size));
    return construct_term_array(ast, size);
}

auto HeadAggregateElement::literal() -> Literal {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast));
    return construct_literal(ast);
}

auto HeadAggregateElement::condition() -> LiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size));
    return construct_literal_array(ast, size);
}

auto HeadAggregateElement::construct(Library &lib, Location const &location, TermIterable const &tuple,
                                     Literal const &literal, LiteralIterable const &condition) -> HeadAggregateElement {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_head_aggregate_element, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(tuple).data(),
                                      tuple.size(), c_cast(literal), c_cast(condition).data(), condition.size()));
    return HeadAggregateElement::acquire(res_);
}

void HeadAggregateElement::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                 [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_tuple, visitor, args, kwargs, construct_term);
    visitor(literal(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_condition, visitor, args, kwargs, construct_literal);
}

auto HeadAggregateElement::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                     [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<HeadAggregateElement> {
    auto [tuple_value, tuple_changed] = transform_array(tuple(), transform, args, kwargs);
    auto [literal_value, literal_changed] = transform_value(literal(), transform, args, kwargs);
    auto [condition_value, condition_changed] = transform_array(condition(), transform, args, kwargs);
    if (tuple_changed || literal_changed || condition_changed) {
        return HeadAggregateElement::construct(lib, location(), tuple_value, literal_value, condition_value);
    }
    return std::nullopt;
}

auto HeadAggregateElement::update(Library &lib, py::kwargs const &kwargs) -> HeadAggregateElement {
    return HeadAggregateElement::construct(
        lib, update_value<Location>(this, &HeadAggregateElement::location, kwargs, "location"),
        update_value<TermArray>(this, &HeadAggregateElement::tuple, kwargs, "tuple"),
        update_value<Literal>(this, &HeadAggregateElement::literal, kwargs, "literal"),
        update_value<LiteralArray>(this, &HeadAggregateElement::condition, kwargs, "condition"));
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
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        raise_error();
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
        default: {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    }
}

auto HeadSimpleLiteral::literal() -> Literal {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_literal, &ast));
    return construct_literal(ast);
}

auto HeadSimpleLiteral::construct(Library &lib, Literal const &literal) -> HeadSimpleLiteral {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_head_simple_literal, &res_, c_cast(literal)));
    return HeadSimpleLiteral::acquire(res_);
}

void HeadSimpleLiteral::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                              [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(literal(), *args, **kwargs);
}

auto HeadSimpleLiteral::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                  [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<HeadSimpleLiteral> {
    auto [literal_value, literal_changed] = transform_value(literal(), transform, args, kwargs);
    if (literal_changed) {
        return HeadSimpleLiteral::construct(lib, literal_value);
    }
    return std::nullopt;
}

auto HeadSimpleLiteral::update(Library &lib, py::kwargs const &kwargs) -> HeadSimpleLiteral {
    return HeadSimpleLiteral::construct(lib,
                                        update_value<Literal>(this, &HeadSimpleLiteral::literal, kwargs, "literal"));
}

auto HeadAggregate::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto HeadAggregate::left() -> OptionalLeftGuard {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast));
    if (ast != nullptr) {
        // left_guard
        return LeftGuard::acquire(ast);
    }
    return std::nullopt;
}

auto HeadAggregate::function() -> AggregateFunction {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_function, &ret));
    return static_cast<AggregateFunction>(ret);
}

auto HeadAggregate::elements() -> HeadAggregateElementArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size));
    return construct_head_aggregate_element_array(ast, size);
}

auto HeadAggregate::right() -> OptionalRightGuard {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast));
    if (ast != nullptr) {
        // right_guard
        return RightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto HeadAggregate::construct(Library &lib, Location const &location, OptionalLeftGuard const &left,
                              AggregateFunction const &function, HeadAggregateElementIterable const &elements,
                              OptionalRightGuard const &right) -> HeadAggregate {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(
        lib, clingo_ast_type_head_aggregate, &res_, static_cast<clingo_location_t const *>(location), c_cast(left),
        static_cast<int>(function), c_cast(elements).data(), elements.size(), c_cast(right)));
    return HeadAggregate::acquire(res_);
}

void HeadAggregate::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                          [[maybe_unused]] py::kwargs const &kwargs) {
    if (auto opt = left()) {
        visitor(*opt, *args, **kwargs);
    }
    visit_array(ast_, clingo_ast_attribute_elements, visitor, args, kwargs, HeadAggregateElement::acquire);
    if (auto opt = right()) {
        visitor(*opt, *args, **kwargs);
    }
}

auto HeadAggregate::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                              [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<HeadAggregate> {
    auto [left_value, left_changed] = transform_opt_value(left(), transform, args, kwargs);
    auto [elements_value, elements_changed] = transform_array(elements(), transform, args, kwargs);
    auto [right_value, right_changed] = transform_opt_value(right(), transform, args, kwargs);
    if (left_changed || elements_changed || right_changed) {
        return HeadAggregate::construct(lib, location(), left_value, function(), elements_value, right_value);
    }
    return std::nullopt;
}

auto HeadAggregate::update(Library &lib, py::kwargs const &kwargs) -> HeadAggregate {
    return HeadAggregate::construct(
        lib, update_value<Location>(this, &HeadAggregate::location, kwargs, "location"),
        update_value<OptionalLeftGuard>(this, &HeadAggregate::left, kwargs, "left"),
        update_value<AggregateFunction>(this, &HeadAggregate::function, kwargs, "function"),
        update_value<HeadAggregateElementArray>(this, &HeadAggregate::elements, kwargs, "elements"),
        update_value<OptionalRightGuard>(this, &HeadAggregate::right, kwargs, "right"));
}

auto HeadSetAggregate::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto HeadSetAggregate::left() -> OptionalLeftGuard {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ast));
    if (ast != nullptr) {
        // left_guard
        return LeftGuard::acquire(ast);
    }
    return std::nullopt;
}

auto HeadSetAggregate::elements() -> SetAggregateElementArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size));
    return construct_set_aggregate_element_array(ast, size);
}

auto HeadSetAggregate::right() -> OptionalRightGuard {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast));
    if (ast != nullptr) {
        // right_guard
        return RightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto HeadSetAggregate::construct(Library &lib, Location const &location, OptionalLeftGuard const &left,
                                 SetAggregateElementIterable const &elements, OptionalRightGuard const &right)
    -> HeadSetAggregate {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_head_set_aggregate, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(left),
                                      c_cast(elements).data(), elements.size(), c_cast(right)));
    return HeadSetAggregate::acquire(res_);
}

void HeadSetAggregate::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                             [[maybe_unused]] py::kwargs const &kwargs) {
    if (auto opt = left()) {
        visitor(*opt, *args, **kwargs);
    }
    visit_array(ast_, clingo_ast_attribute_elements, visitor, args, kwargs, SetAggregateElement::acquire);
    if (auto opt = right()) {
        visitor(*opt, *args, **kwargs);
    }
}

auto HeadSetAggregate::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                 [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<HeadSetAggregate> {
    auto [left_value, left_changed] = transform_opt_value(left(), transform, args, kwargs);
    auto [elements_value, elements_changed] = transform_array(elements(), transform, args, kwargs);
    auto [right_value, right_changed] = transform_opt_value(right(), transform, args, kwargs);
    if (left_changed || elements_changed || right_changed) {
        return HeadSetAggregate::construct(lib, location(), left_value, elements_value, right_value);
    }
    return std::nullopt;
}

auto HeadSetAggregate::update(Library &lib, py::kwargs const &kwargs) -> HeadSetAggregate {
    return HeadSetAggregate::construct(
        lib, update_value<Location>(this, &HeadSetAggregate::location, kwargs, "location"),
        update_value<OptionalLeftGuard>(this, &HeadSetAggregate::left, kwargs, "left"),
        update_value<SetAggregateElementArray>(this, &HeadSetAggregate::elements, kwargs, "elements"),
        update_value<OptionalRightGuard>(this, &HeadSetAggregate::right, kwargs, "right"));
}

auto HeadTheoryAtom::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto HeadTheoryAtom::name() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_name, &ast));
    return construct_term(ast);
}

auto HeadTheoryAtom::elements() -> TheoryAtomElementArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size));
    return construct_theory_atom_element_array(ast, size);
}

auto HeadTheoryAtom::right() -> OptionalTheoryRightGuard {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ast));
    if (ast != nullptr) {
        // theory_right_guard
        return TheoryRightGuard::acquire(ast);
    }
    return std::nullopt;
}

auto HeadTheoryAtom::construct(Library &lib, Location const &location, Term const &name,
                               TheoryAtomElementIterable const &elements, OptionalTheoryRightGuard const &right)
    -> HeadTheoryAtom {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_head_theory_atom, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(name),
                                      c_cast(elements).data(), elements.size(), c_cast(right)));
    return HeadTheoryAtom::acquire(res_);
}

void HeadTheoryAtom::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                           [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(name(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_elements, visitor, args, kwargs, TheoryAtomElement::acquire);
    if (auto opt = right()) {
        visitor(*opt, *args, **kwargs);
    }
}

auto HeadTheoryAtom::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                               [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<HeadTheoryAtom> {
    auto [name_value, name_changed] = transform_value(name(), transform, args, kwargs);
    auto [elements_value, elements_changed] = transform_array(elements(), transform, args, kwargs);
    auto [right_value, right_changed] = transform_opt_value(right(), transform, args, kwargs);
    if (name_changed || elements_changed || right_changed) {
        return HeadTheoryAtom::construct(lib, location(), name_value, elements_value, right_value);
    }
    return std::nullopt;
}

auto HeadTheoryAtom::update(Library &lib, py::kwargs const &kwargs) -> HeadTheoryAtom {
    return HeadTheoryAtom::construct(
        lib, update_value<Location>(this, &HeadTheoryAtom::location, kwargs, "location"),
        update_value<Term>(this, &HeadTheoryAtom::name, kwargs, "name"),
        update_value<TheoryAtomElementArray>(this, &HeadTheoryAtom::elements, kwargs, "elements"),
        update_value<OptionalTheoryRightGuard>(this, &HeadTheoryAtom::right, kwargs, "right"));
}

auto HeadDisjunction::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto HeadDisjunction::elements() -> DisjunctionElementArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size));
    return construct_disjunction_element_array(ast, size);
}

auto HeadDisjunction::construct(Library &lib, Location const &location, DisjunctionElementIterable const &elements)
    -> HeadDisjunction {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_head_disjunction, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(elements).data(),
                                      elements.size()));
    return HeadDisjunction::acquire(res_);
}

void HeadDisjunction::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                            [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_elements, visitor, args, kwargs, construct_disjunction_element);
}

auto HeadDisjunction::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<HeadDisjunction> {
    auto [elements_value, elements_changed] = transform_array(elements(), transform, args, kwargs);
    if (elements_changed) {
        return HeadDisjunction::construct(lib, location(), elements_value);
    }
    return std::nullopt;
}

auto HeadDisjunction::update(Library &lib, py::kwargs const &kwargs) -> HeadDisjunction {
    return HeadDisjunction::construct(
        lib, update_value<Location>(this, &HeadDisjunction::location, kwargs, "location"),
        update_value<DisjunctionElementArray>(this, &HeadDisjunction::elements, kwargs, "elements"));
}

auto TheoryOperatorDefinition::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TheoryOperatorDefinition::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto TheoryOperatorDefinition::priority() -> int {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_priority, &ret));
    return ret;
}

auto TheoryOperatorDefinition::operator_type() -> TheoryOperatorType {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_operator_type, &ret));
    return static_cast<TheoryOperatorType>(ret);
}

auto TheoryOperatorDefinition::construct(Library &lib, Location const &location, std::string_view name, int priority,
                                         TheoryOperatorType const &operator_type) -> TheoryOperatorDefinition {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_theory_operator_definition, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(),
                                      priority, static_cast<int>(operator_type)));
    return TheoryOperatorDefinition::acquire(res_);
}

void TheoryOperatorDefinition::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                     [[maybe_unused]] py::kwargs const &kwargs) {
}

auto TheoryOperatorDefinition::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                         [[maybe_unused]] py::args const &args,
                                         [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TheoryOperatorDefinition> {
    return std::nullopt;
}

auto TheoryOperatorDefinition::update(Library &lib, py::kwargs const &kwargs) -> TheoryOperatorDefinition {
    return TheoryOperatorDefinition::construct(
        lib, update_value<Location>(this, &TheoryOperatorDefinition::location, kwargs, "location"),
        update_value<std::string_view>(this, &TheoryOperatorDefinition::name, kwargs, "name"),
        update_value<int>(this, &TheoryOperatorDefinition::priority, kwargs, "priority"),
        update_value<TheoryOperatorType>(this, &TheoryOperatorDefinition::operator_type, kwargs, "operator_type"));
}

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

auto TheoryTermDefinition::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TheoryTermDefinition::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto TheoryTermDefinition::operators() -> TheoryOperatorDefinitionArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_operators, &ast, &size));
    return construct_theory_operator_definition_array(ast, size);
}

auto TheoryTermDefinition::construct(Library &lib, Location const &location, std::string_view name,
                                     TheoryOperatorDefinitionIterable const &operators) -> TheoryTermDefinition {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_theory_term_definition, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(),
                                      c_cast(operators).data(), operators.size()));
    return TheoryTermDefinition::acquire(res_);
}

void TheoryTermDefinition::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                 [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_operators, visitor, args, kwargs, TheoryOperatorDefinition::acquire);
}

auto TheoryTermDefinition::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                     [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TheoryTermDefinition> {
    auto [operators_value, operators_changed] = transform_array(operators(), transform, args, kwargs);
    if (operators_changed) {
        return TheoryTermDefinition::construct(lib, location(), name(), operators_value);
    }
    return std::nullopt;
}

auto TheoryTermDefinition::update(Library &lib, py::kwargs const &kwargs) -> TheoryTermDefinition {
    return TheoryTermDefinition::construct(
        lib, update_value<Location>(this, &TheoryTermDefinition::location, kwargs, "location"),
        update_value<std::string_view>(this, &TheoryTermDefinition::name, kwargs, "name"),
        update_value<TheoryOperatorDefinitionArray>(this, &TheoryTermDefinition::operators, kwargs, "operators"));
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

auto TheoryGuardDefinition::operators() -> std::vector<std::string_view> {
    clingo_string_t const *value = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_string_array(ast_, clingo_ast_attribute_operators, &value, &size));
    std::vector<std::string_view> ret;
    ret.reserve(size);
    for (auto const &x : std::span{value, size}) {
        ret.emplace_back(x.data, x.size);
    }
    return ret;
}

auto TheoryGuardDefinition::term() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_term, &ret));
    return {ret.data, ret.size};
}

auto TheoryGuardDefinition::construct(Library &lib, StringIterable const &operators, std::string_view term)
    -> TheoryGuardDefinition {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_theory_guard_definition, &res_, c_cast(operators).data(),
                                      operators.size(), term.data(), term.size()));
    return TheoryGuardDefinition::acquire(res_);
}

void TheoryGuardDefinition::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                  [[maybe_unused]] py::kwargs const &kwargs) {
}

auto TheoryGuardDefinition::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                      [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TheoryGuardDefinition> {
    return std::nullopt;
}

auto TheoryGuardDefinition::update(Library &lib, py::kwargs const &kwargs) -> TheoryGuardDefinition {
    return TheoryGuardDefinition::construct(
        lib, update_value<StringArray>(this, &TheoryGuardDefinition::operators, kwargs, "operators"),
        update_value<std::string_view>(this, &TheoryGuardDefinition::term, kwargs, "term"));
}

auto TheoryAtomDefinition::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto TheoryAtomDefinition::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto TheoryAtomDefinition::arity() -> int {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_arity, &ret));
    return ret;
}

auto TheoryAtomDefinition::term() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_term, &ret));
    return {ret.data, ret.size};
}

auto TheoryAtomDefinition::guard() -> OptionalTheoryGuardDefinition {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_guard, &ast));
    if (ast != nullptr) {
        // theory_guard_definition
        return TheoryGuardDefinition::acquire(ast);
    }
    return std::nullopt;
}

auto TheoryAtomDefinition::atom_type() -> TheoryAtomType {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_atom_type, &ret));
    return static_cast<TheoryAtomType>(ret);
}

auto TheoryAtomDefinition::construct(Library &lib, Location const &location, std::string_view name, int arity,
                                     std::string_view term, OptionalTheoryGuardDefinition const &guard,
                                     TheoryAtomType const &atom_type) -> TheoryAtomDefinition {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_theory_atom_definition, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(), arity,
                                      term.data(), term.size(), c_cast(guard), static_cast<int>(atom_type)));
    return TheoryAtomDefinition::acquire(res_);
}

void TheoryAtomDefinition::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                 [[maybe_unused]] py::kwargs const &kwargs) {
    if (auto opt = guard()) {
        visitor(*opt, *args, **kwargs);
    }
}

auto TheoryAtomDefinition::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                     [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<TheoryAtomDefinition> {
    auto [guard_value, guard_changed] = transform_opt_value(guard(), transform, args, kwargs);
    if (guard_changed) {
        return TheoryAtomDefinition::construct(lib, location(), name(), arity(), term(), guard_value, atom_type());
    }
    return std::nullopt;
}

auto TheoryAtomDefinition::update(Library &lib, py::kwargs const &kwargs) -> TheoryAtomDefinition {
    return TheoryAtomDefinition::construct(
        lib, update_value<Location>(this, &TheoryAtomDefinition::location, kwargs, "location"),
        update_value<std::string_view>(this, &TheoryAtomDefinition::name, kwargs, "name"),
        update_value<int>(this, &TheoryAtomDefinition::arity, kwargs, "arity"),
        update_value<std::string_view>(this, &TheoryAtomDefinition::term, kwargs, "term"),
        update_value<OptionalTheoryGuardDefinition>(this, &TheoryAtomDefinition::guard, kwargs, "guard"),
        update_value<TheoryAtomType>(this, &TheoryAtomDefinition::atom_type, kwargs, "atom_type"));
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
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_weight, &ast));
    return construct_term(ast);
}

auto OptimizeTuple::priority() -> OptionalTerm {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_priority, &ast));
    if (ast != nullptr) {
        // term
        return construct_term(ast);
    }
    return std::nullopt;
}

auto OptimizeTuple::terms() -> TermArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_terms, &ast, &size));
    return construct_term_array(ast, size);
}

auto OptimizeTuple::construct(Library &lib, Term const &weight, OptionalTerm const &priority, TermIterable const &terms)
    -> OptimizeTuple {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_optimize_tuple, &res_, c_cast(weight), c_cast(priority),
                                      c_cast(terms).data(), terms.size()));
    return OptimizeTuple::acquire(res_);
}

void OptimizeTuple::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                          [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(weight(), *args, **kwargs);
    if (auto opt = priority()) {
        visitor(*opt, *args, **kwargs);
    }
    visit_array(ast_, clingo_ast_attribute_terms, visitor, args, kwargs, construct_term);
}

auto OptimizeTuple::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                              [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<OptimizeTuple> {
    auto [weight_value, weight_changed] = transform_value(weight(), transform, args, kwargs);
    auto [priority_value, priority_changed] = transform_opt_value(priority(), transform, args, kwargs);
    auto [terms_value, terms_changed] = transform_array(terms(), transform, args, kwargs);
    if (weight_changed || priority_changed || terms_changed) {
        return OptimizeTuple::construct(lib, weight_value, priority_value, terms_value);
    }
    return std::nullopt;
}

auto OptimizeTuple::update(Library &lib, py::kwargs const &kwargs) -> OptimizeTuple {
    return OptimizeTuple::construct(lib, update_value<Term>(this, &OptimizeTuple::weight, kwargs, "weight"),
                                    update_value<OptionalTerm>(this, &OptimizeTuple::priority, kwargs, "priority"),
                                    update_value<TermArray>(this, &OptimizeTuple::terms, kwargs, "terms"));
}

auto OptimizeElement::tuple() -> OptimizeTuple {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_tuple, &ast));
    return OptimizeTuple::acquire(ast);
}

auto OptimizeElement::condition() -> LiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_condition, &ast, &size));
    return construct_literal_array(ast, size);
}

auto OptimizeElement::construct(Library &lib, OptimizeTuple const &tuple, LiteralIterable const &condition)
    -> OptimizeElement {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_optimize_element, &res_, c_cast(tuple),
                                      c_cast(condition).data(), condition.size()));
    return OptimizeElement::acquire(res_);
}

void OptimizeElement::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                            [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(tuple(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_condition, visitor, args, kwargs, construct_literal);
}

auto OptimizeElement::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<OptimizeElement> {
    auto [tuple_value, tuple_changed] = transform_value(tuple(), transform, args, kwargs);
    auto [condition_value, condition_changed] = transform_array(condition(), transform, args, kwargs);
    if (tuple_changed || condition_changed) {
        return OptimizeElement::construct(lib, tuple_value, condition_value);
    }
    return std::nullopt;
}

auto OptimizeElement::update(Library &lib, py::kwargs const &kwargs) -> OptimizeElement {
    return OptimizeElement::construct(
        lib, update_value<OptimizeTuple>(this, &OptimizeElement::tuple, kwargs, "tuple"),
        update_value<LiteralArray>(this, &OptimizeElement::condition, kwargs, "condition"));
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
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_u, &ast));
    return construct_term(ast);
}

auto Edge::v() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_v, &ast));
    return construct_term(ast);
}

auto Edge::construct(Library &lib, Term const &u, Term const &v) -> Edge {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_edge, &res_, c_cast(u), c_cast(v)));
    return Edge::acquire(res_);
}

void Edge::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                 [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(u(), *args, **kwargs);
    visitor(v(), *args, **kwargs);
}

auto Edge::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                     [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<Edge> {
    auto [u_value, u_changed] = transform_value(u(), transform, args, kwargs);
    auto [v_value, v_changed] = transform_value(v(), transform, args, kwargs);
    if (u_changed || v_changed) {
        return Edge::construct(lib, u_value, v_value);
    }
    return std::nullopt;
}

auto Edge::update(Library &lib, py::kwargs const &kwargs) -> Edge {
    return Edge::construct(lib, update_value<Term>(this, &Edge::u, kwargs, "u"),
                           update_value<Term>(this, &Edge::v, kwargs, "v"));
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

auto ProgramPart::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto ProgramPart::arguments() -> SymbolArray {
    clingo_symbol_t const *value = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_symbol_array(ast_, clingo_ast_attribute_arguments, &value, &size));
    SymbolArray ret;
    ret.reserve(size);
    for (auto const &x : std::span{value, size}) {
        ret.emplace_back(x, true);
    }
    return ret;
}

auto ProgramPart::construct(Library &lib, std::string_view name, SymbolIterable const &arguments) -> ProgramPart {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_program_part, &res_, name.data(), name.size(),
                                      c_cast(arguments).data(), arguments.size()));
    return ProgramPart::acquire(res_);
}

void ProgramPart::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                        [[maybe_unused]] py::kwargs const &kwargs) {
}

auto ProgramPart::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                            [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<ProgramPart> {
    return std::nullopt;
}

auto ProgramPart::update(Library &lib, py::kwargs const &kwargs) -> ProgramPart {
    return ProgramPart::construct(lib, update_value<std::string_view>(this, &ProgramPart::name, kwargs, "name"),
                                  update_value<SymbolArray>(this, &ProgramPart::arguments, kwargs, "arguments"));
}

auto construct_program_part_array(clingo_ast_t **ast, size_t size) -> ProgramPartArray {
    ProgramPartArray ret;
    try {
        ret.reserve(size);
        std::for_each_n(ast, size, [&ret](auto &arg) {
            auto tmp = arg;
            arg = nullptr;
            ret.emplace_back(ProgramPart::acquire(tmp));
        });
        clingo_ast_array_free(ast, size);
    } catch (...) {
        clingo_ast_array_free(ast, size);
        throw;
    }
    return ret;
}

auto construct_statement(clingo_ast_t *ast) -> Statement {
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast, &type)) {
        clingo_ast_free(ast);
        raise_error();
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
        case clingo_ast_type_statement_show_nothing: {
            return StatementShowNothing::acquire(ast);
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
        case clingo_ast_type_statement_parts: {
            return StatementParts::acquire(ast);
        }
        case clingo_ast_type_statement_const: {
            return StatementConst::acquire(ast);
        }
        case clingo_ast_type_statement_comment: {
            return StatementComment::acquire(ast);
        }
        default: {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    }
}

auto StatementRule::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementRule::head() -> HeadLiteral {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_head, &ast));
    return construct_head_literal(ast);
}

auto StatementRule::body() -> BodyLiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size));
    return construct_body_literal_array(ast, size);
}

auto StatementRule::construct(Library &lib, Location const &location, HeadLiteral const &head,
                              BodyLiteralIterable const &body) -> StatementRule {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_rule, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(head),
                                      c_cast(body).data(), body.size()));
    return StatementRule::acquire(res_);
}

void StatementRule::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                          [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(head(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_body, visitor, args, kwargs, construct_body_literal);
}

auto StatementRule::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                              [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementRule> {
    auto [head_value, head_changed] = transform_value(head(), transform, args, kwargs);
    auto [body_value, body_changed] = transform_array(body(), transform, args, kwargs);
    if (head_changed || body_changed) {
        return StatementRule::construct(lib, location(), head_value, body_value);
    }
    return std::nullopt;
}

auto StatementRule::update(Library &lib, py::kwargs const &kwargs) -> StatementRule {
    return StatementRule::construct(lib, update_value<Location>(this, &StatementRule::location, kwargs, "location"),
                                    update_value<HeadLiteral>(this, &StatementRule::head, kwargs, "head"),
                                    update_value<BodyLiteralArray>(this, &StatementRule::body, kwargs, "body"));
}

auto StatementTheory::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementTheory::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto StatementTheory::terms() -> TheoryTermDefinitionArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_terms, &ast, &size));
    return construct_theory_term_definition_array(ast, size);
}

auto StatementTheory::atoms() -> TheoryAtomDefinitionArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_atoms, &ast, &size));
    return construct_theory_atom_definition_array(ast, size);
}

auto StatementTheory::construct(Library &lib, Location const &location, std::string_view name,
                                TheoryTermDefinitionIterable const &terms, TheoryAtomDefinitionIterable const &atoms)
    -> StatementTheory {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_theory, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(),
                                      c_cast(terms).data(), terms.size(), c_cast(atoms).data(), atoms.size()));
    return StatementTheory::acquire(res_);
}

void StatementTheory::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                            [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_terms, visitor, args, kwargs, TheoryTermDefinition::acquire);
    visit_array(ast_, clingo_ast_attribute_atoms, visitor, args, kwargs, TheoryAtomDefinition::acquire);
}

auto StatementTheory::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementTheory> {
    auto [terms_value, terms_changed] = transform_array(terms(), transform, args, kwargs);
    auto [atoms_value, atoms_changed] = transform_array(atoms(), transform, args, kwargs);
    if (terms_changed || atoms_changed) {
        return StatementTheory::construct(lib, location(), name(), terms_value, atoms_value);
    }
    return std::nullopt;
}

auto StatementTheory::update(Library &lib, py::kwargs const &kwargs) -> StatementTheory {
    return StatementTheory::construct(
        lib, update_value<Location>(this, &StatementTheory::location, kwargs, "location"),
        update_value<std::string_view>(this, &StatementTheory::name, kwargs, "name"),
        update_value<TheoryTermDefinitionArray>(this, &StatementTheory::terms, kwargs, "terms"),
        update_value<TheoryAtomDefinitionArray>(this, &StatementTheory::atoms, kwargs, "atoms"));
}

auto StatementOptimize::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementOptimize::elements() -> OptimizeElementArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size));
    return construct_optimize_element_array(ast, size);
}

auto StatementOptimize::optimize_type() -> OptimizeType {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_optimize_type, &ret));
    return static_cast<OptimizeType>(ret);
}

auto StatementOptimize::construct(Library &lib, Location const &location, OptimizeElementIterable const &elements,
                                  OptimizeType const &optimize_type) -> StatementOptimize {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_optimize, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(elements).data(),
                                      elements.size(), static_cast<int>(optimize_type)));
    return StatementOptimize::acquire(res_);
}

void StatementOptimize::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                              [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_elements, visitor, args, kwargs, OptimizeElement::acquire);
}

auto StatementOptimize::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                  [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementOptimize> {
    auto [elements_value, elements_changed] = transform_array(elements(), transform, args, kwargs);
    if (elements_changed) {
        return StatementOptimize::construct(lib, location(), elements_value, optimize_type());
    }
    return std::nullopt;
}

auto StatementOptimize::update(Library &lib, py::kwargs const &kwargs) -> StatementOptimize {
    return StatementOptimize::construct(
        lib, update_value<Location>(this, &StatementOptimize::location, kwargs, "location"),
        update_value<OptimizeElementArray>(this, &StatementOptimize::elements, kwargs, "elements"),
        update_value<OptimizeType>(this, &StatementOptimize::optimize_type, kwargs, "optimize_type"));
}

auto StatementWeakConstraint::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementWeakConstraint::body() -> BodyLiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size));
    return construct_body_literal_array(ast, size);
}

auto StatementWeakConstraint::tuple() -> OptimizeTuple {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_tuple, &ast));
    return OptimizeTuple::acquire(ast);
}

auto StatementWeakConstraint::construct(Library &lib, Location const &location, BodyLiteralIterable const &body,
                                        OptimizeTuple const &tuple) -> StatementWeakConstraint {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_weak_constraint, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(body).data(),
                                      body.size(), c_cast(tuple)));
    return StatementWeakConstraint::acquire(res_);
}

void StatementWeakConstraint::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                    [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_body, visitor, args, kwargs, construct_body_literal);
    visitor(tuple(), *args, **kwargs);
}

auto StatementWeakConstraint::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                        [[maybe_unused]] py::args const &args,
                                        [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementWeakConstraint> {
    auto [body_value, body_changed] = transform_array(body(), transform, args, kwargs);
    auto [tuple_value, tuple_changed] = transform_value(tuple(), transform, args, kwargs);
    if (body_changed || tuple_changed) {
        return StatementWeakConstraint::construct(lib, location(), body_value, tuple_value);
    }
    return std::nullopt;
}

auto StatementWeakConstraint::update(Library &lib, py::kwargs const &kwargs) -> StatementWeakConstraint {
    return StatementWeakConstraint::construct(
        lib, update_value<Location>(this, &StatementWeakConstraint::location, kwargs, "location"),
        update_value<BodyLiteralArray>(this, &StatementWeakConstraint::body, kwargs, "body"),
        update_value<OptimizeTuple>(this, &StatementWeakConstraint::tuple, kwargs, "tuple"));
}

auto StatementShow::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementShow::term() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast));
    return construct_term(ast);
}

auto StatementShow::body() -> BodyLiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size));
    return construct_body_literal_array(ast, size);
}

auto StatementShow::construct(Library &lib, Location const &location, Term const &term, BodyLiteralIterable const &body)
    -> StatementShow {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_show, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(term),
                                      c_cast(body).data(), body.size()));
    return StatementShow::acquire(res_);
}

void StatementShow::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                          [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(term(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_body, visitor, args, kwargs, construct_body_literal);
}

auto StatementShow::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                              [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementShow> {
    auto [term_value, term_changed] = transform_value(term(), transform, args, kwargs);
    auto [body_value, body_changed] = transform_array(body(), transform, args, kwargs);
    if (term_changed || body_changed) {
        return StatementShow::construct(lib, location(), term_value, body_value);
    }
    return std::nullopt;
}

auto StatementShow::update(Library &lib, py::kwargs const &kwargs) -> StatementShow {
    return StatementShow::construct(lib, update_value<Location>(this, &StatementShow::location, kwargs, "location"),
                                    update_value<Term>(this, &StatementShow::term, kwargs, "term"),
                                    update_value<BodyLiteralArray>(this, &StatementShow::body, kwargs, "body"));
}

auto StatementShowNothing::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementShowNothing::construct(Library &lib, Location const &location) -> StatementShowNothing {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_show_nothing, &res_,
                                      static_cast<clingo_location_t const *>(location)));
    return StatementShowNothing::acquire(res_);
}

void StatementShowNothing::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                 [[maybe_unused]] py::kwargs const &kwargs) {
}

auto StatementShowNothing::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                     [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementShowNothing> {
    return std::nullopt;
}

auto StatementShowNothing::update(Library &lib, py::kwargs const &kwargs) -> StatementShowNothing {
    return StatementShowNothing::construct(
        lib, update_value<Location>(this, &StatementShowNothing::location, kwargs, "location"));
}

auto StatementShowSignature::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementShowSignature::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto StatementShowSignature::arity() -> int {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_arity, &ret));
    return ret;
}

auto StatementShowSignature::sign() -> bool {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret));
    return ret != 0;
}

auto StatementShowSignature::value() -> bool {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_value, &ret));
    return ret != 0;
}

auto StatementShowSignature::construct(Library &lib, Location const &location, std::string_view name, int arity,
                                       bool sign, bool value) -> StatementShowSignature {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_show_signature, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(), arity,
                                      static_cast<int>(sign), static_cast<int>(value)));
    return StatementShowSignature::acquire(res_);
}

void StatementShowSignature::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                   [[maybe_unused]] py::kwargs const &kwargs) {
}

auto StatementShowSignature::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                       [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementShowSignature> {
    return std::nullopt;
}

auto StatementShowSignature::update(Library &lib, py::kwargs const &kwargs) -> StatementShowSignature {
    return StatementShowSignature::construct(
        lib, update_value<Location>(this, &StatementShowSignature::location, kwargs, "location"),
        update_value<std::string_view>(this, &StatementShowSignature::name, kwargs, "name"),
        update_value<int>(this, &StatementShowSignature::arity, kwargs, "arity"),
        update_value<bool>(this, &StatementShowSignature::sign, kwargs, "sign"),
        update_value<bool>(this, &StatementShowSignature::value, kwargs, "value"));
}

auto StatementProject::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementProject::atom() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_atom, &ast));
    return construct_term(ast);
}

auto StatementProject::body() -> BodyLiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size));
    return construct_body_literal_array(ast, size);
}

auto StatementProject::construct(Library &lib, Location const &location, Term const &atom,
                                 BodyLiteralIterable const &body) -> StatementProject {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_project, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(atom),
                                      c_cast(body).data(), body.size()));
    return StatementProject::acquire(res_);
}

void StatementProject::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                             [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(atom(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_body, visitor, args, kwargs, construct_body_literal);
}

auto StatementProject::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                 [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementProject> {
    auto [atom_value, atom_changed] = transform_value(atom(), transform, args, kwargs);
    auto [body_value, body_changed] = transform_array(body(), transform, args, kwargs);
    if (atom_changed || body_changed) {
        return StatementProject::construct(lib, location(), atom_value, body_value);
    }
    return std::nullopt;
}

auto StatementProject::update(Library &lib, py::kwargs const &kwargs) -> StatementProject {
    return StatementProject::construct(lib,
                                       update_value<Location>(this, &StatementProject::location, kwargs, "location"),
                                       update_value<Term>(this, &StatementProject::atom, kwargs, "atom"),
                                       update_value<BodyLiteralArray>(this, &StatementProject::body, kwargs, "body"));
}

auto StatementProjectSignature::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementProjectSignature::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto StatementProjectSignature::arity() -> int {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_arity, &ret));
    return ret;
}

auto StatementProjectSignature::sign() -> bool {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret));
    return ret != 0;
}

auto StatementProjectSignature::construct(Library &lib, Location const &location, std::string_view name, int arity,
                                          bool sign) -> StatementProjectSignature {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_project_signature, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(), arity,
                                      static_cast<int>(sign)));
    return StatementProjectSignature::acquire(res_);
}

void StatementProjectSignature::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                                      [[maybe_unused]] py::kwargs const &kwargs) {
}

auto StatementProjectSignature::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                          [[maybe_unused]] py::args const &args,
                                          [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementProjectSignature> {
    return std::nullopt;
}

auto StatementProjectSignature::update(Library &lib, py::kwargs const &kwargs) -> StatementProjectSignature {
    return StatementProjectSignature::construct(
        lib, update_value<Location>(this, &StatementProjectSignature::location, kwargs, "location"),
        update_value<std::string_view>(this, &StatementProjectSignature::name, kwargs, "name"),
        update_value<int>(this, &StatementProjectSignature::arity, kwargs, "arity"),
        update_value<bool>(this, &StatementProjectSignature::sign, kwargs, "sign"));
}

auto StatementDefined::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementDefined::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto StatementDefined::arity() -> int {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_arity, &ret));
    return ret;
}

auto StatementDefined::sign() -> bool {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_sign, &ret));
    return ret != 0;
}

auto StatementDefined::construct(Library &lib, Location const &location, std::string_view name, int arity, bool sign)
    -> StatementDefined {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_defined, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(), arity,
                                      static_cast<int>(sign)));
    return StatementDefined::acquire(res_);
}

void StatementDefined::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                             [[maybe_unused]] py::kwargs const &kwargs) {
}

auto StatementDefined::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                 [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementDefined> {
    return std::nullopt;
}

auto StatementDefined::update(Library &lib, py::kwargs const &kwargs) -> StatementDefined {
    return StatementDefined::construct(lib,
                                       update_value<Location>(this, &StatementDefined::location, kwargs, "location"),
                                       update_value<std::string_view>(this, &StatementDefined::name, kwargs, "name"),
                                       update_value<int>(this, &StatementDefined::arity, kwargs, "arity"),
                                       update_value<bool>(this, &StatementDefined::sign, kwargs, "sign"));
}

auto StatementExternal::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementExternal::atom() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_atom, &ast));
    return construct_term(ast);
}

auto StatementExternal::body() -> BodyLiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size));
    return construct_body_literal_array(ast, size);
}

auto StatementExternal::external_type() -> OptionalTerm {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_external_type, &ast));
    if (ast != nullptr) {
        // term
        return construct_term(ast);
    }
    return std::nullopt;
}

auto StatementExternal::construct(Library &lib, Location const &location, Term const &atom,
                                  BodyLiteralIterable const &body, OptionalTerm const &external_type)
    -> StatementExternal {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_external, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(atom),
                                      c_cast(body).data(), body.size(), c_cast(external_type)));
    return StatementExternal::acquire(res_);
}

void StatementExternal::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                              [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(atom(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_body, visitor, args, kwargs, construct_body_literal);
    if (auto opt = external_type()) {
        visitor(*opt, *args, **kwargs);
    }
}

auto StatementExternal::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                  [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementExternal> {
    auto [atom_value, atom_changed] = transform_value(atom(), transform, args, kwargs);
    auto [body_value, body_changed] = transform_array(body(), transform, args, kwargs);
    auto [external_type_value, external_type_changed] = transform_opt_value(external_type(), transform, args, kwargs);
    if (atom_changed || body_changed || external_type_changed) {
        return StatementExternal::construct(lib, location(), atom_value, body_value, external_type_value);
    }
    return std::nullopt;
}

auto StatementExternal::update(Library &lib, py::kwargs const &kwargs) -> StatementExternal {
    return StatementExternal::construct(
        lib, update_value<Location>(this, &StatementExternal::location, kwargs, "location"),
        update_value<Term>(this, &StatementExternal::atom, kwargs, "atom"),
        update_value<BodyLiteralArray>(this, &StatementExternal::body, kwargs, "body"),
        update_value<OptionalTerm>(this, &StatementExternal::external_type, kwargs, "external_type"));
}

auto StatementEdge::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementEdge::pool() -> EdgeArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_pool, &ast, &size));
    return construct_edge_array(ast, size);
}

auto StatementEdge::body() -> BodyLiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size));
    return construct_body_literal_array(ast, size);
}

auto StatementEdge::construct(Library &lib, Location const &location, EdgeIterable const &pool,
                              BodyLiteralIterable const &body) -> StatementEdge {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_edge, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(pool).data(),
                                      pool.size(), c_cast(body).data(), body.size()));
    return StatementEdge::acquire(res_);
}

void StatementEdge::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                          [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_pool, visitor, args, kwargs, Edge::acquire);
    visit_array(ast_, clingo_ast_attribute_body, visitor, args, kwargs, construct_body_literal);
}

auto StatementEdge::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                              [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementEdge> {
    auto [pool_value, pool_changed] = transform_array(pool(), transform, args, kwargs);
    auto [body_value, body_changed] = transform_array(body(), transform, args, kwargs);
    if (pool_changed || body_changed) {
        return StatementEdge::construct(lib, location(), pool_value, body_value);
    }
    return std::nullopt;
}

auto StatementEdge::update(Library &lib, py::kwargs const &kwargs) -> StatementEdge {
    return StatementEdge::construct(lib, update_value<Location>(this, &StatementEdge::location, kwargs, "location"),
                                    update_value<EdgeArray>(this, &StatementEdge::pool, kwargs, "pool"),
                                    update_value<BodyLiteralArray>(this, &StatementEdge::body, kwargs, "body"));
}

auto StatementHeuristic::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementHeuristic::atom() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_atom, &ast));
    return construct_term(ast);
}

auto StatementHeuristic::body() -> BodyLiteralArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_body, &ast, &size));
    return construct_body_literal_array(ast, size);
}

auto StatementHeuristic::weight() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_weight, &ast));
    return construct_term(ast);
}

auto StatementHeuristic::modifier() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_modifier, &ast));
    return construct_term(ast);
}

auto StatementHeuristic::priority() -> OptionalTerm {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_priority, &ast));
    if (ast != nullptr) {
        // term
        return construct_term(ast);
    }
    return std::nullopt;
}

auto StatementHeuristic::construct(Library &lib, Location const &location, Term const &atom,
                                   BodyLiteralIterable const &body, Term const &weight, Term const &modifier,
                                   OptionalTerm const &priority) -> StatementHeuristic {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(
        lib, clingo_ast_type_statement_heuristic, &res_, static_cast<clingo_location_t const *>(location), c_cast(atom),
        c_cast(body).data(), body.size(), c_cast(weight), c_cast(modifier), c_cast(priority)));
    return StatementHeuristic::acquire(res_);
}

void StatementHeuristic::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                               [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(atom(), *args, **kwargs);
    visit_array(ast_, clingo_ast_attribute_body, visitor, args, kwargs, construct_body_literal);
    visitor(weight(), *args, **kwargs);
    visitor(modifier(), *args, **kwargs);
    if (auto opt = priority()) {
        visitor(*opt, *args, **kwargs);
    }
}

auto StatementHeuristic::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                   [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementHeuristic> {
    auto [atom_value, atom_changed] = transform_value(atom(), transform, args, kwargs);
    auto [body_value, body_changed] = transform_array(body(), transform, args, kwargs);
    auto [weight_value, weight_changed] = transform_value(weight(), transform, args, kwargs);
    auto [modifier_value, modifier_changed] = transform_value(modifier(), transform, args, kwargs);
    auto [priority_value, priority_changed] = transform_opt_value(priority(), transform, args, kwargs);
    if (atom_changed || body_changed || weight_changed || modifier_changed || priority_changed) {
        return StatementHeuristic::construct(lib, location(), atom_value, body_value, weight_value, modifier_value,
                                             priority_value);
    }
    return std::nullopt;
}

auto StatementHeuristic::update(Library &lib, py::kwargs const &kwargs) -> StatementHeuristic {
    return StatementHeuristic::construct(
        lib, update_value<Location>(this, &StatementHeuristic::location, kwargs, "location"),
        update_value<Term>(this, &StatementHeuristic::atom, kwargs, "atom"),
        update_value<BodyLiteralArray>(this, &StatementHeuristic::body, kwargs, "body"),
        update_value<Term>(this, &StatementHeuristic::weight, kwargs, "weight"),
        update_value<Term>(this, &StatementHeuristic::modifier, kwargs, "modifier"),
        update_value<OptionalTerm>(this, &StatementHeuristic::priority, kwargs, "priority"));
}

auto StatementScript::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementScript::value() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_value, &ret));
    return {ret.data, ret.size};
}

auto StatementScript::script_type() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_script_type, &ret));
    return {ret.data, ret.size};
}

auto StatementScript::construct(Library &lib, Location const &location, std::string_view value,
                                std::string_view script_type) -> StatementScript {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_script, &res_,
                                      static_cast<clingo_location_t const *>(location), value.data(), value.size(),
                                      script_type.data(), script_type.size()));
    return StatementScript::acquire(res_);
}

void StatementScript::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                            [[maybe_unused]] py::kwargs const &kwargs) {
}

auto StatementScript::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementScript> {
    return std::nullopt;
}

auto StatementScript::update(Library &lib, py::kwargs const &kwargs) -> StatementScript {
    return StatementScript::construct(
        lib, update_value<Location>(this, &StatementScript::location, kwargs, "location"),
        update_value<std::string_view>(this, &StatementScript::value, kwargs, "value"),
        update_value<std::string_view>(this, &StatementScript::script_type, kwargs, "script_type"));
}

auto StatementInclude::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementInclude::value() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_value, &ret));
    return {ret.data, ret.size};
}

auto StatementInclude::include_type() -> IncludeType {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_include_type, &ret));
    return static_cast<IncludeType>(ret);
}

auto StatementInclude::construct(Library &lib, Location const &location, std::string_view value,
                                 IncludeType const &include_type) -> StatementInclude {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_include, &res_,
                                      static_cast<clingo_location_t const *>(location), value.data(), value.size(),
                                      static_cast<int>(include_type)));
    return StatementInclude::acquire(res_);
}

void StatementInclude::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                             [[maybe_unused]] py::kwargs const &kwargs) {
}

auto StatementInclude::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                 [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementInclude> {
    return std::nullopt;
}

auto StatementInclude::update(Library &lib, py::kwargs const &kwargs) -> StatementInclude {
    return StatementInclude::construct(
        lib, update_value<Location>(this, &StatementInclude::location, kwargs, "location"),
        update_value<std::string_view>(this, &StatementInclude::value, kwargs, "value"),
        update_value<IncludeType>(this, &StatementInclude::include_type, kwargs, "include_type"));
}

auto StatementProgram::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementProgram::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto StatementProgram::arguments() -> std::vector<std::string_view> {
    clingo_string_t const *value = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_string_array(ast_, clingo_ast_attribute_arguments, &value, &size));
    std::vector<std::string_view> ret;
    ret.reserve(size);
    for (auto const &x : std::span{value, size}) {
        ret.emplace_back(x.data, x.size);
    }
    return ret;
}

auto StatementProgram::construct(Library &lib, Location const &location, std::string_view name,
                                 StringIterable const &arguments) -> StatementProgram {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_program, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(),
                                      c_cast(arguments).data(), arguments.size()));
    return StatementProgram::acquire(res_);
}

void StatementProgram::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                             [[maybe_unused]] py::kwargs const &kwargs) {
}

auto StatementProgram::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                 [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementProgram> {
    return std::nullopt;
}

auto StatementProgram::update(Library &lib, py::kwargs const &kwargs) -> StatementProgram {
    return StatementProgram::construct(
        lib, update_value<Location>(this, &StatementProgram::location, kwargs, "location"),
        update_value<std::string_view>(this, &StatementProgram::name, kwargs, "name"),
        update_value<StringArray>(this, &StatementProgram::arguments, kwargs, "arguments"));
}

auto StatementParts::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementParts::elements() -> ProgramPartArray {
    clingo_ast_t **ast = nullptr;
    size_t size = 0;
    handle_error(clingo_ast_attribute_get_ast_array(ast_, clingo_ast_attribute_elements, &ast, &size));
    return construct_program_part_array(ast, size);
}

auto StatementParts::precedence() -> Precedence {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_precedence, &ret));
    return static_cast<Precedence>(ret);
}

auto StatementParts::construct(Library &lib, Location const &location, ProgramPartIterable const &elements,
                               Precedence const &precedence) -> StatementParts {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_parts, &res_,
                                      static_cast<clingo_location_t const *>(location), c_cast(elements).data(),
                                      elements.size(), static_cast<int>(precedence)));
    return StatementParts::acquire(res_);
}

void StatementParts::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                           [[maybe_unused]] py::kwargs const &kwargs) {
    visit_array(ast_, clingo_ast_attribute_elements, visitor, args, kwargs, ProgramPart::acquire);
}

auto StatementParts::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                               [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementParts> {
    auto [elements_value, elements_changed] = transform_array(elements(), transform, args, kwargs);
    if (elements_changed) {
        return StatementParts::construct(lib, location(), elements_value, precedence());
    }
    return std::nullopt;
}

auto StatementParts::update(Library &lib, py::kwargs const &kwargs) -> StatementParts {
    return StatementParts::construct(
        lib, update_value<Location>(this, &StatementParts::location, kwargs, "location"),
        update_value<ProgramPartArray>(this, &StatementParts::elements, kwargs, "elements"),
        update_value<Precedence>(this, &StatementParts::precedence, kwargs, "precedence"));
}

auto StatementConst::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementConst::name() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret));
    return {ret.data, ret.size};
}

auto StatementConst::value() -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_value, &ast));
    return construct_term(ast);
}

auto StatementConst::precedence() -> Precedence {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_precedence, &ret));
    return static_cast<Precedence>(ret);
}

auto StatementConst::construct(Library &lib, Location const &location, std::string_view name, Term const &value,
                               Precedence const &precedence) -> StatementConst {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_const, &res_,
                                      static_cast<clingo_location_t const *>(location), name.data(), name.size(),
                                      c_cast(value), static_cast<int>(precedence)));
    return StatementConst::acquire(res_);
}

void StatementConst::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                           [[maybe_unused]] py::kwargs const &kwargs) {
    visitor(value(), *args, **kwargs);
}

auto StatementConst::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                               [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementConst> {
    auto [value_value, value_changed] = transform_value(value(), transform, args, kwargs);
    if (value_changed) {
        return StatementConst::construct(lib, location(), name(), value_value, precedence());
    }
    return std::nullopt;
}

auto StatementConst::update(Library &lib, py::kwargs const &kwargs) -> StatementConst {
    return StatementConst::construct(lib, update_value<Location>(this, &StatementConst::location, kwargs, "location"),
                                     update_value<std::string_view>(this, &StatementConst::name, kwargs, "name"),
                                     update_value<Term>(this, &StatementConst::value, kwargs, "value"),
                                     update_value<Precedence>(this, &StatementConst::precedence, kwargs, "precedence"));
}

auto StatementComment::location() -> Location {
    clingo_location_t const *ret = nullptr;
    handle_error(clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret));
    return Location{ret};
}

auto StatementComment::value() -> std::string_view {
    clingo_string_t ret;
    handle_error(clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_value, &ret));
    return {ret.data, ret.size};
}

auto StatementComment::comment_type() -> CommentType {
    int ret = 0;
    handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_comment_type, &ret));
    return static_cast<CommentType>(ret);
}

auto StatementComment::construct(Library &lib, Location const &location, std::string_view value,
                                 CommentType const &comment_type) -> StatementComment {
    clingo_ast_t *res_ = nullptr;
    handle_error(clingo_ast_construct(lib, clingo_ast_type_statement_comment, &res_,
                                      static_cast<clingo_location_t const *>(location), value.data(), value.size(),
                                      static_cast<int>(comment_type)));
    return StatementComment::acquire(res_);
}

void StatementComment::visit([[maybe_unused]] py::handle visitor, [[maybe_unused]] py::args const &args,
                             [[maybe_unused]] py::kwargs const &kwargs) {
}

auto StatementComment::transform([[maybe_unused]] Library &lib, [[maybe_unused]] py::handle transform,
                                 [[maybe_unused]] py::args const &args, [[maybe_unused]] py::kwargs const &kwargs)
    -> std::optional<StatementComment> {
    return std::nullopt;
}

auto StatementComment::update(Library &lib, py::kwargs const &kwargs) -> StatementComment {
    return StatementComment::construct(
        lib, update_value<Location>(this, &StatementComment::location, kwargs, "location"),
        update_value<std::string_view>(this, &StatementComment::value, kwargs, "value"),
        update_value<CommentType>(this, &StatementComment::comment_type, kwargs, "comment_type"));
}

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

template <class Cons>
void visit_array(clingo_ast_t *ast, clingo_ast_attribute_t attr, py::handle visitor, py::args const &args,
                 py::kwargs const &kwargs, Cons cons) {
    struct Array {
        ~Array() { clingo_ast_array_free(begin, size); }
        clingo_ast_t **begin = nullptr;
        size_t size = 0;
    };
    auto array = Array{};
    handle_error(clingo_ast_attribute_get_ast_array(ast, attr, &array.begin, &array.size));
    std::for_each_n(array.begin, array.size, [&](auto *&cld) {
        auto *cpy = cld;
        cld = nullptr;
        visitor(std::invoke(cons, cpy), *args, **kwargs);
    });
}

template <class Array>
auto transform_array(Array arr, py::handle transform, py::args const &args, py::kwargs const &kwargs)
    -> std::pair<Array, bool> {
    bool changed = false;
    for (auto &elem : arr) {
        if (auto res = transform(elem, *args, **kwargs); !res.is_none()) {
            elem = py::cast<typename Array::value_type>(std::move(res));
            changed = true;
        }
    }
    return {std::move(arr), changed};
}

template <class Value>
auto transform_value(Value val, py::handle transform, py::args const &args, py::kwargs const &kwargs)
    -> std::pair<Value, bool> {
    auto res = transform(val, *args, **kwargs);
    return {res.is_none() ? std::move(val) : py::cast<Value>(res), !res.is_none()};
}

template <class Value>
auto transform_opt_value(Value opt, py::handle transform, py::args const &args, py::kwargs const &kwargs)
    -> std::pair<Value, bool> {
    if (opt) {
        auto res = transform(*opt, *args, **kwargs);
        return {res.is_none() ? std::move(opt) : py::cast<Value>(res), !res.is_none()};
    }
    return {std::nullopt, false};
}

namespace {

struct CString {
    CString(std::string &&str) : str_{std::move(str)} {}
    operator std::string_view() const { return std::string_view{str_}; }
    std::string str_;
};

} // namespace

template <class T, class F, class M>
auto update_value(F *self, M fun, py::kwargs const &kwargs, char const *attr) -> update_result_t<T> {
    if constexpr (std::is_same_v<T, std::string_view>) {
        if (kwargs.contains(attr)) {
            return py::cast<std::string>(kwargs[attr]);
        }
    } else {
        if (kwargs.contains(attr)) {
            return py::cast<T>(kwargs[attr]);
        }
    }
    if constexpr (std::is_same_v<T, StringArray>) {
        auto arr = (self->*fun)();
        return StringArray{arr.begin(), arr.end()};
    } else {
        return (self->*fun)();
    }
}

auto parse_term(Library &lib, std::string_view string) -> Term {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_parse_expression(lib, clingo_ast_parse_type_term, string.data(), string.size(), &ast));
    return construct_term(ast);
}

auto parse_theory_term(Library &lib, std::string_view string) -> TheoryTerm {
    clingo_ast_t *ast = nullptr;
    handle_error(
        clingo_ast_parse_expression(lib, clingo_ast_parse_type_theory_term, string.data(), string.size(), &ast));
    return construct_theory_term(ast);
}

auto parse_literal(Library &lib, std::string_view string) -> Literal {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_parse_expression(lib, clingo_ast_parse_type_literal, string.data(), string.size(), &ast));
    return construct_literal(ast);
}

auto parse_head_literal(Library &lib, std::string_view string) -> HeadLiteral {
    clingo_ast_t *ast = nullptr;
    handle_error(
        clingo_ast_parse_expression(lib, clingo_ast_parse_type_head_literal, string.data(), string.size(), &ast));
    return construct_head_literal(ast);
}

auto parse_body_literal(Library &lib, std::string_view string) -> BodyLiteral {
    clingo_ast_t *ast = nullptr;
    handle_error(
        clingo_ast_parse_expression(lib, clingo_ast_parse_type_body_literal, string.data(), string.size(), &ast));
    return construct_body_literal(ast);
}

auto parse_statement(Library &lib, std::string_view string) -> Statement {
    clingo_ast_t *ast = nullptr;
    handle_error(clingo_ast_parse_expression(lib, clingo_ast_parse_type_statement, string.data(), string.size(), &ast));
    return construct_statement(ast);
}

void parse_string(Library const &lib, std::string_view program, std::function<void(Statement)> callback,
                  TypeHint<"clingo.control.Control|None"> const &ctl) {
    auto *ptr = !ctl.is_none() ? ctl.cast<Control *>()->c_ptr() : nullptr;
    handle_error(clingo_ast_parse_string(
        lib, program.data(), program.size(), ptr,
        [](clingo_ast_t *ast, void *data) {
            CLINGO_TRY {
                auto &callback = *static_cast<std::function<void(Statement)> *>(data);
                clingo_ast_t *copy = nullptr;
                handle_error(clingo_ast_copy(ast, &copy));
                callback(construct_statement(copy));
            }
            CLINGO_CATCH;
        },
        &callback));
}

inline void parse_files(Library const &lib, std::span<std::string_view const> files,
                        std::function<void(Statement)> callback, TypeHint<"clingo.control.Control|None"> const &ctl) {
    auto *ptr = !ctl.is_none() ? ctl.cast<Control *>()->c_ptr() : nullptr;
    auto cfiles = transform_vec(files, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
    handle_error(clingo_ast_parse_files(
        lib, cfiles.data(), cfiles.size(), ptr,
        [](clingo_ast_t *ast, void *data) {
            CLINGO_TRY {
                auto &callback = *static_cast<std::function<void(Statement)> *>(data);
                clingo_ast_t *copy = nullptr;
                handle_error(clingo_ast_copy(ast, &copy));
                callback(construct_statement(copy));
            }
            CLINGO_CATCH;
        },
        &callback));
}

class RewriteContext {
  public:
    RewriteContext(Library &lib) { handle_error(clingo_ast_rewrite_context_create(lib, &ctx_)); }
    ~RewriteContext() { clingo_ast_rewrite_context_free(ctx_); }
    void set_project_mode(ProjectionMode value) {
        clingo_ast_rewrite_context_set_project_mode(ctx_, static_cast<clingo_projection_mode_t>(value));
    }
    auto get_project_mode() -> ProjectionMode {
        return static_cast<ProjectionMode>(clingo_ast_rewrite_context_get_project_mode(ctx_));
    }
    void set_project_anoymous(bool value) { clingo_ast_rewrite_context_set_project_anonymous(ctx_, value); }
    auto get_project_anonymous() -> bool { return clingo_ast_rewrite_context_get_project_mode(ctx_) != 0; }
    void add_param(std::string const &name) {
        handle_error(clingo_ast_rewrite_context_add_param(ctx_, name.data(), name.size()));
    }
    void clear_params() { clingo_ast_rewrite_context_clear_params(ctx_); }
    void add_theory(StatementTheory const &stm) {
        handle_error(clingo_ast_rewrite_context_add_theory(ctx_, c_cast(stm)));
    }
    friend auto c_cast(RewriteContext const &x) -> clingo_ast_rewrite_context_t * { return x.ctx_; }

  private:
    clingo_ast_rewrite_context_t *ctx_ = nullptr;
};

auto rewrite_statement(RewriteContext &ctx, Statement &stm) -> std::vector<Statement> {
    auto *c_ctx = c_cast(ctx);
    struct Array {
        ~Array() { clingo_ast_array_free(result, result_size); }
        clingo_ast_t **result = nullptr;
        size_t result_size = 0;
    };
    auto arr = Array{};
    handle_error(clingo_ast_rewrite(c_ctx, c_cast(stm), &arr.result, &arr.result_size));
    std::vector<Statement> res;
    res.reserve(arr.result_size);
    std::for_each_n(arr.result, arr.result_size, [&res](clingo_ast *&ast) {
        auto *cpy = ast;
        ast = nullptr;
        res.emplace_back(construct_statement(cpy));
    });
    return res;
}

Program::Program(Library &lib) {
    clingo_program_t *prg = nullptr;
    handle_error(clingo_program_new(lib, &prg));
    prg_.reset(prg);
}

void Program::free(clingo_program_t *prg) noexcept {
    clingo_program_free(prg);
}

void add(Program &prg, Statement &stm) {
    handle_error(clingo_program_add(prg, c_cast(stm)));
}

template <typename Variant> struct pointer_variant;
template <typename... Ts> struct pointer_variant<std::variant<Ts...>> {
    using type = std::variant<std::add_pointer_t<Ts>...>;
};
template <typename Variant> using pointer_variant_t = typename pointer_variant<Variant>::type;

auto convert_stm(py::handle hnd) -> clingo_ast_t * {
    return c_cast(hnd.cast<pointer_variant_t<Statement>>());
}

auto convert_stm(clingo_ast_t *ast) -> py::object {
    clingo_ast_t *copy = nullptr;
    handle_error(clingo_ast_copy(ast, &copy));
    return py::cast(construct_statement(copy));
}

void register_ast(pybind11::module &m) {
    auto ast = m.def_submodule(
        "ast", R"doc(This module provides functions to work with Abstract Syntax Trees of logic programs.

# Examples

The following example shows how to parse individual statements and add them to a control object.

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>> from clingo import ast
>>>
>>> lib = Library()
>>> ctl = Control(lib, ["--mode=ground"])
>>> ctl.parse_string("a(1).")
>>> prg = ast.Program(lib)
>>> prg.add(ast.parse_statement(lib, "b(X+1) :- a(X)."))
>>> prg.add(ast.parse_statement(lib, "c(X+1) :- b(X)."))
>>> ctl.join(prg)
>>> ctl.ground()
>>> ctl.buffer
"""
a(1).
b(2).
c(3).
#show.
#show a/1.
#show b/1.
#show c/1.
"""
```

The next example hows how to visit and AST and collect variables.

```python
from functools import singledispatch

from clingo.core import Library
from clingo import ast

@singledispatch
def collect(stm, vars):
    """
    Collect all variables occurring in statements.
    """
    return stm.visit(collect, vars)

@collect.register
def _(var: ast.TermVariable | ast.TheoryTermVariable, vars):
    vars.add(var.name)

lib = Library()
vars = set()

ast.parse_string(
    lib,
    """\
    a(X) :- b(X,Y).
    b(Z).
    """,
    lambda stm: collect(stm, vars),
)

print(vars)
```

The last example shows how to use a transformer to modify an AST.

```python
from functools import singledispatch

from clingo.core import Library
from clingo import ast

@singledispatch
def rename(stm, lib):
    """
    Replaces all occurrences of variable `X` with `Y` in the given statement
    returning a new statement. If no replacement was made, `None` is returned.
    """
    return stm.transform(lib, rename, lib)

@rename.register
def _(var: ast.TermVariable, lib):
    return var.update(lib, name="Y") if var.name == "X" else None

lib = Library()
stms = []

ast.parse_string(
    lib,
    """\
    b(1,1).
    b(2,3).
    a(X) :- b(X,Y).
    """,
    stms.append,
)

for stm in stms:
    print(rename(stm, lib) or stm)
```
)doc");

    ast.def("_type_info_yaml", &clingo_ast_type_info_yaml, R"doc(Return a yaml description of the AST.

This can be used to auto-generate most of the binding.)doc");

    py::native_enum<ProjectionMode>(ast, "ProjectionMode", "enum.IntEnum", R"doc(Available projection modes.)doc")
        .value("Disabled", ProjectionMode::Disabled, R"doc(Do not project.)doc")
        .value("Anonymous", ProjectionMode::Anonymous, R"doc(Only project anonymous variables.)doc")
        .value("Pure", ProjectionMode::Pure, R"doc(Project pure variables.)doc")
        .finalize();

    auto py_unary_operator =
        py::native_enum<UnaryOperator>(ast, "UnaryOperator", "enum.IntEnum", R"doc(Available unary operators.)doc");

    auto py_binary_operator =
        py::native_enum<BinaryOperator>(ast, "BinaryOperator", "enum.IntEnum", R"doc(Available binary operators.)doc");

    auto py_sign = py::native_enum<Sign>(ast, "Sign", "enum.IntEnum", R"doc(The available signs.)doc");

    auto py_relation =
        py::native_enum<Relation>(ast, "Relation", "enum.IntEnum", R"doc(Available relation symbols.)doc");

    auto py_aggregate_function = py::native_enum<AggregateFunction>(ast, "AggregateFunction", "enum.IntEnum",
                                                                    R"doc(Enumeration of aggregate functions.)doc");

    auto py_theory_operator_type = py::native_enum<TheoryOperatorType>(ast, "TheoryOperatorType", "enum.IntEnum",
                                                                       R"doc(Enumeration of theory operators.)doc");

    auto py_theory_tuple_type = py::native_enum<TheoryTupleType>(ast, "TheoryTupleType", "enum.IntEnum",
                                                                 R"doc(Enumeration of theory tuple types.)doc");

    auto py_theory_atom_type = py::native_enum<TheoryAtomType>(ast, "TheoryAtomType", "enum.IntEnum",
                                                               R"doc(Enumeration of the theory atom types.)doc");

    auto py_optimize_type = py::native_enum<OptimizeType>(ast, "OptimizeType", "enum.IntEnum",
                                                          R"doc(Enumeration of optimization types.)doc");

    auto py_include_type =
        py::native_enum<IncludeType>(ast, "IncludeType", "enum.IntEnum", R"doc(Enumeration of include types.)doc");

    auto py_precedence =
        py::native_enum<Precedence>(ast, "Precedence", "enum.IntEnum", R"doc(Enumeration of precedences values.)doc");

    auto py_comment_type =
        py::native_enum<CommentType>(ast, "CommentType", "enum.IntEnum", R"doc(Enumeration of comment types.)doc");

    auto py_format_field_literal =
        py::class_<FormatFieldLiteral>(ast, "FormatFieldLiteral", R"doc(A literal part of a format string.)doc");

    auto py_format_field_expression = py::class_<FormatFieldExpression>(
        ast, "FormatFieldExpression", R"doc(An expression part of a format string.)doc");

    auto py_projection =
        py::class_<Projection>(ast, "Projection", R"doc(A placeholder for an argument to project.)doc");

    auto py_term_format_string =
        py::class_<TermFormatString>(ast, "TermFormatString", R"doc(A term representing a format string.)doc");

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

    auto py_left_guard =
        py::class_<LeftGuard>(ast, "LeftGuard", R"doc(A left hand side guard consisting of a term and relation.)doc");

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

    auto py_program_part = py::class_<ProgramPart>(ast, "ProgramPart", R"doc(A program part to ground.)doc");

    auto py_statement_rule = py::class_<StatementRule>(ast, "StatementRule", R"doc(A rule.)doc");

    auto py_statement_theory = py::class_<StatementTheory>(ast, "StatementTheory", R"doc(A theory definition.)doc");

    auto py_statement_optimize =
        py::class_<StatementOptimize>(ast, "StatementOptimize", R"doc(An optimization statement.)doc");

    auto py_statement_weak_constraint =
        py::class_<StatementWeakConstraint>(ast, "StatementWeakConstraint", R"doc(A weak constraint.)doc");

    auto py_statement_show = py::class_<StatementShow>(ast, "StatementShow", R"doc(A show statement.)doc");

    auto py_statement_show_nothing =
        py::class_<StatementShowNothing>(ast, "StatementShowNothing", R"doc(An empty show statement.)doc");

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

    auto py_statement_program = py::class_<StatementProgram>(ast, "StatementProgram", R"doc(A program statement.)doc");

    auto py_statement_parts = py::class_<StatementParts>(ast, "StatementParts", R"doc(A program parts statement.)doc");

    auto py_statement_const = py::class_<StatementConst>(ast, "StatementConst", R"doc(A const statement.)doc");

    auto py_statement_comment = py::class_<StatementComment>(ast, "StatementComment", R"doc(A comment.)doc");

    py_unary_operator.value("Minus", UnaryOperator::Minus, R"doc(Operator `-`.)doc")
        .value("Negation", UnaryOperator::Negation, R"doc(Operator `~`.)doc")
        .finalize();

    py_binary_operator.value("And", BinaryOperator::And, R"doc(Operator `&`.)doc")
        .value("Division", BinaryOperator::Division, R"doc(Operator `/`.)doc")
        .value("Minus", BinaryOperator::Minus, R"doc(Operator `-`.)doc")
        .value("Modulo", BinaryOperator::Modulo, R"doc(Operator `%`.)doc")
        .value("Multiplication", BinaryOperator::Multiplication, R"doc(Operator `*`.)doc")
        .value("Or", BinaryOperator::Or, R"doc(Operator `|`.)doc")
        .value("Plus", BinaryOperator::Plus, R"doc(Operator `+`.)doc")
        .value("Power", BinaryOperator::Power, R"doc(Operator `**`.)doc")
        .value("Xor", BinaryOperator::Xor, R"doc(Operator `^`.)doc")
        .finalize();

    py_sign.value("NoSign", Sign::NoSign, R"doc(No sign.)doc")
        .value("Single", Sign::Single, R"doc(One sign.)doc")
        .value("Double", Sign::Double, R"doc(Two signs.)doc")
        .finalize();

    py_relation.value("Equal", Relation::Equal, R"doc(The equal to relation.)doc")
        .value("NotEqual", Relation::NotEqual, R"doc(The not equal to relation.)doc")
        .value("Less", Relation::Less, R"doc(The less than relation.)doc")
        .value("LessEqual", Relation::LessEqual, R"doc(The less than or equal to relation.)doc")
        .value("Greater", Relation::Greater, R"doc(The greater than relation.)doc")
        .value("GreaterEqual", Relation::GreaterEqual, R"doc(The greater than or equal to relation.)doc")
        .finalize();

    py_aggregate_function.value("Count", AggregateFunction::Count, R"doc(Aggregate function `#count`.)doc")
        .value("Sum", AggregateFunction::Sum, R"doc(Aggregate function `#sum`.)doc")
        .value("Sump", AggregateFunction::Sump, R"doc(Aggregate function `#sum+`)doc")
        .value("Min", AggregateFunction::Min, R"doc(Aggregate function `#min`.)doc")
        .value("Max", AggregateFunction::Max, R"doc(Aggregate function `#max`.)doc")
        .finalize();

    py_theory_operator_type.value("Unary", TheoryOperatorType::Unary, R"doc(An unary theory operator.)doc")
        .value("BinaryLeft", TheoryOperatorType::BinaryLeft, R"doc(A left associative binary operator.)doc")
        .value("BinaryRight", TheoryOperatorType::BinaryRight, R"doc(A right associative binary operator.)doc")
        .finalize();

    py_theory_tuple_type.value("Tuple", TheoryTupleType::Tuple, R"doc(Theory tuples "(t1,...,tn)".)doc")
        .value("Set", TheoryTupleType::Set, R"doc(Theory sets "{t1,...,tn}".)doc")
        .value("List", TheoryTupleType::List, R"doc(Theory lists "[t1,...,tn]".)doc")
        .finalize();

    py_theory_atom_type.value("Head", TheoryAtomType::Head, R"doc(For theory atoms that can appear in the head.)doc")
        .value("Body", TheoryAtomType::Body, R"doc(For theory atoms that can appear in the body.)doc")
        .value("Any", TheoryAtomType::Any, R"doc(For theory atoms that can appear in both head and body.)doc")
        .value("Directive", TheoryAtomType::Directive, R"doc(For theory atoms that must not have a body.)doc")
        .finalize();

    py_optimize_type.value("Minimize", OptimizeType::Minimize, R"doc(For `#minimize` statements.)doc")
        .value("Maximize", OptimizeType::Maximize, R"doc(For `#maximize` statements.)doc")
        .finalize();

    py_include_type.value("System", IncludeType::System, R"doc(For file includes.)doc")
        .value("Inbuild", IncludeType::Inbuild, R"doc(For inbuild includes.)doc")
        .finalize();

    py_precedence.value("Default", Precedence::Default, R"doc(The default precedence.)doc")
        .value("Override", Precedence::Override, R"doc(Override values with default precedence.)doc")
        .finalize();

    py_comment_type.value("Line", CommentType::Line, R"doc(For line comments.)doc")
        .value("Block", CommentType::Block, R"doc(For block comments.)doc")
        .finalize();

    make_comparable_base<FormatField>(py_format_field_literal)
        .def(py::init(&FormatFieldLiteral::construct), py::arg("lib"), py::arg("location"), py::arg("value"),
             R"doc(Construct a FormatFieldLiteral object.

Args:
    lib: The library object for storing symbols.
    location: The location of the literal.
    value: The value of the literal.)doc")
        .def("__str__", &FormatFieldLiteral::to_string)
        .def_property_readonly("location", &FormatFieldLiteral::location, R"doc(The location of the literal.)doc")
        .def_property_readonly("value", &FormatFieldLiteral::value, R"doc(The value of the literal.)doc")
        .def("visit", &FormatFieldLiteral::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &FormatFieldLiteral::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &FormatFieldLiteral::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<FormatField>(py_format_field_expression)
        .def(py::init(&FormatFieldExpression::construct), py::arg("lib"), py::arg("location"), py::arg("left"),
             py::arg("right"), R"doc(Construct a FormatFieldExpression object.

Args:
    lib: The library object for storing symbols.
    location: The location of the expression.
    left: The term of the expression.
    right: The format specifier of the expression.)doc")
        .def("__str__", &FormatFieldExpression::to_string)
        .def_property_readonly("location", &FormatFieldExpression::location, R"doc(The location of the expression.)doc")
        .def_property_readonly("left", &FormatFieldExpression::left, R"doc(The term of the expression.)doc")
        .def_property_readonly("right", &FormatFieldExpression::right,
                               R"doc(The format specifier of the expression.)doc")
        .def("visit", &FormatFieldExpression::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &FormatFieldExpression::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &FormatFieldExpression::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<TermOrProjection>(py_projection)
        .def(py::init(&Projection::construct), py::arg("lib"), py::arg("location"), R"doc(Construct a Projection object.

Args:
    lib: The library object for storing symbols.
    location: The location of the placeholder.)doc")
        .def("__str__", &Projection::to_string)
        .def_property_readonly("location", &Projection::location, R"doc(The location of the placeholder.)doc")
        .def("visit", &Projection::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &Projection::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &Projection::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Term>(py_term_format_string)
        .def(py::init(&TermFormatString::construct), py::arg("lib"), py::arg("location"), py::arg("elements"),
             R"doc(Construct a TermFormatString object.

Args:
    lib: The library object for storing symbols.
    location: The location of the format string.
    elements: The elements of the format string.)doc")
        .def("__str__", &TermFormatString::to_string)
        .def_property_readonly("location", &TermFormatString::location, R"doc(The location of the format string.)doc")
        .def_property_readonly("elements", &TermFormatString::elements, R"doc(The elements of the format string.)doc")
        .def("visit", &TermFormatString::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TermFormatString::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TermFormatString::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Term>(py_term_variable)
        .def(py::init(&TermVariable::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("anonymous") = false, R"doc(Construct a TermVariable object.

Args:
    lib: The library object for storing symbols.
    location: The location of the variable.
    name: The name of the variable.
    anonymous: Whether the variable is anonymous.

        Anonymous variables receive a unique name during
        preprocessing.)doc")
        .def("__str__", &TermVariable::to_string)
        .def_property_readonly("location", &TermVariable::location, R"doc(The location of the variable.)doc")
        .def_property_readonly("name", &TermVariable::name, R"doc(The name of the variable.)doc")
        .def_property_readonly("anonymous", &TermVariable::anonymous, R"doc(Whether the variable is anonymous.
Anonymous variables receive a unique name during preprocessing.)doc")
        .def("visit", &TermVariable::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TermVariable::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TermVariable::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Term>(py_term_symbolic)
        .def(py::init(&TermSymbolic::construct), py::arg("lib"), py::arg("location"), py::arg("symbol"),
             R"doc(Construct a TermSymbolic object.

Args:
    lib: The library object for storing symbols.
    location: The location of the symbol.
    symbol: The symbol.)doc")
        .def("__str__", &TermSymbolic::to_string)
        .def_property_readonly("location", &TermSymbolic::location, R"doc(The location of the symbol.)doc")
        .def_property_readonly("symbol", &TermSymbolic::symbol, R"doc(The symbol.)doc")
        .def("visit", &TermSymbolic::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TermSymbolic::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TermSymbolic::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Term>(py_term_absolute)
        .def(py::init(&TermAbsolute::construct), py::arg("lib"), py::arg("location"), py::arg("pool"),
             R"doc(Construct a TermAbsolute object.

Args:
    lib: The library object for storing symbols.
    location: The location of the operation.
    pool: The argument pool.

        If there is more than one argument in the pool, the term is
        unpooled during preprocessing.)doc")
        .def("__str__", &TermAbsolute::to_string)
        .def_property_readonly("location", &TermAbsolute::location, R"doc(The location of the operation.)doc")
        .def_property_readonly("pool", &TermAbsolute::pool, R"doc(The argument pool.
If there is more than one argument in the pool, the term is unpooled during preprocessing.)doc")
        .def("visit", &TermAbsolute::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TermAbsolute::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TermAbsolute::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Term>(py_term_unary_operation)
        .def(py::init(&TermUnaryOperation::construct), py::arg("lib"), py::arg("location"), py::arg("operator_type"),
             py::arg("right"), R"doc(Construct a TermUnaryOperation object.

Args:
    lib: The library object for storing symbols.
    location: The location of the operation.
    operator_type: The type of the operation.
    right: The argument of the operation.)doc")
        .def("__str__", &TermUnaryOperation::to_string)
        .def_property_readonly("location", &TermUnaryOperation::location, R"doc(The location of the operation.)doc")
        .def_property_readonly("operator_type", &TermUnaryOperation::operator_type,
                               R"doc(The type of the operation.)doc")
        .def_property_readonly("right", &TermUnaryOperation::right, R"doc(The argument of the operation.)doc")
        .def("visit", &TermUnaryOperation::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TermUnaryOperation::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TermUnaryOperation::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Term>(py_term_binary_operation)
        .def(py::init(&TermBinaryOperation::construct), py::arg("lib"), py::arg("location"), py::arg("left"),
             py::arg("operator_type"), py::arg("right"), R"doc(Construct a TermBinaryOperation object.

Args:
    lib: The library object for storing symbols.
    location: The location of the operation.
    left: The left argument of the operation.
    operator_type: The type of the operation.
    right: The right argument of the operation.)doc")
        .def("__str__", &TermBinaryOperation::to_string)
        .def_property_readonly("location", &TermBinaryOperation::location, R"doc(The location of the operation.)doc")
        .def_property_readonly("left", &TermBinaryOperation::left, R"doc(The left argument of the operation.)doc")
        .def_property_readonly("operator_type", &TermBinaryOperation::operator_type,
                               R"doc(The type of the operation.)doc")
        .def_property_readonly("right", &TermBinaryOperation::right, R"doc(The right argument of the operation.)doc")
        .def("visit", &TermBinaryOperation::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TermBinaryOperation::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TermBinaryOperation::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Term>(py_term_tuple)
        .def(py::init(&TermTuple::construct), py::arg("lib"), py::arg("location"), py::arg("pool"),
             R"doc(Construct a TermTuple object.

Args:
    lib: The library object for storing symbols.
    location: The location of the tuple.
    pool: The argument pool of the tuple.

        If there is more than one element in the pool, the term is
        unpooled during preprocessing.)doc")
        .def("__str__", &TermTuple::to_string)
        .def_property_readonly("location", &TermTuple::location, R"doc(The location of the tuple.)doc")
        .def_property_readonly("pool", &TermTuple::pool, R"doc(The argument pool of the tuple.
If there is more than one element in the pool, the term is unpooled during preprocessing.)doc")
        .def("visit", &TermTuple::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TermTuple::transform, py::arg("lib"), py::arg("transformer"), R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TermTuple::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Term>(py_term_function)
        .def(py::init(&TermFunction::construct), py::arg("lib"), py::arg("location"), py::arg("name"), py::arg("pool"),
             py::arg("external") = false, R"doc(Construct a TermFunction object.

Args:
    lib: The library object for storing symbols.
    location: The location of the function.
    name: The name of the function.
    pool: The argument pool of the function.

        If there is more than one element in the pool, the term is
        unpooled during preprocessing.
    external: Whether the function is external.)doc")
        .def("__str__", &TermFunction::to_string)
        .def_property_readonly("location", &TermFunction::location, R"doc(The location of the function.)doc")
        .def_property_readonly("name", &TermFunction::name, R"doc(The name of the function.)doc")
        .def_property_readonly("pool", &TermFunction::pool, R"doc(The argument pool of the function.
If there is more than one element in the pool, the term is unpooled during preprocessing.)doc")
        .def_property_readonly("external", &TermFunction::external, R"doc(Whether the function is external.)doc")
        .def("visit", &TermFunction::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TermFunction::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TermFunction::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<TermOrArgumentTuple>(py_argument_tuple)
        .def(py::init(&ArgumentTuple::construct), py::arg("lib"), py::arg("arguments") = TermOrProjectionArray{},
             R"doc(Construct a ArgumentTuple object.

Args:
    lib: The library object for storing symbols.
    arguments: The arguments of the tuple.)doc")
        .def("__str__", &ArgumentTuple::to_string)
        .def_property_readonly("arguments", &ArgumentTuple::arguments, R"doc(The arguments of the tuple.)doc")
        .def("visit", &ArgumentTuple::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &ArgumentTuple::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &ArgumentTuple::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_left_guard)
        .def(py::init(&LeftGuard::construct), py::arg("lib"), py::arg("term"), py::arg("relation"),
             R"doc(Construct a LeftGuard object.

Args:
    lib: The library object for storing symbols.
    term: The term of the guard.
    relation: The relation of the guard.)doc")
        .def("__str__", &LeftGuard::to_string)
        .def_property_readonly("term", &LeftGuard::term, R"doc(The term of the guard.)doc")
        .def_property_readonly("relation", &LeftGuard::relation, R"doc(The relation of the guard.)doc")
        .def("visit", &LeftGuard::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &LeftGuard::transform, py::arg("lib"), py::arg("transformer"), R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &LeftGuard::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_right_guard)
        .def(py::init(&RightGuard::construct), py::arg("lib"), py::arg("relation"), py::arg("term"),
             R"doc(Construct a RightGuard object.

Args:
    lib: The library object for storing symbols.
    relation: The relation of the guard.
    term: The term of the guard.)doc")
        .def("__str__", &RightGuard::to_string)
        .def_property_readonly("relation", &RightGuard::relation, R"doc(The relation of the guard.)doc")
        .def_property_readonly("term", &RightGuard::term, R"doc(The term of the guard.)doc")
        .def("visit", &RightGuard::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &RightGuard::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &RightGuard::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Literal>(py_literal_boolean)
        .def(py::init(&LiteralBoolean::construct), py::arg("lib"), py::arg("location"), py::arg("sign"),
             py::arg("value"), R"doc(Construct a LiteralBoolean object.

Args:
    lib: The library object for storing symbols.
    location: The location of the symbol.
    sign: The sign of the literal.
    value: The fixed value of the literal.)doc")
        .def("__str__", &LiteralBoolean::to_string)
        .def_property_readonly("location", &LiteralBoolean::location, R"doc(The location of the symbol.)doc")
        .def_property_readonly("sign", &LiteralBoolean::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("value", &LiteralBoolean::value, R"doc(The fixed value of the literal.)doc")
        .def("visit", &LiteralBoolean::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &LiteralBoolean::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &LiteralBoolean::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Literal>(py_literal_comparison)
        .def(py::init(&LiteralComparison::construct), py::arg("lib"), py::arg("location"), py::arg("sign"),
             py::arg("left"), py::arg("right"), R"doc(Construct a LiteralComparison object.

Args:
    lib: The library object for storing symbols.
    location: The location of the symbol.
    sign: The sign of the literal.
    left: The first term of the comparison.
    right: The chain of comparisons.

        Note that the chain must have at least length one.)doc")
        .def("__str__", &LiteralComparison::to_string)
        .def_property_readonly("location", &LiteralComparison::location, R"doc(The location of the symbol.)doc")
        .def_property_readonly("sign", &LiteralComparison::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("left", &LiteralComparison::left, R"doc(The first term of the comparison.)doc")
        .def_property_readonly("right", &LiteralComparison::right, R"doc(The chain of comparisons.
Note that the chain must have at least length one.)doc")
        .def("visit", &LiteralComparison::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &LiteralComparison::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &LiteralComparison::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Literal>(py_literal_symbolic)
        .def(py::init(&LiteralSymbolic::construct), py::arg("lib"), py::arg("location"), py::arg("sign"),
             py::arg("atom"), R"doc(Construct a LiteralSymbolic object.

Args:
    lib: The library object for storing symbols.
    location: The location of the symbol.
    sign: The sign of the literal.
    atom: The term representing the atom.)doc")
        .def("__str__", &LiteralSymbolic::to_string)
        .def_property_readonly("location", &LiteralSymbolic::location, R"doc(The location of the symbol.)doc")
        .def_property_readonly("sign", &LiteralSymbolic::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("atom", &LiteralSymbolic::atom, R"doc(The term representing the atom.)doc")
        .def("visit", &LiteralSymbolic::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &LiteralSymbolic::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &LiteralSymbolic::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_unparsed_element)
        .def(py::init(&UnparsedElement::construct), py::arg("lib"), py::arg("operators"), py::arg("term"),
             R"doc(Construct a UnparsedElement object.

Args:
    lib: The library object for storing symbols.
    operators: The list of theory operators.
    term: The theory term.)doc")
        .def("__str__", &UnparsedElement::to_string)
        .def_property_readonly("operators", &UnparsedElement::operators, R"doc(The list of theory operators.)doc")
        .def_property_readonly("term", &UnparsedElement::term, R"doc(The theory term.)doc")
        .def("visit", &UnparsedElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &UnparsedElement::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &UnparsedElement::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<TheoryTerm>(py_theory_term_variable)
        .def(py::init(&TheoryTermVariable::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("anonymous") = false, R"doc(Construct a TheoryTermVariable object.

Args:
    lib: The library object for storing symbols.
    location: The location of the variable.
    name: The name of the variable.
    anonymous: Whether the variable is anonymous.

        Anonymous variables receive a unique name during
        preprocessing.)doc")
        .def("__str__", &TheoryTermVariable::to_string)
        .def_property_readonly("location", &TheoryTermVariable::location, R"doc(The location of the variable.)doc")
        .def_property_readonly("name", &TheoryTermVariable::name, R"doc(The name of the variable.)doc")
        .def_property_readonly("anonymous", &TheoryTermVariable::anonymous, R"doc(Whether the variable is anonymous.
Anonymous variables receive a unique name during preprocessing.)doc")
        .def("visit", &TheoryTermVariable::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TheoryTermVariable::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TheoryTermVariable::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<TheoryTerm>(py_theory_term_symbolic)
        .def(py::init(&TheoryTermSymbolic::construct), py::arg("lib"), py::arg("location"), py::arg("symbol"),
             R"doc(Construct a TheoryTermSymbolic object.

Args:
    lib: The library object for storing symbols.
    location: The location of the symbol.
    symbol: The symbol.)doc")
        .def("__str__", &TheoryTermSymbolic::to_string)
        .def_property_readonly("location", &TheoryTermSymbolic::location, R"doc(The location of the symbol.)doc")
        .def_property_readonly("symbol", &TheoryTermSymbolic::symbol, R"doc(The symbol.)doc")
        .def("visit", &TheoryTermSymbolic::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TheoryTermSymbolic::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TheoryTermSymbolic::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<TheoryTerm>(py_theory_term_tuple)
        .def(py::init(&TheoryTermTuple::construct), py::arg("lib"), py::arg("location"), py::arg("tuple_type"),
             py::arg("arguments"), R"doc(Construct a TheoryTermTuple object.

Args:
    lib: The library object for storing symbols.
    location: The location of the tuple.
    tuple_type: The type of the tuple.
    arguments: The arguments of the tuple.)doc")
        .def("__str__", &TheoryTermTuple::to_string)
        .def_property_readonly("location", &TheoryTermTuple::location, R"doc(The location of the tuple.)doc")
        .def_property_readonly("tuple_type", &TheoryTermTuple::tuple_type, R"doc(The type of the tuple.)doc")
        .def_property_readonly("arguments", &TheoryTermTuple::arguments, R"doc(The arguments of the tuple.)doc")
        .def("visit", &TheoryTermTuple::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TheoryTermTuple::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TheoryTermTuple::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<TheoryTerm>(py_theory_term_function)
        .def(py::init(&TheoryTermFunction::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arguments"), R"doc(Construct a TheoryTermFunction object.

Args:
    lib: The library object for storing symbols.
    location: The location of the function.
    name: The name of the function.
    arguments: The arguments of the function.)doc")
        .def("__str__", &TheoryTermFunction::to_string)
        .def_property_readonly("location", &TheoryTermFunction::location, R"doc(The location of the function.)doc")
        .def_property_readonly("name", &TheoryTermFunction::name, R"doc(The name of the function.)doc")
        .def_property_readonly("arguments", &TheoryTermFunction::arguments, R"doc(The arguments of the function.)doc")
        .def("visit", &TheoryTermFunction::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TheoryTermFunction::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TheoryTermFunction::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<TheoryTerm>(py_theory_term_unparsed)
        .def(py::init(&TheoryTermUnparsed::construct), py::arg("lib"), py::arg("location"), py::arg("elements"),
             R"doc(Construct a TheoryTermUnparsed object.

Args:
    lib: The library object for storing symbols.
    location: The location of the theory term.
    elements: The unparsed theory elements.)doc")
        .def("__str__", &TheoryTermUnparsed::to_string)
        .def_property_readonly("location", &TheoryTermUnparsed::location, R"doc(The location of the theory term.)doc")
        .def_property_readonly("elements", &TheoryTermUnparsed::elements, R"doc(The unparsed theory elements.)doc")
        .def("visit", &TheoryTermUnparsed::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TheoryTermUnparsed::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TheoryTermUnparsed::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_theory_right_guard)
        .def(py::init(&TheoryRightGuard::construct), py::arg("lib"), py::arg("theory_operator"), py::arg("term"),
             R"doc(Construct a TheoryRightGuard object.

Args:
    lib: The library object for storing symbols.
    theory_operator: The operator of the guard.
    term: The theory term of the guard.)doc")
        .def("__str__", &TheoryRightGuard::to_string)
        .def_property_readonly("theory_operator", &TheoryRightGuard::theory_operator,
                               R"doc(The operator of the guard.)doc")
        .def_property_readonly("term", &TheoryRightGuard::term, R"doc(The theory term of the guard.)doc")
        .def("visit", &TheoryRightGuard::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TheoryRightGuard::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TheoryRightGuard::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_set_aggregate_element)
        .def(py::init(&SetAggregateElement::construct), py::arg("lib"), py::arg("location"), py::arg("literal"),
             py::arg("condition"), R"doc(Construct a SetAggregateElement object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    literal: The literal of the element.
    condition: The condition of the element.)doc")
        .def("__str__", &SetAggregateElement::to_string)
        .def_property_readonly("location", &SetAggregateElement::location, R"doc(The location of the element.)doc")
        .def_property_readonly("literal", &SetAggregateElement::literal, R"doc(The literal of the element.)doc")
        .def_property_readonly("condition", &SetAggregateElement::condition, R"doc(The condition of the element.)doc")
        .def("visit", &SetAggregateElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &SetAggregateElement::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &SetAggregateElement::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_body_aggregate_element)
        .def(py::init(&BodyAggregateElement::construct), py::arg("lib"), py::arg("location"), py::arg("tuple"),
             py::arg("condition"), R"doc(Construct a BodyAggregateElement object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    tuple: The term tuple of the element.
    condition: The condition of the element.)doc")
        .def("__str__", &BodyAggregateElement::to_string)
        .def_property_readonly("location", &BodyAggregateElement::location, R"doc(The location of the element.)doc")
        .def_property_readonly("tuple", &BodyAggregateElement::tuple, R"doc(The term tuple of the element.)doc")
        .def_property_readonly("condition", &BodyAggregateElement::condition, R"doc(The condition of the element.)doc")
        .def("visit", &BodyAggregateElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &BodyAggregateElement::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &BodyAggregateElement::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_theory_atom_element)
        .def(py::init(&TheoryAtomElement::construct), py::arg("lib"), py::arg("location"), py::arg("tuple"),
             py::arg("condition"), R"doc(Construct a TheoryAtomElement object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    tuple: The theory term tuple of the element.
    condition: The condition of the element.)doc")
        .def("__str__", &TheoryAtomElement::to_string)
        .def_property_readonly("location", &TheoryAtomElement::location, R"doc(The location of the element.)doc")
        .def_property_readonly("tuple", &TheoryAtomElement::tuple, R"doc(The theory term tuple of the element.)doc")
        .def_property_readonly("condition", &TheoryAtomElement::condition, R"doc(The condition of the element.)doc")
        .def("visit", &TheoryAtomElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TheoryAtomElement::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TheoryAtomElement::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<BodyLiteral>(py_body_simple_literal)
        .def(py::init(&BodySimpleLiteral::construct), py::arg("lib"), py::arg("literal"),
             R"doc(Construct a BodySimpleLiteral object.

Args:
    lib: The library object for storing symbols.
    literal: The literal.)doc")
        .def("__str__", &BodySimpleLiteral::to_string)
        .def_property_readonly("literal", &BodySimpleLiteral::literal, R"doc(The literal.)doc")
        .def("visit", &BodySimpleLiteral::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &BodySimpleLiteral::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &BodySimpleLiteral::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<BodyLiteral>(py_body_aggregate)
        .def(py::init(&BodyAggregate::construct), py::arg("lib"), py::arg("location"), py::arg("sign"), py::arg("left"),
             py::arg("function"), py::arg("elements"), py::arg("right"), R"doc(Construct a BodyAggregate object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    sign: The sign of the literal.
    left: The left guard of the aggregate.
    function: The aggregate function.
    elements: The aggregate elements.
    right: The right guard of the aggregate.)doc")
        .def("__str__", &BodyAggregate::to_string)
        .def_property_readonly("location", &BodyAggregate::location, R"doc(The location of the element.)doc")
        .def_property_readonly("sign", &BodyAggregate::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("left", &BodyAggregate::left, R"doc(The left guard of the aggregate.)doc")
        .def_property_readonly("function", &BodyAggregate::function, R"doc(The aggregate function.)doc")
        .def_property_readonly("elements", &BodyAggregate::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &BodyAggregate::right, R"doc(The right guard of the aggregate.)doc")
        .def("visit", &BodyAggregate::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &BodyAggregate::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &BodyAggregate::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<BodyLiteral>(py_body_set_aggregate)
        .def(py::init(&BodySetAggregate::construct), py::arg("lib"), py::arg("location"), py::arg("sign"),
             py::arg("left"), py::arg("elements"), py::arg("right"), R"doc(Construct a BodySetAggregate object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    sign: The sign of the literal.
    left: The left guard of the aggregate.
    elements: The aggregate elements.
    right: The right guard of the aggregate.)doc")
        .def("__str__", &BodySetAggregate::to_string)
        .def_property_readonly("location", &BodySetAggregate::location, R"doc(The location of the element.)doc")
        .def_property_readonly("sign", &BodySetAggregate::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("left", &BodySetAggregate::left, R"doc(The left guard of the aggregate.)doc")
        .def_property_readonly("elements", &BodySetAggregate::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &BodySetAggregate::right, R"doc(The right guard of the aggregate.)doc")
        .def("visit", &BodySetAggregate::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &BodySetAggregate::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &BodySetAggregate::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<BodyLiteral>(py_body_theory_atom)
        .def(py::init(&BodyTheoryAtom::construct), py::arg("lib"), py::arg("location"), py::arg("sign"),
             py::arg("name"), py::arg("elements"), py::arg("right"), R"doc(Construct a BodyTheoryAtom object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    sign: The sign of the literal.
    name: The name of the theory atom.
    elements: The aggregate elements.
    right: The right guard of the theory atom.)doc")
        .def("__str__", &BodyTheoryAtom::to_string)
        .def_property_readonly("location", &BodyTheoryAtom::location, R"doc(The location of the element.)doc")
        .def_property_readonly("sign", &BodyTheoryAtom::sign, R"doc(The sign of the literal.)doc")
        .def_property_readonly("name", &BodyTheoryAtom::name, R"doc(The name of the theory atom.)doc")
        .def_property_readonly("elements", &BodyTheoryAtom::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &BodyTheoryAtom::right, R"doc(The right guard of the theory atom.)doc")
        .def("visit", &BodyTheoryAtom::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &BodyTheoryAtom::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &BodyTheoryAtom::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<BodyLiteral>(py_body_conditional_literal)
        .def(py::init(&BodyConditionalLiteral::construct), py::arg("lib"), py::arg("location"), py::arg("literal"),
             py::arg("condition"), R"doc(Construct a BodyConditionalLiteral object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    literal: The literal of the element.
    condition: The condition of the element.)doc")
        .def("__str__", &BodyConditionalLiteral::to_string)
        .def_property_readonly("location", &BodyConditionalLiteral::location, R"doc(The location of the element.)doc")
        .def_property_readonly("literal", &BodyConditionalLiteral::literal, R"doc(The literal of the element.)doc")
        .def_property_readonly("condition", &BodyConditionalLiteral::condition,
                               R"doc(The condition of the element.)doc")
        .def("visit", &BodyConditionalLiteral::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &BodyConditionalLiteral::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &BodyConditionalLiteral::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<DisjunctionElement>(py_head_conditional_literal)
        .def(py::init(&HeadConditionalLiteral::construct), py::arg("lib"), py::arg("location"), py::arg("literal"),
             py::arg("condition"), R"doc(Construct a HeadConditionalLiteral object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    literal: The literal of the element.
    condition: The condition of the element.)doc")
        .def("__str__", &HeadConditionalLiteral::to_string)
        .def_property_readonly("location", &HeadConditionalLiteral::location, R"doc(The location of the element.)doc")
        .def_property_readonly("literal", &HeadConditionalLiteral::literal, R"doc(The literal of the element.)doc")
        .def_property_readonly("condition", &HeadConditionalLiteral::condition,
                               R"doc(The condition of the element.)doc")
        .def("visit", &HeadConditionalLiteral::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &HeadConditionalLiteral::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &HeadConditionalLiteral::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_head_aggregate_element)
        .def(py::init(&HeadAggregateElement::construct), py::arg("lib"), py::arg("location"), py::arg("tuple"),
             py::arg("literal"), py::arg("condition"), R"doc(Construct a HeadAggregateElement object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    tuple: The term tuple of the element.
    literal: The literal of the element.
    condition: The condition of the element.)doc")
        .def("__str__", &HeadAggregateElement::to_string)
        .def_property_readonly("location", &HeadAggregateElement::location, R"doc(The location of the element.)doc")
        .def_property_readonly("tuple", &HeadAggregateElement::tuple, R"doc(The term tuple of the element.)doc")
        .def_property_readonly("literal", &HeadAggregateElement::literal, R"doc(The literal of the element.)doc")
        .def_property_readonly("condition", &HeadAggregateElement::condition, R"doc(The condition of the element.)doc")
        .def("visit", &HeadAggregateElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &HeadAggregateElement::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &HeadAggregateElement::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<HeadLiteral>(py_head_simple_literal)
        .def(py::init(&HeadSimpleLiteral::construct), py::arg("lib"), py::arg("literal"),
             R"doc(Construct a HeadSimpleLiteral object.

Args:
    lib: The library object for storing symbols.
    literal: The literal.)doc")
        .def("__str__", &HeadSimpleLiteral::to_string)
        .def_property_readonly("literal", &HeadSimpleLiteral::literal, R"doc(The literal.)doc")
        .def("visit", &HeadSimpleLiteral::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &HeadSimpleLiteral::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &HeadSimpleLiteral::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<HeadLiteral>(py_head_aggregate)
        .def(py::init(&HeadAggregate::construct), py::arg("lib"), py::arg("location"), py::arg("left"),
             py::arg("function"), py::arg("elements"), py::arg("right"), R"doc(Construct a HeadAggregate object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    left: The left guard of the aggregate.
    function: The aggregate function.
    elements: The aggregate elements.
    right: The right guard of the aggregate.)doc")
        .def("__str__", &HeadAggregate::to_string)
        .def_property_readonly("location", &HeadAggregate::location, R"doc(The location of the element.)doc")
        .def_property_readonly("left", &HeadAggregate::left, R"doc(The left guard of the aggregate.)doc")
        .def_property_readonly("function", &HeadAggregate::function, R"doc(The aggregate function.)doc")
        .def_property_readonly("elements", &HeadAggregate::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &HeadAggregate::right, R"doc(The right guard of the aggregate.)doc")
        .def("visit", &HeadAggregate::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &HeadAggregate::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &HeadAggregate::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<HeadLiteral>(py_head_set_aggregate)
        .def(py::init(&HeadSetAggregate::construct), py::arg("lib"), py::arg("location"), py::arg("left"),
             py::arg("elements"), py::arg("right"), R"doc(Construct a HeadSetAggregate object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    left: The left guard of the aggregate.
    elements: The aggregate elements.
    right: The right guard of the aggregate.)doc")
        .def("__str__", &HeadSetAggregate::to_string)
        .def_property_readonly("location", &HeadSetAggregate::location, R"doc(The location of the element.)doc")
        .def_property_readonly("left", &HeadSetAggregate::left, R"doc(The left guard of the aggregate.)doc")
        .def_property_readonly("elements", &HeadSetAggregate::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &HeadSetAggregate::right, R"doc(The right guard of the aggregate.)doc")
        .def("visit", &HeadSetAggregate::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &HeadSetAggregate::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &HeadSetAggregate::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<HeadLiteral>(py_head_theory_atom)
        .def(py::init(&HeadTheoryAtom::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("elements"), py::arg("right"), R"doc(Construct a HeadTheoryAtom object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    name: The name of the theory atom.
    elements: The aggregate elements.
    right: The right guard of the theory atom.)doc")
        .def("__str__", &HeadTheoryAtom::to_string)
        .def_property_readonly("location", &HeadTheoryAtom::location, R"doc(The location of the element.)doc")
        .def_property_readonly("name", &HeadTheoryAtom::name, R"doc(The name of the theory atom.)doc")
        .def_property_readonly("elements", &HeadTheoryAtom::elements, R"doc(The aggregate elements.)doc")
        .def_property_readonly("right", &HeadTheoryAtom::right, R"doc(The right guard of the theory atom.)doc")
        .def("visit", &HeadTheoryAtom::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &HeadTheoryAtom::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &HeadTheoryAtom::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<HeadLiteral>(py_head_disjunction)
        .def(py::init(&HeadDisjunction::construct), py::arg("lib"), py::arg("location"), py::arg("elements"),
             R"doc(Construct a HeadDisjunction object.

Args:
    lib: The library object for storing symbols.
    location: The location of the element.
    elements: The elements of the disjunction.)doc")
        .def("__str__", &HeadDisjunction::to_string)
        .def_property_readonly("location", &HeadDisjunction::location, R"doc(The location of the element.)doc")
        .def_property_readonly("elements", &HeadDisjunction::elements, R"doc(The elements of the disjunction.)doc")
        .def("visit", &HeadDisjunction::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &HeadDisjunction::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &HeadDisjunction::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_theory_operator_definition)
        .def(py::init(&TheoryOperatorDefinition::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("priority"), py::arg("operator_type"), R"doc(Construct a TheoryOperatorDefinition object.

Args:
    lib: The library object for storing symbols.
    location: The location of the definition.
    name: The name of the definition.
    priority: The priority of the operator.
    operator_type: The type of the operator.)doc")
        .def("__str__", &TheoryOperatorDefinition::to_string)
        .def_property_readonly("location", &TheoryOperatorDefinition::location,
                               R"doc(The location of the definition.)doc")
        .def_property_readonly("name", &TheoryOperatorDefinition::name, R"doc(The name of the definition.)doc")
        .def_property_readonly("priority", &TheoryOperatorDefinition::priority,
                               R"doc(The priority of the operator.)doc")
        .def_property_readonly("operator_type", &TheoryOperatorDefinition::operator_type,
                               R"doc(The type of the operator.)doc")
        .def("visit", &TheoryOperatorDefinition::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TheoryOperatorDefinition::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TheoryOperatorDefinition::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_theory_term_definition)
        .def(py::init(&TheoryTermDefinition::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("operators"), R"doc(Construct a TheoryTermDefinition object.

Args:
    lib: The library object for storing symbols.
    location: The location of the definition.
    name: The name of the definition.
    operators: The operator definitions to construct terms.)doc")
        .def("__str__", &TheoryTermDefinition::to_string)
        .def_property_readonly("location", &TheoryTermDefinition::location, R"doc(The location of the definition.)doc")
        .def_property_readonly("name", &TheoryTermDefinition::name, R"doc(The name of the definition.)doc")
        .def_property_readonly("operators", &TheoryTermDefinition::operators,
                               R"doc(The operator definitions to construct terms.)doc")
        .def("visit", &TheoryTermDefinition::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TheoryTermDefinition::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TheoryTermDefinition::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_theory_guard_definition)
        .def(py::init(&TheoryGuardDefinition::construct), py::arg("lib"), py::arg("operators"), py::arg("term"),
             R"doc(Construct a TheoryGuardDefinition object.

Args:
    lib: The library object for storing symbols.
    operators: A list of operator definition names.
    term: The name of a term definition.)doc")
        .def("__str__", &TheoryGuardDefinition::to_string)
        .def_property_readonly("operators", &TheoryGuardDefinition::operators,
                               R"doc(A list of operator definition names.)doc")
        .def_property_readonly("term", &TheoryGuardDefinition::term, R"doc(The name of a term definition.)doc")
        .def("visit", &TheoryGuardDefinition::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TheoryGuardDefinition::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TheoryGuardDefinition::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_theory_atom_definition)
        .def(py::init(&TheoryAtomDefinition::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arity"), py::arg("term"), py::arg("guard"), py::arg("atom_type"),
             R"doc(Construct a TheoryAtomDefinition object.

Args:
    lib: The library object for storing symbols.
    location: The location of the definition.
    name: The name of the atom.
    arity: The arity of the atom.
    term: The name of a term definition.
    guard: An optional guard definition.
    atom_type: The type of the atom definition.)doc")
        .def("__str__", &TheoryAtomDefinition::to_string)
        .def_property_readonly("location", &TheoryAtomDefinition::location, R"doc(The location of the definition.)doc")
        .def_property_readonly("name", &TheoryAtomDefinition::name, R"doc(The name of the atom.)doc")
        .def_property_readonly("arity", &TheoryAtomDefinition::arity, R"doc(The arity of the atom.)doc")
        .def_property_readonly("term", &TheoryAtomDefinition::term, R"doc(The name of a term definition.)doc")
        .def_property_readonly("guard", &TheoryAtomDefinition::guard, R"doc(An optional guard definition.)doc")
        .def_property_readonly("atom_type", &TheoryAtomDefinition::atom_type,
                               R"doc(The type of the atom definition.)doc")
        .def("visit", &TheoryAtomDefinition::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &TheoryAtomDefinition::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &TheoryAtomDefinition::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_optimize_tuple)
        .def(py::init(&OptimizeTuple::construct), py::arg("lib"), py::arg("weight"), py::arg("priority"),
             py::arg("terms"), R"doc(Construct a OptimizeTuple object.

Args:
    lib: The library object for storing symbols.
    weight: The weight of the tuple.
    priority: An optional priority.
    terms: The remaining terms in the tuple.)doc")
        .def("__str__", &OptimizeTuple::to_string)
        .def_property_readonly("weight", &OptimizeTuple::weight, R"doc(The weight of the tuple.)doc")
        .def_property_readonly("priority", &OptimizeTuple::priority, R"doc(An optional priority.)doc")
        .def_property_readonly("terms", &OptimizeTuple::terms, R"doc(The remaining terms in the tuple.)doc")
        .def("visit", &OptimizeTuple::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &OptimizeTuple::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &OptimizeTuple::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_optimize_element)
        .def(py::init(&OptimizeElement::construct), py::arg("lib"), py::arg("tuple"), py::arg("condition"),
             R"doc(Construct a OptimizeElement object.

Args:
    lib: The library object for storing symbols.
    tuple: The tuple of the element.
    condition: The condition of the element.)doc")
        .def("__str__", &OptimizeElement::to_string)
        .def_property_readonly("tuple", &OptimizeElement::tuple, R"doc(The tuple of the element.)doc")
        .def_property_readonly("condition", &OptimizeElement::condition, R"doc(The condition of the element.)doc")
        .def("visit", &OptimizeElement::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &OptimizeElement::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &OptimizeElement::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_edge)
        .def(py::init(&Edge::construct), py::arg("lib"), py::arg("u"), py::arg("v"), R"doc(Construct a Edge object.

Args:
    lib: The library object for storing symbols.
    u: The start vertex.
    v: The end vertex.)doc")
        .def("__str__", &Edge::to_string)
        .def_property_readonly("u", &Edge::u, R"doc(The start vertex.)doc")
        .def_property_readonly("v", &Edge::v, R"doc(The end vertex.)doc")
        .def("visit", &Edge::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &Edge::transform, py::arg("lib"), py::arg("transformer"), R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &Edge::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable(py_program_part)
        .def(py::init(&ProgramPart::construct), py::arg("lib"), py::arg("name"), py::arg("arguments"),
             R"doc(Construct a ProgramPart object.

Args:
    lib: The library object for storing symbols.
    name: The name of the program part.
    arguments: The arguments of the program part.)doc")
        .def("__str__", &ProgramPart::to_string)
        .def_property_readonly("name", &ProgramPart::name, R"doc(The name of the program part.)doc")
        .def_property_readonly("arguments", &ProgramPart::arguments, R"doc(The arguments of the program part.)doc")
        .def("visit", &ProgramPart::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &ProgramPart::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &ProgramPart::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_rule)
        .def(py::init(&StatementRule::construct), py::arg("lib"), py::arg("location"), py::arg("head"), py::arg("body"),
             R"doc(Construct a StatementRule object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    head: The head literal.
    body: The body of the statement.)doc")
        .def("__str__", &StatementRule::to_string)
        .def_property_readonly("location", &StatementRule::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("head", &StatementRule::head, R"doc(The head literal.)doc")
        .def_property_readonly("body", &StatementRule::body, R"doc(The body of the statement.)doc")
        .def("visit", &StatementRule::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementRule::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementRule::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_theory)
        .def(py::init(&StatementTheory::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("terms"), py::arg("atoms"), R"doc(Construct a StatementTheory object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    name: The name of the theory.
    terms: A list of term definitions.
    atoms: A list of atom definitions.)doc")
        .def("__str__", &StatementTheory::to_string)
        .def_property_readonly("location", &StatementTheory::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementTheory::name, R"doc(The name of the theory.)doc")
        .def_property_readonly("terms", &StatementTheory::terms, R"doc(A list of term definitions.)doc")
        .def_property_readonly("atoms", &StatementTheory::atoms, R"doc(A list of atom definitions.)doc")
        .def("visit", &StatementTheory::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementTheory::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementTheory::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_optimize)
        .def(py::init(&StatementOptimize::construct), py::arg("lib"), py::arg("location"), py::arg("elements"),
             py::arg("optimize_type"), R"doc(Construct a StatementOptimize object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    elements: The elements of the statement.
    optimize_type: The type of the statement.)doc")
        .def("__str__", &StatementOptimize::to_string)
        .def_property_readonly("location", &StatementOptimize::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("elements", &StatementOptimize::elements, R"doc(The elements of the statement.)doc")
        .def_property_readonly("optimize_type", &StatementOptimize::optimize_type,
                               R"doc(The type of the statement.)doc")
        .def("visit", &StatementOptimize::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementOptimize::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementOptimize::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_weak_constraint)
        .def(py::init(&StatementWeakConstraint::construct), py::arg("lib"), py::arg("location"), py::arg("body"),
             py::arg("tuple"), R"doc(Construct a StatementWeakConstraint object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    body: The body of the statement.
    tuple: The tuple of the statement.)doc")
        .def("__str__", &StatementWeakConstraint::to_string)
        .def_property_readonly("location", &StatementWeakConstraint::location,
                               R"doc(The location of the statement.)doc")
        .def_property_readonly("body", &StatementWeakConstraint::body, R"doc(The body of the statement.)doc")
        .def_property_readonly("tuple", &StatementWeakConstraint::tuple, R"doc(The tuple of the statement.)doc")
        .def("visit", &StatementWeakConstraint::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementWeakConstraint::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementWeakConstraint::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_show)
        .def(py::init(&StatementShow::construct), py::arg("lib"), py::arg("location"), py::arg("term"), py::arg("body"),
             R"doc(Construct a StatementShow object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    term: The term to show.
    body: The body of the statement.)doc")
        .def("__str__", &StatementShow::to_string)
        .def_property_readonly("location", &StatementShow::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("term", &StatementShow::term, R"doc(The term to show.)doc")
        .def_property_readonly("body", &StatementShow::body, R"doc(The body of the statement.)doc")
        .def("visit", &StatementShow::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementShow::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementShow::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_show_nothing)
        .def(py::init(&StatementShowNothing::construct), py::arg("lib"), py::arg("location"),
             R"doc(Construct a StatementShowNothing object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.)doc")
        .def("__str__", &StatementShowNothing::to_string)
        .def_property_readonly("location", &StatementShowNothing::location, R"doc(The location of the statement.)doc")
        .def("visit", &StatementShowNothing::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementShowNothing::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementShowNothing::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_show_signature)
        .def(py::init(&StatementShowSignature::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arity"), py::arg("sign") = false, py::arg("value") = true,
             R"doc(Construct a StatementShowSignature object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    name: The name of the predicate to show.
    arity: The arity of the predicate to show.
    sign: The classical sign of the atom.
    value: Whether to show or hide the predicate.)doc")
        .def("__str__", &StatementShowSignature::to_string)
        .def_property_readonly("location", &StatementShowSignature::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementShowSignature::name, R"doc(The name of the predicate to show.)doc")
        .def_property_readonly("arity", &StatementShowSignature::arity, R"doc(The arity of the predicate to show.)doc")
        .def_property_readonly("sign", &StatementShowSignature::sign, R"doc(The classical sign of the atom.)doc")
        .def_property_readonly("value", &StatementShowSignature::value,
                               R"doc(Whether to show or hide the predicate.)doc")
        .def("visit", &StatementShowSignature::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementShowSignature::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementShowSignature::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_project)
        .def(py::init(&StatementProject::construct), py::arg("lib"), py::arg("location"), py::arg("atom"),
             py::arg("body"), R"doc(Construct a StatementProject object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    atom: The atom to project.
    body: The body of the statement.)doc")
        .def("__str__", &StatementProject::to_string)
        .def_property_readonly("location", &StatementProject::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("atom", &StatementProject::atom, R"doc(The atom to project.)doc")
        .def_property_readonly("body", &StatementProject::body, R"doc(The body of the statement.)doc")
        .def("visit", &StatementProject::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementProject::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementProject::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_project_signature)
        .def(py::init(&StatementProjectSignature::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arity"), py::arg("sign") = false, R"doc(Construct a StatementProjectSignature object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    name: The name of the atom to project.
    arity: The arity of the atom to project.
    sign: The classical sign of the atom.)doc")
        .def("__str__", &StatementProjectSignature::to_string)
        .def_property_readonly("location", &StatementProjectSignature::location,
                               R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementProjectSignature::name, R"doc(The name of the atom to project.)doc")
        .def_property_readonly("arity", &StatementProjectSignature::arity, R"doc(The arity of the atom to project.)doc")
        .def_property_readonly("sign", &StatementProjectSignature::sign, R"doc(The classical sign of the atom.)doc")
        .def("visit", &StatementProjectSignature::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementProjectSignature::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementProjectSignature::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_defined)
        .def(py::init(&StatementDefined::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arity"), py::arg("sign") = false, R"doc(Construct a StatementDefined object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    name: The name of the atom to project.
    arity: The arity of the atom to project.
    sign: The classical sign of the atom.)doc")
        .def("__str__", &StatementDefined::to_string)
        .def_property_readonly("location", &StatementDefined::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementDefined::name, R"doc(The name of the atom to project.)doc")
        .def_property_readonly("arity", &StatementDefined::arity, R"doc(The arity of the atom to project.)doc")
        .def_property_readonly("sign", &StatementDefined::sign, R"doc(The classical sign of the atom.)doc")
        .def("visit", &StatementDefined::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementDefined::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementDefined::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_external)
        .def(py::init(&StatementExternal::construct), py::arg("lib"), py::arg("location"), py::arg("atom"),
             py::arg("body"), py::arg("external_type") = std::nullopt, R"doc(Construct a StatementExternal object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    atom: The atom to project.
    body: The body of the statement.
    external_type: The type of the external.)doc")
        .def("__str__", &StatementExternal::to_string)
        .def_property_readonly("location", &StatementExternal::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("atom", &StatementExternal::atom, R"doc(The atom to project.)doc")
        .def_property_readonly("body", &StatementExternal::body, R"doc(The body of the statement.)doc")
        .def_property_readonly("external_type", &StatementExternal::external_type, R"doc(The type of the external.)doc")
        .def("visit", &StatementExternal::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementExternal::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementExternal::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_edge)
        .def(py::init(&StatementEdge::construct), py::arg("lib"), py::arg("location"), py::arg("pool"), py::arg("body"),
             R"doc(Construct a StatementEdge object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    pool: The edge pool of the statement.
    body: The body of the statement.)doc")
        .def("__str__", &StatementEdge::to_string)
        .def_property_readonly("location", &StatementEdge::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("pool", &StatementEdge::pool, R"doc(The edge pool of the statement.)doc")
        .def_property_readonly("body", &StatementEdge::body, R"doc(The body of the statement.)doc")
        .def("visit", &StatementEdge::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementEdge::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementEdge::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_heuristic)
        .def(py::init(&StatementHeuristic::construct), py::arg("lib"), py::arg("location"), py::arg("atom"),
             py::arg("body"), py::arg("weight"), py::arg("modifier"), py::arg("priority") = std::nullopt,
             R"doc(Construct a StatementHeuristic object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    atom: The atom to heuristically modify.
    body: The body of the statement.
    weight: The weight of the heuristic modification.
    modifier: The heuristic modifier.
    priority: An optional priority.)doc")
        .def("__str__", &StatementHeuristic::to_string)
        .def_property_readonly("location", &StatementHeuristic::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("atom", &StatementHeuristic::atom, R"doc(The atom to heuristically modify.)doc")
        .def_property_readonly("body", &StatementHeuristic::body, R"doc(The body of the statement.)doc")
        .def_property_readonly("weight", &StatementHeuristic::weight,
                               R"doc(The weight of the heuristic modification.)doc")
        .def_property_readonly("modifier", &StatementHeuristic::modifier, R"doc(The heuristic modifier.)doc")
        .def_property_readonly("priority", &StatementHeuristic::priority, R"doc(An optional priority.)doc")
        .def("visit", &StatementHeuristic::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementHeuristic::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementHeuristic::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_script)
        .def(py::init(&StatementScript::construct), py::arg("lib"), py::arg("location"), py::arg("value"),
             py::arg("script_type"), R"doc(Construct a StatementScript object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    value: The content of the script.
    script_type: The type of the script.)doc")
        .def("__str__", &StatementScript::to_string)
        .def_property_readonly("location", &StatementScript::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("value", &StatementScript::value, R"doc(The content of the script.)doc")
        .def_property_readonly("script_type", &StatementScript::script_type, R"doc(The type of the script.)doc")
        .def("visit", &StatementScript::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementScript::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementScript::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_include)
        .def(py::init(&StatementInclude::construct), py::arg("lib"), py::arg("location"), py::arg("value"),
             py::arg("include_type"), R"doc(Construct a StatementInclude object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    value: The path of the statement.
    include_type: The type of the include.)doc")
        .def("__str__", &StatementInclude::to_string)
        .def_property_readonly("location", &StatementInclude::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("value", &StatementInclude::value, R"doc(The path of the statement.)doc")
        .def_property_readonly("include_type", &StatementInclude::include_type, R"doc(The type of the include.)doc")
        .def("visit", &StatementInclude::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementInclude::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementInclude::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_program)
        .def(py::init(&StatementProgram::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("arguments"), R"doc(Construct a StatementProgram object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    name: The name of the program.
    arguments: The arguments of the program.)doc")
        .def("__str__", &StatementProgram::to_string)
        .def_property_readonly("location", &StatementProgram::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementProgram::name, R"doc(The name of the program.)doc")
        .def_property_readonly("arguments", &StatementProgram::arguments, R"doc(The arguments of the program.)doc")
        .def("visit", &StatementProgram::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementProgram::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementProgram::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_parts)
        .def(py::init(&StatementParts::construct), py::arg("lib"), py::arg("location"), py::arg("elements"),
             py::arg("precedence"), R"doc(Construct a StatementParts object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    elements: The program parts to ground.
    precedence: The precedence of the statement.)doc")
        .def("__str__", &StatementParts::to_string)
        .def_property_readonly("location", &StatementParts::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("elements", &StatementParts::elements, R"doc(The program parts to ground.)doc")
        .def_property_readonly("precedence", &StatementParts::precedence, R"doc(The precedence of the statement.)doc")
        .def("visit", &StatementParts::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementParts::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementParts::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_const)
        .def(py::init(&StatementConst::construct), py::arg("lib"), py::arg("location"), py::arg("name"),
             py::arg("value"), py::arg("precedence"), R"doc(Construct a StatementConst object.

Args:
    lib: The library object for storing symbols.
    location: The location of the statement.
    name: The name of the statement.
    value: The term of the statement.
    precedence: The precedence of the statement.)doc")
        .def("__str__", &StatementConst::to_string)
        .def_property_readonly("location", &StatementConst::location, R"doc(The location of the statement.)doc")
        .def_property_readonly("name", &StatementConst::name, R"doc(The name of the statement.)doc")
        .def_property_readonly("value", &StatementConst::value, R"doc(The term of the statement.)doc")
        .def_property_readonly("precedence", &StatementConst::precedence, R"doc(The precedence of the statement.)doc")
        .def("visit", &StatementConst::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementConst::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementConst::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    make_comparable_base<Statement>(py_statement_comment)
        .def(py::init(&StatementComment::construct), py::arg("lib"), py::arg("location"), py::arg("value"),
             py::arg("comment_type"), R"doc(Construct a StatementComment object.

Args:
    lib: The library object for storing symbols.
    location: The location of the comment.
    value: The value of the comment.
    comment_type: The type of the comment.)doc")
        .def("__str__", &StatementComment::to_string)
        .def_property_readonly("location", &StatementComment::location, R"doc(The location of the comment.)doc")
        .def_property_readonly("value", &StatementComment::value, R"doc(The value of the comment.)doc")
        .def_property_readonly("comment_type", &StatementComment::comment_type, R"doc(The type of the comment.)doc")
        .def("visit", &StatementComment::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Args:
    visitor: The visitor accepting the sub expressions.
)doc")
        .def("transform", &StatementComment::transform, py::arg("lib"), py::arg("transformer"),
             R"doc(Transform the expression.

Additional arguments are passed to the transformer.

Args:
    lib: The library object for storing symbols.
    transformer: The transformer accepting the sub expressions.
Returns:
    The transformed object or None.
)doc")
        .def("update", &StatementComment::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Args:
    lib: The library object for storing symbols.
Returns:
    The updated object.
)doc");

    ast.def("parse_string", &parse_string, py::arg("lib"), py::arg("program"), py::arg("callback"),
            py::arg("control") = std::nullopt,
            R"doc(Parse the program in the given string.

Args:
    lib: A library object to store symbols.
    program: The program to parse.
    callback: Function to report statements.
    control: Optional Control object to handle ASPIF.
)doc");
    ast.def("parse_files", &parse_files, py::arg("lib"), py::arg("files"), py::arg("callback"),
            py::arg("control") = std::nullopt, R"(
Parse the program in the given files.

The parser follows clingo's handling of files on the command line. Filename
"-" is treated as "STDIN" and if an empty list is given, then the parser will
read from "STDIN".

Args:
    lib: A library object to store symbols.
    files: The files to parse.
    callback: Function to report statements.
    control: Optional Control object to handle ASPIF.
)");
    ast.def("parse_term", &parse_term, py::arg("lib"), py::arg("string"), R"doc(Parse a term.

Args:
    lib: The library object for storing symbols.
    string: The string to parse.

Returns:
    The parsed Term object.)doc");
    ast.def("parse_theory_term", &parse_theory_term, py::arg("lib"), py::arg("string"), R"doc(Parse a theory term.

Args:
    lib: The library object for storing symbols.
    string: The string to parse.

Returns:
    The parsed TheoryTerm object.)doc");
    ast.def("parse_literal", &parse_literal, py::arg("lib"), py::arg("string"), R"doc(Parse a literal.

Args:
    lib: The library object for storing symbols.
    string: The string to parse.

Returns:
    The parsed Literal object.)doc");
    ast.def("parse_head_literal", &parse_head_literal, py::arg("lib"), py::arg("string"), R"doc(Parse a head literal.

Args:
    lib: The library object for storing symbols.
    string: The string to parse.

Returns:
    The parsed HeadLiteral object.)doc");
    ast.def("parse_body_literal", &parse_body_literal, py::arg("lib"), py::arg("string"), R"doc(Parse a body literal.

Args:
    lib: The library object for storing symbols.
    string: The string to parse.

Returns:
    The parsed BodyLiteral object.)doc");
    ast.def("parse_statement", &parse_statement, py::arg("lib"), py::arg("string"), R"doc(Parse a statement.

Args:
    lib: The library object for storing symbols.
    string: The string to parse.

Returns:
    The parsed Statement object.)doc");

    py::class_<RewriteContext>(ast, "RewriteContext", R"doc(Context to rewrite statements.)doc")
        .def(py::init<Library &>(), py::arg("lib"),
             R"doc(Create a context to rewrite statements.

Args:
    lib: A library object to store symbols.
)doc")
        .def("add_param", &RewriteContext::add_param, py::arg("name"), R"doc(
Add a parameter.

Parameters are protected from simplification.

Args:
    name: The name of the parameter.
)doc")
        .def("clear_params", &RewriteContext::clear_params, R"doc(
Remove previously added params
)doc")
        .def("add_theory", &RewriteContext::add_theory, py::arg("theory"), R"doc(
Add a theory definition statement.

The theory definition is used to rewrite theory atoms in statements.

Args:
    theory: The theory statement to add.
)doc")
        .def_property("project_mode", &RewriteContext::get_project_mode, &RewriteContext::set_project_mode,
                      R"doc(The active projection mode.)doc")
        .def_property("project_anonymous", &RewriteContext::get_project_anonymous,
                      &RewriteContext::set_project_anoymous,
                      R"doc(Whether to project anonymous variables in negative literals.)doc");

    ast.def("rewrite_statement", &rewrite_statement, py::arg("ctx"), py::arg("statement"),
            R"doc(Simplify the given statement.

Args:
    ctx: The rewrite context.
    statement: The statement to rewrite.

Returns:
    A list of rewritten statements.)doc");

    py::class_<Program>(ast, "Program", R"doc(A non-ground program.)doc")
        .def(py::init<Library &>(), py::arg("lib"), R"doc(Create an empty non-ground program.

Args:
    lib: A library object to store symbols.
)doc")
        .def("add", &add, py::arg("statement"), R"doc(Add a statement to a program.

Args:
    statement: The statement to add.)doc");
}

} // namespace PyClingo::AST

// NOLINTEND(readability-convert-member-functions-to-static,performance-enum-size)
