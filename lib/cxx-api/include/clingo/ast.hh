#pragma once

#include <clingo/ast_detail.hh>
#include <clingo/core.hh>
#include <clingo/symbol.hh>

#include <clingo/ast.h>

#include <cassert>

namespace Clingo::AST {

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

class Node {
  public:
    Node(Node const &other) { Detail::handle_error(clingo_ast_copy(other.ast_, &ast_)); }

    auto operator=(Node const &other) -> Node & {
        if (this != &other) {
            assert(ast_ == nullptr || ast_ != other.ast_);
            clingo_ast_free(std::exchange(ast_, nullptr));
            Detail::handle_error(clingo_ast_copy(other.ast_, &ast_));
        }
        return *this;
    }

    Node(Node &&other) noexcept : ast_{std::exchange(other.ast_, nullptr)} {}

    auto operator=(Node &&other) noexcept -> Node & {
        if (this != &other) {
            assert(ast_ == nullptr || ast_ != other.ast_);
            clingo_ast_free(std::exchange(ast_, std::exchange(other.ast_, nullptr)));
        }
        return *this;
    }

    ~Node() { clingo_ast_free(ast_); }

    explicit Node(clingo_ast_t *ast) : ast_{ast} {}

    template <NodeType type, class... Args> static auto create(Library const &lib, Args const &...args) {
        return Node{create_<type, 0>(lib, args..., sentinel{})};
    }

    template <NodeType type, class Updater> auto update(Library const &lib, Updater const &fun) const -> Node {
        return Node{update_<type, 0>(lib, fun)};
    }

    void visit(std::function<bool(Node const &)> const &fun) const {
        if (fun(*this)) {
            auto t = type();
            for (auto const arg : Detail::cons.at(static_cast<size_t>(t))) {
                if (arg.type == Detail::Arg::node) {
                    node(static_cast<Attribute>(arg.attr)).visit(fun);
                } else if (arg.type == Detail::Arg::optional_node) {
                    if (auto node = optional_node(static_cast<Attribute>(arg.attr))) {
                        node->visit(fun);
                    }
                } else if (arg.type == Detail::Arg::node_array) {
                    for (auto const &node : nodes(static_cast<Attribute>(arg.attr))) {
                        node.visit(fun);
                    }
                }
            }
        }
    }

    auto transform(std::function<std::optional<Node>(Node const &)> const &fun) const -> std::optional<Node> {
        if (auto node = fun(*this)) {
            return node;
        }
        return select_<0>(static_cast<size_t>(type()), fun);
    }

    template <size_t Type>
    auto select_(size_t type, std::function<std::optional<Node>(Node const &)> const &fun) const
        -> std::optional<Node> {
        if (type == Type) {
            return transform_<Type, 0>(fun);
        }
        if constexpr (Type + 1 < Detail::cons.size()) {
            return select_<Type + 1>(type, fun);
        }
        assert(false);
    }

    template <typename Arg> static auto notnull_(std::optional<Arg> const &arg) { return arg.has_value(); }

    static auto notnull_([[maybe_unused]] std::nullopt_t null) { return false; }

    template <size_t Type, size_t i, typename... Args>
    auto transform_(std::function<std::optional<Node>(Node const &)> const &fun, Args const &...args) const
        -> std::optional<Node> {
        constexpr auto const &cons = Detail::cons.at(Type);
        if constexpr (i == cons.size()) {
            if ((notnull_(args) || ...)) {
                throw std::logic_error("implement something similar to update_");
            }
            return std::nullopt;
        } else {
            if constexpr (cons[i].type == Detail::Arg::node) {
                transform_<Type, i + 1>(fun, args..., fun(node(static_cast<Attribute>(cons[i].attr))));
            } else if constexpr (cons[i].type == Detail::Arg::optional_node) {
                throw std::logic_error("some way to handle optional nodes");
            } else if constexpr (cons[i].type == Detail::Arg::node_array) {
                throw std::logic_error("some way to handle node arrays");
            } else {
                // TODO: optional_node and node_array
                transform_<Type, i + 1>(fun, args..., std::nullopt);
            }
        }
        return std::nullopt;
    }

    friend auto c_cast(Node const &x) -> clingo_ast_t * { return x.ast_; }

    [[nodiscard]] auto type() const -> NodeType {
        clingo_ast_type_t value = 0;
        Detail::handle_error(clingo_ast_get_type(ast_, &value));
        return static_cast<NodeType>(value);
    }

    [[nodiscard]] auto number(Attribute attribute) const -> int {
        int value = 0;
        Detail::handle_error(
            clingo_ast_attribute_get_number(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value));
        return value;
    }

    [[nodiscard]] auto symbol(Attribute attribute) const -> Symbol {
        clingo_symbol_t value = 0;
        Detail::handle_error(
            clingo_ast_attribute_get_symbol(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value));
        return Symbol{value, true};
    }

    [[nodiscard]] auto symbols(Attribute attribute) const -> SymbolVector {
        clingo_symbol_t const *value = nullptr;
        size_t size = 0;
        Detail::handle_error(
            clingo_ast_attribute_get_symbol_array(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value, &size));
        return Detail::transform(std::span{value, size}, [](auto x) { return Symbol{x, true}; });
    }

    [[nodiscard]] auto location(Attribute attribute) const -> Location {
        clingo_location_t const *value = nullptr;
        Detail::handle_error(
            clingo_ast_attribute_get_location(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value));
        return Location{value};
    }

    [[nodiscard]] auto string(Attribute attribute) const -> std::string_view {
        clingo_string_t value;
        Detail::handle_error(
            clingo_ast_attribute_get_string(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value));
        return {value.data, value.size};
    }

    [[nodiscard]] auto strings(Attribute attribute) const -> std::vector<std::string_view> {
        clingo_string_t const *value = nullptr;
        size_t size = 0;
        Detail::handle_error(
            clingo_ast_attribute_get_string_array(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value, &size));
        return Detail::transform(std::span{value, size}, [](auto x) { return std::string_view{x.data, x.size}; });
    }

    [[nodiscard]] auto node(Attribute attribute) const -> Node {
        clingo_ast_t *value = nullptr;
        Detail::handle_error(
            clingo_ast_attribute_get_ast(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value));
        if (value == nullptr) {
            throw std::runtime_error("invalid attribute");
        }
        return Node{value};
    }

    [[nodiscard]] auto optional_node(Attribute attribute) const -> std::optional<Node> {
        clingo_ast_t *value = nullptr;
        Detail::handle_error(
            clingo_ast_attribute_get_ast(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value));
        return value != nullptr ? std::make_optional<Node>(value) : std::nullopt;
    }

    [[nodiscard]] auto nodes(Attribute attribute) const -> std::vector<Node> {
        class Free {
          public:
            Free(size_t size) : size_{size} {}
            void operator()(clingo_ast_t **value) const { clingo_ast_array_free(value, size_); }

          private:
            size_t size_;
        };
        clingo_ast_t **value = nullptr;
        size_t size = 0;
        Detail::handle_error(
            clingo_ast_attribute_get_ast_array(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value, &size));
        auto ptr = std::unique_ptr<clingo_ast_t *, Free>{value, Free{size}};
        return Detail::transform(std::span{value, size},
                                 [](auto *&node) { return Node{std::exchange(node, nullptr)}; });
    }

    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_ast_to_string(ast_, c_cast(bld)));
        return std::string{bld.str()};
    }

    [[nodiscard]] auto hash() const noexcept -> size_t { return clingo_ast_hash(ast_); }
    friend auto operator==(Node const &a, Node const &b) noexcept -> bool { return clingo_ast_equal(a.ast_, b.ast_); }
    friend auto operator<=>(Node const &a, Node const &b) noexcept -> std::strong_ordering {
        return clingo_ast_compare(a.ast_, b.ast_) <=> 0;
    }

  private:
    struct sentinel {};

    template <NodeType type, size_t i, class Arg, class... Args>
    static auto create_(Library const &lib, Arg const &value, Args const &...args) -> clingo_ast_t * {
        constexpr auto const &cons = Detail::cons.at(static_cast<size_t>(type));
        if constexpr (std::is_same_v<Arg, sentinel>) {
            static_assert(i == cons.size(), "too few arguments");
            clingo_ast_t *ast = nullptr;
            Detail::handle_error(
                clingo_ast_construct(c_cast(lib), static_cast<clingo_ast_type_t>(type), &ast, args...));
            return ast;
        } else if constexpr (Detail::is_contiguous_range_over<Arg, Symbol>) {
            static_assert(cons[i].type == Detail::Arg::symbol_array);
            return create_<type, i + 1>(lib, args..., std::ranges::data(value), std::ranges::size(value));
        } else if constexpr (Detail::is_contiguous_range_over<Arg, Node>) {
            static_assert(cons[i].type == Detail::Arg::node_array);
            return create_<type, i + 1>(lib, args..., std::ranges::data(value), std::ranges::size(value));
        } else if constexpr (Detail::is_contiguous_range_over<Arg, char> || std::is_same_v<Arg, char const *>) {
            static_assert(cons[i].type == Detail::Arg::string);
            auto str = std::string_view{value};
            return create_<type, i + 1>(lib, args..., std::ranges::data(str), std::ranges::size(str));
        } else if constexpr (Detail::is_range_over<Arg, std::string_view>) {
            static_assert(cons[i].type == Detail::Arg::string_array);
            auto strs = Detail::transform(value, [](auto str) { return clingo_string_t{str.data(), str.size()}; });
            return create_<type, i + 1>(lib, args..., strs.data(), strs.size());
        } else if constexpr (std::is_same_v<Arg, Node>) {
            static_assert(cons[i].type == Detail::Arg::node || cons[i].type == Detail::Arg::optional_node);
            return create_<type, i + 1>(lib, args..., c_cast(value));
        } else if constexpr (std::is_same_v<Arg, std::optional<Node>>) {
            static_assert(cons[i].type == Detail::Arg::optional_node);
            return create_<type, i + 1>(lib, args..., value ? c_cast(*value) : nullptr);
        } else if constexpr (std::is_same_v<Arg, Symbol>) {
            static_assert(cons[i].type == Detail::Arg::symbol);
            return create_<type, i + 1>(lib, args..., c_cast(value));
        } else if constexpr (std::is_same_v<Arg, Location>) {
            static_assert(cons[i].type == Detail::Arg::location);
            return create_<type, i + 1>(lib, args..., c_cast(value));
        } else if constexpr (std::is_same_v<Arg, int>) {
            static_assert(cons[i].type == Detail::Arg::integer);
            return create_<type, i + 1>(lib, args..., value);
        } else if constexpr (std::is_same_v<Arg, bool>) {
            static_assert(cons[i].type == Detail::Arg::integer);
            return create_<type, i + 1>(lib, args..., static_cast<int>(value));
        } else {
            static_assert(Detail::always_false<Arg>, "unsupported argument type");
        }
    }

    template <NodeType type, size_t i, typename F, typename... Args>
    auto update_(Library const &lib, F const &fun, Args const &...args) const -> clingo_ast_t * {
        // TODO: better use create + index sequence with a getter
        constexpr auto const &cons = Detail::cons.at(static_cast<size_t>(type));
        if constexpr (i == cons.size()) {
            clingo_ast_t *ast = nullptr;
            Detail::handle_error(
                clingo_ast_construct(c_cast(lib), static_cast<clingo_ast_type_t>(type), &ast, args...));
            return ast;
        } else {
            constexpr auto attr = static_cast<Attribute>(cons[i].attr);
            if constexpr (Detail::invokable<F, attr>) {
                auto value = fun.template operator()<attr>();
                using Arg = decltype(value);
                if constexpr (Detail::is_contiguous_range_over<Arg, Symbol>) {
                    static_assert(cons[i].type == Detail::Arg::symbol_array);
                    return update_<type, i + 1>(lib, fun, args..., std::ranges::data(value), std::ranges::size(value));
                } else if constexpr (Detail::is_contiguous_range_over<Arg, Node>) {
                    static_assert(cons[i].type == Detail::Arg::node_array);
                    return update_<type, i + 1>(lib, fun, args..., std::ranges::data(value), std::ranges::size(value));
                } else if constexpr (Detail::is_contiguous_range_over<Arg, char> || std::is_same_v<Arg, char const *>) {
                    static_assert(cons[i].type == Detail::Arg::string);
                    auto str = std::string_view{value};
                    return update_<type, i + 1>(lib, fun, args..., std::ranges::data(str), std::ranges::size(str));
                } else if constexpr (Detail::is_range_over<Arg, std::string_view>) {
                    static_assert(cons[i].type == Detail::Arg::string_array);
                    auto strs =
                        Detail::transform(value, [](auto str) { return clingo_string_t{str.data(), str.size()}; });
                    return update_<type, i + 1>(lib, fun, args..., strs.data(), strs.size());
                } else if constexpr (std::is_same_v<Arg, Node>) {
                    static_assert(cons[i].type == Detail::Arg::node || cons[i].type == Detail::Arg::optional_node);
                    return update_<type, i + 1>(lib, fun, args..., c_cast(value));
                } else if constexpr (std::is_same_v<Arg, std::optional<Node>>) {
                    static_assert(cons[i].type == Detail::Arg::optional_node);
                    return update_<type, i + 1>(lib, fun, args..., value ? c_cast(*value) : nullptr);
                } else if constexpr (std::is_same_v<Arg, Symbol>) {
                    static_assert(cons[i].type == Detail::Arg::symbol);
                    return update_<type, i + 1>(lib, fun, args..., c_cast(value));
                } else if constexpr (std::is_same_v<Arg, Location>) {
                    static_assert(cons[i].type == Detail::Arg::location);
                    return update_<type, i + 1>(lib, fun, args..., c_cast(value));
                } else if constexpr (std::is_same_v<Arg, int>) {
                    static_assert(cons[i].type == Detail::Arg::integer);
                    return update_<type, i + 1>(lib, fun, args..., value);
                } else if constexpr (std::is_same_v<Arg, bool>) {
                    static_assert(cons[i].type == Detail::Arg::integer);
                    return update_<type, i + 1>(lib, fun, args..., static_cast<int>(value));
                } else {
                    static_assert(Detail::always_false<Arg>, "unsupported argument type");
                }
            } else if constexpr (cons[i].type == Detail::Arg::integer) {
                int value = 0;
                Detail::handle_error(clingo_ast_attribute_get_number(ast_, cons[i].attr, &value));
                return update_<type, i + 1>(lib, fun, args..., value);
            } else if constexpr (cons[i].type == Detail::Arg::location) {
                clingo_location_t const *value = nullptr;
                Detail::handle_error(clingo_ast_attribute_get_location(ast_, cons[i].attr, &value));
                return update_<type, i + 1>(lib, fun, args..., value);
            } else if constexpr (cons[i].type == Detail::Arg::string) {
                clingo_string_t value;
                Detail::handle_error(clingo_ast_attribute_get_string(ast_, cons[i].attr, &value));
                return update_<type, i + 1>(lib, fun, args..., value.data, value.size);
            } else if constexpr (cons[i].type == Detail::Arg::string_array) {
                clingo_string_t const *value = nullptr;
                size_t size = 0;
                Detail::handle_error(clingo_ast_attribute_get_string_array(ast_, cons[i].attr, &value, &size));
                return update_<type, i + 1>(lib, fun, args..., value, size);
            } else if constexpr (cons[i].type == Detail::Arg::symbol) {
                clingo_symbol_t value = 0;
                Detail::handle_error(clingo_ast_attribute_get_symbol(ast_, cons[i].attr, &value));
                return update_<type, i + 1>(lib, fun, args..., value);
            } else if constexpr (cons[i].type == Detail::Arg::symbol_array) {
                clingo_symbol_t const *value = nullptr;
                size_t size = 0;
                Detail::handle_error(clingo_ast_attribute_get_symbol_array(ast_, cons[i].attr, &value, &size));
                return update_<type, i + 1>(lib, fun, args..., value, size);
            } else if constexpr (cons[i].type == Detail::Arg::node || cons[i].type == Detail::Arg::optional_node) {
                // FIXME: ast is leaked
                clingo_ast_t *value = nullptr;
                Detail::handle_error(clingo_ast_attribute_get_ast(ast_, cons[i].attr, &value));
                return update_<type, i + 1>(lib, fun, args..., value);
            } else if constexpr (cons[i].type == Detail::Arg::node_array) {
                auto arr = Detail::Array{};
                Detail::handle_error(clingo_ast_attribute_get_ast_array(ast_, cons[i].attr, &arr.value, &arr.size));
                return update_<type, i + 1>(lib, fun, args..., arr.value, arr.size);
            } else {
                static_assert(i == cons.size(), "unsupported argument type");
            }
        }
    }

    clingo_ast_t *ast_ = nullptr;
};

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
            assert(scanner_ != nullptr);
            return scanner_->value_.value();
        }

        auto operator->() const -> pointer {
            assert(scanner_ != nullptr);
            return &scanner_->value_.value();
        }

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

        friend auto operator==(iterator const &a, iterator const &b) -> bool {
            assert(a.scanner_ == b.scanner_);
            return true;
        }

        friend auto operator==(iterator const &a, [[maybe_unused]] sentinel const &b) -> bool {
            assert(a.scanner_ != nullptr);
            return a.scanner_->value_.has_value();
        }

      private:
        Scanner *scanner_ = nullptr;
    };
    using difference_type = iterator::difference_type;
    using value_type = iterator::value_type;
    using reference = iterator::reference;
    using pointer = iterator::pointer;

    explicit Scanner(Library lib, std::string_view program) : lib_{std::move(lib)} {
        clingo_ast_scanner_t *scanner = nullptr;
        Detail::handle_error(clingo_ast_scan_string(c_cast(lib_), program.data(), program.size(), &scanner));
        scanner_.reset(scanner);
    }

    explicit Scanner(Library &lib, StringSpan files) : lib_{lib} {
        std::vector<clingo_string_t> cfiles;
        cfiles.reserve(cfiles.size());
        std::ranges::transform(files, std::back_inserter(cfiles),
                               [](auto const &file) { return clingo_string_t{file.data(), file.size()}; });
        clingo_ast_scanner_t *scanner = nullptr;
        Detail::handle_error(clingo_ast_scan_files(c_cast(lib), cfiles.data(), cfiles.size(), &scanner));
        scanner_.reset(scanner);
    }

    explicit Scanner(Library &lib, StringList files) : Scanner{lib, std::span{files}} {}

    auto begin() -> iterator { return iterator{*this}; }

    auto end() -> sentinel {
        static_cast<void>(this);
        return sentinel{};
    }

  private:
    struct Free {
        void operator()(clingo_ast_scanner_t *scanner) const { clingo_ast_scanner_close(scanner); }
    };

    void next_() {
        clingo_ast_t *ast = nullptr;
        Detail::handle_error(clingo_ast_scanner_next(scanner_.get(), &ast));
        if (ast != nullptr) {
            value_.emplace(ast);
        } else {
            value_.reset();
        }
    }

    Library lib_;
    std::unique_ptr<clingo_ast_scanner_t, Free> scanner_;
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
    RewriteContext(Library const &lib) {
        clingo_ast_rewrite_context_t *ctx = nullptr;
        Detail::handle_error(clingo_ast_rewrite_context_create(c_cast(lib), &ctx));
        ctx_.reset(ctx);
    }

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
    struct Free {
        auto operator()(clingo_ast_rewrite_context_t *ctx) { clingo_ast_rewrite_context_free(ctx); }
    };

    std::unique_ptr<clingo_ast_rewrite_context_t, Free> ctx_ = nullptr;
};

auto rewrite(RewriteContext &ctx, Node const &stm) -> std::vector<Node> {
    auto arr = Detail::Array{};
    Detail::handle_error(clingo_ast_rewrite(c_cast(ctx), c_cast(stm), &arr.value, &arr.size));
    return Detail::transform(std::span{arr.value, arr.size},
                             [](auto *&ast) { return Node{std::exchange(ast, nullptr)}; });
}

class Program {
  public:
    Program(Library const &lib) {
        clingo_program_t *prg = nullptr;
        Detail::handle_error(clingo_program_new(c_cast(lib), &prg));
        prg_.reset(prg);
    }

    void add(Node const &stm) { Detail::handle_error(clingo_program_add(prg_.get(), c_cast(stm))); }

    friend auto c_cast(Program const &x) -> clingo_program_t * { return x.prg_.get(); }

  private:
    struct Free {
        void operator()(clingo_program_t *prg) noexcept { clingo_program_free(prg); }
    };

    std::unique_ptr<clingo_program_t, Free> prg_;
};

inline auto parse(Library const &lib, std::string_view string, ParseType type = ParseType::statement) -> Node {
    clingo_ast_t *ast = nullptr;
    Detail::handle_error(clingo_ast_parse_expression(c_cast(lib), static_cast<clingo_ast_parse_type_t>(type),
                                                     string.data(), string.size(), &ast));
    return Node{ast};
}

} // namespace Clingo::AST

namespace std {

template <> struct hash<Clingo::AST::Node> {
    auto operator()(Clingo::AST::Node const &x) const noexcept -> size_t { return x.hash(); }
};

} // namespace std
