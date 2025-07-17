#ifndef CLINGO_AST_H
#define CLINGO_AST_H

#include <clingo/core.h>
#include <clingo/symbol.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clingo_control clingo_control_t;

//! @example ast.c
//! The example shows how to rewrite a non-ground logic program.
//!
//! ## Output ##
//!
//! ~~~~~~~~
//! ./ast 0
//! Solving with enable = false...
//! Model:
//! Solving with enable = true...
//! Model: enable a
//! Model: enable b
//! Solving with enable = false...
//! Model:
//! ~~~~~~~~
//!
//! ## Code ##

//! @addtogroup c_ast
//! Functions and data structures to work with program ASTs.
//!
//! @{

//! Enumeration of AST types.
enum clingo_ast_type_e {
    // terms
    clingo_ast_type_projection,
    clingo_ast_type_term_variable,
    clingo_ast_type_term_symbolic,
    clingo_ast_type_term_absolute,
    clingo_ast_type_term_unary_operation,
    clingo_ast_type_term_binary_operation,
    clingo_ast_type_term_tuple,
    clingo_ast_type_term_function,
    clingo_ast_type_argument_tuple,
    // theory terms
    clingo_ast_type_unparsed_element,
    clingo_ast_type_theory_term_variable,
    clingo_ast_type_theory_term_symbolic,
    clingo_ast_type_theory_term_tuple,
    clingo_ast_type_theory_term_function,
    clingo_ast_type_theory_term_unparsed,
    // literals
    clingo_ast_type_left_guard,
    clingo_ast_type_right_guard,
    clingo_ast_type_literal_boolean,
    clingo_ast_type_literal_comparison,
    clingo_ast_type_literal_symbolic,
    // set aggregates and theory atoms
    clingo_ast_type_set_aggregate_element,
    clingo_ast_type_theory_atom_element,
    clingo_ast_type_theory_right_guard,
    // body literals
    clingo_ast_type_body_simple_literal,
    clingo_ast_type_body_aggregate_element,
    clingo_ast_type_body_aggregate,
    clingo_ast_type_body_set_aggregate,
    clingo_ast_type_body_theory_atom,
    clingo_ast_type_body_conditional_literal,
    // head literals
    clingo_ast_type_head_simple_literal,
    clingo_ast_type_head_aggregate_element,
    clingo_ast_type_head_aggregate,
    clingo_ast_type_head_set_aggregate,
    clingo_ast_type_head_theory_atom,
    clingo_ast_type_head_conditional_literal,
    clingo_ast_type_head_disjunction,
    // theory definition
    clingo_ast_type_theory_operator_definition,
    clingo_ast_type_theory_term_definition,
    clingo_ast_type_theory_guard_definition,
    clingo_ast_type_theory_atom_definition,
    // elements
    clingo_ast_type_optimize_tuple,
    clingo_ast_type_optimize_element,
    clingo_ast_type_edge,
    clingo_ast_type_program_part,
    // statements
    clingo_ast_type_statement_rule,
    clingo_ast_type_statement_theory,
    clingo_ast_type_statement_optimize,
    clingo_ast_type_statement_weak_constraint,
    clingo_ast_type_statement_show,
    clingo_ast_type_statement_show_nothing,
    clingo_ast_type_statement_show_signature,
    clingo_ast_type_statement_project,
    clingo_ast_type_statement_project_signature,
    clingo_ast_type_statement_defined,
    clingo_ast_type_statement_external,
    clingo_ast_type_statement_edge,
    clingo_ast_type_statement_heuristic,
    clingo_ast_type_statement_script,
    clingo_ast_type_statement_program,
    clingo_ast_type_statement_include,
    clingo_ast_type_statement_const,
    clingo_ast_type_statement_parts,
    clingo_ast_type_statement_comment
};
//! Corresponding type to ::clingo_ast_type_e.
typedef int clingo_ast_type_t;

//! Enumeration of attributes used by the AST.
enum clingo_ast_attribute_e {
    clingo_ast_attribute_anonymous,
    clingo_ast_attribute_arguments,
    clingo_ast_attribute_arity,
    clingo_ast_attribute_atom,
    clingo_ast_attribute_atoms,
    clingo_ast_attribute_atom_type,
    clingo_ast_attribute_body,
    clingo_ast_attribute_comment_type,
    clingo_ast_attribute_condition,
    clingo_ast_attribute_precedence,
    clingo_ast_attribute_elements,
    clingo_ast_attribute_external,
    clingo_ast_attribute_external_type,
    clingo_ast_attribute_function,
    clingo_ast_attribute_guard,
    clingo_ast_attribute_head,
    clingo_ast_attribute_include_type,
    clingo_ast_attribute_left,
    clingo_ast_attribute_literal,
    clingo_ast_attribute_location,
    clingo_ast_attribute_modifier,
    clingo_ast_attribute_name,
    clingo_ast_attribute_operators,
    clingo_ast_attribute_operator_type,
    clingo_ast_attribute_optimize_type,
    clingo_ast_attribute_pool,
    clingo_ast_attribute_priority,
    clingo_ast_attribute_relation,
    clingo_ast_attribute_right,
    clingo_ast_attribute_script_type,
    clingo_ast_attribute_sign,
    clingo_ast_attribute_symbol,
    clingo_ast_attribute_term,
    clingo_ast_attribute_terms,
    clingo_ast_attribute_theory_operator,
    clingo_ast_attribute_tuple,
    clingo_ast_attribute_tuple_type,
    clingo_ast_attribute_u,
    clingo_ast_attribute_v,
    clingo_ast_attribute_value,
    clingo_ast_attribute_weight,
};

//! Corresponding type to ::clingo_ast_attribute_e.
typedef int clingo_ast_attribute_t;

//! This struct provides a view to nodes in the AST.
typedef struct clingo_ast clingo_ast_t;

//! @name Functions to create/free ASTs
//! @{

//! Construct an AST of the given type.
//!
//! @param[in] lib the library object to store symbols
//! @param[in] type the type of AST to construct
//! @param[out] ast the resulting AST
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_construct(clingo_lib_t *lib, clingo_ast_type_t type, clingo_ast_t **ast, ...);

//! Copy the given AST node.
//!
//! @param[in] ast the AST to copy
//! @param[out] copy the resulting AST
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_copy(clingo_ast_t *ast, clingo_ast_t **copy);

//! Enumeration of expressions that can be parsed.
enum clingo_ast_parse_type_e {
    clingo_ast_parse_type_term,
    clingo_ast_parse_type_theory_term,
    clingo_ast_parse_type_literal,
    clingo_ast_parse_type_body_literal,
    clingo_ast_parse_type_head_literal,
    clingo_ast_parse_type_statement,
};
//! Corresponding type to ::clingo_ast_parse_type_e.
typedef int clingo_ast_parse_type_t;

//! Parse a single expression given as string.
//!
//! @note Parse errors are reported via the logger.
//!
//! @param[in] lib the library object to store symbols
//! @param[in] type the expression type to parse
//! @param[in] string the expression to parse
//! @param[in] size the size of the string
//! @param[in] ast the resulting ast
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_parse_expression(clingo_lib_t *lib, clingo_ast_parse_type_t type,
                                                           char const *string, size_t size, clingo_ast_t **ast);

//! Free an AST node.
//!
//! @param[in] ast the target AST
CLINGO_VISIBILITY_DEFAULT void clingo_ast_free(clingo_ast_t *ast);

//! Free an AST array.
//!
//! This also frees the individual ast's in the array using clingo_ast_free.
//!
//! @param[in] ast the target AST array
//! @param[in] size the size of the AST array
CLINGO_VISIBILITY_DEFAULT void clingo_ast_array_free(clingo_ast_t **ast, size_t size);

//! @}

//! @name Functions to convert ASTs to strings
//! @{

//! Get the string representation of an AST node.
//!
//! @param[in] ast the target AST
//! @param[in] builder the string builder
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_to_string(clingo_ast_t *ast, clingo_string_builder_t *builder);

//! @}

//! @name Functions to compare ASTs
//! @{

//! Less than compare two AST nodes.
//!
//! @param[in] a the left-hand-side AST
//! @param[in] b the right-hand-side AST
//! @return the result of the comparison
CLINGO_VISIBILITY_DEFAULT int clingo_ast_compare(clingo_ast_t *a, clingo_ast_t *b);

//! Equality compare two AST nodes.
//!
//! @param[in] a the left-hand-side AST
//! @param[in] b the right-hand-side AST
//! @return the result of the comparison
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_equal(clingo_ast_t *a, clingo_ast_t *b);

//! Compute a hash for an AST node.
//!
//! @param[in] ast the target AST
//! @return the resulting hash code
CLINGO_VISIBILITY_DEFAULT size_t clingo_ast_hash(clingo_ast_t *ast);

//! @}

//! @name Functions to inspect ASTs
//! @{

//! Get the type of an AST node.
//!
//! @param[in] ast the target AST
//! @param[out] type the resulting type
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_get_type(clingo_ast_t *ast, clingo_ast_type_t *type);

//! Get the value of numeric attribute.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_number(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                               int *value);

//! Get the value of a symbol attribute.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_symbol(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                               clingo_symbol_t *value);

//! Get the value of a location attribute.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_location(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                                 clingo_location_t const **value);

//! Get the value of a string attribute.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_string(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                               clingo_string_t *value);

//! Get the value of a string array attribute.
//!
//! @note The the required size of the array can be queried setting value to NULL.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @param[out] size the size of the array
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_string_array(clingo_ast_t *ast,
                                                                     clingo_ast_attribute_t attribute,
                                                                     clingo_string_t const **value, size_t *size);

//! Get the value of a symbol array attribute.
//!
//! @note The the required size of the array can be queried setting value to NULL.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @param[out] size the size of the array
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_symbol_array(clingo_ast_t *ast,
                                                                     clingo_ast_attribute_t attribute,
                                                                     clingo_symbol_t const **value, size_t *size);

//! Get the value of an ast attribute.
//!
//! The value will be set to NULL if an optional AST does not have a value.
//!
//! @note The resulting ast has to be freed using clingo_ast_free.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_ast(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                            clingo_ast_t **value);

//! Get the value of an ast array attribute.
//!
//! @note The resulting array has to be freed using clingo_ast_array_free.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @param[out] size the size of the array
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_ast_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                                  clingo_ast_t ***value, size_t *size);

//! Get a description of the AST structure in form of a YAML document.
//!
//! @return A YAML string.
CLINGO_VISIBILITY_DEFAULT char const *clingo_ast_type_info_yaml(void);

//! @}

//! @name Functions to scan ASTs
//! @{

//! A callback to intercept ast's.
//!
//! @param[in] ast the paresd ast
//! @param[in] data the user data of the callback
//! @return whether the call was successful
typedef bool (*clingo_ast_callback_t)(clingo_ast_t *ast, void *data);

//! Parse a program from a string.
//!
//! @param[in] lib the library object to store symbols
//! @param[in] program the string to read from
//! @param[in] size the size of the string
//! @param[in] control optional control object to handle aspif
//! @param[in] callback the callback for the ast nodes
//! @param[in] data the user data of the callback
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_parse_string(clingo_lib_t *lib, char const *program, size_t size,
                                                       clingo_control_t *control, clingo_ast_callback_t callback,
                                                       void *data);

//! Parse a program from the given files.
//!
//! @param[in] lib the library object to store symbols
//! @param[in] files the file paths to read from
//! @param[in] size the number of file paths
//! @param[in] control optional control object to handle aspif
//! @param[in] callback the callback for the ast nodes
//! @param[in] data the user data of the callback
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_parse_files(clingo_lib_t *lib, clingo_string_t const *files, size_t size,
                                                      clingo_control_t *control, clingo_ast_callback_t callback,
                                                      void *data);

//! @}

//! The available projection modes.
enum clingo_projection_mode_e {
    clingo_projection_mode_disabled = 0,  //!< Disable projection.
    clingo_projection_mode_anonymous = 1, //!< Only project anonymous variables.
    clingo_projection_mode_pure = 2,      //!< Project pure variables.
};
//! Corresponding type to ::clingo_projection_mode_e.
typedef int clingo_projection_mode_t;

//! Context object to rewrite statements.
typedef struct clingo_ast_rewrite_context clingo_ast_rewrite_context_t;

//! Create a new rewrite context.
//!
//! @param[in] lib the library object to store symbols
//! @param[out] context the resulting context object
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_rewrite_context_create(clingo_lib_t *lib,
                                                                 clingo_ast_rewrite_context_t **context);

//! Free the given rewrite context.
//!
//! @param[in] context the context to free
CLINGO_VISIBILITY_DEFAULT void clingo_ast_rewrite_context_free(clingo_ast_rewrite_context_t *context);

//! Protect the given parameter from simplifications.
//!
//! Parameters from const and program statements should be protected.
//!
//! @param[in] context the context object
//! @param[in] param the parameter to protect
//! @param[in] size the size of the string
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_rewrite_context_add_param(clingo_ast_rewrite_context_t *context,
                                                                    char const *param, size_t size);

//! Remove all previously added parameters.
//!
//! @param[in] context the context object
CLINGO_VISIBILITY_DEFAULT void clingo_ast_rewrite_context_clear_params(clingo_ast_rewrite_context_t *context);

//! Add a theory definition to the rewrite context.
//!
//! Added definitions are used to rewrite theory atoms.
//!
//! @param[in] context the context object
//! @param[in] theory the theory definition
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_rewrite_context_add_theory(clingo_ast_rewrite_context_t *context,
                                                                     clingo_ast_t const *theory);

//! Configure whether anonymous variables in negative literals are projected.
//!
//! @param[in] context the context object
//! @param[in] value whether to enable projection
CLINGO_VISIBILITY_DEFAULT void clingo_ast_rewrite_context_set_project_anonymous(clingo_ast_rewrite_context_t *context,
                                                                                bool value);

//! Return whether anonymous variables in negative literals are projected.
//!
//! @param[in] context the context object
//! @return whether anonymous variables in negative literals are projected
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_rewrite_context_get_project_anonymous(clingo_ast_rewrite_context_t *context);

//! Configure the projection mode.
//!
//! @param[in] context the context object
//! @param[in] value the projection mode
CLINGO_VISIBILITY_DEFAULT void clingo_ast_rewrite_context_set_project_mode(clingo_ast_rewrite_context_t *context,
                                                                           clingo_projection_mode_t value);

//! Get the configured projection mode.
//!
//! @param[in] context the context object
//! @return the projection mode
CLINGO_VISIBILITY_DEFAULT clingo_projection_mode_t
clingo_ast_rewrite_context_get_project_mode(clingo_ast_rewrite_context_t *context);

//! Get the library object used to create the context.
//!
//! @param[in] context the context object
//! @return the library object
CLINGO_VISIBILITY_DEFAULT clingo_lib_t *clingo_ast_rewrite_context_get_lib(clingo_ast_rewrite_context_t *context);

//! Rewrite the given statement.
//!
//! @note The returned statements have to be freed using clingo_ast_array_free().
//!
//! @param[in] context the context object
//! @param[in] statement the statement object
//! @param[out] result the resulting rewritten statements
//! @param[out] result_size the number of resulting statements
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_rewrite(clingo_ast_rewrite_context_t *context, clingo_ast_t *statement,
                                                  clingo_ast_t ***result, size_t *result_size);

//! Object to store
typedef struct clingo_program clingo_program_t;

//! @name Functions to add ASTs to logic programs
//! @{

//! Create an empty non-ground program.
//!
//! @param[in] lib the library object
//! @param[out] program the program builder object
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_program_new(clingo_lib_t *lib, clingo_program_t **program);
//! Destroy the given program object.
//!
//! @param[in] program the program object
CLINGO_VISIBILITY_DEFAULT void clingo_program_free(clingo_program_t *program);
//! Adds a statement to the program.
//!
//! @param[in] program the target program
//! @param[in] statement the statement to add
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_program_add(clingo_program_t *program, clingo_ast_t *statement);

//! @}

//! Extend the control objects's program with the given one.
//!
//! After extending the logic program, the corresponding program parts are typically grounded with
//! ::clingo_control_ground.
//!
//! @param[in] control the target
//! @param[in] program the program to add
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_join(clingo_control_t *control, clingo_program_t const *program);

//! @}

#ifdef __cplusplus
}
#endif

#endif
