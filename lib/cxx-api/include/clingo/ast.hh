#pragma once

#include <clingo/core.hh>
#include <clingo/symbol.hh>

#include <clingo/ast.h>

namespace Clingo {

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

class Node {
  public:
    [[nodiscard]] auto get_number(Attribute attribute) const -> int {
        int value = 0;
        Detail::handle_error(
            clingo_ast_attribute_get_number(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value));
        return value;
    }

    [[nodiscard]] auto get_symbol(Attribute attribute) const -> Symbol {
        clingo_symbol_t value = 0;
        Detail::handle_error(
            clingo_ast_attribute_get_symbol(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value));
        return Symbol{value, true};
    }

    /*
    [[nodiscard]] auto get_location(Attribute attribute) const -> Location {
        clingo_location_t const *value = nullptr;
        Detail::handle_error(
            clingo_ast_attribute_get_location(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value));
        return Location{value};
    }
    */

    [[nodiscard]] auto get_string(Attribute attribute) const -> std::string_view {
        clingo_string_t value;
        Detail::handle_error(
            clingo_ast_attribute_get_string(ast_, static_cast<clingo_ast_attribute_t>(attribute), &value));
        return {value.data, value.size};
    }

  private:
    clingo_ast_t *ast_;
};

} // namespace Clingo
