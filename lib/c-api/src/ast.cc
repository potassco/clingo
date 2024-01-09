#include <cstdarg>

#include "lib.hh"

#include <gringo/input/algo/parse.hh>

struct clingo_ast {
    virtual auto get_number(clingo_ast_attribute_t attr) -> std::optional<int> = 0;
    virtual auto get_symbol(clingo_ast_attribute_t attr) -> std::optional<clingo_symbol_t> = 0;
    virtual auto get_location(clingo_ast_attribute_t attr) -> std::optional<clingo_location_t> = 0;
    virtual auto get_string(clingo_ast_attribute_t attr) -> std::optional<char const *> = 0;
    virtual ~clingo_ast() = default;
};

namespace {

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

struct GetType {
    // terms
    auto operator()(Gringo::Input::TermVariable const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_variable;
    }
    auto operator()(Gringo::Input::TermSymbol const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_symbolic;
    }
    auto operator()(Gringo::Input::TermTuple const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_tuple;
    }
    auto operator()(Gringo::Input::TermFunction const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_function;
    }
    auto operator()(Gringo::Input::TermAbs const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_absolute;
    }
    auto operator()(Gringo::Input::TermUnary const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_unary;
    }
    auto operator()(Gringo::Input::TermBinary const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_binary;
    }
};

struct GetNumber {
    // default
    template <class T> auto operator()(T const &term) const -> std::optional<int> {
        static_cast<void>(term);
        return std::nullopt;
    }
    // terms
    auto operator()(Gringo::Input::TermVariable const &term) const -> std::optional<int> {
        switch (attr) {
            case clingo_ast_attribute_anonymous: {
                return static_cast<int>(term.is_anonymous);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::TermFunction const &term) const -> std::optional<int> {
        switch (attr) {
            case clingo_ast_attribute_external: {
                return static_cast<int>(term.external);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::TermUnary const &term) const -> std::optional<int> {
        switch (attr) {
            case clingo_ast_attribute_operator_type: {
                return static_cast<int>(term.op);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::TermBinary const &term) const -> std::optional<int> {
        switch (attr) {
            case clingo_ast_attribute_operator_type: {
                return static_cast<int>(term.op);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    clingo_ast_attribute_t attr;
};

struct GetSymbol {
    // default
    template <class T> auto operator()(T const &term) const -> std::optional<clingo_symbol_t> {
        static_cast<void>(term);
        return std::nullopt;
    }
    // terms
    auto operator()(Gringo::Input::TermSymbol const &term) const -> std::optional<clingo_symbol_t> {
        switch (attr) {
            case clingo_ast_attribute_symbol: {
                return static_cast<clingo_symbol_t>(Gringo::Symbol::to_rep(term.value));
            }
            default: {
                return std::nullopt;
            }
        }
    }
    clingo_ast_attribute_t attr;
};

struct GetString {
    // default
    template <class T> auto operator()(T const &term) const -> std::optional<char const *> {
        static_cast<void>(term);
        return std::nullopt;
    }
    // terms
    auto operator()(Gringo::Input::TermVariable const &term) const -> std::optional<char const *> {
        switch (attr) {
            case clingo_ast_attribute_name: {
                return term.name.c_str();
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::TermFunction const &term) const -> std::optional<char const *> {
        switch (attr) {
            case clingo_ast_attribute_name: {
                return term.name.c_str();
            }
            default: {
                return std::nullopt;
            }
        }
    }
    clingo_ast_attribute_t attr;
};

auto convert_loc(clingo_lib_t *lib, clingo_location_t const *loc) -> Gringo::Input::Location {
    return {{lib->store->string(loc->begin_file), loc->begin_line, loc->begin_column},
            {lib->store->string(loc->end_file), loc->end_line, loc->end_column}};
}

auto convert_loc(Gringo::Input::Location const &loc) -> clingo_location_t {
    return {loc.begin.file.c_str(), loc.end.file.c_str(), loc.begin.line,
            loc.end.line,           loc.begin.column,     loc.end.column};
}

struct ASTTerm : clingo_ast {
    ASTTerm(Gringo::Input::Term term) : term{std::move(term)} {}
    auto get_type() -> clingo_ast_type_e { return std::visit(GetType{}, term); }
    auto get_number(clingo_ast_attribute_t attr) -> std::optional<int> override {
        return std::visit(GetNumber{attr}, term);
    }
    auto get_symbol(clingo_ast_attribute_t attr) -> std::optional<clingo_symbol_t> override {
        return std::visit(GetSymbol{attr}, term);
    }
    auto get_location(clingo_ast_attribute_t attr) -> std::optional<clingo_location_t> override {
        if (attr == clingo_ast_attribute_location) {
            return convert_loc(location(term));
        }
        return std::nullopt;
    }
    auto get_string(clingo_ast_attribute_t attr) -> std::optional<char const *> override {
        return std::visit(GetString{attr}, term);
    }
    Gringo::Input::Term term;
};

}; // namespace

extern "C" auto clingo_ast_construct(clingo_lib_t *lib, clingo_ast_type_t type, clingo_ast_t **ast, ...) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || ast == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *ast = nullptr;
        switch (static_cast<clingo_ast_type_e>(type)) {
            case clingo_ast_type_term_variable: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                va_end(args);
                *ast = new ASTTerm{Gringo::Input::TermVariable{convert_loc(lib, loc), lib->store->string(name)}};
                return true;
            }
            case clingo_ast_type_term_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sym = va_arg(args, clingo_symbol_t);
                va_end(args);
                *ast = new ASTTerm{Gringo::Input::TermSymbol{convert_loc(lib, loc), Gringo::Symbol::from_rep(sym)}};
                return true;
            }
            default: {
                throw std::logic_error("implement me!!!");
            }
        }
    }
    CLINGO_CATCH(lib);
}

extern "C" void clingo_ast_free(clingo_ast_t *ast) { delete ast; }

extern "C" auto clingo_ast_attribute_get_number(clingo_ast_t *ast, clingo_ast_attribute_t attribute, int *value)
    -> bool {
    if (auto num = ast->get_number(attribute); num) {
        *value = *num;
        return true;
    }
    return false;
}

extern "C" auto clingo_ast_attribute_get_symbol(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                clingo_symbol_t *value) -> bool {
    if (auto sym = ast->get_symbol(attribute); sym) {
        *value = *sym;
        return true;
    }
    return false;
}

extern "C" auto clingo_ast_attribute_get_location(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                  clingo_location_t *value) -> bool {
    if (auto loc = ast->get_location(attribute); loc) {
        *value = *loc;
        return true;
    }
    return false;
}

extern "C" auto clingo_ast_attribute_get_string(clingo_ast_t *ast, clingo_ast_attribute_t attribute, char const **value)
    -> bool {
    if (auto str = ast->get_string(attribute); str) {
        *value = *str;
        return true;
    }
    return false;
}

/*
static constexpr auto attribute_names = std::array{
    "anonymous", "argument",      "arguments",     "arity",        "atom",       "atoms",     "atom_type",  "bias",
    "body",          "code",          "coefficient",  "comparison", "condition", "elements",   "external",
    "external_type", "function",      "guard",        "guards",     "head",      "is_default", "left",
    "left_guard",    "literal",       "location",     "modifier",   "name",      "node_u",     "node_v",
    "operator_name", "operator_type", "operators",    "parameters", "positive",  "priority",   "right",
    "right_guard",   "sequence_type", "sign",         "symbol",     "term",      "terms",      "value",
    "variable",      "weight",        "comment_type",
};

clingo_ast_attribute_names_t g_clingo_ast_attribute_names = {attribute_names.data(), attribute_names.size()};


struct TODO {};

#define CONSTRUCTORS                                                                                                   \
    C(id, TODO, A(location, location), A(name, string)) \
    C(variable, Gringo::Input::TermVariable, A(location, location), A(name, string)) \
    C(symbolic_term, TODO, A(location, location), A(symbol, symbol)) \
    C(unary_operation, TODO, A(location, location), A(operator_type, number), A(argument, ast)) \
    C(binary_operation, TODO, A(location, location), A(operator_type, number), A(left, ast), A(right, ast)) \
    C(interval, TODO, A(location, location), A(left, ast), A(right, ast)) \
    C(function, TODO, A(location, location), A(name, string), A(arguments, ast_array), A(external, number)) \
    C(pool, TODO, A(location, location), A(arguments, ast_array)) \
    C(boolean_constant, TODO, A(value, number)) \
    C(symbolic_atom, TODO, A(symbol, ast)) \
    C(comparison, TODO, A(term, ast), A(guards, ast_array)) \
    C(guard, TODO, A(comparison, number), A(term, ast)) \
    C(conditional_literal, TODO, A(location, location), A(literal, ast), A(condition, ast_array)) \
    C(aggregate, TODO, A(location, location), A(left_guard, optional_ast), A(elements, ast_array), \
      A(right_guard, optional_ast))                                                                                    \
    C(body_aggregate_element, TODO, A(terms, ast_array), A(condition, ast_array)) \
    C(body_aggregate, TODO, A(location, location), A(left_guard, optional_ast), A(function, number), A(elements,
ast_array), \
      A(right_guard, optional_ast))                                                                                    \
    C(head_aggregate_element, TODO, A(terms, ast_array), A(condition, ast)) \
    C(head_aggregate, TODO, A(location, location), A(left_guard, optional_ast), A(function, number), A(elements,
ast_array), \
      A(right_guard, optional_ast))                                                                                    \
    C(disjunction, TODO, A(location, location), A(elements, ast_array)) \
    C(theory_sequence, TODO, A(location, location), A(sequence_type, number), A(terms, ast_array)) \
    C(theory_function, TODO, A(location, location), A(name, string), A(arguments, ast_array)) \
    C(theory_unparsed_term_element, TODO, A(operators, string_array), A(term, ast)) \
    C(theory_unparsed_term, TODO, A(location, location), A(elements, ast_array)) \
    C(theory_guard, TODO, A(operator_name, string), A(term, ast)) \
    C(theory_atom_element, TODO, A(terms, ast_array), A(condition, ast_array)) \
    C(theory_atom, TODO, A(location, location), A(term, ast), A(elements, ast_array), A(guard, optional_ast)) \
    C(literal, TODO, A(location, location), A(sign, number), A(atom, ast)) \
    C(theory_operator_definition, TODO, A(location, location), A(name, string), A(priority, number), \
      A(operator_type, number))                                                                                        \
    C(theory_term_definition, TODO, A(location, location), A(name, string), A(operators, ast_array)) \
    C(theory_guard_definition, TODO, A(operators, string_array), A(term, string)) \
    C(theory_atom_definition, TODO, A(location, location), A(atom_type, number), A(name, string), A(arity, number), \
      A(term, string), A(guard, optional_ast))                                                                         \
    C(rule, TODO, A(location, location), A(head, ast), A(body, ast_array)) \
    C(definition, TODO, A(location, location), A(name, string), A(value, ast), A(is_default, number)) \
    C(show_signature, TODO, A(location, location), A(name, string), A(arity, number), A(positive, number)) \
    C(show_term, TODO, A(location, location), A(term, ast), A(body, ast_array)) \
    C(minimize, TODO, A(location, location), A(weight, ast), A(priority, ast), A(terms, ast_array), A(body, ast_array))
\
    C(script, TODO, A(location, location), A(name, string), A(code, string)) \
    C(program, TODO, A(location, location), A(name, string), A(parameters, ast_array)) \
    C(external, TODO, A(location, location), A(atom, ast), A(body, ast_array), A(external_type, ast)) \
    C(edge, TODO, A(location, location), A(node_u, ast), A(node_v, ast), A(body, ast_array)) \
    C(heuristic, TODO, A(location, location), A(atom, ast), A(body, ast_array), A(bias, ast), A(priority, ast), \
      A(modifier, ast))                                                                                                \
    C(project_atom, TODO, A(location, location), A(atom, ast), A(body, ast_array)) \
    C(project_signature, TODO, A(location, location), A(name, string), A(arity, number), A(positive, number)) \
    C(defined, TODO, A(location, location), A(name, string), A(arity, number), A(positive, number)) \
    C(theory_definition, TODO, A(location, location), A(name, string), A(terms, ast_array), A(atoms, ast_array)) \
    C(comment, TODO, A(location, location), A(value, string), A(comment_type, number))

#define A(name, type)                                                                                                  \
    clingo_ast_argument_t { clingo_ast_attribute_##name, clingo_ast_attribute_type_##type }
#define C(name, type, ...) static constexpr auto clingo_ast_argument_##name = std::array{__VA_ARGS__};
CONSTRUCTORS
#undef C
#undef A

#define C(name, type, ...) \ clingo_ast_constructor_t{#name, clingo_ast_argument_##name.data(),
clingo_ast_argument_##name.size()}, #define A(name, type) 1 static constexpr auto ast_constructors =
std::array{CONSTRUCTORS}; #undef C #undef A

clingo_ast_constructors_t g_clingo_ast_constructors = {ast_constructors.data(), ast_constructors.size()};

struct clingo_ast {
    clingo_ast_type_t type;
    size_t use_count;
};

template <class T>
struct clingo_ast_value : clingo_ast {
    clingo_ast_value(clingo_ast_type_t type, T value) : clingo_ast{type, 1}, value(std::move(value)) {}
    T value;
};

*/
