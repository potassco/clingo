#include "lib.hh"

#include <gringo/input/algo/parse.hh>

static_assert(clingo_ast_theory_sequence_type_tuple ==
              static_cast<clingo_ast_theory_sequence_type_e>(Gringo::Input::TheoryTermTupleType::tuple));
static_assert(clingo_ast_theory_sequence_type_list ==
              static_cast<clingo_ast_theory_sequence_type_e>(Gringo::Input::TheoryTermTupleType::list));
static_assert(clingo_ast_theory_sequence_type_set ==
              static_cast<clingo_ast_theory_sequence_type_e>(Gringo::Input::TheoryTermTupleType::set));

static_assert(clingo_ast_comparison_operator_equal ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::equal));
static_assert(clingo_ast_comparison_operator_not_equal ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::inequal));
static_assert(clingo_ast_comparison_operator_less_than ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::less));
static_assert(clingo_ast_comparison_operator_less_equal ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::less_equal));
static_assert(clingo_ast_comparison_operator_greater_than ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::greater));
static_assert(clingo_ast_comparison_operator_greater_equal ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::greater_equal));

static_assert(clingo_ast_sign_no_sign == static_cast<clingo_ast_sign_e>(Gringo::Input::Sign::none));
static_assert(clingo_ast_sign_negation == static_cast<clingo_ast_sign_e>(Gringo::Input::Sign::once));
static_assert(clingo_ast_sign_double_negation == static_cast<clingo_ast_sign_e>(Gringo::Input::Sign::twice));

static_assert(clingo_ast_unary_operator_minus ==
              static_cast<clingo_ast_unary_operator_e>(Gringo::Input::UnaryOperator::negate));
static_assert(clingo_ast_unary_operator_negation ==
              static_cast<clingo_ast_unary_operator_e>(Gringo::Input::UnaryOperator::invert));

static_assert(clingo_ast_binary_operator_and ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::and_));
static_assert(clingo_ast_binary_operator_division ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::div));
static_assert(clingo_ast_binary_operator_minus ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::minus));
static_assert(clingo_ast_binary_operator_modulo ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::mod));
static_assert(clingo_ast_binary_operator_multiplication ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::times));
static_assert(clingo_ast_binary_operator_or ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::or_));
static_assert(clingo_ast_binary_operator_plus ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::plus));
static_assert(clingo_ast_binary_operator_power ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::pow));
static_assert(clingo_ast_binary_operator_xor ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::xor_));

static_assert(clingo_ast_aggregate_function_count ==
              static_cast<clingo_ast_aggregate_function_e>(Gringo::Input::AggregateFunction::count));
static_assert(clingo_ast_aggregate_function_sum ==
              static_cast<clingo_ast_aggregate_function_e>(Gringo::Input::AggregateFunction::sum));
static_assert(clingo_ast_aggregate_function_sump ==
              static_cast<clingo_ast_aggregate_function_e>(Gringo::Input::AggregateFunction::sump));
static_assert(clingo_ast_aggregate_function_min ==
              static_cast<clingo_ast_aggregate_function_e>(Gringo::Input::AggregateFunction::min));
static_assert(clingo_ast_aggregate_function_max ==
              static_cast<clingo_ast_aggregate_function_e>(Gringo::Input::AggregateFunction::max));

static_assert(clingo_ast_theory_operator_type_unary ==
              static_cast<clingo_ast_theory_operator_type_e>(Gringo::Input::TheoryOpType::unary));
static_assert(clingo_ast_theory_operator_type_binary_left ==
              static_cast<clingo_ast_theory_operator_type_e>(Gringo::Input::TheoryOpType::binary_left));
static_assert(clingo_ast_theory_operator_type_binary_right ==
              static_cast<clingo_ast_theory_operator_type_e>(Gringo::Input::TheoryOpType::binary_right));

static_assert(clingo_ast_theory_atom_definition_type_head ==
              static_cast<clingo_ast_theory_atom_definition_type_e>(Gringo::Input::TheoryAtomType::head));
static_assert(clingo_ast_theory_atom_definition_type_body ==
              static_cast<clingo_ast_theory_atom_definition_type_e>(Gringo::Input::TheoryAtomType::body));
static_assert(clingo_ast_theory_atom_definition_type_any ==
              static_cast<clingo_ast_theory_atom_definition_type_e>(Gringo::Input::TheoryAtomType::any));
static_assert(clingo_ast_theory_atom_definition_type_directive ==
              static_cast<clingo_ast_theory_atom_definition_type_e>(Gringo::Input::TheoryAtomType::directive));

static constexpr auto attribute_names = std::array{
    "argument",      "arguments",     "arity",        "atom",       "atoms",     "atom_type",  "bias",
    "body",          "code",          "coefficient",  "comparison", "condition", "elements",   "external",
    "external_type", "function",      "guard",        "guards",     "head",      "is_default", "left",
    "left_guard",    "literal",       "location",     "modifier",   "name",      "node_u",     "node_v",
    "operator_name", "operator_type", "operators",    "parameters", "positive",  "priority",   "right",
    "right_guard",   "sequence_type", "sign",         "symbol",     "term",      "terms",      "value",
    "variable",      "weight",        "comment_type",
};

clingo_ast_attribute_names_t g_clingo_ast_attribute_names = {attribute_names.data(), attribute_names.size()};

#define CONSTRUCTORS                                                                                                   \
    C(id, A(location, location), A(name, string))                                                                      \
    C(variable, A(location, location), A(name, string))                                                                \
    C(symbolic_term, A(location, location), A(symbol, symbol))                                                         \
    C(unary_operation, A(location, location), A(operator_type, number), A(argument, ast))                              \
    C(binary_operation, A(location, location), A(operator_type, number), A(left, ast), A(right, ast))                  \
    C(interval, A(location, location), A(left, ast), A(right, ast))                                                    \
    C(function, A(location, location), A(name, string), A(arguments, ast_array), A(external, number))                  \
    C(pool, A(location, location), A(arguments, ast_array))                                                            \
    /* simple atoms */                                                                                                 \
    C(boolean_constant, A(value, number))                                                                              \
    C(symbolic_atom, A(symbol, ast))                                                                                   \
    C(comparison, A(term, ast), A(guards, ast_array))                                                                  \
    /* aggregates */                                                                                                   \
    C(guard, A(comparison, number), A(term, ast))                                                                      \
    C(conditional_literal, A(location, location), A(literal, ast), A(condition, ast_array))                            \
    C(aggregate, A(location, location), A(left_guard, optional_ast), A(elements, ast_array),                           \
      A(right_guard, optional_ast))                                                                                    \
    C(body_aggregate_element, A(terms, ast_array), A(condition, ast_array))                                            \
    C(body_aggregate, A(location, location), A(left_guard, optional_ast), A(function, number), A(elements, ast_array), \
      A(right_guard, optional_ast))                                                                                    \
    C(head_aggregate_element, A(terms, ast_array), A(condition, ast))                                                  \
    C(head_aggregate, A(location, location), A(left_guard, optional_ast), A(function, number), A(elements, ast_array), \
      A(right_guard, optional_ast))                                                                                    \
    C(disjunction, A(location, location), A(elements, ast_array))                                                      \
    /* theory atoms */                                                                                                 \
    C(theory_sequence, A(location, location), A(sequence_type, number), A(terms, ast_array))                           \
    C(theory_function, A(location, location), A(name, string), A(arguments, ast_array))                                \
    C(theory_unparsed_term_element, A(operators, string_array), A(term, ast))                                          \
    C(theory_unparsed_term, A(location, location), A(elements, ast_array))                                             \
    C(theory_guard, A(operator_name, string), A(term, ast))                                                            \
    C(theory_atom_element, A(terms, ast_array), A(condition, ast_array))                                               \
    C(theory_atom, A(location, location), A(term, ast), A(elements, ast_array), A(guard, optional_ast))                \
    /* literals */                                                                                                     \
    C(literal, A(location, location), A(sign, number), A(atom, ast))                                                   \
    /* theory definition */                                                                                            \
    C(theory_operator_definition, A(location, location), A(name, string), A(priority, number),                         \
      A(operator_type, number))                                                                                        \
    C(theory_term_definition, A(location, location), A(name, string), A(operators, ast_array))                         \
    C(theory_guard_definition, A(operators, string_array), A(term, string))                                            \
    C(theory_atom_definition, A(location, location), A(atom_type, number), A(name, string), A(arity, number),          \
      A(term, string), A(guard, optional_ast))                                                                         \
    /* statemets */                                                                                                    \
    C(rule, A(location, location), A(head, ast), A(body, ast_array))                                                   \
    C(definition, A(location, location), A(name, string), A(value, ast), A(is_default, number))                        \
    C(show_signature, A(location, location), A(name, string), A(arity, number), A(positive, number))                   \
    C(show_term, A(location, location), A(term, ast), A(body, ast_array))                                              \
    C(minimize, A(location, location), A(weight, ast), A(priority, ast), A(terms, ast_array), A(body, ast_array))      \
    C(script, A(location, location), A(name, string), A(code, string))                                                 \
    C(program, A(location, location), A(name, string), A(parameters, ast_array))                                       \
    C(external, A(location, location), A(atom, ast), A(body, ast_array), A(external_type, ast))                        \
    C(edge, A(location, location), A(node_u, ast), A(node_v, ast), A(body, ast_array))                                 \
    C(heuristic, A(location, location), A(atom, ast), A(body, ast_array), A(bias, ast), A(priority, ast),              \
      A(modifier, ast))                                                                                                \
    C(project_atom, A(location, location), A(atom, ast), A(body, ast_array))                                           \
    C(project_signature, A(location, location), A(name, string), A(arity, number), A(positive, number))                \
    C(defined, A(location, location), A(name, string), A(arity, number), A(positive, number))                          \
    C(theory_definition, A(location, location), A(name, string), A(terms, ast_array), A(atoms, ast_array))             \
    C(comment, A(location, location), A(value, string), A(comment_type, number))

#define A(name, type)                                                                                                  \
    clingo_ast_argument_t { clingo_ast_attribute_##name, clingo_ast_attribute_type_##type }
#define C(name, ...) static constexpr auto clingo_ast_argument_##name = std::array{__VA_ARGS__};
CONSTRUCTORS
#undef C
#undef A

#define C(name, ...)                                                                                                   \
    clingo_ast_constructor_t{#name, clingo_ast_argument_##name.data(), clingo_ast_argument_##name.size()},
#define A(name, type) 1
static constexpr auto ast_constructors = std::array{CONSTRUCTORS};
#undef C
#undef A

clingo_ast_constructors_t g_clingo_ast_constructors = {ast_constructors.data(), ast_constructors.size()};

#undef CONSTRUCTORS
