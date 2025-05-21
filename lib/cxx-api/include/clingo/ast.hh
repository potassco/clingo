#pragma once

#include <clingo/core.hh>
#include <clingo/detail/ast.hh>
#include <clingo/symbol.hh>

#include <clingo/ast.h>

#include <cassert>
#include <cstring>
#include <utility>

namespace Clingo::AST {

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

using Visitor = std::function<void(Node const &)>;
void visit(Visitor const &fun, Node const &node);
void visit(Visitor const &fun, std::optional<Node> const &node);
void visit(Visitor const &fun, std::span<Node const> nodes);

using Transformer = std::function<std::optional<Node>(Node const &)>;
auto transform(Transformer const &fun, Node const &node) -> std::optional<Node>;
auto transform(Transformer const &fun, std::optional<Node> const &node) -> std::optional<std::optional<Node>>;
auto transform(Transformer const &fun, std::vector<Node> nodes) -> std::optional<std::vector<Node>>;

class Node {
  public:
    explicit Node(clingo_ast_t *ast) : ast_{ast, false} {}

    [[nodiscard]] friend auto c_cast(Node const &x) -> clingo_ast_t * { return x.ast_.get(); }

    template <NodeType Type, class... Args>
    [[nodiscard]] static auto create(Library const &lib, Args const &...args) -> Node {
        constexpr auto type = static_cast<size_t>(Type);
        static_assert(type < Detail::cons.size(), "invalid type");
        static_assert(sizeof...(Args) == Detail::cons.at(type).size(), "wrong number of arguments");
        return Node{create_<type>(lib, std::make_index_sequence<Detail::cons.at(type).size()>(), args...)};
    }

    template <NodeType Type, class Updater>
    [[nodiscard]] auto update(Library const &lib, Updater const &fun) const -> Node {
        constexpr auto type = static_cast<size_t>(Type);
        static_assert(type < Detail::cons.size(), "invalid type");
        return Node{update_<type>(lib, fun, std::make_index_sequence<Detail::cons.at(type).size()>())};
    }

    void accept(Visitor const &fun) const {
        auto t = type();
        for (auto const arg : Detail::cons.at(static_cast<size_t>(t))) {
            if (arg.type == Detail::Arg::node) {
                visit(fun, node(static_cast<Attribute>(arg.attr)));
            } else if (arg.type == Detail::Arg::optional_node) {
                visit(fun, optional_node(static_cast<Attribute>(arg.attr)));
            } else if (arg.type == Detail::Arg::node_array) {
                visit(fun, nodes(static_cast<Attribute>(arg.attr)));
            }
        }
    }

    [[nodiscard]] auto accept(Library const &lib, Transformer const &fun) const -> std::optional<Node> {
        return dispatch_(lib, static_cast<size_t>(type()), fun, std::make_index_sequence<Detail::cons.size()>());
    }

    [[nodiscard]] auto type() const -> NodeType {
        return static_cast<NodeType>(Detail::call<clingo_ast_get_type>(ast_.get()));
    }

    [[nodiscard]] auto number(Attribute attribute) const -> int {
        return Detail::call<clingo_ast_attribute_get_number>(ast_.get(),
                                                             static_cast<clingo_ast_attribute_t>(attribute));
    }

    [[nodiscard]] auto symbol(Attribute attribute) const -> Symbol {
        return Symbol{
            Detail::call<clingo_ast_attribute_get_symbol>(ast_.get(), static_cast<clingo_ast_attribute_t>(attribute)),
            true};
    }

    [[nodiscard]] auto symbols(Attribute attribute) const -> SymbolVector {
        clingo_symbol_t const *value = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_ast_attribute_get_symbol_array(
            ast_.get(), static_cast<clingo_ast_attribute_t>(attribute), &value, &size));
        return Detail::transform(std::span{value, size}, [](auto x) { return Symbol{x, true}; });
    }

    [[nodiscard]] auto location(Attribute attribute) const -> Location {
        return Location{Detail::call<clingo_ast_attribute_get_location>(
            ast_.get(), static_cast<clingo_ast_attribute_t>(attribute))};
    }

    [[nodiscard]] auto string(Attribute attribute) const -> std::string_view {
        auto [data, size] =
            Detail::call<clingo_ast_attribute_get_string>(ast_.get(), static_cast<clingo_ast_attribute_t>(attribute));
        return {data, size};
    }

    [[nodiscard]] auto strings(Attribute attribute) const -> std::vector<std::string_view> {
        clingo_string_t const *value = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_ast_attribute_get_string_array(
            ast_.get(), static_cast<clingo_ast_attribute_t>(attribute), &value, &size));
        return Detail::transform(std::span{value, size}, [](auto x) { return std::string_view{x.data, x.size}; });
    }

    [[nodiscard]] auto node(Attribute attribute) const -> Node {
        clingo_ast_t *value =
            Detail::call<clingo_ast_attribute_get_ast>(ast_.get(), static_cast<clingo_ast_attribute_t>(attribute));
        if (value == nullptr) {
            throw std::runtime_error("invalid attribute");
        }
        return Node{value};
    }

    [[nodiscard]] auto optional_node(Attribute attribute) const -> std::optional<Node> {
        clingo_ast_t *value =
            Detail::call<clingo_ast_attribute_get_ast>(ast_.get(), static_cast<clingo_ast_attribute_t>(attribute));
        return value != nullptr ? std::make_optional<Node>(value) : std::nullopt;
    }

    [[nodiscard]] auto nodes(Attribute attribute) const -> std::vector<Node> {
        auto arr = Detail::Array{};
        Detail::handle_error(clingo_ast_attribute_get_ast_array(
            ast_.get(), static_cast<clingo_ast_attribute_t>(attribute), &arr.value, &arr.size));
        return Detail::transform(std::span{arr.value, arr.size},
                                 [](auto *&node) { return Node{std::exchange(node, nullptr)}; });
    }

    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_ast_to_string(ast_.get(), c_cast(bld)));
        return std::string{bld.str()};
    }

    [[nodiscard]] auto hash() const noexcept -> size_t { return clingo_ast_hash(ast_.get()); }
    friend auto operator==(Node const &a, Node const &b) noexcept -> bool {
        return clingo_ast_equal(a.ast_.get(), b.ast_.get());
    }
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

enum class ParseType : clingo_ast_parse_type_t {
    term = clingo_ast_parse_type_term,
    theory_term = clingo_ast_parse_type_theory_term,
    literal = clingo_ast_parse_type_literal,
    body_literal = clingo_ast_parse_type_body_literal,
    head_literal = clingo_ast_parse_type_head_literal,
    statement = clingo_ast_parse_type_statement,
};

class Scanner {
  public:
    struct sentinel {};
    class iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = Node *;
        using reference = Node &;

        iterator() = default;

        explicit iterator(Scanner &hnd) : scanner_{&hnd} { operator++(); }

        auto operator*() const -> reference {
            assert(scanner_ != nullptr && scanner_->value_);
            return *scanner_->value_;
        }

        auto operator->() const -> pointer { return &**this; }

        auto operator++() -> iterator & {
            assert(scanner_ != nullptr);
            scanner_->next_();
            return *this;
        }

        auto operator++(int) -> iterator {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend auto operator==([[maybe_unused]] iterator const &a, [[maybe_unused]] iterator const &b) -> bool {
            assert(a.scanner_ == b.scanner_);
            return true;
        }

        friend auto operator==(iterator const &a, [[maybe_unused]] sentinel const &b) -> bool {
            assert(a.scanner_ != nullptr);
            return !a.scanner_->value_.has_value();
        }

      private:
        Scanner *scanner_ = nullptr;
    };
    using difference_type = iterator::difference_type;
    using value_type = iterator::value_type;
    using reference = iterator::reference;
    using pointer = iterator::pointer;

    explicit Scanner(Library lib, std::string_view program)
        : lib_{std::move(lib)},
          scanner_{Detail::call<clingo_ast_scan_string>(c_cast(lib_), program.data(), program.size())} {}

    explicit Scanner(Library &lib, StringSpan files) : lib_{lib} {
        auto cfiles = Detail::transform(files, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
        scanner_.reset(Detail::call<clingo_ast_scan_files>(c_cast(lib), cfiles.data(), cfiles.size()));
    }

    explicit Scanner(Library &lib, StringList files) : Scanner{lib, std::span{files}} {}

    auto begin() -> iterator { return iterator{*this}; }

    auto end() -> sentinel {
        static_cast<void>(this);
        return sentinel{};
    }

  private:
    void next_() {
        clingo_ast_t *ast = Detail::call<clingo_ast_scanner_next>(scanner_.get());
        if (ast != nullptr) {
            value_.emplace(ast);
        } else {
            value_.reset();
        }
    }

    Library lib_;
    Detail::unique_handle<clingo_ast_scanner_t, clingo_ast_scanner_close> scanner_;
    std::optional<Node> value_;
};
static_assert(std::input_iterator<Scanner::iterator>);
static_assert(std::sentinel_for<Scanner::sentinel, Scanner::iterator>);

enum class ProjectionMode : clingo_projection_mode_t {
    disabled = clingo_projection_mode_disabled,
    anonymous = clingo_projection_mode_anonymous,
    pure = clingo_projection_mode_pure,
};

class RewriteContext {
  public:
    explicit RewriteContext(Library const &lib) : ctx_{Detail::call<clingo_ast_rewrite_context_create>(c_cast(lib))} {}

    friend auto c_cast(RewriteContext const &x) -> clingo_ast_rewrite_context_t * { return x.ctx_.get(); }

    void project_mode(ProjectionMode value) {
        clingo_ast_rewrite_context_set_project_mode(ctx_.get(), static_cast<clingo_projection_mode_t>(value));
    }

    auto project_mode() -> ProjectionMode {
        return static_cast<ProjectionMode>(clingo_ast_rewrite_context_get_project_mode(ctx_.get()));
    }

    void project_anonymous(bool value) { clingo_ast_rewrite_context_set_project_anonymous(ctx_.get(), value); }

    auto project_anonymous() -> bool { return clingo_ast_rewrite_context_get_project_mode(ctx_.get()) != 0; }

    void add_param(std::string_view name) {
        Detail::handle_error(clingo_ast_rewrite_context_add_param(ctx_.get(), name.data(), name.size()));
    }

    void clear_params() { clingo_ast_rewrite_context_clear_params(ctx_.get()); }

    void add_theory(Node const &stm) {
        Detail::handle_error(clingo_ast_rewrite_context_add_theory(ctx_.get(), c_cast(stm)));
    }

  private:
    Detail::unique_handle<clingo_ast_rewrite_context_t, clingo_ast_rewrite_context_free> ctx_;
};

inline auto rewrite(RewriteContext &ctx, Node const &stm) -> std::vector<Node> {
    auto arr = Detail::Array{};
    Detail::handle_error(clingo_ast_rewrite(c_cast(ctx), c_cast(stm), &arr.value, &arr.size));
    return Detail::transform(std::span{arr.value, arr.size},
                             [](auto *&ast) { return Node{std::exchange(ast, nullptr)}; });
}

class Program {
  public:
    Program(Library const &lib) : prg_{Detail::call<clingo_program_new>(c_cast(lib))} {}

    void add(Node const &stm) { Detail::handle_error(clingo_program_add(prg_.get(), c_cast(stm))); }

    friend auto c_cast(Program const &x) -> clingo_program_t * { return x.prg_.get(); }

  private:
    Detail::unique_handle<clingo_program_t, clingo_program_free> prg_;
};

inline auto parse(Library const &lib, std::string_view string, ParseType type = ParseType::statement) -> Node {
    return Node{Detail::call<clingo_ast_parse_expression>(c_cast(lib), static_cast<clingo_ast_parse_type_t>(type),
                                                          string.data(), string.size())};
}

} // namespace Clingo::AST

namespace std {

template <> struct hash<Clingo::AST::Node> {
    auto operator()(Clingo::AST::Node const &x) const noexcept -> size_t { return x.hash(); }
};

} // namespace std
