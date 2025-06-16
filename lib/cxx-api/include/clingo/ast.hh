#pragma once

#include <clingo/control.hh>
#include <clingo/core.hh>
#include <clingo/detail/ast.hh>
#include <clingo/symbol.hh>

#include <clingo/ast.h>

#include <cassert>
#include <cstring>
#include <utility>

namespace Clingo::AST {

//! @addtogroup cpp_ast
//! Functions and data structures to work with program ASTs.
//!
//! @{

namespace UnaryOperator {
inline constexpr int minus = 0;
inline constexpr int negation = 1;
} // namespace UnaryOperator

namespace BinaryOperator {
inline constexpr int and_ = 0;
inline constexpr int division = 1;
inline constexpr int minus = 2;
inline constexpr int modulo = 3;
inline constexpr int multiplication = 4;
inline constexpr int or_ = 5;
inline constexpr int plus = 6;
inline constexpr int power = 7;
inline constexpr int xor_ = 8;
} // namespace BinaryOperator

namespace Sign {
inline constexpr int no_sign = 0;
inline constexpr int single = 1;
inline constexpr int double_ = 2;
} // namespace Sign

namespace Relation {
inline constexpr int equal = 0;
inline constexpr int not_equal = 1;
inline constexpr int less = 2;
inline constexpr int less_equal = 3;
inline constexpr int greater = 4;
inline constexpr int greater_equal = 5;
} // namespace Relation

namespace AggregateFunction {
inline constexpr int count = 0;
inline constexpr int sum = 1;
inline constexpr int sump = 2;
inline constexpr int min = 3;
inline constexpr int max = 4;
} // namespace AggregateFunction

namespace TheoryOperatorType {
inline constexpr int unary = 0;
inline constexpr int binary_left = 1;
inline constexpr int binary_right = 2;
} // namespace TheoryOperatorType

namespace TheoryTupleType {
inline constexpr int tuple = 0;
inline constexpr int set = 1;
inline constexpr int list = 2;
} // namespace TheoryTupleType

namespace TheoryAtomType {
inline constexpr int head = 0;
inline constexpr int body = 1;
inline constexpr int any = 2;
inline constexpr int directive = 3;
} // namespace TheoryAtomType

namespace OptimizeType {
inline constexpr int minimize = 0;
inline constexpr int maximize = 1;
} // namespace OptimizeType

namespace IncludeType {
inline constexpr int system = 0;
inline constexpr int inbuild = 1;
} // namespace IncludeType

namespace Precedence {
inline constexpr int default_ = 0;
inline constexpr int override = 1;
} // namespace Precedence

namespace CommentType {
inline constexpr int line = 0;
inline constexpr int block = 1;
} // namespace CommentType

//! Enumeration of available ast attributes.
enum class Attribute : clingo_ast_attribute_t {
    anonymous = clingo_ast_attribute_anonymous,
    arguments = clingo_ast_attribute_arguments,
    arity = clingo_ast_attribute_arity,
    atom = clingo_ast_attribute_atom,
    atoms = clingo_ast_attribute_atoms,
    atom_type = clingo_ast_attribute_atom_type,
    body = clingo_ast_attribute_body,
    comment_type = clingo_ast_attribute_comment_type,
    condition = clingo_ast_attribute_condition,
    precedence = clingo_ast_attribute_precedence,
    elements = clingo_ast_attribute_elements,
    external = clingo_ast_attribute_external,
    external_type = clingo_ast_attribute_external_type,
    function = clingo_ast_attribute_function,
    guard = clingo_ast_attribute_guard,
    head = clingo_ast_attribute_head,
    include_type = clingo_ast_attribute_include_type,
    left = clingo_ast_attribute_left,
    literal = clingo_ast_attribute_literal,
    location = clingo_ast_attribute_location,
    modifier = clingo_ast_attribute_modifier,
    name = clingo_ast_attribute_name,
    operators = clingo_ast_attribute_operators,
    operator_type = clingo_ast_attribute_operator_type,
    optimize_type = clingo_ast_attribute_optimize_type,
    pool = clingo_ast_attribute_pool,
    priority = clingo_ast_attribute_priority,
    relation = clingo_ast_attribute_relation,
    right = clingo_ast_attribute_right,
    script_type = clingo_ast_attribute_script_type,
    sign = clingo_ast_attribute_sign,
    symbol = clingo_ast_attribute_symbol,
    term = clingo_ast_attribute_term,
    terms = clingo_ast_attribute_terms,
    theory_operator = clingo_ast_attribute_theory_operator,
    tuple = clingo_ast_attribute_tuple,
    tuple_type = clingo_ast_attribute_tuple_type,
    u = clingo_ast_attribute_u,
    v = clingo_ast_attribute_v,
    value = clingo_ast_attribute_value,
    weight = clingo_ast_attribute_weight,
};

//! Enumeration of available ast node types.
//!
//! Each note type is associated with a set of attributes.
enum class NodeType : clingo_ast_type_t {
    // terms
    projection = clingo_ast_type_projection,
    term_variable = clingo_ast_type_term_variable,
    term_symbolic = clingo_ast_type_term_symbolic,
    term_absolute = clingo_ast_type_term_absolute,
    term_unary_operation = clingo_ast_type_term_unary_operation,
    term_binary_operation = clingo_ast_type_term_binary_operation,
    term_tuple = clingo_ast_type_term_tuple,
    term_function = clingo_ast_type_term_function,
    argument_tuple = clingo_ast_type_argument_tuple,
    // theory terms
    unparsed_element = clingo_ast_type_unparsed_element,
    theory_term_variable = clingo_ast_type_theory_term_variable,
    theory_term_symbolic = clingo_ast_type_theory_term_symbolic,
    theory_term_tuple = clingo_ast_type_theory_term_tuple,
    theory_term_function = clingo_ast_type_theory_term_function,
    theory_term_unparsed = clingo_ast_type_theory_term_unparsed,
    // literals
    left_guard = clingo_ast_type_left_guard,
    right_guard = clingo_ast_type_right_guard,
    literal_boolean = clingo_ast_type_literal_boolean,
    literal_comparison = clingo_ast_type_literal_comparison,
    literal_symbolic = clingo_ast_type_literal_symbolic,
    // set aggregates and theory atoms
    set_aggregate_element = clingo_ast_type_set_aggregate_element,
    theory_atom_element = clingo_ast_type_theory_atom_element,
    theory_right_guard = clingo_ast_type_theory_right_guard,
    // body literals
    body_simple_literal = clingo_ast_type_body_simple_literal,
    body_aggregate_element = clingo_ast_type_body_aggregate_element,
    body_aggregate = clingo_ast_type_body_aggregate,
    body_set_aggregate = clingo_ast_type_body_set_aggregate,
    body_theory_atom = clingo_ast_type_body_theory_atom,
    body_conditional_literal = clingo_ast_type_body_conditional_literal,
    // head literals
    head_simple_literal = clingo_ast_type_head_simple_literal,
    head_aggregate_element = clingo_ast_type_head_aggregate_element,
    head_aggregate = clingo_ast_type_head_aggregate,
    head_set_aggregate = clingo_ast_type_head_set_aggregate,
    head_theory_atom = clingo_ast_type_head_theory_atom,
    head_conditional_literal = clingo_ast_type_head_conditional_literal,
    head_disjunction = clingo_ast_type_head_disjunction,
    // theory definition
    theory_operator_definition = clingo_ast_type_theory_operator_definition,
    theory_term_definition = clingo_ast_type_theory_term_definition,
    theory_guard_definition = clingo_ast_type_theory_guard_definition,
    theory_atom_definition = clingo_ast_type_theory_atom_definition,
    // elements
    optimize_tuple = clingo_ast_type_optimize_tuple,
    optimize_element = clingo_ast_type_optimize_element,
    edge = clingo_ast_type_edge,
    program_part = clingo_ast_type_program_part,
    // statements
    statement_rule = clingo_ast_type_statement_rule,
    statement_theory = clingo_ast_type_statement_theory,
    statement_optimize = clingo_ast_type_statement_optimize,
    statement_weak_constraint = clingo_ast_type_statement_weak_constraint,
    statement_show = clingo_ast_type_statement_show,
    statement_show_nothing = clingo_ast_type_statement_show_nothing,
    statement_show_signature = clingo_ast_type_statement_show_signature,
    statement_project = clingo_ast_type_statement_project,
    statement_project_signature = clingo_ast_type_statement_project_signature,
    statement_defined = clingo_ast_type_statement_defined,
    statement_external = clingo_ast_type_statement_external,
    statement_edge = clingo_ast_type_statement_edge,
    statement_heuristic = clingo_ast_type_statement_heuristic,
    statement_script = clingo_ast_type_statement_script,
    statement_program = clingo_ast_type_statement_program,
    statement_include = clingo_ast_type_statement_include,
    statement_const = clingo_ast_type_statement_const,
    statement_parts = clingo_ast_type_statement_parts,
    statement_comment = clingo_ast_type_statement_comment
};

class Node;

//! Function to visit ast nodes.
using Visitor = std::function<void(Node const &)>;
//! Visit the given node with the visitor.
//!
//! This function simply calls `fun(node)`.
//!
//! @param fun the visitor
//! @param node the node to visit
void visit(Visitor const &fun, Node const &node);
//! Visit the given node with the visitor if engaged.
//!
//! @param fun the visitor
//! @param node the node to visit
void visit(Visitor const &fun, std::optional<Node> const &node);
//! Visit the nodes in the given span with the visitor.
//!
//! @param fun the visitor
//! @param nodes the node to visit
void visit(Visitor const &fun, std::span<Node const> nodes);

//! Function to transform ast nodes.
//!
//! The function can either return an updated node or return std::nullopt to
//! keep it as is.
using Transformer = std::function<std::optional<Node>(Node const &)>;
//! Transform the given node with the transformer.
//!
//! This function simply calls `fun(node)`.
//!
//! @param fun the visitor
//! @param node the node to transform
auto transform(Transformer const &fun, Node const &node) -> std::optional<Node>;
//! Transform the given node with the transformer if engaged.
//!
//! @param fun the visitor
//! @param node the node to transform
auto transform(Transformer const &fun, std::optional<Node> const &node) -> std::optional<std::optional<Node>>;
//! Transform the nodes in the given span with the transformer.
//!
//! @param fun the visitor
//! @param nodes the node to visit
auto transform(Transformer const &fun, std::vector<Node> nodes) -> std::optional<std::vector<Node>>;

//! Node capturing expressions in logic programs.
//!
//! Nodes are immutable objects supporting record updates.
//!
//! Refer to `lib/c-api/src/ast_yaml.cc` for node types and their arguments.
class Node {
  public:
    //! Construct a node form its C representation.
    //!
    //! If copy is true copies the given node or otherwise takes ownership.
    //! Copying itself is cheap and just involves updating reference counts.
    //!
    //! For internal sue.
    //!
    //! @param ast the C ast
    //! @param copy whether to copy
    explicit Node(clingo_ast_t *ast, bool copy = false) : ast_{ast, copy} {}

    //! Cast the node to its underlying C representation.
    [[nodiscard]] friend auto c_cast(Node const &x) -> clingo_ast_t * { return x.ast_.get(); }

    //! Construct a node of the given type.
    //!
    //! @tparam Type the type of the node
    //! @param lib the library object to store symbols
    //! @param args the attributes that make up the nodes
    //! @return the constructed node
    template <NodeType Type, class... Args>
    [[nodiscard]] static auto create(Library const &lib, Args const &...args) -> Node {
        constexpr auto type = static_cast<size_t>(Type);
        static_assert(type < Detail::cons.size(), "invalid type");
        static_assert(sizeof...(Args) == Detail::cons.at(type).size(), "wrong number of arguments");
        return Node{create_<type>(lib, std::make_index_sequence<Detail::cons.at(type).size()>(), args...)};
    }

    //! Visit a node of the given type.
    //!
    //! The given type must correspond to the actual type of the node. The
    //! given updater must be templated for the attributes of the node that are
    //! to be changed. If the return type of this function is not void, it will
    //! be used to update the respective attribute.
    //!
    //! @tparam Type the type of the node
    //! @param lib the library object to store symbols
    //! @param fun the templated updater
    //! @return the updated node
    template <NodeType Type, class Updater>
    [[nodiscard]] auto update(Library const &lib, Updater const &fun) const -> Node {
        assert(Type == type());
        constexpr auto type = static_cast<size_t>(Type);
        static_assert(type < Detail::cons.size(), "invalid type");
        return Node{update_<type>(lib, fun, std::make_index_sequence<Detail::cons.at(type).size()>())};
    }

    //! Visit the direct children of the node with the given visitor.
    //!
    //! Note that the visitor is in charge of recursively visiting further
    //! nodes.
    //!
    //! @param fun the visitor
    void accept(Visitor const &fun) const {
        auto t = type();
        for (auto const [attr, type] : Detail::cons.at(static_cast<size_t>(t))) {
            if (type == Detail::Arg::node) {
                visit(fun, node(static_cast<Attribute>(attr)));
            } else if (type == Detail::Arg::optional_node) {
                visit(fun, optional_node(static_cast<Attribute>(attr)));
            } else if (type == Detail::Arg::node_array) {
                visit(fun, nodes(static_cast<Attribute>(attr)));
            }
        }
    }

    //! Transform the direct children of the node with the given transformer.
    //!
    //! @param lib the library object to store symbols
    //! @param fun the transformer
    [[nodiscard]] auto accept(Library const &lib, Transformer const &fun) const -> std::optional<Node> {
        return dispatch_(lib, static_cast<size_t>(type()), fun, std::make_index_sequence<Detail::cons.size()>());
    }

    //! Get the type of the node.
    //!
    //! @return the type
    [[nodiscard]] auto type() const -> NodeType {
        return static_cast<NodeType>(Detail::call<clingo_ast_get_type>(ast_.get()));
    }

    //! Get the numeric value of the given attribute.
    //!
    //! @param attribute the attribute
    //! @return the value
    [[nodiscard]] auto number(Attribute attribute) const -> int {
        return Detail::call<clingo_ast_attribute_get_number>(ast_.get(),
                                                             static_cast<clingo_ast_attribute_t>(attribute));
    }

    //! Get the symbolic value of the given attribute.
    //!
    //! @param attribute the attribute
    //! @return the value
    [[nodiscard]] auto symbol(Attribute attribute) const -> Symbol {
        return Symbol{
            Detail::call<clingo_ast_attribute_get_symbol>(ast_.get(), static_cast<clingo_ast_attribute_t>(attribute)),
            true};
    }

    //! Get the symbolic values of the given attribute.
    //!
    //! @param attribute the attribute
    //! @return the value
    [[nodiscard]] auto symbols(Attribute attribute) const -> SymbolVector {
        clingo_symbol_t const *value = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_ast_attribute_get_symbol_array(
            ast_.get(), static_cast<clingo_ast_attribute_t>(attribute), &value, &size));
        return Detail::transform(std::span{value, size}, [](auto x) { return Symbol{x, true}; });
    }

    //! Get the location value of the given attribute.
    //!
    //! @param attribute the attribute
    //! @return the value
    [[nodiscard]] auto location(Attribute attribute) const -> Location {
        return Location{Detail::call<clingo_ast_attribute_get_location>(
            ast_.get(), static_cast<clingo_ast_attribute_t>(attribute))};
    }

    //! Get the string value of the given attribute.
    //!
    //! @param attribute the attribute
    //! @return the value
    [[nodiscard]] auto string(Attribute attribute) const -> std::string_view {
        auto [data, size] =
            Detail::call<clingo_ast_attribute_get_string>(ast_.get(), static_cast<clingo_ast_attribute_t>(attribute));
        return {data, size};
    }

    //! Get the string values of the given attribute.
    //!
    //! @param attribute the attribute
    //! @return the value
    [[nodiscard]] auto strings(Attribute attribute) const -> std::vector<std::string_view> {
        clingo_string_t const *value = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_ast_attribute_get_string_array(
            ast_.get(), static_cast<clingo_ast_attribute_t>(attribute), &value, &size));
        return Detail::transform(std::span{value, size}, [](auto x) { return std::string_view{x.data, x.size}; });
    }

    //! Get the node value of the given attribute.
    //!
    //! @param attribute the attribute
    //! @return the value
    [[nodiscard]] auto node(Attribute attribute) const -> Node {
        clingo_ast_t *value =
            Detail::call<clingo_ast_attribute_get_ast>(ast_.get(), static_cast<clingo_ast_attribute_t>(attribute));
        if (value == nullptr) {
            throw std::runtime_error("invalid attribute");
        }
        return Node{value};
    }

    //! Get the optional node value of the given attribute.
    //!
    //! @param attribute the attribute
    //! @return the value
    [[nodiscard]] auto optional_node(Attribute attribute) const -> std::optional<Node> {
        clingo_ast_t *value =
            Detail::call<clingo_ast_attribute_get_ast>(ast_.get(), static_cast<clingo_ast_attribute_t>(attribute));
        return value != nullptr ? std::make_optional<Node>(value) : std::nullopt;
    }

    //! Get the node values of the given attribute.
    //!
    //! @param attribute the attribute
    //! @return the value
    [[nodiscard]] auto nodes(Attribute attribute) const -> std::vector<Node> {
        auto arr = Detail::Array{};
        Detail::handle_error(clingo_ast_attribute_get_ast_array(
            ast_.get(), static_cast<clingo_ast_attribute_t>(attribute), &arr.value, &arr.size));
        return Detail::transform(std::span{arr.value, arr.size},
                                 [](auto *&node) { return Node{std::exchange(node, nullptr)}; });
    }

    //! Get a string representation of the node.
    //!
    //! The string representation is intended to be parsable.
    //!
    //! @return the string representation
    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_ast_to_string(ast_.get(), c_cast(bld)));
        return std::string{bld.str()};
    }

    //! Get a hash for the node.
    //!
    //! The hash is designed for usage in hash tables. There is also an
    //! associated specialization for std::hash.
    //!
    //! @return the hash
    [[nodiscard]] auto hash() const noexcept -> size_t { return clingo_ast_hash(ast_.get()); }

    //! Equality compare two nodes.
    //!
    //! @param a the first node
    //! @param b the second node
    //! @return whether the nodes are equal
    friend auto operator==(Node const &a, Node const &b) noexcept -> bool {
        return clingo_ast_equal(a.ast_.get(), b.ast_.get());
    }

    //! Compare two nodes.
    //!
    //! @param a the first node
    //! @param b the second node
    //! @return the result of the comparison
    friend auto operator<=>(Node const &a, Node const &b) noexcept -> std::strong_ordering {
        return clingo_ast_compare(a.ast_.get(), b.ast_.get()) <=> 0;
    }

  private:
    // Convert strings and string arrays into mappable types.
    template <class Arg> [[nodiscard]] static auto convert_(Arg const &arg) -> decltype(auto) {
        if constexpr (Detail::is_contiguous_range_over<Arg, char> || std::is_same_v<Arg, char const *>) {
            return std::string_view{arg};
        } else if constexpr (Detail::is_range_over<Arg, char const *>) {
            return Detail::transform(arg, [](auto str) { return clingo_string_t{str, std::strlen(str)}; });
        } else if constexpr (Detail::is_range_over<Arg, std::string_view>) {
            return Detail::transform(arg, [](auto str) { return clingo_string_t{str.data(), str.size()}; });
        } else if constexpr (std::is_same_v<Arg, bool>) {
            return static_cast<int>(arg);
        } else {
            return arg;
        }
    }

    // Map arguments to their C representation.
    template <Detail::Arg::Type Type, class Arg> [[nodiscard]] static auto map_(Arg const &arg) {
        if constexpr (Detail::is_contiguous_range_over<Arg, Symbol>) {
            static_assert(Type == Detail::Arg::symbol_array);
            return std::make_tuple(std::ranges::data(arg), std::ranges::size(arg));
        } else if constexpr (Detail::is_contiguous_range_over<Arg, Node>) {
            static_assert(Type == Detail::Arg::node_array);
            return std::make_tuple(std::ranges::data(arg), std::ranges::size(arg));
        } else if constexpr (std::is_same_v<Arg, std::string_view>) {
            static_assert(Type == Detail::Arg::string);
            return std::make_tuple(std::ranges::data(arg), std::ranges::size(arg));
        } else if constexpr (Detail::is_range_over<Arg, clingo_string_t>) {
            static_assert(Type == Detail::Arg::string_array);
            return std::make_tuple(std::ranges::data(arg), std::ranges::size(arg));
        } else if constexpr (std::is_same_v<Arg, Node>) {
            static_assert(Type == Detail::Arg::node || Type == Detail::Arg::optional_node);
            return std::make_tuple(c_cast(arg));
        } else if constexpr (std::is_same_v<Arg, std::nullopt_t>) {
            static_assert(Type == Detail::Arg::optional_node);
            return std::make_tuple(nullptr);
        } else if constexpr (std::is_same_v<Arg, std::optional<Node>>) {
            static_assert(Type == Detail::Arg::optional_node);
            return std::make_tuple(arg ? c_cast(*arg) : nullptr);
        } else if constexpr (std::is_same_v<Arg, Symbol>) {
            static_assert(Type == Detail::Arg::symbol);
            return std::make_tuple(c_cast(arg));
        } else if constexpr (std::is_same_v<Arg, Location>) {
            static_assert(Type == Detail::Arg::location);
            return std::make_tuple(c_cast(arg));
        } else if constexpr (std::is_same_v<Arg, int>) {
            static_assert(Type == Detail::Arg::integer);
            return std::make_tuple(arg);
        } else {
            static_assert(Detail::always_false<Arg>, "unsupported argument type");
        }
    }

    //! Construct a node of the given type.
    template <size_t Type, size_t... Is, class... Args>
    [[nodiscard]] static auto create_(Library const &lib, [[maybe_unused]] std::index_sequence<Is...> seq,
                                      Args const &...args) -> clingo_ast_t * {
        constexpr auto const &cons = Detail::cons.at(Type);
        clingo_ast_t *ast = nullptr;
        Detail::handle_error(
            std::apply(clingo_ast_construct,
                       std::tuple_cat(std::make_tuple(c_cast(lib), static_cast<clingo_ast_type_t>(Type), &ast),
                                      map_<cons[Is].type>(convert_(args))...)));
        return ast;
    }

    //! Get the ith member of the given node.
    template <size_t Type, size_t I> [[nodiscard]] auto get_() const {
        constexpr auto const &cons = Detail::cons.at(Type);
        constexpr auto attr = static_cast<Attribute>(cons[I].attr);
        if constexpr (cons[I].type == Detail::Arg::integer) {
            return number(attr);
        } else if constexpr (cons[I].type == Detail::Arg::location) {
            return location(attr);
        } else if constexpr (cons[I].type == Detail::Arg::string) {
            return string(attr);
        } else if constexpr (cons[I].type == Detail::Arg::string_array) {
            return strings(attr);
        } else if constexpr (cons[I].type == Detail::Arg::symbol) {
            return symbol(attr);
        } else if constexpr (cons[I].type == Detail::Arg::symbol_array) {
            return symbols(attr);
        } else if constexpr (cons[I].type == Detail::Arg::node) {
            return node(attr);
        } else if constexpr (cons[I].type == Detail::Arg::optional_node) {
            return optional_node(attr);
        } else if constexpr (cons[I].type == Detail::Arg::node_array) {
            return nodes(attr);
        }
    }

    //! Get the ith member of the given node.
    template <size_t Type, size_t I, typename F> [[nodiscard]] auto update_child_(F const &fun) const {
        constexpr auto const &cons = Detail::cons.at(Type);
        constexpr auto attr = static_cast<Attribute>(cons[I].attr);
        if constexpr (Detail::invokable<F, attr>) {
            return fun.template operator()<attr>();
        } else {
            return get_<Type, I>();
        }
    }

    //! Update the current node replacing selected members.
    template <size_t Type, typename F, size_t... Is>
    [[nodiscard]] auto update_(Library const &lib, F const &fun, std::index_sequence<Is...> seq) const
        -> clingo_ast_t * {
        return create_<Type>(lib, seq, update_child_<Type, Is>(fun)...);
    }

    //! Dispatch to the type-specific transform method.
    template <size_t... Type>
    [[nodiscard]] auto dispatch_(Library const &lib, size_t type, Transformer const &fun,
                                 [[maybe_unused]] std::index_sequence<Type...> seq) const -> std::optional<Node> {
        std::optional<Node> ret;
        std::ignore =
            (((type == Type)
                  ? (ret = apply_<Type>(lib, fun, std::make_index_sequence<Detail::cons.at(Type).size()>{}), true)
                  : false) ||
             ...);
        return ret;
    }

    //! Check if the argument has a value.
    template <typename Arg> [[nodiscard]] static auto has_value_(std::optional<Arg> const &arg) {
        return arg.has_value();
    }
    //! Check if the argument has a value.
    [[nodiscard]] static auto has_value_([[maybe_unused]] std::nullopt_t null) { return false; }

    //! Get the ith member of the given node or the given value if engaged.
    template <size_t Type, size_t I, typename Arg> [[nodiscard]] auto get_value_(std::optional<Arg> arg) const {
        return arg ? *std::move(arg) : get_<Type, I>();
    }

    //! Get the ith member of the given node.
    template <size_t Type, size_t I> [[nodiscard]] auto get_value_([[maybe_unused]] std::nullopt_t null) const {
        return get_<Type, I>();
    }

    //! Transform the ith member of the node.
    template <size_t Type, size_t I> [[nodiscard]] auto transform_child_(Transformer const &fun) const {
        constexpr auto const &cons = Detail::cons.at(Type);
        constexpr auto attr = static_cast<Attribute>(cons[I].attr);
        if constexpr (cons[I].type == Detail::Arg::node) {
            return transform(fun, node(attr));
        } else if constexpr (cons[I].type == Detail::Arg::optional_node) {
            return transform(fun, optional_node(attr));
        } else if constexpr (cons[I].type == Detail::Arg::node_array) {
            return transform(fun, nodes(attr));
        } else {
            return std::nullopt;
        }
    }

    //! Apply the transformation function to the node's members.
    template <size_t Type, size_t... Is>
    [[nodiscard]] auto apply_(Library const &lib, Transformer const &fun, std::index_sequence<Is...> seq) const
        -> std::optional<Node> {
        return transform_<Type, Is...>(lib, seq, transform_child_<Type, Is>(fun)...);
    }

    //! Transform the node if one of its members changed.
    template <size_t Type, size_t... Is, typename... Args>
    [[nodiscard]] auto transform_(Library const &lib, std::index_sequence<Is...> seq, Args &&...args) const
        -> std::optional<Node> {
        if ((has_value_(args) || ...)) {
            return Node{create_<Type>(lib, seq, get_value_<Type, Is>(std::forward<Args>(args))...)};
        }
        return std::nullopt;
    }

    using Traits = Detail::value_handle_traits<clingo_ast_copy, clingo_ast_free>;
    Detail::value_handle<Traits> ast_;
};

inline void visit(Visitor const &fun, Node const &node) {
    fun(node);
}

inline void visit(Visitor const &fun, std::optional<Node> const &node) {
    if (node) {
        fun(*node);
    }
}

inline void visit(Visitor const &fun, std::span<Node const> nodes) {
    for (auto const &node : nodes) {
        fun(node);
    }
}

inline auto transform(Transformer const &fun, Node const &node) -> std::optional<Node> {
    return fun(node);
}

inline auto transform(Transformer const &fun, std::optional<Node> const &node) -> std::optional<std::optional<Node>> {
    auto res = std::optional<std::optional<Node>>();
    if (node) {
        if (auto trans = fun(*node)) {
            res.emplace(std::move(trans));
        }
    }
    return res;
}

inline auto transform(Transformer const &fun, std::vector<Node> nodes) -> std::optional<std::vector<Node>> {
    bool changed = false;
    for (auto &node : nodes) {
        if (auto trans = fun(node)) {
            changed = true;
            node = *std::move(trans);
        }
    }
    return changed ? std::make_optional(std::move(nodes)) : std::nullopt;
}

//! Enumeration of expression types that can be parsed.
enum class ParseType : clingo_ast_parse_type_t {
    //! Parse a term.
    term = clingo_ast_parse_type_term,
    //! Parse a theory term.
    theory_term = clingo_ast_parse_type_theory_term,
    //! Parse a simple literal.
    literal = clingo_ast_parse_type_literal,
    //! Parse a body literal.
    body_literal = clingo_ast_parse_type_body_literal,
    //! Parse a head literal.
    head_literal = clingo_ast_parse_type_head_literal,
    //! Parse a statement.
    statement = clingo_ast_parse_type_statement,
};

//! The available projection modes.
enum class ProjectionMode : clingo_projection_mode_t {
    //! Do not project.
    disabled = clingo_projection_mode_disabled,
    //! Only project anonymous variables.
    anonymous = clingo_projection_mode_anonymous,
    //! Project pure variables (includes anonymous variables).
    pure = clingo_projection_mode_pure,
};

//! A context object for rewriting.
class RewriteContext {
  public:
    //! Construct the context.
    //!
    //! @param lib the library to store symbols
    explicit RewriteContext(Library const &lib) : ctx_{Detail::call<clingo_ast_rewrite_context_create>(c_cast(lib))} {}

    //! Convert the context to its underlying C object.
    friend auto c_cast(RewriteContext const &x) -> clingo_ast_rewrite_context_t * { return x.ctx_.get(); }

    //! Set the projection mode.
    //!
    //! @param value the projection mode
    void project_mode(ProjectionMode value) {
        clingo_ast_rewrite_context_set_project_mode(ctx_.get(), static_cast<clingo_projection_mode_t>(value));
    }

    //! Get the projection mode.
    //!
    //! @return the projection mode
    auto project_mode() -> ProjectionMode {
        return static_cast<ProjectionMode>(clingo_ast_rewrite_context_get_project_mode(ctx_.get()));
    }

    //! Enable projection of anonymous variables in negated literals.
    //!
    //! This exists mainly for backward compatibility with clingo 5. Program
    //! should use the projection `*` instead.
    //!
    //! @param value whether to enable
    void project_anonymous(bool value) { clingo_ast_rewrite_context_set_project_anonymous(ctx_.get(), value); }

    //! Check whether projection in negative literals is enabled.
    //!
    //! @return whether projection is enabled
    auto project_anonymous() -> bool { return clingo_ast_rewrite_context_get_project_mode(ctx_.get()) != 0; }

    //! Protect a parameter from simplification.
    //!
    //! Parameters from const directives and program directives should be
    //! projected before rewriting.
    //!
    //! @param name the name of the parameter to protect
    void add_param(std::string_view name) {
        Detail::handle_error(clingo_ast_rewrite_context_add_param(ctx_.get(), name.data(), name.size()));
    }

    //! Clear previously protected parameters.
    void clear_params() { clingo_ast_rewrite_context_clear_params(ctx_.get()); }

    //! Add a theory node that is used to rewrite theory atoms in statements.
    //!
    //! @param stm the theory statement
    void add_theory(Node const &stm) {
        Detail::handle_error(clingo_ast_rewrite_context_add_theory(ctx_.get(), c_cast(stm)));
    }

  private:
    Detail::unique_handle<clingo_ast_rewrite_context_t, clingo_ast_rewrite_context_free> ctx_;
};

//! Rewrite a statement with the given rewrite context.
//!
//! Removes pools, simplifies arithmetics, parses theory atoms, etc.
//!
//! @param ctx the rewrite context
//! @param stm the statement to rewrite
//! @return the resulting statements
inline auto rewrite(RewriteContext &ctx, Node const &stm) -> std::vector<Node> {
    auto arr = Detail::Array{};
    Detail::handle_error(clingo_ast_rewrite(c_cast(ctx), c_cast(stm), &arr.value, &arr.size));
    return Detail::transform(std::span{arr.value, arr.size},
                             [](auto *&ast) { return Node{std::exchange(ast, nullptr)}; });
}

//! A program to add statements to.
class Program {
  public:
    //! Construct an empty program.
    //!
    //! @param lib a library to store symbols
    Program(Library const &lib) : prg_{Detail::call<clingo_program_new>(c_cast(lib))} {}

    //! Add a statemente node to the program.
    //!
    //! @param stm the statement to add
    void add(Node const &stm) const { Detail::handle_error(clingo_program_add(prg_.get(), c_cast(stm))); }

    //! Convert the program to its underlying C representation.
    //!
    //! @param x the prgoram
    //! @return the C object
    friend auto c_cast(Program const &x) -> clingo_program_t * { return x.prg_.get(); }

  private:
    Detail::unique_handle<clingo_program_t, clingo_program_free> prg_;
};

//! Parse an expression of the given type.
//!
//! @param lib the library to store symbols in
//! @param string the string to parse
//! @param type the node type to parse
//! @return the parsed node
inline auto parse(Library const &lib, std::string_view string, ParseType type = ParseType::statement) -> Node {
    return Node{Detail::call<clingo_ast_parse_expression>(c_cast(lib), static_cast<clingo_ast_parse_type_t>(type),
                                                          string.data(), string.size())};
}

//! Parse the program in the given string.
//!
//! @param lib the library to store symbols in
//! @param string the string to parse
//! @param callback callback to report nodes
//! @param control optional control object to handle aspif
template <class Callback>
// NOLINTNEXTLINE
inline void parse(Library const &lib, std::string_view program, Callback &&callback,
                  Clingo::Control const *ctl = nullptr) {
    clingo_ast_parse_string(
        c_cast(lib), program.data(), program.size(), ctl != nullptr ? c_cast(*ctl) : nullptr,
        [](clingo_ast_t *ast, void *data) {
            auto &callback = *static_cast<Callback *>(data);
            CLINGO_TRY {
                callback(Node{ast, true});
            }
            CLINGO_CATCH;
        },
        &callback);
}

//! Parse the program in the given files.
//!
//! @param lib the library to store symbols in
//! @param files the files to parse
//! @param callback callback to report nodes
//! @param control optional control object to handle aspif
template <class Callback>
// NOLINTNEXTLINE
inline void parse(Library const &lib, std::span<std::string_view const> files, Callback &&callback,
                  Clingo::Control const *ctl = nullptr) {
    auto cfiles = Detail::transform(files, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
    clingo_ast_parse_files(
        c_cast(lib), cfiles.data(), cfiles.size(), ctl != nullptr ? c_cast(*ctl) : nullptr,
        [](clingo_ast_t *ast, void *data) {
            CLINGO_TRY {
                auto &callback = *static_cast<Callback *>(data);
                std::invoke(callback, Node{ast, true});
            }
            CLINGO_CATCH;
        },
        &callback);
}

//! Parse the program in the given files.
//!
//! @param lib the library to store symbols in
//! @param files the files to parse
//! @param callback callback to report nodes
//! @param control optional control object to handle aspif
template <class Callback>
inline void parse(Library const &lib, std::initializer_list<std::string_view> files, Callback &&callback,
                  Clingo::Control const *ctl = nullptr) {
    parse(lib, std::span{files.begin(), files.end()}, std::forward<Callback>(callback), ctl);
}
//! @}

} // namespace Clingo::AST

namespace std {

template <> struct hash<Clingo::AST::Node> {
    auto operator()(Clingo::AST::Node const &x) const noexcept -> size_t { return x.hash(); }
};

} // namespace std
