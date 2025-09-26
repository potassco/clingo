#pragma once

#include <clingo/core.hh>

#include <clingo/ast.h>

#include <array>
#include <ranges>
#include <span>

namespace Clingo::Detail {

template <typename T, typename V>
concept is_contiguous_range_over =
    std::ranges::contiguous_range<T> && std::is_same_v<std::remove_const_t<std::ranges::range_value_t<T>>, V>;

template <typename T, typename V>
concept is_range_over =
    std::ranges::forward_range<T> && std::is_same_v<std::remove_const_t<std::ranges::range_value_t<T>>, V>;

template <typename T, typename U>
concept not_same_as = !std::same_as<T, U>;

template <class F, auto param>
concept invokable = requires(F const &f) {
    { f.template operator()<param>() } -> not_same_as<void>;
};

inline void join(clingo_control_t *ctl, clingo_program_t const *prg) {
    Detail::handle_error(clingo_control_join(ctl, prg));
}

struct Array {
    ~Array() { clingo_ast_array_free(value, size); }
    clingo_ast_t **value = nullptr;
    size_t size = 0;
};

struct Arg {
    enum class Type { node, node_array, optional_node, integer, symbol, symbol_array, string, string_array, location };
    using enum Type;
    clingo_ast_attribute_t attr;
    Type type;
};

// NOTE: a tuple of arrays would have been the better choice
constexpr auto cons_format_field_literal =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_value, Arg::string}};
constexpr auto cons_format_field_expression =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_left, Arg::node},
               Arg{clingo_ast_attribute_right, Arg::string}};
constexpr auto cons_projection = std::array{Arg{clingo_ast_attribute_location, Arg::location}};
constexpr auto cons_term_variable =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_anonymous, Arg::integer}};
constexpr auto cons_term_format_string =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_elements, Arg::node_array}};
constexpr auto cons_term_symbolic =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_symbol, Arg::symbol}};
constexpr auto cons_term_absolute =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_pool, Arg::node_array}};
constexpr auto cons_term_unary_operation =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_operator_type, Arg::integer},
               Arg{clingo_ast_attribute_right, Arg::node}};
constexpr auto cons_term_binary_operation =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_left, Arg::node},
               Arg{clingo_ast_attribute_operator_type, Arg::integer}, Arg{clingo_ast_attribute_right, Arg::node}};
constexpr auto cons_term_tuple =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_pool, Arg::node_array}};
constexpr auto cons_term_function =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_pool, Arg::node_array}, Arg{clingo_ast_attribute_external, Arg::integer}};
constexpr auto cons_argument_tuple = std::array{Arg{clingo_ast_attribute_arguments, Arg::node_array}};
constexpr auto cons_unparsed_element =
    std::array{Arg{clingo_ast_attribute_operators, Arg::string_array}, Arg{clingo_ast_attribute_term, Arg::node}};
constexpr auto cons_theory_term_variable =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_anonymous, Arg::integer}};
constexpr auto cons_theory_term_symbolic =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_symbol, Arg::symbol}};
constexpr auto cons_theory_term_tuple =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_tuple_type, Arg::integer},
               Arg{clingo_ast_attribute_arguments, Arg::node_array}};
constexpr auto cons_theory_term_function =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_arguments, Arg::node_array}};
constexpr auto cons_theory_term_unparsed =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_elements, Arg::node_array}};
constexpr auto cons_left_guard =
    std::array{Arg{clingo_ast_attribute_term, Arg::node}, Arg{clingo_ast_attribute_relation, Arg::integer}};
constexpr auto cons_right_guard =
    std::array{Arg{clingo_ast_attribute_relation, Arg::integer}, Arg{clingo_ast_attribute_term, Arg::node}};
constexpr auto cons_literal_boolean =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_sign, Arg::integer},
               Arg{clingo_ast_attribute_value, Arg::integer}};
constexpr auto cons_literal_comparison =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_sign, Arg::integer},
               Arg{clingo_ast_attribute_left, Arg::node}, Arg{clingo_ast_attribute_right, Arg::node_array}};
constexpr auto cons_literal_symbolic =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_sign, Arg::integer},
               Arg{clingo_ast_attribute_atom, Arg::node}};
constexpr auto cons_set_aggregate_element =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_literal, Arg::node},
               Arg{clingo_ast_attribute_condition, Arg::node_array}};
constexpr auto cons_theory_atom_element =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_tuple, Arg::node_array},
               Arg{clingo_ast_attribute_condition, Arg::node_array}};
constexpr auto cons_theory_right_guard =
    std::array{Arg{clingo_ast_attribute_theory_operator, Arg::string}, Arg{clingo_ast_attribute_term, Arg::node}};
constexpr auto cons_body_simple_literal = std::array{Arg{clingo_ast_attribute_literal, Arg::node}};
constexpr auto cons_body_aggregate_element =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_tuple, Arg::node_array},
               Arg{clingo_ast_attribute_condition, Arg::node_array}};
constexpr auto cons_body_aggregate = std::array{
    Arg{clingo_ast_attribute_location, Arg::location},   Arg{clingo_ast_attribute_sign, Arg::integer},
    Arg{clingo_ast_attribute_left, Arg::optional_node},  Arg{clingo_ast_attribute_function, Arg::integer},
    Arg{clingo_ast_attribute_elements, Arg::node_array}, Arg{clingo_ast_attribute_right, Arg::optional_node}};
constexpr auto cons_body_set_aggregate =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_sign, Arg::integer},
               Arg{clingo_ast_attribute_left, Arg::optional_node}, Arg{clingo_ast_attribute_elements, Arg::node_array},
               Arg{clingo_ast_attribute_right, Arg::optional_node}};
constexpr auto cons_body_theory_atom =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_sign, Arg::integer},
               Arg{clingo_ast_attribute_name, Arg::node}, Arg{clingo_ast_attribute_elements, Arg::node_array},
               Arg{clingo_ast_attribute_right, Arg::optional_node}};
constexpr auto cons_body_conditional_literal =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_literal, Arg::node},
               Arg{clingo_ast_attribute_condition, Arg::node_array}};
constexpr auto cons_head_simple_literal = std::array{Arg{clingo_ast_attribute_literal, Arg::node}};
constexpr auto cons_head_aggregate_element =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_tuple, Arg::node_array},
               Arg{clingo_ast_attribute_literal, Arg::node}, Arg{clingo_ast_attribute_condition, Arg::node_array}};
constexpr auto cons_head_aggregate =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_left, Arg::optional_node},
               Arg{clingo_ast_attribute_function, Arg::integer}, Arg{clingo_ast_attribute_elements, Arg::node_array},
               Arg{clingo_ast_attribute_right, Arg::optional_node}};
constexpr auto cons_head_set_aggregate = std::array{
    Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_left, Arg::optional_node},
    Arg{clingo_ast_attribute_elements, Arg::node_array}, Arg{clingo_ast_attribute_right, Arg::optional_node}};
constexpr auto cons_head_theory_atom = std::array{
    Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::node},
    Arg{clingo_ast_attribute_elements, Arg::node_array}, Arg{clingo_ast_attribute_right, Arg::optional_node}};
constexpr auto cons_head_conditional_literal =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_literal, Arg::node},
               Arg{clingo_ast_attribute_condition, Arg::node_array}};
constexpr auto cons_head_disjunction =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_elements, Arg::node_array}};
constexpr auto cons_theory_operator_definition =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_priority, Arg::integer}, Arg{clingo_ast_attribute_operator_type, Arg::integer}};
constexpr auto cons_theory_term_definition =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_operators, Arg::node_array}};
constexpr auto cons_theory_guard_definition =
    std::array{Arg{clingo_ast_attribute_operators, Arg::string_array}, Arg{clingo_ast_attribute_term, Arg::string}};
constexpr auto cons_theory_atom_definition =
    std::array{Arg{clingo_ast_attribute_location, Arg::location},   Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_arity, Arg::integer},       Arg{clingo_ast_attribute_term, Arg::string},
               Arg{clingo_ast_attribute_guard, Arg::optional_node}, Arg{clingo_ast_attribute_atom_type, Arg::integer}};
constexpr auto cons_optimize_tuple =
    std::array{Arg{clingo_ast_attribute_weight, Arg::node}, Arg{clingo_ast_attribute_priority, Arg::optional_node},
               Arg{clingo_ast_attribute_terms, Arg::node_array}};
constexpr auto cons_optimize_element =
    std::array{Arg{clingo_ast_attribute_tuple, Arg::node}, Arg{clingo_ast_attribute_condition, Arg::node_array}};
constexpr auto cons_edge = std::array{Arg{clingo_ast_attribute_u, Arg::node}, Arg{clingo_ast_attribute_v, Arg::node}};
constexpr auto cons_program_part =
    std::array{Arg{clingo_ast_attribute_name, Arg::string}, Arg{clingo_ast_attribute_arguments, Arg::symbol_array}};
constexpr auto cons_statement_rule =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_head, Arg::node},
               Arg{clingo_ast_attribute_body, Arg::node_array}};
constexpr auto cons_statement_theory =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_terms, Arg::node_array}, Arg{clingo_ast_attribute_atoms, Arg::node_array}};
constexpr auto cons_statement_optimize =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_elements, Arg::node_array},
               Arg{clingo_ast_attribute_optimize_type, Arg::integer}};
constexpr auto cons_statement_weak_constraint =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_body, Arg::node_array},
               Arg{clingo_ast_attribute_tuple, Arg::node}};
constexpr auto cons_statement_show =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_term, Arg::node},
               Arg{clingo_ast_attribute_body, Arg::node_array}};
constexpr auto cons_statement_show_nothing = std::array{Arg{clingo_ast_attribute_location, Arg::location}};
constexpr auto cons_statement_show_signature =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_arity, Arg::integer}, Arg{clingo_ast_attribute_sign, Arg::integer}};
constexpr auto cons_statement_project =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_atom, Arg::node},
               Arg{clingo_ast_attribute_body, Arg::node_array}};
constexpr auto cons_statement_project_signature =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_arity, Arg::integer}, Arg{clingo_ast_attribute_sign, Arg::integer}};
constexpr auto cons_statement_defined =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_arity, Arg::integer}, Arg{clingo_ast_attribute_sign, Arg::integer}};
constexpr auto cons_statement_external = std::array{
    Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_atom, Arg::node},
    Arg{clingo_ast_attribute_body, Arg::node_array}, Arg{clingo_ast_attribute_external_type, Arg::optional_node}};
constexpr auto cons_statement_edge =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_pool, Arg::node_array},
               Arg{clingo_ast_attribute_body, Arg::node_array}};
constexpr auto cons_statement_heuristic = std::array{
    Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_atom, Arg::node},
    Arg{clingo_ast_attribute_body, Arg::node_array},   Arg{clingo_ast_attribute_weight, Arg::node},
    Arg{clingo_ast_attribute_modifier, Arg::node},     Arg{clingo_ast_attribute_priority, Arg::optional_node}};
constexpr auto cons_statement_script =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_value, Arg::string},
               Arg{clingo_ast_attribute_script_type, Arg::string}};
constexpr auto cons_statement_program =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_arguments, Arg::string_array}};
constexpr auto cons_statement_include =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_value, Arg::string},
               Arg{clingo_ast_attribute_include_type, Arg::integer}};
constexpr auto cons_statement_const =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_name, Arg::string},
               Arg{clingo_ast_attribute_value, Arg::node}, Arg{clingo_ast_attribute_precedence, Arg::integer}};
constexpr auto cons_statement_parts =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_elements, Arg::node_array},
               Arg{clingo_ast_attribute_precedence, Arg::integer}};
constexpr auto cons_statement_comment =
    std::array{Arg{clingo_ast_attribute_location, Arg::location}, Arg{clingo_ast_attribute_value, Arg::string},
               Arg{clingo_ast_attribute_comment_type, Arg::integer}};

constexpr auto cons = std::array{
    std::span{cons_format_field_literal.data(), cons_format_field_literal.size()},
    std::span{cons_format_field_expression.data(), cons_format_field_expression.size()},
    std::span{cons_projection.data(), cons_projection.size()},
    std::span{cons_term_variable.data(), cons_term_variable.size()},
    std::span{cons_term_symbolic.data(), cons_term_symbolic.size()},
    std::span{cons_term_format_string.data(), cons_term_format_string.size()},
    std::span{cons_term_absolute.data(), cons_term_absolute.size()},
    std::span{cons_term_unary_operation.data(), cons_term_unary_operation.size()},
    std::span{cons_term_binary_operation.data(), cons_term_binary_operation.size()},
    std::span{cons_term_tuple.data(), cons_term_tuple.size()},
    std::span{cons_term_function.data(), cons_term_function.size()},
    std::span{cons_argument_tuple.data(), cons_argument_tuple.size()},
    std::span{cons_unparsed_element.data(), cons_unparsed_element.size()},
    std::span{cons_theory_term_variable.data(), cons_theory_term_variable.size()},
    std::span{cons_theory_term_symbolic.data(), cons_theory_term_symbolic.size()},
    std::span{cons_theory_term_tuple.data(), cons_theory_term_tuple.size()},
    std::span{cons_theory_term_function.data(), cons_theory_term_function.size()},
    std::span{cons_theory_term_unparsed.data(), cons_theory_term_unparsed.size()},
    std::span{cons_left_guard.data(), cons_left_guard.size()},
    std::span{cons_right_guard.data(), cons_right_guard.size()},
    std::span{cons_literal_boolean.data(), cons_literal_boolean.size()},
    std::span{cons_literal_comparison.data(), cons_literal_comparison.size()},
    std::span{cons_literal_symbolic.data(), cons_literal_symbolic.size()},
    std::span{cons_set_aggregate_element.data(), cons_set_aggregate_element.size()},
    std::span{cons_theory_atom_element.data(), cons_theory_atom_element.size()},
    std::span{cons_theory_right_guard.data(), cons_theory_right_guard.size()},
    std::span{cons_body_simple_literal.data(), cons_body_simple_literal.size()},
    std::span{cons_body_aggregate_element.data(), cons_body_aggregate_element.size()},
    std::span{cons_body_aggregate.data(), cons_body_aggregate.size()},
    std::span{cons_body_set_aggregate.data(), cons_body_set_aggregate.size()},
    std::span{cons_body_theory_atom.data(), cons_body_theory_atom.size()},
    std::span{cons_body_conditional_literal.data(), cons_body_conditional_literal.size()},
    std::span{cons_head_simple_literal.data(), cons_head_simple_literal.size()},
    std::span{cons_head_aggregate_element.data(), cons_head_aggregate_element.size()},
    std::span{cons_head_aggregate.data(), cons_head_aggregate.size()},
    std::span{cons_head_set_aggregate.data(), cons_head_set_aggregate.size()},
    std::span{cons_head_theory_atom.data(), cons_head_theory_atom.size()},
    std::span{cons_head_conditional_literal.data(), cons_head_conditional_literal.size()},
    std::span{cons_head_disjunction.data(), cons_head_disjunction.size()},
    std::span{cons_theory_operator_definition.data(), cons_theory_operator_definition.size()},
    std::span{cons_theory_term_definition.data(), cons_theory_term_definition.size()},
    std::span{cons_theory_guard_definition.data(), cons_theory_guard_definition.size()},
    std::span{cons_theory_atom_definition.data(), cons_theory_atom_definition.size()},
    std::span{cons_optimize_tuple.data(), cons_optimize_tuple.size()},
    std::span{cons_optimize_element.data(), cons_optimize_element.size()},
    std::span{cons_edge.data(), cons_edge.size()},
    std::span{cons_program_part.data(), cons_program_part.size()},
    std::span{cons_statement_rule.data(), cons_statement_rule.size()},
    std::span{cons_statement_theory.data(), cons_statement_theory.size()},
    std::span{cons_statement_optimize.data(), cons_statement_optimize.size()},
    std::span{cons_statement_weak_constraint.data(), cons_statement_weak_constraint.size()},
    std::span{cons_statement_show.data(), cons_statement_show.size()},
    std::span{cons_statement_show_nothing.data(), cons_statement_show_nothing.size()},
    std::span{cons_statement_show_signature.data(), cons_statement_show_signature.size()},
    std::span{cons_statement_project.data(), cons_statement_project.size()},
    std::span{cons_statement_project_signature.data(), cons_statement_project_signature.size()},
    std::span{cons_statement_defined.data(), cons_statement_defined.size()},
    std::span{cons_statement_external.data(), cons_statement_external.size()},
    std::span{cons_statement_edge.data(), cons_statement_edge.size()},
    std::span{cons_statement_heuristic.data(), cons_statement_heuristic.size()},
    std::span{cons_statement_script.data(), cons_statement_script.size()},
    std::span{cons_statement_program.data(), cons_statement_program.size()},
    std::span{cons_statement_include.data(), cons_statement_include.size()},
    std::span{cons_statement_const.data(), cons_statement_const.size()},
    std::span{cons_statement_parts.data(), cons_statement_parts.size()},
    std::span{cons_statement_comment.data(), cons_statement_comment.size()},
};

} // namespace Clingo::Detail
