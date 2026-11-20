// The description of the structure of ASTs (see clingo_ast_info()).
//
// NOTE: Records are indexed by clingo_ast_type_e and their arguments are listed
// in the order expected by clingo_ast_construct().

#include <clingo/ast.h>

#include <array>
#include <cstddef>

namespace {

using A = clingo_ast_argument_info_t;
using E = clingo_ast_enum_value_info_t;

enum class EnumIndex : uint8_t {
    enum_unary_operator,
    enum_binary_operator,
    enum_sign,
    enum_relation,
    enum_aggregate_function,
    enum_theory_operator_type,
    enum_theory_tuple_type,
    enum_theory_atom_type,
    enum_optimize_type,
    enum_include_type,
    enum_precedence,
    enum_comment_type,
};

enum class UnionIndex : uint8_t {
    union_term,
    union_format_field,
    union_term_or_projection,
    union_term_or_argument_tuple,
    union_literal,
    union_theory_term,
    union_body_literal,
    union_disjunction_element,
    union_head_literal,
    union_statement,
};

constexpr auto none = clingo_ast_ref_t{clingo_ast_ref_type_none, 0};

constexpr auto enum_ref(EnumIndex index) -> clingo_ast_ref_t {
    return {clingo_ast_ref_type_enum, static_cast<size_t>(index)};
}

constexpr auto union_ref(UnionIndex index) -> clingo_ast_ref_t {
    return {clingo_ast_ref_type_union, static_cast<size_t>(index)};
}

constexpr auto record_ref(clingo_ast_type_e type) -> clingo_ast_ref_t {
    return {clingo_ast_ref_type_record, static_cast<size_t>(type)};
}

// enumerations

constexpr auto unary_operator_values = std::array{
    E{"minus", 0, "The unary minus `-`."},
    E{"negation", 1, "The bitwise negation `~`."},
};

constexpr auto binary_operator_values = std::array{
    E{"and", 0, "The bitwise and `&`."},
    E{"division", 1, "The integer division `/`."},
    E{"minus", 2, "The subtraction `-`."},
    E{"modulo", 3, "The modulo operation `\\`."},
    E{"multiplication", 4, "The multiplication `*`."},
    E{"or", 5, "The bitwise or `?`."},
    E{"plus", 6, "The addition `+`."},
    E{"power", 7, "The exponentiation `**`."},
    E{"xor", 8, "The bitwise exclusive or `^`."},
    E{"interval", 9, "The interval `..`."},
};

constexpr auto sign_values = std::array{
    E{"no_sign", 0, "No sign."},
    E{"single", 1, "Default negation `not`."},
    E{"double", 2, "Double default negation `not not`."},
};

constexpr auto relation_values = std::array{
    E{"equal", 0, "The relation `=`."},   E{"not_equal", 1, "The relation `!=`."},
    E{"less", 2, "The relation `<`."},    E{"less_equal", 3, "The relation `<=`."},
    E{"greater", 4, "The relation `>`."}, E{"greater_equal", 5, "The relation `>=`."},
};

constexpr auto aggregate_function_values = std::array{
    E{"count", 0, "The `#count` function."}, E{"sum", 1, "The `#sum` function."}, E{"sump", 2, "The `#sum+` function."},
    E{"min", 3, "The `#min` function."},     E{"max", 4, "The `#max` function."},
};

constexpr auto theory_operator_type_values = std::array{
    E{"unary", 0, "A unary operator."},
    E{"binary_left", 1, "A left-associative binary operator."},
    E{"binary_right", 2, "A right-associative binary operator."},
};

constexpr auto theory_tuple_type_values = std::array{
    E{"tuple", 0, "A tuple `(t1,...,tn)`."},
    E{"set", 1, "A set `{t1,...,tn}`."},
    E{"list", 2, "A list `[t1,...,tn]`."},
};

constexpr auto theory_atom_type_values = std::array{
    E{"head", 0, "A theory atom that may only occur in rule heads."},
    E{"body", 1, "A theory atom that may only occur in rule bodies."},
    E{"any", 2, "A theory atom that may occur in rule heads and bodies."},
    E{"directive", 3, "A theory atom that may only occur in the head of a rule with an empty body."},
};

constexpr auto optimize_type_values = std::array{
    E{"minimize", 0, "A `#minimize` statement."},
    E{"maximize", 1, "A `#maximize` statement."},
};

constexpr auto include_type_values = std::array{
    E{"system", 0, "An include of a file, for example `#include \"file.lp\".`"},
    E{"inbuild", 1, "An include of a built-in program, for example `#include <incmode>.`"},
};

constexpr auto precedence_values = std::array{
    E{"default", 0, "A definition providing a default value (`[default]`)."},
    E{"override", 1, "A definition overriding a default value (`[override]`)."},
};

constexpr auto comment_type_values = std::array{
    E{"line", 0, "A line comment starting with `%`."},
    E{"block", 1, "A block comment enclosed in `%*` and `*%`."},
};

constexpr auto enums = std::array{
    clingo_ast_enum_info_t{
        "unary_operator",
        "terms",
        "Enumeration of unary operators.",
        unary_operator_values.data(),
        unary_operator_values.size(),
    },
    clingo_ast_enum_info_t{
        "binary_operator",
        "terms",
        "Enumeration of binary operators.",
        binary_operator_values.data(),
        binary_operator_values.size(),
    },
    clingo_ast_enum_info_t{
        "sign",
        "literals",
        "Enumeration of signs of literals.",
        sign_values.data(),
        sign_values.size(),
    },
    clingo_ast_enum_info_t{
        "relation",
        "literals",
        "Enumeration of comparison relations.",
        relation_values.data(),
        relation_values.size(),
    },
    clingo_ast_enum_info_t{
        "aggregate_function",
        "aggregates",
        "Enumeration of aggregate functions.",
        aggregate_function_values.data(),
        aggregate_function_values.size(),
    },
    clingo_ast_enum_info_t{
        "theory_operator_type",
        "statements",
        "Enumeration of theory operator types.",
        theory_operator_type_values.data(),
        theory_operator_type_values.size(),
    },
    clingo_ast_enum_info_t{
        "theory_tuple_type",
        "theory",
        "Enumeration of theory tuple types.",
        theory_tuple_type_values.data(),
        theory_tuple_type_values.size(),
    },
    clingo_ast_enum_info_t{
        "theory_atom_type",
        "statements",
        "Enumeration of theory atom types.",
        theory_atom_type_values.data(),
        theory_atom_type_values.size(),
    },
    clingo_ast_enum_info_t{
        "optimize_type",
        "statements",
        "Enumeration of optimization types.",
        optimize_type_values.data(),
        optimize_type_values.size(),
    },
    clingo_ast_enum_info_t{
        "include_type",
        "statements",
        "Enumeration of include types.",
        include_type_values.data(),
        include_type_values.size(),
    },
    clingo_ast_enum_info_t{
        "precedence",
        "statements",
        "Enumeration of precedences of definitions.",
        precedence_values.data(),
        precedence_values.size(),
    },
    clingo_ast_enum_info_t{
        "comment_type",
        "statements",
        "Enumeration of comment types.",
        comment_type_values.data(),
        comment_type_values.size(),
    },
};

// unions

constexpr auto term_types = std::array{
    record_ref(clingo_ast_type_term_variable),         record_ref(clingo_ast_type_term_symbolic),
    record_ref(clingo_ast_type_term_absolute),         record_ref(clingo_ast_type_term_unary_operation),
    record_ref(clingo_ast_type_term_binary_operation), record_ref(clingo_ast_type_term_tuple),
    record_ref(clingo_ast_type_term_function),         record_ref(clingo_ast_type_term_format_string),
};

constexpr auto format_field_types = std::array{
    record_ref(clingo_ast_type_format_field_literal),
    record_ref(clingo_ast_type_format_field_expression),
};

constexpr auto term_or_projection_types = std::array{
    union_ref(UnionIndex::union_term),
    record_ref(clingo_ast_type_projection),
};

constexpr auto term_or_argument_tuple_types = std::array{
    union_ref(UnionIndex::union_term),
    record_ref(clingo_ast_type_argument_tuple),
};

constexpr auto literal_types = std::array{
    record_ref(clingo_ast_type_literal_boolean),
    record_ref(clingo_ast_type_literal_comparison),
    record_ref(clingo_ast_type_literal_symbolic),
};

constexpr auto theory_term_types = std::array{
    record_ref(clingo_ast_type_theory_term_variable), record_ref(clingo_ast_type_theory_term_symbolic),
    record_ref(clingo_ast_type_theory_term_tuple),    record_ref(clingo_ast_type_theory_term_function),
    record_ref(clingo_ast_type_theory_term_unparsed),
};

constexpr auto body_literal_types = std::array{
    record_ref(clingo_ast_type_body_simple_literal), record_ref(clingo_ast_type_body_aggregate),
    record_ref(clingo_ast_type_body_sort),           record_ref(clingo_ast_type_body_set_aggregate),
    record_ref(clingo_ast_type_body_theory_atom),    record_ref(clingo_ast_type_body_conditional_literal),
};

constexpr auto disjunction_element_types = std::array{
    union_ref(UnionIndex::union_literal),
    record_ref(clingo_ast_type_head_conditional_literal),
};

constexpr auto head_literal_types = std::array{
    record_ref(clingo_ast_type_head_simple_literal), record_ref(clingo_ast_type_head_aggregate),
    record_ref(clingo_ast_type_head_set_aggregate),  record_ref(clingo_ast_type_head_theory_atom),
    record_ref(clingo_ast_type_head_disjunction),
};

constexpr auto statement_types = std::array{
    record_ref(clingo_ast_type_statement_rule),
    record_ref(clingo_ast_type_statement_theory),
    record_ref(clingo_ast_type_statement_optimize),
    record_ref(clingo_ast_type_statement_weak_constraint),
    record_ref(clingo_ast_type_statement_show),
    record_ref(clingo_ast_type_statement_show_nothing),
    record_ref(clingo_ast_type_statement_show_signature),
    record_ref(clingo_ast_type_statement_project),
    record_ref(clingo_ast_type_statement_project_signature),
    record_ref(clingo_ast_type_statement_defined),
    record_ref(clingo_ast_type_statement_external),
    record_ref(clingo_ast_type_statement_edge),
    record_ref(clingo_ast_type_statement_heuristic),
    record_ref(clingo_ast_type_statement_script),
    record_ref(clingo_ast_type_statement_include),
    record_ref(clingo_ast_type_statement_program),
    record_ref(clingo_ast_type_statement_parts),
    record_ref(clingo_ast_type_statement_const),
    record_ref(clingo_ast_type_statement_comment),
};

constexpr auto unions = std::array{
    clingo_ast_union_info_t{"term", "terms", "The available terms.", term_types.data(), term_types.size()},
    clingo_ast_union_info_t{
        "format_field",
        "terms",
        "The available parts of a format string.",
        format_field_types.data(),
        format_field_types.size(),
    },
    clingo_ast_union_info_t{
        "term_or_projection",
        "terms",
        "A term or a placeholder for a projected argument.",
        term_or_projection_types.data(),
        term_or_projection_types.size(),
    },
    clingo_ast_union_info_t{
        "term_or_argument_tuple",
        "terms",
        "A term or an argument tuple in the pool of a tuple term.",
        term_or_argument_tuple_types.data(),
        term_or_argument_tuple_types.size(),
    },
    clingo_ast_union_info_t{
        "literal",
        "literals",
        "The available literals.",
        literal_types.data(),
        literal_types.size(),
    },
    clingo_ast_union_info_t{
        "theory_term",
        "theory",
        "The available theory terms.",
        theory_term_types.data(),
        theory_term_types.size(),
    },
    clingo_ast_union_info_t{
        "body_literal",
        "body",
        "The available body literals.",
        body_literal_types.data(),
        body_literal_types.size(),
    },
    clingo_ast_union_info_t{
        "disjunction_element",
        "head",
        "An element of a disjunction.",
        disjunction_element_types.data(),
        disjunction_element_types.size(),
    },
    clingo_ast_union_info_t{
        "head_literal",
        "head",
        "The available head literals.",
        head_literal_types.data(),
        head_literal_types.size(),
    },
    clingo_ast_union_info_t{
        "statement",
        "statements",
        "The available statements.",
        statement_types.data(),
        statement_types.size(),
    },
};

// records

constexpr auto format_field_literal_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the part.",
    },
    A{"value", clingo_ast_attribute_value, clingo_ast_value_type_string, none, false, 0, "The text of the part."},
};

constexpr auto format_field_expression_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the part.",
    },
    A{
        "left",
        clingo_ast_attribute_left,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The term to format.",
    },
    A{
        "right",
        clingo_ast_attribute_right,
        clingo_ast_value_type_string,
        none,
        false,
        0,
        "The format specification, for example `:x`, or the empty string.",
    },
};

constexpr auto projection_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the placeholder.",
    },
};

constexpr auto term_variable_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the variable.",
    },
    A{"name", clingo_ast_attribute_name, clingo_ast_value_type_string, none, false, 0, "The name of the variable."},
    A{
        "anonymous",
        clingo_ast_attribute_anonymous,
        clingo_ast_value_type_bool,
        none,
        true,
        0,
        "Whether the variable is anonymous.\n\nAnonymous variables receive a unique name during preprocessing.",
    },
};

constexpr auto term_symbolic_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the symbol.",
    },
    A{"symbol", clingo_ast_attribute_symbol, clingo_ast_value_type_symbol, none, false, 0, "The symbol of the term."},
};

constexpr auto term_format_string_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the format string.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_format_field),
        false,
        0,
        "The parts of the format string.",
    },
};

constexpr auto term_absolute_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the term.",
    },
    A{
        "pool",
        clingo_ast_attribute_pool,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The argument pool of the term.\n\n"
        "If there is more than one element in the pool, the term is unpooled during preprocessing.",
    },
};

constexpr auto term_unary_operation_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the operation.",
    },
    A{
        "operator_type",
        clingo_ast_attribute_operator_type,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_unary_operator),
        false,
        0,
        "The operator of the operation.",
    },
    A{
        "right",
        clingo_ast_attribute_right,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The argument of the operation.",
    },
};

constexpr auto term_binary_operation_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the operation.",
    },
    A{
        "left",
        clingo_ast_attribute_left,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The left argument of the operation.",
    },
    A{
        "operator_type",
        clingo_ast_attribute_operator_type,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_binary_operator),
        false,
        0,
        "The operator of the operation.",
    },
    A{
        "right",
        clingo_ast_attribute_right,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The right argument of the operation.",
    },
};

constexpr auto term_tuple_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the tuple.",
    },
    A{
        "pool",
        clingo_ast_attribute_pool,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_term_or_argument_tuple),
        false,
        0,
        "The argument pool of the tuple.\n\n"
        "Elements are terms or argument tuples. If there is more than one element in the pool, the term is unpooled "
        "during preprocessing.",
    },
};

constexpr auto term_function_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the function.",
    },
    A{"name", clingo_ast_attribute_name, clingo_ast_value_type_string, none, false, 0, "The name of the function."},
    A{
        "pool",
        clingo_ast_attribute_pool,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_argument_tuple),
        false,
        0,
        "The argument pool of the function.\n\n"
        "If there is more than one element in the pool, the term is unpooled during preprocessing.",
    },
    A{
        "external",
        clingo_ast_attribute_external,
        clingo_ast_value_type_bool,
        none,
        true,
        0,
        "Whether the function is an external function written with a leading `@`.",
    },
};

constexpr auto argument_tuple_arguments = std::array{
    A{
        "arguments",
        clingo_ast_attribute_arguments,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_term_or_projection),
        true,
        0,
        "The arguments of the tuple.",
    },
};

constexpr auto unparsed_element_arguments = std::array{
    A{
        "operators",
        clingo_ast_attribute_operators,
        clingo_ast_value_type_string_array,
        none,
        false,
        0,
        "The operators preceding the theory term.",
    },
    A{
        "term",
        clingo_ast_attribute_term,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_theory_term),
        false,
        0,
        "The theory term.",
    },
};

constexpr auto theory_term_variable_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the variable.",
    },
    A{"name", clingo_ast_attribute_name, clingo_ast_value_type_string, none, false, 0, "The name of the variable."},
    A{
        "anonymous",
        clingo_ast_attribute_anonymous,
        clingo_ast_value_type_bool,
        none,
        true,
        0,
        "Whether the variable is anonymous.\n\nAnonymous variables receive a unique name during preprocessing.",
    },
};

constexpr auto theory_term_symbolic_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the symbol.",
    },
    A{"symbol", clingo_ast_attribute_symbol, clingo_ast_value_type_symbol, none, false, 0, "The symbol."},
};

constexpr auto theory_term_tuple_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the tuple.",
    },
    A{
        "tuple_type",
        clingo_ast_attribute_tuple_type,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_theory_tuple_type),
        false,
        0,
        "The kind of the tuple.",
    },
    A{
        "arguments",
        clingo_ast_attribute_arguments,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_theory_term),
        false,
        0,
        "The arguments of the tuple.",
    },
};

constexpr auto theory_term_function_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the function.",
    },
    A{"name", clingo_ast_attribute_name, clingo_ast_value_type_string, none, false, 0, "The name of the function."},
    A{
        "arguments",
        clingo_ast_attribute_arguments,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_theory_term),
        false,
        0,
        "The arguments of the function.",
    },
};

constexpr auto theory_term_unparsed_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the theory term.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_unparsed_element),
        false,
        0,
        "The elements of the theory term.\n\n"
        "There is at least one element and all but the first element have at least one operator.",
    },
};

constexpr auto left_guard_arguments = std::array{
    A{
        "term",
        clingo_ast_attribute_term,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The term of the guard.",
    },
    A{
        "relation",
        clingo_ast_attribute_relation,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_relation),
        false,
        0,
        "The relation of the guard.",
    },
};

constexpr auto right_guard_arguments = std::array{
    A{
        "relation",
        clingo_ast_attribute_relation,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_relation),
        false,
        0,
        "The relation of the guard.",
    },
    A{
        "term",
        clingo_ast_attribute_term,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The term of the guard.",
    },
};

constexpr auto literal_boolean_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the literal.",
    },
    A{
        "sign",
        clingo_ast_attribute_sign,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_sign),
        false,
        0,
        "The sign of the literal.",
    },
    A{
        "value",
        clingo_ast_attribute_value,
        clingo_ast_value_type_bool,
        none,
        false,
        0,
        "The value of the constant.",
    },
};

constexpr auto literal_comparison_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the literal.",
    },
    A{
        "sign",
        clingo_ast_attribute_sign,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_sign),
        false,
        0,
        "The sign of the literal.",
    },
    A{
        "left",
        clingo_ast_attribute_left,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The leftmost term of the comparison.",
    },
    A{
        "right",
        clingo_ast_attribute_right,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_right_guard),
        false,
        0,
        "The guards following the leftmost term.\n\n"
        "The sequence contains at least one guard.",
    },
};

constexpr auto literal_symbolic_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the literal.",
    },
    A{
        "sign",
        clingo_ast_attribute_sign,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_sign),
        false,
        0,
        "The sign of the literal.",
    },
    A{
        "atom",
        clingo_ast_attribute_atom,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The term representing the atom.",
    },
};

constexpr auto set_aggregate_element_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the element.",
    },
    A{
        "literal",
        clingo_ast_attribute_literal,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The literal of the element.",
    },
    A{
        "condition",
        clingo_ast_attribute_condition,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The condition of the element.",
    },
};

constexpr auto theory_atom_element_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the element.",
    },
    A{
        "tuple",
        clingo_ast_attribute_tuple,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_theory_term),
        false,
        0,
        "The theory terms of the element.",
    },
    A{
        "condition",
        clingo_ast_attribute_condition,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The condition of the element.",
    },
};

constexpr auto theory_right_guard_arguments = std::array{
    A{
        "theory_operator",
        clingo_ast_attribute_theory_operator,
        clingo_ast_value_type_string,
        none,
        false,
        0,
        "The theory operator of the guard.",
    },
    A{
        "term",
        clingo_ast_attribute_term,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_theory_term),
        false,
        0,
        "The theory term of the guard.",
    },
};

constexpr auto body_simple_literal_arguments = std::array{
    A{
        "literal",
        clingo_ast_attribute_literal,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The literal.",
    },
};

constexpr auto body_aggregate_element_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the element.",
    },
    A{
        "tuple",
        clingo_ast_attribute_tuple,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The terms of the element.",
    },
    A{
        "condition",
        clingo_ast_attribute_condition,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The condition of the element.",
    },
};

constexpr auto body_aggregate_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the aggregate.",
    },
    A{
        "sign",
        clingo_ast_attribute_sign,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_sign),
        false,
        0,
        "The sign of the aggregate.",
    },
    A{
        "left",
        clingo_ast_attribute_left,
        clingo_ast_value_type_optional_ast,
        record_ref(clingo_ast_type_left_guard),
        false,
        0,
        "The optional left guard of the aggregate.",
    },
    A{
        "function",
        clingo_ast_attribute_function,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_aggregate_function),
        false,
        0,
        "The aggregate function.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_body_aggregate_element),
        false,
        0,
        "The elements of the aggregate.",
    },
    A{
        "right",
        clingo_ast_attribute_right,
        clingo_ast_value_type_optional_ast,
        record_ref(clingo_ast_type_right_guard),
        false,
        0,
        "The optional right guard of the aggregate.",
    },
};

constexpr auto body_set_aggregate_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the aggregate.",
    },
    A{
        "sign",
        clingo_ast_attribute_sign,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_sign),
        false,
        0,
        "The sign of the aggregate.",
    },
    A{
        "left",
        clingo_ast_attribute_left,
        clingo_ast_value_type_optional_ast,
        record_ref(clingo_ast_type_left_guard),
        false,
        0,
        "The optional left guard of the aggregate.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_set_aggregate_element),
        false,
        0,
        "The elements of the aggregate.",
    },
    A{
        "right",
        clingo_ast_attribute_right,
        clingo_ast_value_type_optional_ast,
        record_ref(clingo_ast_type_right_guard),
        false,
        0,
        "The optional right guard of the aggregate.",
    },
};

constexpr auto body_theory_atom_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the theory atom.",
    },
    A{
        "sign",
        clingo_ast_attribute_sign,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_sign),
        false,
        0,
        "The sign of the theory atom.",
    },
    A{
        "name",
        clingo_ast_attribute_name,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The name of the theory atom.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_theory_atom_element),
        false,
        0,
        "The elements of the theory atom.",
    },
    A{
        "right",
        clingo_ast_attribute_right,
        clingo_ast_value_type_optional_ast,
        record_ref(clingo_ast_type_theory_right_guard),
        false,
        0,
        "The optional right guard of the theory atom.",
    },
};

constexpr auto body_conditional_literal_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the conditional literal.",
    },
    A{
        "literal",
        clingo_ast_attribute_literal,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The literal in front of the colon.",
    },
    A{
        "condition",
        clingo_ast_attribute_condition,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The condition following the colon.",
    },
};

constexpr auto body_sort_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the literal.",
    },
    A{
        "left",
        clingo_ast_attribute_left,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The pair of output terms.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_body_aggregate_element),
        false,
        0,
        "The elements of the sort literal.",
    },
};

constexpr auto head_simple_literal_arguments = std::array{
    A{
        "literal",
        clingo_ast_attribute_literal,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The literal.",
    },
};

constexpr auto head_aggregate_element_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the element.",
    },
    A{
        "tuple",
        clingo_ast_attribute_tuple,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The terms of the element.",
    },
    A{
        "literal",
        clingo_ast_attribute_literal,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The literal of the element.",
    },
    A{
        "condition",
        clingo_ast_attribute_condition,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The condition of the element.",
    },
};

constexpr auto head_aggregate_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the aggregate.",
    },
    A{
        "left",
        clingo_ast_attribute_left,
        clingo_ast_value_type_optional_ast,
        record_ref(clingo_ast_type_left_guard),
        false,
        0,
        "The optional left guard of the aggregate.",
    },
    A{
        "function",
        clingo_ast_attribute_function,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_aggregate_function),
        false,
        0,
        "The aggregate function.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_head_aggregate_element),
        false,
        0,
        "The elements of the aggregate.",
    },
    A{
        "right",
        clingo_ast_attribute_right,
        clingo_ast_value_type_optional_ast,
        record_ref(clingo_ast_type_right_guard),
        false,
        0,
        "The optional right guard of the aggregate.",
    },
};

constexpr auto head_set_aggregate_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the aggregate.",
    },
    A{
        "left",
        clingo_ast_attribute_left,
        clingo_ast_value_type_optional_ast,
        record_ref(clingo_ast_type_left_guard),
        false,
        0,
        "The optional left guard of the aggregate.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_set_aggregate_element),
        false,
        0,
        "The elements of the aggregate.",
    },
    A{
        "right",
        clingo_ast_attribute_right,
        clingo_ast_value_type_optional_ast,
        record_ref(clingo_ast_type_right_guard),
        false,
        0,
        "The optional right guard of the aggregate.",
    },
};

constexpr auto head_theory_atom_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the theory atom.",
    },
    A{
        "name",
        clingo_ast_attribute_name,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The name of the theory atom.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_theory_atom_element),
        false,
        0,
        "The elements of the theory atom.",
    },
    A{
        "right",
        clingo_ast_attribute_right,
        clingo_ast_value_type_optional_ast,
        record_ref(clingo_ast_type_theory_right_guard),
        false,
        0,
        "The optional right guard of the theory atom.",
    },
};

constexpr auto head_conditional_literal_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the conditional literal.",
    },
    A{
        "literal",
        clingo_ast_attribute_literal,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The literal in front of the colon.",
    },
    A{
        "condition",
        clingo_ast_attribute_condition,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The condition following the colon.",
    },
};

constexpr auto head_disjunction_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the disjunction.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_disjunction_element),
        false,
        0,
        "The elements of the disjunction.",
    },
};

constexpr auto theory_operator_definition_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the definition.",
    },
    A{"name", clingo_ast_attribute_name, clingo_ast_value_type_string, none, false, 0, "The operator."},
    A{
        "priority",
        clingo_ast_attribute_priority,
        clingo_ast_value_type_number,
        none,
        false,
        0,
        "The priority of the operator.",
    },
    A{
        "operator_type",
        clingo_ast_attribute_operator_type,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_theory_operator_type),
        false,
        0,
        "The type of the operator.",
    },
};

constexpr auto theory_term_definition_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the definition.",
    },
    A{
        "name",
        clingo_ast_attribute_name,
        clingo_ast_value_type_string,
        none,
        false,
        0,
        "The name of the theory term definition.",
    },
    A{
        "operators",
        clingo_ast_attribute_operators,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_theory_operator_definition),
        false,
        0,
        "The operator definitions of the theory term definition.",
    },
};

constexpr auto theory_guard_definition_arguments = std::array{
    A{
        "operators",
        clingo_ast_attribute_operators,
        clingo_ast_value_type_string_array,
        none,
        false,
        0,
        "The theory operators allowed in the guard.",
    },
    A{
        "term",
        clingo_ast_attribute_term,
        clingo_ast_value_type_string,
        none,
        false,
        0,
        "The name of the theory term definition of the guard term.",
    },
};

constexpr auto theory_atom_definition_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the definition.",
    },
    A{"name", clingo_ast_attribute_name, clingo_ast_value_type_string, none, false, 0, "The name of the atom."},
    A{"arity", clingo_ast_attribute_arity, clingo_ast_value_type_number, none, false, 0, "The arity of the atom."},
    A{
        "term",
        clingo_ast_attribute_term,
        clingo_ast_value_type_string,
        none,
        false,
        0,
        "The name of the theory term definition of the elements.",
    },
    A{
        "guard",
        clingo_ast_attribute_guard,
        clingo_ast_value_type_optional_ast,
        record_ref(clingo_ast_type_theory_guard_definition),
        false,
        0,
        "The optional definition of the right guard.",
    },
    A{
        "atom_type",
        clingo_ast_attribute_atom_type,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_theory_atom_type),
        false,
        0,
        "Where the theory atom may occur.",
    },
};

constexpr auto optimize_tuple_arguments = std::array{
    A{
        "weight",
        clingo_ast_attribute_weight,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The weight of the tuple.",
    },
    A{
        "priority",
        clingo_ast_attribute_priority,
        clingo_ast_value_type_optional_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The optional priority of the tuple.",
    },
    A{
        "terms",
        clingo_ast_attribute_terms,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The remaining terms in the tuple.",
    },
};

constexpr auto optimize_element_arguments = std::array{
    A{
        "tuple",
        clingo_ast_attribute_tuple,
        clingo_ast_value_type_ast,
        record_ref(clingo_ast_type_optimize_tuple),
        false,
        0,
        "The tuple of the element.",
    },
    A{
        "condition",
        clingo_ast_attribute_condition,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_literal),
        false,
        0,
        "The condition of the element.",
    },
};

constexpr auto edge_arguments = std::array{
    A{
        "u",
        clingo_ast_attribute_u,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The source vertex.",
    },
    A{
        "v",
        clingo_ast_attribute_v,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The target vertex.",
    },
};

constexpr auto program_part_arguments = std::array{
    A{"name", clingo_ast_attribute_name, clingo_ast_value_type_string, none, false, 0, "The name of the program part."},
    A{
        "arguments",
        clingo_ast_attribute_arguments,
        clingo_ast_value_type_symbol_array,
        none,
        false,
        0,
        "The arguments of the program part.",
    },
};

constexpr auto statement_rule_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "head",
        clingo_ast_attribute_head,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_head_literal),
        false,
        0,
        "The head of the rule.",
    },
    A{
        "body",
        clingo_ast_attribute_body,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_body_literal),
        false,
        0,
        "The body of the rule.",
    },
};

constexpr auto statement_theory_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{"name", clingo_ast_attribute_name, clingo_ast_value_type_string, none, false, 0, "The name of the theory."},
    A{
        "terms",
        clingo_ast_attribute_terms,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_theory_term_definition),
        false,
        0,
        "The theory term definitions.",
    },
    A{
        "atoms",
        clingo_ast_attribute_atoms,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_theory_atom_definition),
        false,
        0,
        "The theory atom definitions.",
    },
};

constexpr auto statement_optimize_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_optimize_element),
        false,
        0,
        "The elements of the statement.",
    },
    A{
        "optimize_type",
        clingo_ast_attribute_optimize_type,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_optimize_type),
        false,
        0,
        "Whether to minimize or maximize.",
    },
};

constexpr auto statement_weak_constraint_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "body",
        clingo_ast_attribute_body,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_body_literal),
        false,
        0,
        "The body of the weak constraint.",
    },
    A{
        "tuple",
        clingo_ast_attribute_tuple,
        clingo_ast_value_type_ast,
        record_ref(clingo_ast_type_optimize_tuple),
        false,
        0,
        "The weight, priority, and terms of the weak constraint.",
    },
};

constexpr auto statement_show_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "term",
        clingo_ast_attribute_term,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The term to show.",
    },
    A{
        "body",
        clingo_ast_attribute_body,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_body_literal),
        false,
        0,
        "The condition of the statement.",
    },
};

constexpr auto statement_show_nothing_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
};

constexpr auto statement_show_signature_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "name",
        clingo_ast_attribute_name,
        clingo_ast_value_type_string,
        none,
        false,
        0,
        "The name of the signature.",
    },
    A{
        "arity",
        clingo_ast_attribute_arity,
        clingo_ast_value_type_number,
        none,
        false,
        0,
        "The arity of the signature.",
    },
    A{
        "sign",
        clingo_ast_attribute_sign,
        clingo_ast_value_type_bool,
        none,
        true,
        0,
        "Whether the signature is classically negated, for example, in `#show -p/1.`",
    },
    A{
        "value",
        clingo_ast_attribute_value,
        clingo_ast_value_type_bool,
        none,
        true,
        1,
        "Whether to show or hide the atoms over the signature.",
    },
};

constexpr auto statement_project_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "atom",
        clingo_ast_attribute_atom,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The atom to project on.",
    },
    A{
        "body",
        clingo_ast_attribute_body,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_body_literal),
        false,
        0,
        "The condition of the statement.",
    },
};

constexpr auto statement_project_signature_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "name",
        clingo_ast_attribute_name,
        clingo_ast_value_type_string,
        none,
        false,
        0,
        "The name of the signature.",
    },
    A{
        "arity",
        clingo_ast_attribute_arity,
        clingo_ast_value_type_number,
        none,
        false,
        0,
        "The arity of the signature.",
    },
    A{
        "sign",
        clingo_ast_attribute_sign,
        clingo_ast_value_type_bool,
        none,
        true,
        0,
        "Whether the signature is classically negated.",
    },
};

constexpr auto statement_defined_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "name",
        clingo_ast_attribute_name,
        clingo_ast_value_type_string,
        none,
        false,
        0,
        "The name of the signature.",
    },
    A{
        "arity",
        clingo_ast_attribute_arity,
        clingo_ast_value_type_number,
        none,
        false,
        0,
        "The arity of the signature.",
    },
    A{
        "sign",
        clingo_ast_attribute_sign,
        clingo_ast_value_type_bool,
        none,
        true,
        0,
        "Whether the signature is classically negated.",
    },
};

constexpr auto statement_external_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "atom",
        clingo_ast_attribute_atom,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The external atom.",
    },
    A{
        "body",
        clingo_ast_attribute_body,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_body_literal),
        false,
        0,
        "The condition of the statement.",
    },
    A{
        "external_type",
        clingo_ast_attribute_external_type,
        clingo_ast_value_type_optional_ast,
        union_ref(UnionIndex::union_term),
        true,
        0,
        "The optional initial value of the external atom, for example, `true` or `free`.",
    },
};

constexpr auto statement_edge_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "pool",
        clingo_ast_attribute_pool,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_edge),
        false,
        0,
        "The pool of edges.",
    },
    A{
        "body",
        clingo_ast_attribute_body,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_body_literal),
        false,
        0,
        "The condition of the statement.",
    },
};

constexpr auto statement_heuristic_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "atom",
        clingo_ast_attribute_atom,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The atom to heuristically modify.",
    },
    A{
        "body",
        clingo_ast_attribute_body,
        clingo_ast_value_type_ast_array,
        union_ref(UnionIndex::union_body_literal),
        false,
        0,
        "The condition of the statement.",
    },
    A{
        "weight",
        clingo_ast_attribute_weight,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The weight of the modification.",
    },
    A{
        "modifier",
        clingo_ast_attribute_modifier,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The modifier, for example, `level` or `sign`.",
    },
    A{
        "priority",
        clingo_ast_attribute_priority,
        clingo_ast_value_type_optional_ast,
        union_ref(UnionIndex::union_term),
        true,
        0,
        "The optional priority of the modification.",
    },
};

constexpr auto statement_script_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{"value", clingo_ast_attribute_value, clingo_ast_value_type_string, none, false, 0, "The code of the script."},
    A{
        "script_type",
        clingo_ast_attribute_script_type,
        clingo_ast_value_type_string,
        none,
        false,
        0,
        "The language of the script, for example, `python` or `lua`.",
    },
};

constexpr auto statement_program_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{"name", clingo_ast_attribute_name, clingo_ast_value_type_string, none, false, 0, "The name of the program part."},
    A{
        "arguments",
        clingo_ast_attribute_arguments,
        clingo_ast_value_type_string_array,
        none,
        false,
        0,
        "The parameters of the program part.",
    },
};

constexpr auto statement_include_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "value",
        clingo_ast_attribute_value,
        clingo_ast_value_type_string,
        none,
        false,
        0,
        "The file or built-in program to include.",
    },
    A{
        "include_type",
        clingo_ast_attribute_include_type,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_include_type),
        false,
        0,
        "Whether a file or a built-in program is included.",
    },
};

constexpr auto statement_const_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{"name", clingo_ast_attribute_name, clingo_ast_value_type_string, none, false, 0, "The name of the constant."},
    A{
        "value",
        clingo_ast_attribute_value,
        clingo_ast_value_type_ast,
        union_ref(UnionIndex::union_term),
        false,
        0,
        "The value of the constant.",
    },
    A{
        "precedence",
        clingo_ast_attribute_precedence,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_precedence),
        false,
        0,
        "Whether the definition provides or overrides a default value.",
    },
};

constexpr auto statement_parts_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the statement.",
    },
    A{
        "elements",
        clingo_ast_attribute_elements,
        clingo_ast_value_type_ast_array,
        record_ref(clingo_ast_type_program_part),
        false,
        0,
        "The program parts to ground.",
    },
    A{
        "precedence",
        clingo_ast_attribute_precedence,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_precedence),
        false,
        0,
        "Whether the statement provides or overrides a default value.",
    },
};

constexpr auto statement_comment_arguments = std::array{
    A{
        "location",
        clingo_ast_attribute_location,
        clingo_ast_value_type_location,
        none,
        false,
        0,
        "The location of the comment.",
    },
    A{
        "value",
        clingo_ast_attribute_value,
        clingo_ast_value_type_string,
        none,
        false,
        0,
        "The text of the comment including the comment markers.",
    },
    A{
        "comment_type",
        clingo_ast_attribute_comment_type,
        clingo_ast_value_type_enum,
        enum_ref(EnumIndex::enum_comment_type),
        false,
        0,
        "The type of the comment.",
    },
};

constexpr auto records = std::array{
    clingo_ast_record_info_t{
        "format_field_literal",
        "terms",
        "A literal part of a format string.\n\n"
        "For example, `a` in `f\"a{X}\"`.",
        format_field_literal_arguments.data(),
        format_field_literal_arguments.size(),
    },
    clingo_ast_record_info_t{
        "format_field_expression",
        "terms",
        "An expression part of a format string.\n\n"
        "For example, `{X:x}` in `f\"a{X:x}\"`.",
        format_field_expression_arguments.data(),
        format_field_expression_arguments.size(),
    },
    clingo_ast_record_info_t{
        "projection",
        "terms",
        "A placeholder for a projected argument.\n\n"
        "Projected arguments are written as `*`, for example, in `p(X,*)`.",
        projection_arguments.data(),
        projection_arguments.size(),
    },
    clingo_ast_record_info_t{
        "term_variable",
        "terms",
        "A variable term.\n\n"
        "For example, `X` or the anonymous variable `_`.",
        term_variable_arguments.data(),
        term_variable_arguments.size(),
    },
    clingo_ast_record_info_t{
        "term_symbolic",
        "terms",
        "A symbolic term.\n\n"
        "For example, `1`, `a`, or `\"s\"`.",
        term_symbolic_arguments.data(),
        term_symbolic_arguments.size(),
    },
    clingo_ast_record_info_t{
        "term_format_string",
        "terms",
        "A format string term.\n\n"
        "For example, `f\"a{X:x}b\"`.",
        term_format_string_arguments.data(),
        term_format_string_arguments.size(),
    },
    clingo_ast_record_info_t{
        "term_absolute",
        "terms",
        "An absolute value term.\n\n"
        "For example, `|X-Y|`.",
        term_absolute_arguments.data(),
        term_absolute_arguments.size(),
    },
    clingo_ast_record_info_t{
        "term_unary_operation",
        "terms",
        "A unary operation.\n\n"
        "For example, `-X`.",
        term_unary_operation_arguments.data(),
        term_unary_operation_arguments.size(),
    },
    clingo_ast_record_info_t{
        "term_binary_operation",
        "terms",
        "A binary operation.\n\n"
        "For example, `X+1` or the interval `1..3`.",
        term_binary_operation_arguments.data(),
        term_binary_operation_arguments.size(),
    },
    clingo_ast_record_info_t{
        "term_tuple",
        "terms",
        "A tuple term.\n\n"
        "For example, `(a,b)` or the pool `(a;b,c)`.",
        term_tuple_arguments.data(),
        term_tuple_arguments.size(),
    },
    clingo_ast_record_info_t{
        "term_function",
        "terms",
        "A function term.\n\n"
        "For example, `f(X,a)`, the constant `a`, or the external function `@f(X)`.",
        term_function_arguments.data(),
        term_function_arguments.size(),
    },
    clingo_ast_record_info_t{
        "argument_tuple",
        "terms",
        "A tuple of arguments in a pool.\n\n"
        "For example, `a,b` in `f(a,b;c)`.",
        argument_tuple_arguments.data(),
        argument_tuple_arguments.size(),
    },
    clingo_ast_record_info_t{
        "unparsed_element",
        "theory",
        "An element of an unparsed theory term.\n\n"
        "It consists of a sequence of operators followed by a theory term.",
        unparsed_element_arguments.data(),
        unparsed_element_arguments.size(),
    },
    clingo_ast_record_info_t{
        "theory_term_variable",
        "theory",
        "A theory term representing a variable.\n\n"
        "For example, `X`.",
        theory_term_variable_arguments.data(),
        theory_term_variable_arguments.size(),
    },
    clingo_ast_record_info_t{
        "theory_term_symbolic",
        "theory",
        "A theory term representing a symbol.\n\n"
        "For example, `1` or `a`.",
        theory_term_symbolic_arguments.data(),
        theory_term_symbolic_arguments.size(),
    },
    clingo_ast_record_info_t{
        "theory_term_tuple",
        "theory",
        "A theory term representing a tuple, set, or list.\n\n"
        "For example, `(a,b)`, `{a,b}`, or `[a,b]`.",
        theory_term_tuple_arguments.data(),
        theory_term_tuple_arguments.size(),
    },
    clingo_ast_record_info_t{
        "theory_term_function",
        "theory",
        "A theory term representing a function.\n\n"
        "For example, `f(X,a)`.",
        theory_term_function_arguments.data(),
        theory_term_function_arguments.size(),
    },
    clingo_ast_record_info_t{
        "theory_term_unparsed",
        "theory",
        "A theory term whose operators have not been parsed yet.\n\n"
        "The priorities and associativities of the operators have not been applied yet; the elements are simply stored "
        "as a list. For example, `- X + Y * Z`.",
        theory_term_unparsed_arguments.data(),
        theory_term_unparsed_arguments.size(),
    },
    clingo_ast_record_info_t{
        "left_guard",
        "literals",
        "A guard on the left-hand side of an aggregate.\n\n"
        "For example, `1 <` in `1 < #count { X: p(X) }`.",
        left_guard_arguments.data(),
        left_guard_arguments.size(),
    },
    clingo_ast_record_info_t{
        "right_guard",
        "literals",
        "A guard on the right-hand side of a comparison or aggregate.\n\n"
        "For example, `< 3` in `X < 3`.",
        right_guard_arguments.data(),
        right_guard_arguments.size(),
    },
    clingo_ast_record_info_t{
        "literal_boolean",
        "literals",
        "A literal representing a Boolean constant.\n\n"
        "For example, `#true` or `not #false`.",
        literal_boolean_arguments.data(),
        literal_boolean_arguments.size(),
    },
    clingo_ast_record_info_t{
        "literal_comparison",
        "literals",
        "A literal representing a (chain of) comparison(s).\n\n"
        "For example, `X < Y` or `1 <= X < 10`.",
        literal_comparison_arguments.data(),
        literal_comparison_arguments.size(),
    },
    clingo_ast_record_info_t{
        "literal_symbolic",
        "literals",
        "A literal representing an atom.\n\n"
        "For example, `p(X)` or `not -q`.",
        literal_symbolic_arguments.data(),
        literal_symbolic_arguments.size(),
    },
    clingo_ast_record_info_t{
        "set_aggregate_element",
        "aggregates",
        "An element of a set aggregate.\n\n"
        "For example, `p(X): q(X)` in `{ p(X): q(X) }`.",
        set_aggregate_element_arguments.data(),
        set_aggregate_element_arguments.size(),
    },
    clingo_ast_record_info_t{
        "theory_atom_element",
        "theory",
        "An element of a theory atom.\n\n"
        "For example, `X: p(X)` in `&sum { X: p(X) } >= 0`.",
        theory_atom_element_arguments.data(),
        theory_atom_element_arguments.size(),
    },
    clingo_ast_record_info_t{
        "theory_right_guard",
        "theory",
        "The right guard of a theory atom.\n\n"
        "For example, `>= 0` in `&sum { X: p(X) } >= 0`.",
        theory_right_guard_arguments.data(),
        theory_right_guard_arguments.size(),
    },
    clingo_ast_record_info_t{
        "body_simple_literal",
        "body",
        "A literal in a rule body.\n\n"
        "For example, `not p(X)`.",
        body_simple_literal_arguments.data(),
        body_simple_literal_arguments.size(),
    },
    clingo_ast_record_info_t{
        "body_aggregate_element",
        "body",
        "An element of a body aggregate.\n\n"
        "For example, `X: p(X)` in `#sum { X: p(X) } > 2`.",
        body_aggregate_element_arguments.data(),
        body_aggregate_element_arguments.size(),
    },
    clingo_ast_record_info_t{
        "body_aggregate",
        "body",
        "An aggregate in a rule body.\n\n"
        "For example, `not #sum { X: p(X) } > 2`.",
        body_aggregate_arguments.data(),
        body_aggregate_arguments.size(),
    },
    clingo_ast_record_info_t{
        "body_set_aggregate",
        "body",
        "A set aggregate in a rule body.\n\n"
        "For example, `1 { p(X): q(X) } 2`.",
        body_set_aggregate_arguments.data(),
        body_set_aggregate_arguments.size(),
    },
    clingo_ast_record_info_t{
        "body_theory_atom",
        "body",
        "A theory atom in a rule body.\n\n"
        "For example, `&sum { X: p(X) } >= 0`.",
        body_theory_atom_arguments.data(),
        body_theory_atom_arguments.size(),
    },
    clingo_ast_record_info_t{
        "body_conditional_literal",
        "body",
        "A conditional literal in a rule body.\n\n"
        "For example, `p(X): q(X)`.",
        body_conditional_literal_arguments.data(),
        body_conditional_literal_arguments.size(),
    },
    clingo_ast_record_info_t{
        "body_sort",
        "body",
        "A sort literal in a rule body.\n\n"
        "For example, `(X,Y) = #sort { Z: q(Z) }`.",
        body_sort_arguments.data(),
        body_sort_arguments.size(),
    },
    clingo_ast_record_info_t{
        "head_simple_literal",
        "head",
        "A literal in a rule head.\n\n"
        "For example, `p(X)`.",
        head_simple_literal_arguments.data(),
        head_simple_literal_arguments.size(),
    },
    clingo_ast_record_info_t{
        "head_aggregate_element",
        "head",
        "An element of a head aggregate.\n\n"
        "For example, `X: p(X): q(X)` in `#count { X: p(X): q(X) } = 1`.",
        head_aggregate_element_arguments.data(),
        head_aggregate_element_arguments.size(),
    },
    clingo_ast_record_info_t{
        "head_aggregate",
        "head",
        "An aggregate in a rule head.\n\n"
        "For example, `#count { X: p(X): q(X) } = 1`.",
        head_aggregate_arguments.data(),
        head_aggregate_arguments.size(),
    },
    clingo_ast_record_info_t{
        "head_set_aggregate",
        "head",
        "A set aggregate in a rule head.\n\n"
        "Set aggregates in heads are also known as choice rules. For example, `1 { p(X): q(X) } 2`.",
        head_set_aggregate_arguments.data(),
        head_set_aggregate_arguments.size(),
    },
    clingo_ast_record_info_t{
        "head_theory_atom",
        "head",
        "A theory atom in a rule head.\n\n"
        "For example, `&diff { X-Y } <= 3`.",
        head_theory_atom_arguments.data(),
        head_theory_atom_arguments.size(),
    },
    clingo_ast_record_info_t{
        "head_conditional_literal",
        "head",
        "A conditional literal in a disjunction.\n\n"
        "For example, `p(X): q(X)` in `p(X): q(X); r`.",
        head_conditional_literal_arguments.data(),
        head_conditional_literal_arguments.size(),
    },
    clingo_ast_record_info_t{
        "head_disjunction",
        "head",
        "A disjunction in a rule head.\n\n"
        "For example, `p; q(X): r(X)`. Rules with an empty disjunction as head are integrity constraints.",
        head_disjunction_arguments.data(),
        head_disjunction_arguments.size(),
    },
    clingo_ast_record_info_t{
        "theory_operator_definition",
        "statements",
        "A theory operator definition.\n\n"
        "For example, `- : 0, unary`.",
        theory_operator_definition_arguments.data(),
        theory_operator_definition_arguments.size(),
    },
    clingo_ast_record_info_t{
        "theory_term_definition",
        "statements",
        "A theory term definition.\n\n"
        "For example, `term { - : 0, unary }`.",
        theory_term_definition_arguments.data(),
        theory_term_definition_arguments.size(),
    },
    clingo_ast_record_info_t{
        "theory_guard_definition",
        "statements",
        "A definition of the right guard of theory atoms.\n\n"
        "For example, `{<=, >=}, term` in `&sum/0: term, {<=, >=}, term, body`.",
        theory_guard_definition_arguments.data(),
        theory_guard_definition_arguments.size(),
    },
    clingo_ast_record_info_t{
        "theory_atom_definition",
        "statements",
        "A theory atom definition.\n\n"
        "For example, `&sum/0: term, {<=, >=}, term, body`.",
        theory_atom_definition_arguments.data(),
        theory_atom_definition_arguments.size(),
    },
    clingo_ast_record_info_t{
        "optimize_tuple",
        "statements",
        "The weight, priority, and terms of an optimization element.\n\n"
        "For example, `X@2,Y` in `#minimize { X@2,Y: p(X,Y) }`.",
        optimize_tuple_arguments.data(),
        optimize_tuple_arguments.size(),
    },
    clingo_ast_record_info_t{
        "optimize_element",
        "statements",
        "An element of an optimization statement.\n\n"
        "For example, `X@2,Y: p(X,Y)`.",
        optimize_element_arguments.data(),
        optimize_element_arguments.size(),
    },
    clingo_ast_record_info_t{
        "edge",
        "statements",
        "An edge of an edge statement.\n\n"
        "For example, `X,Y` in `#edge (X,Y): e(X,Y).`",
        edge_arguments.data(),
        edge_arguments.size(),
    },
    clingo_ast_record_info_t{
        "program_part",
        "statements",
        "A program part to ground.\n\n"
        "For example, `step(1)` in `#parts step(1).`",
        program_part_arguments.data(),
        program_part_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_rule",
        "statements",
        "A rule.\n\n"
        "For example, `p(X) :- q(X).` Facts have an empty body and integrity constraints an empty disjunction as head.",
        statement_rule_arguments.data(),
        statement_rule_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_theory",
        "statements",
        "A theory definition.\n\n"
        "For example, `#theory t { term { - : 0, unary }; &a/0: term, body }.`",
        statement_theory_arguments.data(),
        statement_theory_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_optimize",
        "statements",
        "An optimization statement.\n\n"
        "For example, `#minimize { X@2,Y: p(X,Y) }.`",
        statement_optimize_arguments.data(),
        statement_optimize_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_weak_constraint",
        "statements",
        "A weak constraint.\n\n"
        "For example, `:~ p(X). [X@1]`.",
        statement_weak_constraint_arguments.data(),
        statement_weak_constraint_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_show",
        "statements",
        "A show statement for terms.\n\n"
        "For example, `#show X: p(X).`",
        statement_show_arguments.data(),
        statement_show_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_show_nothing",
        "statements",
        "A show statement hiding all atoms.\n\n"
        "For example, `#show.`",
        statement_show_nothing_arguments.data(),
        statement_show_nothing_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_show_signature",
        "statements",
        "A show statement for a signature.\n\n"
        "For example, `#show p/1.`",
        statement_show_signature_arguments.data(),
        statement_show_signature_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_project",
        "statements",
        "A project statement for atoms.\n\n"
        "For example, `#project p(X): q(X).`",
        statement_project_arguments.data(),
        statement_project_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_project_signature",
        "statements",
        "A project statement for a signature.\n\n"
        "For example, `#project p/1.`",
        statement_project_signature_arguments.data(),
        statement_project_signature_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_defined",
        "statements",
        "A statement declaring a predicate as defined.\n\n"
        "For example, `#defined p/1.`",
        statement_defined_arguments.data(),
        statement_defined_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_external",
        "statements",
        "An external statement.\n\n"
        "For example, `#external p(X): q(X). [true]`.",
        statement_external_arguments.data(),
        statement_external_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_edge",
        "statements",
        "An edge statement.\n\n"
        "For example, `#edge (X,Y): e(X,Y).`",
        statement_edge_arguments.data(),
        statement_edge_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_heuristic",
        "statements",
        "A heuristic statement.\n\n"
        "For example, `#heuristic p(X): q(X). [X@1,level]`.",
        statement_heuristic_arguments.data(),
        statement_heuristic_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_script",
        "statements",
        "A script statement.\n\n"
        "For example, `#script (python) ... #end.`",
        statement_script_arguments.data(),
        statement_script_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_program",
        "statements",
        "A program statement.\n\n"
        "For example, `#program step(t).`",
        statement_program_arguments.data(),
        statement_program_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_include",
        "statements",
        "An include statement.\n\n"
        "For example, `#include \"file.lp\".` or `#include <incmode>.`",
        statement_include_arguments.data(),
        statement_include_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_const",
        "statements",
        "A constant definition.\n\n"
        "For example, `#const n=10.`",
        statement_const_arguments.data(),
        statement_const_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_parts",
        "statements",
        "A statement selecting the program parts to ground.\n\n"
        "For example, `#parts base, step(1).`",
        statement_parts_arguments.data(),
        statement_parts_arguments.size(),
    },
    clingo_ast_record_info_t{
        "statement_comment",
        "statements",
        "A comment.\n\n"
        "For example, `% comment` or `%* comment *%`.",
        statement_comment_arguments.data(),
        statement_comment_arguments.size(),
    },
};

static_assert(records.size() == clingo_ast_type_statement_comment + 1);

constexpr auto info = clingo_ast_info_t{
    records.data(), records.size(), unions.data(), unions.size(), enums.data(), enums.size(),
};

} // namespace

extern "C" auto clingo_ast_info() -> clingo_ast_info_t const * {
    return &info;
}
