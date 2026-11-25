#include "ast.hh"
#include "control.hh" // IWYU pragma: keep
#include "core.hh"
#include "lib.hh"

#include <clingo/input/parser.hh>
#include <clingo/input/print.hh>
#include <clingo/input/rewrite.hh>

#include <clingo/input/rewrite/rewrite_theory.hh>
#include <clingo/input/rewrite/substitute.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/ordered_map.hh>
#include <clingo/util/ordered_set.hh>
#include <clingo/util/type_traits.hh>

#include <algorithm>
#include <array>
#include <cstring>
#include <forward_list>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
namespace {

class ASTVec;

//! The kinds of attributes corresponding to the getters of clingo_ast.
enum class Kind : uint8_t { location, number, symbol, string, string_array, symbol_array, ast, ast_array };

} // namespace
} // namespace CppClingo::CAPI

struct clingo_ast {
  public:
    template <class T>
    clingo_ast(std::shared_ptr<T> owner, clingo_ast_type_e type, void const *ptr)
        : type_{type}, ptr_{std::move(owner), ptr} {}
    [[nodiscard]] auto copy() const -> std::unique_ptr<clingo_ast_t>;
    template <class T> void print(T &out) const;
    [[nodiscard]] auto hash() const -> size_t;
    [[nodiscard]] auto equal(clingo_ast_t const &other) const -> bool;
    [[nodiscard]] auto compare(clingo_ast_t const &other) const -> std::strong_ordering;
    [[nodiscard]] auto get_type() const -> clingo_ast_type_e;
    [[nodiscard]] auto get_number(clingo_ast_attribute_t attr) const -> std::optional<int>;
    [[nodiscard]] auto get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t>;
    [[nodiscard]] auto get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t const *>;
    [[nodiscard]] auto get_string(clingo_ast_attribute_t attr) const -> std::optional<std::string_view>;
    [[nodiscard]] auto get_string_vec(clingo_ast_attribute_t attr) const -> std::optional<CppClingo::StringSpan>;
    [[nodiscard]] auto get_symbol_vec(clingo_ast_attribute_t attr) const -> std::optional<CppClingo::SymbolSpan>;

    [[nodiscard]] auto get_ast(clingo_ast_attribute_t attr) const -> std::optional<std::unique_ptr<clingo_ast_t>>;
    [[nodiscard]] auto get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec>;

    template <typename T> [[nodiscard]] auto cast() const -> T const & { return *static_cast<T const *>(ptr_.get()); }
    template <class V> auto visit(V &&visit) const -> std::invoke_result_t<V, CppClingo::Input::Projection const &>;
    //! Get an attribute of the given kind converted with the given function.
    template <Kind K, class R, class C>
    [[nodiscard]] auto get_attr(clingo_ast_attribute_t attr, C const &conv) const -> std::optional<R>;
    template <class T> friend auto operator<<(T &out, clingo_ast_t const &ast) -> T & {
        ast.print(out);
        return out;
    }

  private:
    clingo_ast_type_e type_;
    std::shared_ptr<void const> ptr_;
};

namespace CppClingo::CAPI {
namespace {

class ASTVec {
  public:
    ASTVec() = default;
    explicit ASTVec(size_t size) {
        if (size > 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            data_ = new clingo_ast_t *[size] { nullptr };
            size_ = size;
        }
    }
    ASTVec(ASTVec const &other) : ASTVec{other.size()} {
        for (size_t i = 0; i < size_; ++i) {
            operator[](i) = other.operator[](i)->copy().release();
        }
    }
    ASTVec(ASTVec &&other) noexcept {
        std::swap(other.data_, data_);
        std::swap(other.size_, size_);
    }
    auto operator=(ASTVec const &other) -> ASTVec & {
        if (this != &other) {
            *this = ASTVec{other};
        }
        return *this;
    }
    auto operator=(ASTVec &&other) noexcept -> ASTVec & {
        if (this != &other) {
            std::swap(other.data_, data_);
            std::swap(other.size_, size_);
        }
        return *this;
    }
    ~ASTVec() {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (auto it = data_, ie = data_ + size_; it != ie; ++it) {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            delete *it;
        }
        delete[] data_;
    }
    [[nodiscard]] auto empty() const -> bool { return size_ == 0; }
    [[nodiscard]] auto size() const -> size_t { return size_; }
    [[nodiscard]] auto begin() const -> clingo_ast_t ** { return data_; }
    [[nodiscard]] auto end() const -> clingo_ast_t ** {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return data_ + size_;
    }
    auto operator[](size_t i) const -> clingo_ast_t *& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,clang-analyzer-core.uninitialized.UndefReturn)
        return data_[i];
    }
    auto release() -> std::pair<clingo_ast_t **, size_t> {
        auto res = std::make_pair(data_, size_);
        data_ = nullptr;
        size_ = 0;
        return res;
    }
    static auto acquire(clingo_ast_t **data, size_t size) -> ASTVec { return ASTVec{data, size}; }
    static auto copy(clingo_ast_t const **data, size_t size) -> ASTVec {
        auto ret = ASTVec{size};
        for (size_t i = 0; i < size; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            ret[i] = data[i]->copy().release();
        }
        return ret;
    }

  private:
    ASTVec(clingo_ast_t **data, size_t size) : data_{data}, size_{size} {}

    clingo_ast_t **data_ = nullptr;
    size_t size_ = 0;
};

// {{{ nodes

template <class T> constexpr bool is_optional_v = false;
template <class T> constexpr bool is_optional_v<std::optional<T>> = true;
template <class T> constexpr bool is_immutable_v = false;
template <class T> constexpr bool is_immutable_v<CppClingo::Util::immutable_value<T>> = true;
template <class T> constexpr bool is_variant_v = false;
template <class... T> constexpr bool is_variant_v<std::variant<T...>> = true;

//! An attribute of a node of the given kind together with its accessor.
template <Kind K, clingo_ast_attribute_e A, class F> struct Field {
    static constexpr Kind kind = K;
    static constexpr clingo_ast_attribute_e attribute = A;
    template <class T> static auto get(T const &x) -> decltype(auto) { return F{}(x); }
};

//! The list of attributes of a node.
template <class... F> struct Fields {};

template <Kind K, clingo_ast_attribute_e A, class F> constexpr auto field(F /*get*/) -> Field<K, A, F> {
    return {};
}

template <class... F> constexpr auto fields(F... /*fields*/) -> Fields<F...> {
    return {};
}

//! The C++ type of the node with the given AST type and its attributes.
//!
//! The attributes are given in the order of the record arguments in ast_info.cc.
template <int Tag> struct NodeOf {};

template <int Tag>
concept has_node = requires { typename NodeOf<Tag>::type; };

template <int Tag> using node_t = NodeOf<Tag>::type;

#define FIELD(kind, attr, ...)                                                                                         \
    field<Kind::kind, clingo_ast_attribute_##attr>([](auto const &x) -> decltype(auto) { return (__VA_ARGS__); })
#define LOC(attr, ...) FIELD(location, attr, __VA_ARGS__)
#define NUM(attr, ...) FIELD(number, attr, __VA_ARGS__)
#define SYM(attr, ...) FIELD(symbol, attr, __VA_ARGS__)
#define STR(attr, ...) FIELD(string, attr, __VA_ARGS__)
#define STRS(attr, ...) FIELD(string_array, attr, __VA_ARGS__)
#define SYMS(attr, ...) FIELD(symbol_array, attr, __VA_ARGS__)
#define AST(attr, ...) FIELD(ast, attr, __VA_ARGS__)
#define VEC(attr, ...) FIELD(ast_array, attr, __VA_ARGS__)
#define NODE(tag, Type, ...)                                                                                           \
    template <> struct NodeOf<clingo_ast_type_##tag> {                                                                 \
        using type = CppClingo::Input::Type;                                                                           \
        static constexpr auto fields = CppClingo::CAPI::fields(__VA_ARGS__);                                           \
    };

// clang-format off
NODE(format_field_literal, FormatFieldLiteral, LOC(location, x.loc()), STR(value, x.value()))
NODE(format_field_expression, FormatFieldExpression,
     LOC(location, x.loc()),
     AST(left, x.lhs()),
     STR(right, x.rhs_str()))
NODE(projection, Projection, LOC(location, x.loc()))
NODE(term_variable, TermVariable, LOC(location, x.loc()), STR(name, x.name()), NUM(anonymous, x.anonymous()))
NODE(term_symbolic, TermSymbol, LOC(location, x.loc()), SYM(symbol, x.value()))
NODE(term_format_string, TermFormatString, LOC(location, x.loc()), VEC(elements, x.elems()))
NODE(term_absolute, TermAbs, LOC(location, x.loc()), VEC(pool, x.pool()))
NODE(term_unary_operation, TermUnary, LOC(location, x.loc()), NUM(operator_type, x.op()), AST(right, x.rhs()))
NODE(term_binary_operation, TermBinary,
     LOC(location, x.loc()),
     AST(left, x.lhs()),
     NUM(operator_type, x.op()),
     AST(right, x.rhs()))
NODE(term_tuple, TermTuple, LOC(location, x.loc()), VEC(pool, x.pool()))
NODE(term_function, TermFunction,
     LOC(location, x.loc()),
     STR(name, x.name()),
     VEC(pool, x.pool()),
     NUM(external, x.external()))
NODE(argument_tuple, ArgumentTuple, VEC(arguments, x.elems()))
NODE(unparsed_element, UnparsedElement, STRS(operators, x.ops()), AST(term, x.term()))
NODE(theory_term_variable, TheoryTermVariable,
     LOC(location, x.loc()),
     STR(name, x.name()),
     NUM(anonymous, x.anonymous()))
NODE(theory_term_symbolic, TheoryTermSymbol, LOC(location, x.loc()), SYM(symbol, x.value()))
NODE(theory_term_tuple, TheoryTermTuple, LOC(location, x.loc()), NUM(tuple_type, x.type()), VEC(arguments, x.elems()))
NODE(theory_term_function, TheoryTermFunction, LOC(location, x.loc()), STR(name, x.name()), VEC(arguments, x.args()))
NODE(theory_term_unparsed, TheoryTermUnparsed, LOC(location, x.loc()), VEC(elements, x.elems()))
NODE(left_guard, LGuard::value_type, AST(term, x.first), NUM(relation, x.second))
NODE(right_guard, RGuard::value_type, NUM(relation, x.first), AST(term, x.second))
NODE(literal_boolean, LitBool, LOC(location, x.loc()), NUM(sign, x.sign()), NUM(value, x.value()))
NODE(literal_comparison, LitComparison,
     LOC(location, x.loc()),
     NUM(sign, x.sign()),
     AST(left, x.lhs()),
     VEC(right, x.rhs()))
NODE(literal_symbolic, LitSymbolic, LOC(location, x.loc()), NUM(sign, x.sign()), AST(atom, x.term()))
NODE(set_aggregate_element, SetAggregateElement,
     LOC(location, x.loc()),
     AST(literal, x.lit()),
     VEC(condition, x.cond()))
NODE(theory_atom_element, TheoryElement, LOC(location, x.loc()), VEC(tuple, x.tuple()), VEC(condition, x.cond()))
NODE(theory_right_guard, TheoryRGuard, STR(theory_operator, x.op()), AST(term, x.term()))
NODE(body_simple_literal, BdLitSimple, AST(literal, x.lit()))
NODE(body_aggregate_element, BdLitAggregateElement,
     LOC(location, x.loc()),
     VEC(tuple, x.tuple()),
     VEC(condition, x.cond()))
NODE(body_aggregate, BdLitAggregate,
     LOC(location, x.loc()),
     NUM(sign, x.sign()),
     AST(left, x.lhs()),
     NUM(function, x.fun()),
     VEC(elements, x.elems()),
     AST(right, x.rhs()))
NODE(body_set_aggregate, BdLitSetAggregate,
     LOC(location, x.loc()),
     NUM(sign, x.sign()),
     AST(left, x.lhs()),
     VEC(elements, x.elems()),
     AST(right, x.rhs()))
NODE(body_theory_atom, BdLitTheoryAtom,
     LOC(location, x.loc()),
     NUM(sign, x.sign()),
     AST(name, x.name()),
     VEC(elements, x.elems()),
     AST(right, x.rhs()))
NODE(body_conditional_literal, BdLitConjunction,
     LOC(location, x.loc()),
     AST(literal, x.lit().lit()),
     VEC(condition, x.lit().cond()))
NODE(body_sort, BdLitSort, LOC(location, x.loc()), AST(left, x.lhs()), VEC(elements, x.elems()))
NODE(head_simple_literal, HdLitSimple, AST(literal, x.lit()))
NODE(head_aggregate_element, HdLitAggregateElement,
     LOC(location, x.loc()),
     VEC(tuple, x.tuple()),
     AST(literal, x.lit()),
     VEC(condition, x.cond()))
NODE(head_aggregate, HdLitAggregate,
     LOC(location, x.loc()),
     AST(left, x.lhs()),
     NUM(function, x.fun()),
     VEC(elements, x.elems()),
     AST(right, x.rhs()))
NODE(head_set_aggregate, HdLitSetAggregate,
     LOC(location, x.loc()),
     AST(left, x.lhs()),
     VEC(elements, x.elems()),
     AST(right, x.rhs()))
NODE(head_theory_atom, HdLitTheoryAtom,
     LOC(location, x.loc()),
     AST(name, x.name()),
     VEC(elements, x.elems()),
     AST(right, x.rhs()))
NODE(head_conditional_literal, CondLit, LOC(location, x.loc()), AST(literal, x.lit()), VEC(condition, x.cond()))
NODE(head_disjunction, HdLitDisjunction, LOC(location, x.loc()), VEC(elements, x.elems()))
NODE(theory_operator_definition, TheoryOpDefinition,
     LOC(location, x.loc()),
     STR(name, x.op()),
     NUM(priority, x.prio()),
     NUM(operator_type, x.type()))
NODE(theory_term_definition, TheoryTermDefinition,
     LOC(location, x.loc()),
     STR(name, x.name()),
     VEC(operators, x.op_defs()))
NODE(theory_guard_definition, TheoryRGuardDefinition, STRS(operators, x.ops()), STR(term, x.term()))
NODE(theory_atom_definition, TheoryAtomDefinition,
     LOC(location, x.loc()),
     STR(name, x.name()),
     NUM(arity, x.arity()),
     STR(term, x.term()),
     AST(guard, x.rhs()),
     NUM(atom_type, x.type()))
NODE(optimize_tuple, OptimizeTuple, AST(weight, x.weight()), AST(priority, x.prio()), VEC(terms, x.terms()))
NODE(optimize_element, OptimizeElement, AST(tuple, x.tuple()), VEC(condition, x.cond()))
NODE(edge, Edge, AST(u, x.src()), AST(v, x.dst()))
NODE(program_part, ProgramParam, STR(name, x.first.get()), SYMS(arguments, x.second))
NODE(statement_rule, StmRule, LOC(location, x.loc()), AST(head, x.head()), VEC(body, x.body()))
NODE(statement_theory, StmTheory,
     LOC(location, x.loc()),
     STR(name, x.name()),
     VEC(terms, x.term_defs()),
     VEC(atoms, x.atom_defs()))
NODE(statement_optimize, StmOptimize, LOC(location, x.loc()), VEC(elements, x.elems()), NUM(optimize_type, x.type()))
NODE(statement_weak_constraint, StmWeakConstraint, LOC(location, x.loc()), VEC(body, x.body()), AST(tuple, x.tuple()))
NODE(statement_show, StmShow, LOC(location, x.loc()), AST(term, x.term()), VEC(body, x.body()))
NODE(statement_show_nothing, StmShowNothing, LOC(location, x.loc()))
NODE(statement_show_signature, StmShowSig,
     LOC(location, x.loc()),
     STR(name, x.name()),
     NUM(arity, x.arity()),
     NUM(sign, x.sign()),
     NUM(value, x.value()))
NODE(statement_project, StmProject, LOC(location, x.loc()), AST(atom, x.atom()), VEC(body, x.body()))
NODE(statement_project_signature, StmProjectSig,
     LOC(location, x.loc()),
     STR(name, x.name()),
     NUM(arity, x.arity()),
     NUM(sign, x.sign()))
NODE(statement_defined, StmDefined,
     LOC(location, x.loc()),
     STR(name, x.name()),
     NUM(arity, x.arity()),
     NUM(sign, x.sign()))
NODE(statement_external, StmExternal,
     LOC(location, x.loc()),
     AST(atom, x.atom()),
     VEC(body, x.body()),
     AST(external_type, x.type()))
NODE(statement_edge, StmEdge, LOC(location, x.loc()), VEC(pool, x.edges()), VEC(body, x.body()))
NODE(statement_heuristic, StmHeuristic,
     LOC(location, x.loc()),
     AST(atom, x.atom()),
     VEC(body, x.body()),
     AST(weight, x.weight()),
     AST(modifier, x.type()),
     AST(priority, x.prio()))
NODE(statement_script, StmScript, LOC(location, x.loc()), STR(value, x.value()), STR(script_type, x.type()))
NODE(statement_program, StmProgram, LOC(location, x.loc()), STR(name, x.name()), STRS(arguments, x.args()))
NODE(statement_include, StmInclude, LOC(location, x.loc()), STR(value, x.value()), NUM(include_type, x.type()))
NODE(statement_const, StmConst,
     LOC(location, x.loc()),
     STR(name, x.name()),
     AST(value, x.value()),
     NUM(precedence, x.type()))
NODE(statement_parts, StmParts, LOC(location, x.loc()), VEC(elements, x.elems()), NUM(precedence, x.type()))
NODE(statement_comment, StmComment, LOC(location, x.loc()), STR(value, x.value()), NUM(comment_type, x.type()))
// clang-format on

#undef NODE
#undef VEC
#undef AST
#undef SYMS
#undef STRS
#undef STR
#undef SYM
#undef NUM
#undef LOC
#undef FIELD

//! The number of node types.
//!
//! NOTE: the AST types must be contiguous and less than 128.
constexpr int node_count = []<int... I>(std::integer_sequence<int, I...>) {
    return (0 + ... + (has_node<I> ? 1 : 0));
}(std::make_integer_sequence<int, 128>{});

//! The AST type of the given C++ type or -1 if it is not a node.
template <class T>
constexpr int tag_of = []<int... I>(std::integer_sequence<int, I...>) {
    int tag = -1;
    ((std::is_same_v<node_t<I>, T> ? (void)(tag = I) : (void)0), ...);
    return tag;
}(std::make_integer_sequence<int, node_count>{});

template <class T>
concept is_node = tag_of<T> >= 0;

constexpr auto max_node_count = 128;

static_assert([]<int... I>(std::integer_sequence<int, I...>) { return ((has_node<I> == (I < node_count)) && ...); }(
                  std::make_integer_sequence<int, max_node_count>{}),
              "AST types must be contiguous");
static_assert([]<int... I>(std::integer_sequence<int, I...>) { return ((tag_of<node_t<I>> == I) && ...); }(
                  std::make_integer_sequence<int, node_count>{}),
              "each AST type needs its own C++ type");

//! Call the given function with the AST type as an integral constant.
//!
//! If the function does not accept the AST type, the fallback is called.
//!
//! NOTE: the switch allows compilers to generate a jump table and inline the cases.
#define CASE(tag)                                                                                                      \
    case (tag): {                                                                                                      \
        if constexpr (requires { fun(std::integral_constant<int, (tag)>{}); }) {                                       \
            return fun(std::integral_constant<int, (tag)>{});                                                          \
        }                                                                                                              \
        break;                                                                                                         \
    }
#define CASES8(tag)                                                                                                    \
    CASE(tag)                                                                                                          \
    CASE((tag) + 1) CASE((tag) + 2) CASE((tag) + 3) CASE((tag) + 4) CASE((tag) + 5) CASE((tag) + 6) CASE((tag) + 7)
#define CASES64(tag)                                                                                                   \
    CASES8(tag)                                                                                                        \
    CASES8((tag) + 8)                                                                                                  \
    CASES8((tag) + 16) CASES8((tag) + 24) CASES8((tag) + 32) CASES8((tag) + 40) CASES8((tag) + 48) CASES8((tag) + 56)

template <class F, class G> auto dispatch(int tag, F const &fun, G const &fallback) -> decltype(fallback()) {
    switch (tag) {
        CASES64(0)
        CASES64(64)
        default: {
            break;
        }
    }
    return fallback();
}

#undef CASES64
#undef CASES8
#undef CASE

//! Whether the node with the given AST type has attributes of the given kind.
template <Kind K, int I> constexpr auto has_kind() -> bool {
    return []<class... F>(Fields<F...>) { return ((F::kind == K) || ...); }(NodeOf<I>::fields);
}

// }}} nodes
// {{{ make_ast

template <class U, class T> auto make_ast(std::shared_ptr<U> owner, T const &x) -> std::unique_ptr<clingo_ast_t> {
    if constexpr (is_node<T>) {
        return std::make_unique<clingo_ast_t>(std::move(owner), static_cast<clingo_ast_type_e>(tag_of<T>), &x);
    } else if constexpr (is_variant_v<T>) {
        return std::visit([&owner](auto const &y) { return make_ast(std::move(owner), y); }, x);
    } else if constexpr (is_immutable_v<T>) {
        return make_ast(std::move(owner), *x);
    } else {
        static_assert(is_optional_v<T>, "unexpected AST type");
        return x ? make_ast(std::move(owner), *x) : nullptr;
    }
}

template <class U> auto make_ast(U &&value) -> std::unique_ptr<clingo_ast_t> {
    auto owner = std::make_shared<std::remove_cvref_t<U>>(std::forward<U>(value));
    auto *ptr = owner.get();
    return make_ast(std::move(owner), *ptr);
}

template <class U, class R> auto make_ast_vec(std::shared_ptr<U> const &owner, R const &vec) -> ASTVec {
    ASTVec res{std::size(vec)};
    size_t i = 0;
    for (auto const &elem : vec) {
        res[i++] = make_ast(owner, elem).release();
    }
    return res;
}

// }}} make_ast
// {{{ convert

//! The names of the unions in ast_info.cc corresponding to C++ variants.
template <class T> constexpr char const *union_name = nullptr;
template <> constexpr char const *union_name<CppClingo::Input::Term> = "term";
template <> constexpr char const *union_name<CppClingo::Input::FormatField> = "format_field";
template <> constexpr char const *union_name<CppClingo::Input::Argument> = "term_or_projection";
template <> constexpr char const *union_name<CppClingo::Input::TupleElement> = "term_or_argument_tuple";
template <> constexpr char const *union_name<CppClingo::Input::Lit> = "literal";
template <> constexpr char const *union_name<CppClingo::Input::TheoryTerm> = "theory_term";
template <> constexpr char const *union_name<CppClingo::Input::BdLit> = "body_literal";
template <> constexpr char const *union_name<CppClingo::Input::HdLitDisjunctionElement> = "disjunction_element";
template <> constexpr char const *union_name<CppClingo::Input::HdLit> = "head_literal";
template <> constexpr char const *union_name<CppClingo::Input::Stm> = "statement";

//! Throw an exception that a node of the given type was expected.
[[noreturn]] void throw_expected(char const *type) {
    std::string name = type;
    std::ranges::replace(name, '_', ' ');
    throw std::invalid_argument{name + " expected"};
}

//! Check whether an AST of the given type can be converted to the given C++ type.
template <class T> constexpr auto accepts(int tag) -> bool {
    if constexpr (is_node<T>) {
        return tag == tag_of<T>;
    } else {
        return []<class... A>(std::type_identity<std::variant<A...>>, int tag) {
            return (accepts<A>(tag) || ...);
        }(std::type_identity<T>{}, tag);
    }
}

//! Convert an AST accepted by the given C++ type.
template <class T, class A, class... B> auto convert_accepted(clingo_ast_t const *ast) -> T;

template <class T> auto convert_accepted(clingo_ast_t const *ast) -> T {
    if constexpr (is_node<T>) {
        return ast->cast<T>();
    } else {
        return []<class... A>(clingo_ast_t const *ast, std::type_identity<std::variant<A...>>) {
            return convert_accepted<T, A...>(ast);
        }(ast, std::type_identity<T>{});
    }
}

template <class T, class A, class... B> auto convert_accepted(clingo_ast_t const *ast) -> T {
    if constexpr (sizeof...(B) > 0) {
        if (!accepts<A>(ast->get_type())) {
            return convert_accepted<T, B...>(ast);
        }
    }
    return T{std::in_place_type<A>, convert_accepted<A>(ast)};
}

template <class T> auto convert(clingo_ast_t const *ast) -> T {
    if constexpr (is_optional_v<T>) {
        return ast != nullptr ? T{convert<typename T::value_type>(ast)} : T{};
    } else if constexpr (is_immutable_v<T>) {
        return CppClingo::Util::make_immutable<typename T::element_type>(convert<typename T::element_type>(ast));
    } else {
        if (!accepts<T>(ast->get_type())) {
            if constexpr (is_node<T>) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                throw_expected(clingo_ast_info()->records[tag_of<T>].name);
            } else {
                static_assert(union_name<T> != nullptr, "unexpected AST type");
                throw_expected(union_name<T>);
            }
        }
        return convert_accepted<T>(ast);
    }
}
template <class T> auto convert(clingo_ast_t const *const *ast, size_t size) -> std::vector<T> {
    std::vector<T> res;
    res.reserve(size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto it = ast, ie = ast + size; it != ie; ++it) {
        res.emplace_back(convert<T>(*it));
    }
    return res;
}

auto convert(clingo_location_t const *loc) -> CppClingo::Location const & {
    return *cpp_cast(loc);
}

auto convert(clingo_symbol_t const *symbols, size_t size) -> CppClingo::SharedSymbolVec {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return CppClingo::Util::to_vec(symbols, symbols + size,
                                   [](auto sym) { return CppClingo::SharedSymbol{CppClingo::Symbol::from_rep(sym)}; });
}

auto convert(clingo_lib_t *lib, clingo_string_t const *array, size_t size) -> CppClingo::SharedStringArray {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return {array, array + size, [lib](auto str) { return lib->store->string({str.data, str.size}); }};
}

// }}} convert

template <class T, class... A> auto construct_ast(clingo_ast_type_t type, A &&...args) -> clingo_ast_t * {
    auto owner = std::make_shared<T>(std::forward<A>(args)...);
    auto *ptr = owner.get();
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return new clingo_ast_t{std::move(owner), static_cast<clingo_ast_type_e>(type), ptr};
}

template <Kind K, class F, class R, class T, class C>
auto get_field(T const &x, clingo_ast_attribute_t attr, C const &conv, std::optional<R> &res) -> bool {
    if constexpr (F::kind == K) {
        using Value = decltype(F::get(x));
        static_assert(K == Kind::number || K == Kind::symbol || std::is_lvalue_reference_v<Value> ||
                          std::ranges::borrowed_range<Value>,
                      "attributes have to be references or views into the AST");
        if (attr == F::attribute) {
            res = conv(F::get(x));
            return true;
        }
    }
    return false;
}

} // namespace
} // namespace CppClingo::CAPI

template <class V>
auto clingo_ast::visit(V &&visit) const -> std::invoke_result_t<V, CppClingo::Input::Projection const &> {
    using R = std::invoke_result_t<V, CppClingo::Input::Projection const &>;
    return dispatch(
        type_, [&]<int I>(std::integral_constant<int, I>)
            requires has_node<I>
        { return std::forward<V>(visit)(cast<node_t<I>>()); },
        []() -> R { throw std::invalid_argument("invalid ast type"); });
}

template <Kind K, class R, class C>
auto clingo_ast::get_attr(clingo_ast_attribute_t attr, C const &conv) const -> std::optional<R> {
    return dispatch(
        type_,
        [&]<int I>(std::integral_constant<int, I>)
            requires(has_node<I> && has_kind<K, I>())
        {
            return [&]<class... F>(Fields<F...>) {
                std::optional<R> res;
                (get_field<K, F>(cast<node_t<I>>(), attr, conv, res) || ...);
                return res;
            }(NodeOf<I>::fields);
        },
        []() -> std::optional<R> { return std::nullopt; });
}

auto clingo_ast::get_type() const -> clingo_ast_type_e {
    return type_;
}

auto clingo_ast::get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t const *> {
    using namespace CppClingo::Input;
    if (attr != clingo_ast_attribute_location) {
        return std::nullopt;
    }
    return visit([]<class T>(T const &x) -> std::optional<clingo_location_t const *> {
        if constexpr (requires(T const &x) { x.loc(); }) {
            return c_cast(&x.loc());
        }
        return std::nullopt;
    });
}
auto clingo_ast::get_number(clingo_ast_attribute_t attr) const -> std::optional<int> {
    return get_attr<Kind::number, int>(attr, [](auto const &x) { return static_cast<int>(x); });
}

auto clingo_ast::get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t> {
    return get_attr<Kind::symbol, clingo_symbol_t>(
        attr, [](auto const &x) { return static_cast<clingo_symbol_t>(CppClingo::Symbol::to_rep(x)); });
}

auto clingo_ast::get_string(clingo_ast_attribute_t attr) const -> std::optional<std::string_view> {
    return get_attr<Kind::string, std::string_view>(attr, [](auto const &x) -> std::string_view {
        if constexpr (requires { x.view(); }) {
            return x.view();
        } else {
            return x;
        }
    });
}

auto clingo_ast::get_string_vec(clingo_ast_attribute_t attr) const -> std::optional<CppClingo::StringSpan> {
    return get_attr<Kind::string_array, CppClingo::StringSpan>(
        attr, [](auto const &x) -> CppClingo::StringSpan { return x; });
}

auto clingo_ast::get_symbol_vec(clingo_ast_attribute_t attr) const -> std::optional<CppClingo::SymbolSpan> {
    return get_attr<Kind::symbol_array, CppClingo::SymbolSpan>(attr, [](auto const &x) { return as_symbol_span(x); });
}

auto clingo_ast::get_ast(clingo_ast_attribute_t attr) const -> std::optional<std::unique_ptr<clingo_ast_t>> {
    return get_attr<Kind::ast, std::unique_ptr<clingo_ast_t>>(attr,
                                                              [this](auto const &x) { return make_ast(ptr_, x); });
}

auto clingo_ast::get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec> {
    return get_attr<Kind::ast_array, ASTVec>(attr, [this](auto const &x) { return make_ast_vec(ptr_, x); });
}

auto clingo_ast::copy() const -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(*this);
}

template <class T> void clingo_ast::print(T &out) const {
    using namespace CppClingo::Input;
    visit([&out]<class U>(U const &x) {
        if constexpr (CppClingo::Util::matches<U, RGuard::value_type>) {
            out << " " << x.first << " " << x.second;
        } else if constexpr (CppClingo::Util::matches<U, TheoryRGuard>) {
            out << " " << x.op() << " " << x.term();
        } else if constexpr (std::is_same_v<U, UnparsedElement>) {
            for (auto const &op : x.ops()) {
                out << op << " ";
            }
            out << x.term();
        } else if constexpr (std::is_same_v<U, ArgumentTuple>) {
            bool comma = false;
            for (auto const &elem : x.elems()) {
                if (comma) {
                    out << ",";
                } else {
                    comma = true;
                }
                std::visit([&out](auto &x) { out << x; }, elem);
            }
        } else if constexpr (std::is_same_v<U, LGuard::value_type>) {
            out << x.first << " " << x.second << " ";
        } else if constexpr (std::is_same_v<U, ProgramParam>) {
            out << *x.first;
            if (!x.second.empty()) {
                out << "(" << *x.second.front();
                for (auto it = x.second.begin() + 1, ie = x.second.end(); it != ie; ++it) {
                    out << "," << **it;
                }
                out << ")";
            }
        } else {
            out << x;
        }
    });
}

auto clingo_ast::hash() const -> size_t {
    using namespace CppClingo::Input;
    return visit([this](auto &x) {
        return CppClingo::Util::hash_mix(CppClingo::Util::value_hash_record<clingo_ast>(type_, x));
    });
}

auto clingo_ast::equal(clingo_ast_t const &other) const -> bool {
    return type_ == other.type_ &&
           visit([&other](auto const &x) { return x == other.cast<std::decay_t<decltype(x)>>(); });
}

auto clingo_ast::compare(clingo_ast_t const &other) const -> std::strong_ordering {
    if (type_ != other.type_) {
        return type_ <=> other.type_;
    }
    return visit([&other](auto const &x) { return x <=> other.cast<std::decay_t<decltype(x)>>(); });
}

namespace CppClingo::CAPI {
namespace {

//! The order of the constructor arguments if it differs from the order of the record arguments.
template <int Tag> struct CtorOrder {
    static constexpr std::array<clingo_ast_attribute_e, 0> value{};
};

#define A(attr) clingo_ast_attribute_##attr
#define ORDER(tag, ...)                                                                                                \
    template <> struct CtorOrder<clingo_ast_type_##tag> {                                                              \
        static constexpr std::array value{__VA_ARGS__};                                                                \
    };

// clang-format off
ORDER(statement_optimize, A(location), A(optimize_type), A(elements))
ORDER(statement_show_signature, A(location), A(sign), A(name), A(arity), A(value))
ORDER(statement_project_signature, A(location), A(sign), A(name), A(arity))
ORDER(statement_defined, A(location), A(sign), A(name), A(arity))
ORDER(statement_heuristic, A(location), A(atom), A(body), A(weight), A(priority), A(modifier))
ORDER(statement_include, A(location), A(include_type), A(value))
ORDER(statement_script, A(location), A(script_type), A(value))
ORDER(statement_const, A(location), A(precedence), A(name), A(value))
ORDER(statement_parts, A(location), A(precedence), A(elements))
ORDER(statement_comment, A(location), A(comment_type), A(value))
// clang-format on

#undef ORDER
#undef A

//! The index of the field with the given attribute of the node with the given AST type.
template <int I> constexpr auto field_index(clingo_ast_attribute_e attr) -> size_t {
    return [attr]<class... F>(Fields<F...>) {
        size_t res = sizeof...(F);
        size_t idx = 0;
        ((res = F::attribute == attr ? idx : res, ++idx), ...);
        return res;
    }(NodeOf<I>::fields);
}

//! The field with the given index.
template <size_t J, class F, class... G> struct NthField {
    using type = NthField<J - 1, G...>::type;
};

template <class F, class... G> struct NthField<0, F, G...> {
    using type = F;
};

//! A string argument for constructors taking shared or plain strings.
struct StringArg {
    CppClingo::SharedString str;
    // NOLINTBEGIN(cppcoreguidelines-explicit-constructor)
    operator CppClingo::SharedString() && { return std::move(str); }
    operator CppClingo::String() const { return *str; }
    // NOLINTEND(cppcoreguidelines-explicit-constructor)
};

//! Convert the value of a field.
template <class T, class F> auto read_field(clingo_lib_t *lib, clingo_ast_value_t const &val) -> decltype(auto) {
    using V = std::remove_cvref_t<decltype(F::get(std::declval<T const &>()))>;
    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
    if constexpr (F::kind == Kind::location) {
        return convert(val.location);
    } else if constexpr (F::kind == Kind::number) {
        return static_cast<V>(val.number);
    } else if constexpr (F::kind == Kind::symbol) {
        return CppClingo::Symbol::from_rep(val.symbol);
    } else if constexpr (F::kind == Kind::string) {
        auto str = std::string_view{val.string.data, val.string.size};
        if constexpr (std::is_same_v<V, std::string_view>) {
            return str;
        } else {
            return StringArg{lib->store->string(str)};
        }
    } else if constexpr (F::kind == Kind::string_array) {
        return convert(lib, val.string_array.data, val.string_array.size);
    } else if constexpr (F::kind == Kind::symbol_array) {
        return convert(val.symbol_array.data, val.symbol_array.size);
    } else if constexpr (F::kind == Kind::ast) {
        return convert<V>(val.ast);
    } else {
        return convert<std::ranges::range_value_t<V>>(val.ast_array.data, val.ast_array.size);
    }
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)
}

//! The number of fields of the node with the given AST type.
template <int I>
constexpr size_t field_count = []<class... F>(Fields<F...>) { return sizeof...(F); }(NodeOf<I>::fields);

//! The index of the field passed as the given constructor argument.
template <int I> constexpr auto ctor_index(size_t arg) -> size_t {
    if constexpr (CtorOrder<I>::value.empty()) {
        return arg;
    } else {
        return field_index<I>(CtorOrder<I>::value.at(arg));
    }
}

//! Construct a node from the given constructor arguments.
template <class T, class... V> auto construct_value(int tag, V &&...values) -> clingo_ast_t * {
    if constexpr (std::is_aggregate_v<T>) {
        return construct_ast<T>(tag, T{std::forward<V>(values)...});
    } else {
        return construct_ast<T>(tag, std::forward<V>(values)...);
    }
}

//! Construct the node with the given AST type from the given values.
//!
//! The values have to be checked against the record description beforehand.
template <int I> auto construct_node(clingo_lib_t *lib, clingo_ast_value_t const *values) -> clingo_ast_t * {
    using T = node_t<I>;
    return [&]<class... F, size_t... J>(Fields<F...>, std::index_sequence<J...>) {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if constexpr (I == clingo_ast_type_format_field_expression) {
            auto spec = CppClingo::FormatSpec::build(*lib->store,
                                                     read_field<T, typename NthField<2, F...>::type>(lib, values[2]));
            if (!spec) {
                throw std::invalid_argument{"invalid format specification"};
            }
            return construct_value<T>(I, read_field<T, typename NthField<0, F...>::type>(lib, values[0]),
                                      read_field<T, typename NthField<1, F...>::type>(lib, values[1]), *spec);
        } else if constexpr (I == clingo_ast_type_body_conditional_literal) {
            return construct_value<T>(I, CppClingo::Input::CondLit{read_field<T, F>(lib, values[J])...});
        } else {
            return construct_value<T>(
                I, read_field<T, typename NthField<ctor_index<I>(J), F...>::type>(lib, values[ctor_index<I>(J)])...);
        }
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }(NodeOf<I>::fields, std::make_index_sequence<field_count<I>>{});
}

auto construct(clingo_lib_t *lib, clingo_ast_type_t type, clingo_ast_value_t const *values) -> clingo_ast_t * {
    return dispatch(
        type, [&]<int I>(std::integral_constant<int, I>)
            requires has_node<I>
        { return construct_node<I>(lib, values); },
        []() -> clingo_ast_t * { throw std::invalid_argument("invalid AST type"); });
}

//! Check whether the given values match the arguments of the record.
auto check_values(clingo_ast_record_info_t const &record, clingo_ast_info_t const &info,
                  clingo_ast_value_t const *values, size_t size) -> bool {
    // NOLINTBEGIN
    if (size != record.size || (values == nullptr && size > 0)) {
        return fail_with(clingo_result_invalid, "invalid number of AST arguments");
    }
    for (size_t i = 0; i < size; ++i) {
        auto const &arg = record.arguments[i];
        auto const &val = values[i];
        if (val.type != arg.type) {
            return fail_with(clingo_result_invalid, "invalid AST argument type");
        }
        auto valid = true;
        switch (val.type) {
            case clingo_ast_value_type_bool: {
                valid = val.number == 0 || val.number == 1;
                break;
            }
            case clingo_ast_value_type_enum: {
                auto const &enm = info.enums[arg.ref.index];
                valid = std::any_of(enm.values, enm.values + enm.size,
                                    [&val](auto const &x) { return x.value == val.number; });
                break;
            }
            case clingo_ast_value_type_location: {
                valid = val.location != nullptr;
                break;
            }
            case clingo_ast_value_type_string: {
                valid = val.string.data != nullptr || val.string.size == 0;
                break;
            }
            case clingo_ast_value_type_string_array: {
                valid = val.string_array.data != nullptr || val.string_array.size == 0;
                break;
            }
            case clingo_ast_value_type_symbol_array: {
                valid = val.symbol_array.data != nullptr || val.symbol_array.size == 0;
                break;
            }
            case clingo_ast_value_type_ast: {
                valid = val.ast != nullptr;
                break;
            }
            case clingo_ast_value_type_ast_array: {
                auto const *data = val.ast_array.data;
                valid = (data != nullptr || val.ast_array.size == 0) &&
                        std::none_of(data, data + val.ast_array.size, [](auto const *x) { return x == nullptr; });
                break;
            }
            default: {
                break;
            }
        }
        if (!valid) {
            return fail_with(clingo_result_invalid, "invalid AST argument value");
        }
    }
    return true;
    // NOLINTEND
}

} // namespace
} // namespace CppClingo::CAPI

extern "C" auto clingo_ast_construct(clingo_lib_t *lib, clingo_ast_type_t type, clingo_ast_value_t const *values,
                                     size_t size, clingo_ast_t **ast) -> bool {
    CLINGO_TRY {
        require(not_null(lib, ast) && valid_span(values, size));
        *ast = nullptr;
        auto const &info = *clingo_ast_info();
        if (type < 0 || std::cmp_greater_equal(type, info.records_size)) {
            return fail_with(clingo_result_invalid, "invalid AST type");
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (!check_values(info.records[type], info, values, size)) {
            return false;
        }
        *ast = construct(lib, type, values);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_to_string(clingo_ast_t *ast, clingo_string_builder_t *builder) -> bool {
    CLINGO_TRY {
        require(not_null(ast, builder));
        *cpp_cast(builder) << *ast;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_compare(clingo_ast_t *a, clingo_ast_t *b) -> int {
    return c_cast(a->compare(*b));
}

extern "C" auto clingo_ast_equal(clingo_ast_t *a, clingo_ast_t *b) -> bool {
    return a->equal(*b);
}

extern "C" auto clingo_ast_hash(clingo_ast_t *ast) -> size_t {
    return ast->hash();
}

extern "C" auto clingo_ast_copy(clingo_ast_t *ast, clingo_ast_t **copy) -> bool {
    CLINGO_TRY {
        require(copy != nullptr);
        *copy = ast != nullptr ? ast->copy().release() : nullptr;
    }
    CLINGO_CATCH;
}

extern "C" void clingo_ast_free(clingo_ast_t *ast) {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete ast;
}

extern "C" void clingo_ast_array_free(clingo_ast_t **ast, size_t size) {
    if (ast != nullptr) {
        ASTVec::acquire(ast, size);
    }
}

extern "C" auto clingo_ast_get_type(clingo_ast_t *ast, clingo_ast_type_t *type) -> bool {
    CLINGO_TRY {
        require(not_null(ast, type));
        *type = ast->get_type();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_number(clingo_ast_t *ast, clingo_ast_attribute_t attribute, int *value)
    -> bool {
    CLINGO_TRY {
        require(not_null(ast, value));
        if (auto num = ast->get_number(attribute); num) {
            *value = *num;
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_symbol(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                clingo_symbol_t *value) -> bool {
    CLINGO_TRY {
        require(not_null(ast, value));
        if (auto sym = ast->get_symbol(attribute); sym) {
            *value = *sym;
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_location(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                  clingo_location_t const **value) -> bool {
    CLINGO_TRY {
        require(not_null(ast, value));
        if (auto loc = ast->get_location(attribute); loc) {
            *value = *loc;
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_string(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                clingo_string_t *value) -> bool {
    CLINGO_TRY {
        require(not_null(ast, value));
        if (auto str = ast->get_string(attribute); str) {
            value->data = str->data();
            value->size = str->size();
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_string_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                      clingo_string_t const **value, size_t *size) -> bool {
    CLINGO_TRY {
        require(ast != nullptr);
        if (auto vec = ast->get_string_vec(attribute); vec) {
            thread_local auto res = std::vector<clingo_string_t>{};
            res.clear();
            std::transform(vec->begin(), vec->end(), std::back_inserter(res),
                           [](auto const &str) { return clingo_string_t{str.data(), str.size()}; });
            set_opt(value, res.data());
            set_opt(size, res.size());
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_symbol_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                      clingo_symbol_t const **value, size_t *size) -> bool {
    CLINGO_TRY {
        require(ast != nullptr);
        if (auto vec = ast->get_symbol_vec(attribute); vec) {
            thread_local auto res = std::vector<clingo_symbol_t>{};
            res.clear();
            std::transform(vec->begin(), vec->end(), std::back_inserter(res),
                           [](auto const &sym) { return CppClingo::Symbol::to_rep(sym); });
            set_opt(value, res.data());
            set_opt(size, res.size());
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_ast(clingo_ast_t *ast, clingo_ast_attribute_t attribute, clingo_ast_t **value)
    -> bool {
    CLINGO_TRY {
        require(not_null(ast, value));
        if (auto val = ast->get_ast(attribute); val) {
            *value = val->release();
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_ast_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                   clingo_ast_t ***value, size_t *size) -> bool {
    CLINGO_TRY {
        require(not_null(ast, value, size));
        if (auto val = ast->get_ast_vec(attribute); val) {
            std::tie(*value, *size) = val->release();
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_parse_expression(clingo_lib_t *lib, clingo_ast_parse_type_t type, char const *string,
                                            size_t size, clingo_ast_t **ast) -> bool {
    CLINGO_TRY {
        require(not_null(lib, ast) && valid_span(string, size));
        auto p = CppClingo::Input::Parser{lib->log, *lib->store};
        p.init(std::string_view{string, size}, *lib->store->string("<string>"));
        switch (type) {
            case clingo_ast_parse_type_term: {
                auto term = p.parse_term();
                if (!term) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(term)).release();
                break;
            }
            case clingo_ast_parse_type_theory_term: {
                auto term = p.parse_theory_term();
                if (!term) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(term)).release();
                break;
            }
            case clingo_ast_parse_type_literal: {
                auto lit = p.parse_literal();
                if (!lit) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(lit)).release();
                break;
            }
            case clingo_ast_parse_type_head_literal: {
                auto lit = p.parse_head_literal();
                if (!lit) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(lit)).release();
                break;
            }
            case clingo_ast_parse_type_body_literal: {
                auto lit = p.parse_body_literal();
                if (!lit) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(lit)).release();
                break;
            }
            case clingo_ast_parse_type_statement: {
                auto lit = p.parse_statement();
                if (!lit) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(lit)).release();
                break;
            }
            default: {
                fail_invalid(std::source_location::current());
            }
        }
    }
    CLINGO_CATCH;
}

struct clingo_ast_scanner {
  public:
    explicit clingo_ast_scanner(clingo_lib_t *lib) : lib_{lib}, parser_{lib->log, *lib->store} {}
    [[nodiscard]] auto next() -> std::unique_ptr<clingo_ast_t> {
        while (true) {
            if (active_) {
                auto [stm, res] = parser_.scan();
                parse_error_ = parse_error_ || !res;
                if (stm) {
                    return make_ast(*std::move(stm));
                }
                cur_.close();
                active_ = false;
            }
            if (strings_.empty()) {
                break;
            }
            active_ = true;
            if (strings_.front().second) {
                if (strings_.front().first == "-") {
                    parser_.init(std::cin, *lib_->store->string("<string>"));
                } else {
                    cur_.open(strings_.front().first);
                    parser_.init(cur_, *lib_->store->string("<string>"));
                }
            } else {
                parser_.init(strings_.front().first, *lib_->store->string("<string>"));
            }
            strings_.pop_front();
        }
        if (parse_error_) {
            throw CppClingo::parse_error();
        }
        return nullptr;
    }
    [[nodiscard]] auto lib() const -> clingo_lib_t * { return lib_; }
    auto scan_string(std::string_view str) { strings_.emplace_front(str, false); }
    auto scan_file(std::string_view path) { strings_.emplace_front(path, true); }
    [[nodiscard]] auto has_error() const -> bool { return parse_error_; }

  private:
    clingo_lib_t *lib_;
    CppClingo::Input::Parser parser_;
    std::forward_list<std::pair<std::string, bool>> strings_;
    std::ifstream cur_;
    bool active_ = false;
    bool parse_error_ = false;
};

extern "C" auto clingo_ast_parse_string(clingo_lib_t *lib, char const *program, size_t size, clingo_control_t *control,
                                        clingo_ast_callback_t callback, void *data) -> bool {
    CLINGO_TRY {
        require(not_null(lib, callback) && valid_span(program, size));
        CppClingo::GCLock lock{*lib->store};
        auto parse = [&](CppClingo::ProgramBackend *bck, CppClingo::TheoryBackend *thy) {
            auto ph = CppClingo::Control::ParseHelper{lib->log,
                                                      *lib->store,
                                                      [&](CppClingo::Input::Stm stm) {
                                                          auto ast = make_ast(std::move(stm));
                                                          handle_error(callback(ast.get(), data));
                                                      },
                                                      &lib->scripts,
                                                      bck,
                                                      thy};
            return ph.process_string({program, size});
        };
        if (control != nullptr) {
            control->slv->parse_with(parse);
        } else {
            parse(nullptr, nullptr);
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_parse_files(clingo_lib_t *lib, clingo_string_t const *files, size_t size,
                                       clingo_control_t *control, clingo_ast_callback_t callback, void *data) -> bool {
    CLINGO_TRY {
        require(not_null(lib, callback) && valid_span(files, size));
        CppClingo::GCLock lock{*lib->store};
        auto parse = [&](CppClingo::ProgramBackend *bck, CppClingo::TheoryBackend *thy) {
            auto ph = CppClingo::Control::ParseHelper{lib->log,
                                                      *lib->store,
                                                      [&](CppClingo::Input::Stm stm) {
                                                          auto ast = make_ast(std::move(stm));
                                                          handle_error(callback(ast.get(), data));
                                                      },
                                                      &lib->scripts,
                                                      bck,
                                                      thy};
            return ph.process_files(CppClingo::Util::to_vec(
                std::span{files, size}, [](auto const &str) { return std::string_view{str.data, str.size}; }));
        };
        if (control != nullptr) {
            control->slv->parse_with(parse);
        } else {
            parse(nullptr, nullptr);
        }
    }
    CLINGO_CATCH;
}

struct clingo_ast_rewrite_context {
    explicit clingo_ast_rewrite_context(clingo_lib *lib) : lib{lib} {}
    clingo_lib *lib;
    CppClingo::Input::TheoryAtomParser parser;
    CppClingo::Input::ParamMap param_map;
    CppClingo::Input::ConstMap const_map;
    CppClingo::Input::RewriteOptions options;
    CppClingo::Input::RewriteContext ctx = {lib->log, *lib->store, options, parser, param_map, const_map};
    CppClingo::Input::ParamUnmap param_unmap;
};

extern "C" auto clingo_ast_rewrite_context_create(clingo_lib_t *lib, clingo_ast_rewrite_context_t **context) -> bool {
    CLINGO_TRY {
        require(not_null(lib, context));
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        *context = new clingo_ast_rewrite_context{lib};
    }
    CLINGO_CATCH;
}

extern "C" void clingo_ast_rewrite_context_free(clingo_ast_rewrite_context_t *context) {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete context;
}

extern "C" auto clingo_ast_rewrite_context_add_param(clingo_ast_rewrite_context_t *context, char const *param,
                                                     size_t size) -> bool {
    CLINGO_TRY {
        require(context != nullptr && valid_span(param, size));
        auto *lib = context->lib;
        if (auto prm = lib->store->string(std::string_view{param, size}); context->param_map.emplace(prm).second) {
            auto var = lib->store->string("$" + std::to_string(context->param_unmap.size()));
            context->param_unmap.emplace(var, prm);
        }
    }
    CLINGO_CATCH;
}

extern "C" void clingo_ast_rewrite_context_clear_params(clingo_ast_rewrite_context_t *context) {
    context->param_map.clear();
    context->param_unmap.clear();
}

extern "C" auto clingo_ast_rewrite_context_add_theory(clingo_ast_rewrite_context_t *context, clingo_ast_t const *theory)
    -> bool {
    CLINGO_TRY {
        require(not_null(context, theory));
        auto *lib = context->lib;
        auto stm = convert<CppClingo::Input::StmTheory>(theory);
        context->parser.add_theory(lib->log, stm);
        if (context->parser.has_error()) {
            throw std::runtime_error("adding theory failed");
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_rewrite_context_get_project_anonymous(clingo_ast_rewrite_context_t *context) -> bool {
    return context->options.project_anonymous;
}

extern "C" void clingo_ast_rewrite_context_set_project_anonymous(clingo_ast_rewrite_context_t *context, bool value) {
    context->options.project_anonymous = value;
}

extern "C" auto clingo_ast_rewrite_context_get_project_mode(clingo_ast_rewrite_context_t *context)
    -> clingo_projection_mode_t {
    return static_cast<clingo_projection_mode_t>(context->options.project_mode);
}

extern "C" void clingo_ast_rewrite_context_set_project_mode(clingo_ast_rewrite_context_t *context,
                                                            clingo_projection_mode_t value) {
    context->options.project_mode = static_cast<CppClingo::Input::ProjectionMode>(value);
}

extern "C" auto clingo_ast_rewrite_context_get_lib(clingo_ast_rewrite_context_t *context) -> clingo_lib_t * {
    return context->lib;
}

extern "C" auto clingo_ast_rewrite(clingo_ast_rewrite_context_t *context, clingo_ast_t *statement,
                                   clingo_ast_t ***result, size_t *result_size) -> bool {
    using namespace CppClingo::Input;
    CLINGO_TRY {
        require(not_null(context, statement, result, result_size));
        auto *lib = context->lib;
        CppClingo::GCLock lock{*lib->store};
        *result = nullptr;
        *result_size = 0;
        auto stms = StmVec{};
        auto stm = convert<Stm>(statement);
        rewrite(context->ctx, stm, stms);
        ASTVec res{stms.size()};
        int i = 0;
        for (auto &stm : stms) {
            if (auto res_stm = unmap_params(*lib->store, context->param_unmap, stm); res_stm) {
                stm = *std::move(res_stm);
            }
            res[i] = make_ast(std::move(stm)).release();
            ++i;
        }
        std::tie(*result, *result_size) = res.release();
    }
    CLINGO_CATCH;
}

clingo_program::clingo_program(clingo_lib_t *lib) : lib{lib} {
    lib->store->gc_add_owner(*this);
}

clingo_program::~clingo_program() noexcept {
    lib->store->gc_del_owner(*this);
}

void clingo_program::mark(CppClingo::SymbolCollector &gc) const {
    program.mark(gc);
}

extern "C" auto clingo_program_new(clingo_lib_t *lib, clingo_program_t **program) -> bool {
    CLINGO_TRY {
        require(not_null(lib, program));
        *program = std::make_unique<clingo_program>(lib).release();
    }
    CLINGO_CATCH;
}

extern "C" void clingo_program_free(clingo_program_t *program) {
    // NOLINTNEXTLINE
    delete program;
}

extern "C" auto clingo_program_add(clingo_program_t *program, clingo_ast_t *statement) -> bool {
    CLINGO_TRY {
        require(not_null(program, statement));
        program->program.add(*program->lib->store, convert<CppClingo::Input::Stm>(statement));
    }
    CLINGO_CATCH;
}

// NOLINTEND(cppcoreguidelines-macro-usage)
