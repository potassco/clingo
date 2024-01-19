// {{{1 Preamble

//! @file clingo.h
//! Single header containing the whole clingo API.
//!
//! @author Roland Kaminski

//! @mainpage Clingo C API
//! This API provides functions to ground and solve logic programs.
//!
//! The documentation is structured into different modules.
//! To get an overview, checkout the [Topics](topics.html) page.
//! To get started, take a look at the documentation of the @ref Control module.
//!
//! The source code of clingo is available on [github.com/potassco/clingo](https://github.com/potassco/clingo).
//!
//! For information about the syntax and semantics of the clingo language,
//! take a look the [Potassco Guide](https://github.com/potassco/guide/releases/).
//!
//! @note Each module comes with an example highlighting key functionality.
//! The example should be studied along with the module documentation.

#ifndef CLINGO_H
#define CLINGO_H

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-*)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined _WIN32 || defined __CYGWIN__
#define CLINGO_WIN
#endif
#ifdef CLINGO_NO_VISIBILITY
#define CLINGO_VISIBILITY_DEFAULT
#define CLINGO_VISIBILITY_PRIVATE
#else
#ifdef CLINGO_WIN
#ifdef CLINGO_BUILD_LIBRARY
#define CLINGO_VISIBILITY_DEFAULT __declspec(dllexport)
#else
#define CLINGO_VISIBILITY_DEFAULT __declspec(dllimport)
#endif
#define CLINGO_VISIBILITY_PRIVATE
#else
#if __GNUC__ >= 4
#define CLINGO_VISIBILITY_DEFAULT __attribute__((visibility("default")))
#define CLINGO_VISIBILITY_PRIVATE __attribute__((visibility("hidden")))
#else
#define CLINGO_VISIBILITY_DEFAULT
#define CLINGO_VISIBILITY_PRIVATE
#endif
#endif
#endif

#if defined __GNUC__
#define CLINGO_DEPRECATED __attribute__((deprecated))
#elif defined _MSC_VER
#define CLINGO_DEPRECATED __declspec(deprecated)
#else
#define CLINGO_DEPRECATED
#endif

//! @defgroup CAPI C API
//! API providing a stable interface for applications using Clingo.
//!
//! The API is mainly intended for developing higher level language bindings.

// {{{1 Core

//! @example version.c
//! The example shows how to get version information.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! $ ./version
//! Hello, this is clingo version...
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @defgroup Core Core
//! Core types and functions used throughout all modules and version information.
//!
//! @ingroup CAPI
//!
//! For an example, see @ref version.c.

//! @addtogroup Core
//! @{

//! Major version number.
#define CLINGO_VERSION_MAJOR 6
//! Minor version number.
#define CLINGO_VERSION_MINOR 0
//! Revision number.
#define CLINGO_VERSION_REVISION 0
//! String representation of version.
#define CLINGO_VERSION "6.0.0"

//! Obtain the clingo version.
//!
//! @param[out] major major version number
//! @param[out] minor minor version number
//! @param[out] revision revision number
CLINGO_VISIBILITY_DEFAULT void clingo_version(int *major, int *minor, int *revision);

//! Enumeration of error codes.
//!
//! @note Errors can only be recovered from if explicitly mentioned; most
//! functions do not provide strong exception guarantees.  This means that in
//! case of errors associated objects cannot be used further.  If such an
//! object has a free function, this function can and should still be called.
enum clingo_error_e {
    clingo_error_success = 0,   //!< successful API calls
    clingo_error_runtime = 1,   //!< errors only detectable at runtime like invalid input
    clingo_error_logic = 2,     //!< wrong usage of the clingo API
    clingo_error_bad_alloc = 3, //!< memory could not be allocated
    clingo_error_unknown = 4    //!< errors unrelated to clingo
};
//! Corresponding type to ::clingo_error_e.
typedef int clingo_error_t;

//! Convert the given error code into a string.
CLINGO_VISIBILITY_DEFAULT char const *clingo_error_string(clingo_error_t code);

//! Enumeration of warning codes.
enum clingo_message_e {
    clingo_message_trace = 0,               //!< a trace message
    clingo_message_debug = 1,               //!< a debug message
    clingo_message_info = 2,                //!< an info message
    clingo_message_operation_undefined = 3, //!< undefined atom in program
    clingo_message_atom_undefined = 4,      //!< undefined atom in program
    clingo_message_file_included = 5,       //!< same file included multiple times
    clingo_message_global_variable = 6,     //!< global variable in tuple of aggregate element
    clingo_message_warn = 7,                //!< a warning message
    clingo_message_error = 8, //!< to report multiple errors; a corresponding runtime error is raised later
};
//! Corresponding type to ::clingo_message_e.
typedef int clingo_message_t;

//! Convert the giving warning code into a string.
CLINGO_VISIBILITY_DEFAULT char const *clingo_message_string(clingo_message_t code);

//! Callback to intercept warning messages.
//!
//! @param[in] code associated warning code
//! @param[in] message warning message
//! @param[in] data user data for callback
//!
//! @see clingo_control_new()
//! @see clingo_parse_term()
//! @see clingo_parse_program()
typedef void (*clingo_logger_t)(clingo_message_t code, char const *message, void *data);

//! A library object storing global information.
typedef struct clingo_lib clingo_lib_t;

//! Flags to create library objects.
enum clingo_lib_flags_e {
    clingo_lib_flags_slotted = 1, //!< use custom allocator for storing symbols
    clingo_lib_flags_shared = 2,  //!< create symbols in a thread-safe manner
};
//! Bitset of ::clingo_lib_flags_e.
typedef uint32_t clingo_lib_flags_t;

//! Create a library object.
//!
//! A library has to be freed using clingo_lib_free().
//!
//! If the logger is NULL, the default logger printing messages to stderr is used.
//!
//! Note that the function returns NULL, in case the of a memout.
//!
//! @param[in] flags construction flags
//! @param[in] logger callback functions for warnings and info messages
//! @param[in] logger_data user data for the logger callback
//! @param[in] message_limit maximum number of times the logger callback is called
//! @return the resulting library object
CLINGO_VISIBILITY_DEFAULT clingo_lib_t *clingo_lib_new(clingo_lib_flags_t flags, clingo_logger_t logger,
                                                       void *logger_data, size_t message_limit);

//! Free a library object created with clingo_lib_new().
//! @param[in] lib the target
CLINGO_VISIBILITY_DEFAULT void clingo_lib_free(clingo_lib_t *lib);

//! Get the last error code set by a clingo API call.
//! @note Each thread has its own local error code.
//! @return error code
CLINGO_VISIBILITY_DEFAULT clingo_error_t clingo_error_code(clingo_lib_t *lib);

//! Get the last error message set if an API call fails.
//! @return error message or NULL if there is none
CLINGO_VISIBILITY_DEFAULT char const *clingo_error_message(clingo_lib_t *lib);

//! Set a custom error code and message in the given library.
//! @param[in] lib the target library
//! @param[in] code the error code
//! @param[in] message the error message
CLINGO_VISIBILITY_DEFAULT void clingo_set_error(clingo_lib_t *lib, clingo_error_t code, char const *message);

//! Signed integer type used for aspif and solver literals.
typedef int32_t clingo_literal_t;
//! Unsigned integer type used for aspif atoms.
typedef uint32_t clingo_atom_t;
//! Unsigned integer type used in various places.
typedef uint32_t clingo_id_t;
//! Signed integer type for weights in sum aggregates and minimize constraints.
typedef int32_t clingo_weight_t;
//! A Literal with an associated weight.
typedef struct clingo_weighted_literal {
    clingo_literal_t literal;
    clingo_weight_t weight;
} clingo_weighted_literal_t;

//! Represents three-valued truth values.
enum clingo_truth_value_e {
    clingo_truth_value_free = 0, //!< no truth value
    clingo_truth_value_true = 1, //!< true
    clingo_truth_value_false = 2 //!< false
};
//! Corresponding type to ::clingo_truth_value_e.
typedef int clingo_truth_value_t;

//! Internalize a string.
//!
//! This functions takes a string as input and returns an equal unique string
//! that is stored in the given library.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] string the string to internalize
//! @param[out] result the internalized string
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_add_string(clingo_lib_t *lib, char const *string, char const **result);

//! Represents a source code location marking its beginning and end.
//!
//! @note Strings in locations must be internalized.
//!
//! @note Not all locations refer to physical files.
//! By convention, such locations use a name put in angular brackets as filename.
//! The string members of a location object are internalized and stored in a library object.
typedef struct clingo_location {
    char const *begin_file; //!< the file where the location begins
    char const *end_file;   //!< the file where the location ends
    size_t begin_line;      //!< the line where the location begins
    size_t end_line;        //!< the line where the location ends
    size_t begin_column;    //!< the column where the location begins
    size_t end_column;      //!< the column where the location ends
} clingo_location_t;

//! Less than compare two locations.
//!
//! @param[in] a the left-hand-side
//! @param[in] b the right-hand-side
//! @return the result of the comparison
CLINGO_VISIBILITY_DEFAULT bool clingo_location_less_than(clingo_location_t const *a, clingo_location_t const *b);
//! Equality compare two locations.
//!
//! @param[in] a the left-hand-side
//! @param[in] b the right-hand-side
//! @return the result of the comparison
CLINGO_VISIBILITY_DEFAULT bool clingo_location_equal(clingo_location_t const *a, clingo_location_t const *b);
//! Compute a hash for a location.
//!
//! @param[in] ast the target
//! @return the resulting hash code
CLINGO_VISIBILITY_DEFAULT size_t clingo_location_hash(clingo_location_t const *loc);

//! Get the size of the string representation of a location (including the terminating 0).
//!
//! @param[in] location the target location
//! @param[out] size the resulting size
//! @return whether the size has been set
CLINGO_VISIBILITY_DEFAULT bool clingo_location_to_string_size(clingo_location_t location, size_t *size);

//! Get the string representation of a location.
//!
//! @param[in] location the target location
//! @param[out] string the resulting string
//! @param[in] size the size of the string
//! @return whether the string has been set
CLINGO_VISIBILITY_DEFAULT bool clingo_location_to_string(clingo_location_t location, char *string, size_t size);
//! @}

// {{{1 Symbol

//! @example symbol.c
//! The example shows how to create and inspect symbols.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! $ ./symbol 0
//! the hash of 42 is 281474976710698
//! the hash of x is 562949963481760
//! the hash of x(42,x) is 1407374893613792
//! 42 is equal to 42
//! 42 is not equal to x
//! 42 is less than x
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @defgroup Symbols Symbols
//! Working with (evaluated) ground terms and related functions.
//!
//! @ingroup CAPI
//!
//! @note Functions to create symbols are only thread-safe if library flags have been requested accordingly.
//!
//! For an example, see @ref symbol.c.

//! @addtogroup Symbols
//! @{

/*
//! Represents a predicate signature.
//!
//! Signatures have a name and an arity, and can be positive or negative (to
//! represent classical negation).
typedef uint64_t clingo_signature_t;

//! @name Signature Functions
//! @{

//! Create a new signature.
//!
//! @param[in] name name of the signature
//! @param[in] arity arity of the signature
//! @param[in] positive false if the signature has a classical negation sign
//! @param[out] signature the resulting signature
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_signature_create(char const *name, uint32_t arity, bool positive,
                                                       clingo_signature_t *signature);
//! Get the name of a signature.
//!
//! @note
//! The string is internalized and valid for the duration of the process.
//!
//! @param[in] signature the target signature
//! @return the name of the signature
CLINGO_VISIBILITY_DEFAULT char const *clingo_signature_name(clingo_signature_t signature);
//! Get the arity of a signature.
//!
//! @param[in] signature the target signature
//! @return the arity of the signature
CLINGO_VISIBILITY_DEFAULT uint32_t clingo_signature_arity(clingo_signature_t signature);
//! Whether the signature is positive (is not classically negated).
//!
//! @param[in] signature the target signature
//! @return whether the signature has no sign
CLINGO_VISIBILITY_DEFAULT bool clingo_signature_is_positive(clingo_signature_t signature);
//! Whether the signature is negative (is classically negated).
//!
//! @param[in] signature the target signature
//! @return whether the signature has a sign
CLINGO_VISIBILITY_DEFAULT bool clingo_signature_is_negative(clingo_signature_t signature);
//! Check if two signatures are equal.
//!
//! @param[in] a first signature
//! @param[in] b second signature
//! @return whether a == b
CLINGO_VISIBILITY_DEFAULT bool clingo_signature_is_equal_to(clingo_signature_t a, clingo_signature_t b);
//! Check if a signature is less than another signature.
//!
//! Signatures are compared first by sign (unsigned < signed), then by arity,
//! then by name.
//!
//! @param[in] a first signature
//! @param[in] b second signature
//! @return whether a < b
CLINGO_VISIBILITY_DEFAULT bool clingo_signature_is_less_than(clingo_signature_t a, clingo_signature_t b);
//! Calculate a hash code of a signature.
//!
//! @param[in] signature the target signature
//! @return the hash code of the signature
CLINGO_VISIBILITY_DEFAULT size_t clingo_signature_hash(clingo_signature_t signature);

//! @}
*/

//! Enumeration of available symbol types.
enum clingo_symbol_type_e {
    clingo_symbol_type_number = 0,   //!< a numeric symbol, e.g., `1`
    clingo_symbol_type_supremum = 1, //!< the <tt>\#sup</tt> symbol
    clingo_symbol_type_infimum = 2,  //!< the <tt>\#inf</tt> symbol
    clingo_symbol_type_string = 3,   //!< a string symbol, e.g., `"a"`
    clingo_symbol_type_tuple = 4,    //!< a tuple symbol, e.g., `(1, "a")``
    clingo_symbol_type_function = 5  //!< a function symbol, e.g., `c`, `-c`, or `f(1,"a")`
};
//! Corresponding type to ::clingo_symbol_type.
typedef int clingo_symbol_type_t;

//! Type to represent a symbol.
//!
//! This includes numbers, strings, tuples, functions (including constants when arguments are empty), <tt>\#inf</tt> and
//! <tt>\#sup</tt>.
typedef uint64_t clingo_symbol_t;

//! @name Symbol Construction Functions
//! @{

//! Construct a symbol representing <tt>\#inf</tt>.
//!
//! @return the resulting symbol
CLINGO_VISIBILITY_DEFAULT clingo_symbol_t clingo_symbol_create_infimum();

//! Construct a symbol representing \#sup.
//!
//! @return the resulting symbol
CLINGO_VISIBILITY_DEFAULT clingo_symbol_t clingo_symbol_create_supremum();

//! Construct a symbol representing a number.
//!
//! @param[in] number the number
//! @return the resulting symbol
CLINGO_VISIBILITY_DEFAULT clingo_symbol_t clingo_symbol_create_number(int32_t number);

//! Construct a symbol representing a number.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] number the number
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_number_str(clingo_lib_t *lib, char const *number,
                                                               clingo_symbol_t *symbol);

//! Construct a symbol representing a string.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] string the string
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_string(clingo_lib_t *lib, char const *string,
                                                           clingo_symbol_t *symbol);

//! Construct a symbol representing an id.
//!
//! @note This is just a shortcut for clingo_symbol_create_function() with
//! empty arguments.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] name the name
//! @param[in] sign whether the symbol has a classical negation sign
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_id(clingo_lib_t *lib, char const *name, bool sign,
                                                       clingo_symbol_t *symbol);

//! Construct a symbol representing a tuple.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] arguments the arguments of the function
//! @param[in] arguments_size the number of arguments
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_tuple(clingo_lib_t *lib, clingo_symbol_t const *arguments,
                                                          size_t arguments_size, clingo_symbol_t *symbol);

//! Construct a symbol representing a function.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] name the name of the function
//! @param[in] arguments the arguments of the function
//! @param[in] arguments_size the number of arguments
//! @param[in] positive whether the symbol has a classical negation sign
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_function(clingo_lib_t *lib, char const *name,
                                                             clingo_symbol_t const *arguments, size_t arguments_size,
                                                             bool positive, clingo_symbol_t *symbol);

//! Parse a term in string form.
//!
//! The result of this function is a symbol. The input term can contain
//! unevaluated functions, which are evaluated during parsing.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] string the string to parse
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if parsing fails
CLINGO_VISIBILITY_DEFAULT bool clingo_parse_term(clingo_lib_t *lib, char const *string, clingo_symbol_t *symbol);

//! @}

//! @name Symbol Inspection Functions
//! @{

//! Get the number of a symbol.
//!
//! @note
//! There is currently no explicit function to get representation of a number that does not fit an integer.
//! For now, the clingo_symbol_to_string function should be used.
//!
//! @param[in] symbol the target symbol
//! @param[out] number the resulting number
//! @return whether the number has been set
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_number(clingo_symbol_t symbol, int32_t *number);

//! Get the name of a symbol.
//!
//! @note
//! The string is internalized and valid for the duration of the process.
//!
//! @param[in] symbol the target symbol
//! @param[out] name the resulting name
//! @return whether the name has been set
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_name(clingo_symbol_t symbol, char const **name);

//! Get the string of a symbol.
//!
//! @note
//! The string is internalized and valid for the duration of the process.
//!
//! @param[in] symbol the target symbol
//! @param[out] string the resulting string
//! @return whether the string has been set
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_string(clingo_symbol_t symbol, char const **string);

//! Check whether a function or number has a sign.
//!
//! @param[in] symbol the target symbol
//! @param[out] has_sign the result
//! @return whether the sign has been set
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_has_sign(clingo_symbol_t symbol, bool *has_sign);

//! Get the arguments of a symbol.
//!
//! @param[in] symbol the target symbol
//! @param[out] arguments the resulting arguments
//! @param[out] arguments_size the number of arguments
//! @return whether the arguments have been set
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_arguments(clingo_symbol_t symbol, clingo_symbol_t const **arguments,
                                                       size_t *arguments_size);

//! Get the type of a symbol.
//!
//! @param[in] symbol the target symbol
//! @return the type of the symbol
CLINGO_VISIBILITY_DEFAULT clingo_symbol_type_t clingo_symbol_type(clingo_symbol_t symbol);

//! Get the size of the string representation of a symbol (including the terminating 0).
//!
//! @param[in] symbol the target symbol
//! @param[out] size the resulting size
//! @return whether the size has been set
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_to_string_size(clingo_symbol_t symbol, size_t *size);

//! Get the string representation of a symbol.
//!
//! @param[in] symbol the target symbol
//! @param[out] string the resulting string
//! @param[in] size the size of the string
//! @return whether the string has been set
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_to_string(clingo_symbol_t symbol, char *string, size_t size);

//! @}

//! @name Symbol Comparison Functions
//! @{

//! Check if two symbols are equal.
//!
//! @param[in] a first symbol
//! @param[in] b second symbol
//! @return whether a == b
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_is_equal_to(clingo_symbol_t a, clingo_symbol_t b);

//! Check if a symbol is less than another symbol.
//!
//! Symbols are first compared by type.  If the types are equal, the values are
//! compared (where strings are compared using strcmp).  Functions are first
//! compared by signature and then lexicographically by arguments.
//!
//! @param[in] a first symbol
//! @param[in] b second symbol
//! @return whether a < b
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_is_less_than(clingo_symbol_t a, clingo_symbol_t b);

//! Calculate a hash code of a symbol.
//!
//! @param[in] symbol the target symbol
//! @return the hash code of the symbol
CLINGO_VISIBILITY_DEFAULT size_t clingo_symbol_hash(clingo_symbol_t symbol);

//! @}

//! @}

// {{{1 Control

//! Control object holding grounding and solving state.
typedef struct clingo_control clingo_control_t;

// {{{1 AST

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

//! @defgroup AST Abstract Syntax Trees
//! Functions and data structures to work with program ASTs.
//!
//! @ingroup CAPI

//! @addtogroup AST
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
    /*
    // simple atoms
    clingo_ast_type_boolean_constant,
    clingo_ast_type_symbolic_atom,
    clingo_ast_type_comparison,
    // aggregates
    clingo_ast_type_guard,
    clingo_ast_type_conditional_literal,
    clingo_ast_type_aggregate,
    clingo_ast_type_body_aggregate_element,
    clingo_ast_type_body_aggregate,
    clingo_ast_type_head_aggregate_element,
    clingo_ast_type_head_aggregate,
    clingo_ast_type_disjunction,
    // theory atoms
    clingo_ast_type_theory_sequence,
    clingo_ast_type_theory_function,
    clingo_ast_type_theory_unparsed_term_element,
    clingo_ast_type_theory_unparsed_term,
    clingo_ast_type_theory_guard,
    clingo_ast_type_theory_atom_element,
    clingo_ast_type_theory_atom,
    // literals
    clingo_ast_type_literal,
    // theory definition
    clingo_ast_type_theory_operator_definition,
    clingo_ast_type_theory_term_definition,
    clingo_ast_type_theory_guard_definition,
    clingo_ast_type_theory_atom_definition,
    // statements
    clingo_ast_type_rule,
    clingo_ast_type_definition,
    clingo_ast_type_show_signature,
    clingo_ast_type_show_term,
    clingo_ast_type_minimize,
    clingo_ast_type_script,
    clingo_ast_type_program,
    clingo_ast_type_external,
    clingo_ast_type_edge,
    clingo_ast_type_heuristic,
    clingo_ast_type_project_atom,
    clingo_ast_type_project_signature,
    clingo_ast_type_defined,
    clingo_ast_type_theory_definition,
    clingo_ast_type_comment
    */
};
//! Corresponding type to ::clingo_ast_type_e.
typedef int clingo_ast_type_t;

//! Enumeration of attributes used by the AST.
enum clingo_ast_attribute_e {
    clingo_ast_attribute_anonymous,
    clingo_ast_attribute_external,
    clingo_ast_attribute_left,
    clingo_ast_attribute_location,
    clingo_ast_attribute_name,
    clingo_ast_attribute_operator_type,
    clingo_ast_attribute_pool,
    clingo_ast_attribute_right,
    clingo_ast_attribute_symbol,
    clingo_ast_attribute_tuple,
    clingo_ast_attribute_arguments,
    clingo_ast_attribute_sign,
    clingo_ast_attribute_value,
    clingo_ast_attribute_atom,
    clingo_ast_attribute_term,
    clingo_ast_attribute_relation,
    clingo_ast_attribute_tuple_type,
    clingo_ast_attribute_elements,
    clingo_ast_attribute_operators,
    clingo_ast_attribute_literal,
    clingo_ast_attribute_condition,
    clingo_ast_attribute_function,
    clingo_ast_attribute_theory_operator,
    /*
    clingo_ast_attribute_argument,
    clingo_ast_attribute_arity,
    clingo_ast_attribute_atom,
    clingo_ast_attribute_atoms,
    clingo_ast_attribute_atom_type,
    clingo_ast_attribute_bias,
    clingo_ast_attribute_body,
    clingo_ast_attribute_code,
    clingo_ast_attribute_coefficient,
    clingo_ast_attribute_comparison,
    clingo_ast_attribute_condition,
    clingo_ast_attribute_external_type,
    clingo_ast_attribute_function,
    clingo_ast_attribute_guard,
    clingo_ast_attribute_guards,
    clingo_ast_attribute_head,
    clingo_ast_attribute_is_default,
    clingo_ast_attribute_left,
    clingo_ast_attribute_left_guard,
    clingo_ast_attribute_literal,
    clingo_ast_attribute_modifier,
    clingo_ast_attribute_node_u,
    clingo_ast_attribute_node_v,
    clingo_ast_attribute_operator_name,
    clingo_ast_attribute_operators,
    clingo_ast_attribute_parameters,
    clingo_ast_attribute_positive,
    clingo_ast_attribute_priority,
    clingo_ast_attribute_right,
    clingo_ast_attribute_right_guard,
    clingo_ast_attribute_sequence_type,
    clingo_ast_attribute_sign,
    clingo_ast_attribute_term,
    clingo_ast_attribute_terms,
    clingo_ast_attribute_value,
    clingo_ast_attribute_variable,
    clingo_ast_attribute_weight,
    clingo_ast_attribute_comment_type,
    */
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
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if one of the arguments is incompatible with the type
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_construct(clingo_lib_t *lib, clingo_ast_type_t type, clingo_ast_t **ast, ...);

//! Copy the given AST node.
//!
//! @param[in] ast the AST to copy
//! @param[out] copy the resulting AST
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime for invalid arguments
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_copy(clingo_ast_t *ast, clingo_ast_t **copy);

//! Enumeration of expressions that can be parsed.
enum clingo_ast_parse_type_e {
    clingo_ast_parse_type_term,
    clingo_ast_parse_type_literal,
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
//! @param[in] ast the resulting ast
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime for invalid arguments or expressions
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_parse_expression(clingo_lib_t *lib, clingo_ast_parse_type_t type,
                                                           char const *string, clingo_ast_t **ast);

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

//! Get the size of the string representation of an AST node.
//!
//! @param[in] ast the target AST
//! @param[out] size the size of the string representation
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_to_string_size(clingo_ast_t *ast, size_t *size);

//! Get the string representation of an AST node.
//!
//! @param[in] ast the target AST
//! @param[out] string the string representation
//! @param[out] size the size of the string representation
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_to_string(clingo_ast_t *ast, char *string, size_t size);

//! @}

//! @name Functions to compare ASTs
//! @{

//! Less than compare two AST nodes.
//!
//! @param[in] a the left-hand-side AST
//! @param[in] b the right-hand-side AST
//! @return the result of the comparison
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_less_than(clingo_ast_t *a, clingo_ast_t *b);

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
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_get_type(clingo_ast_t *ast, clingo_ast_type_t *type);

//! Get the value of numeric attribute.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_number(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                               int *value);

//! Get the value of a symbol attribute.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_symbol(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                               clingo_symbol_t *value);

//! Get the value of a location attribute.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_location(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                                 clingo_location_t *value);

//! Get the value of a string attribute.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_string(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                               char const **value);

//! Get the value of a string array attribute.
//!
//! @note The the required size of the array can be queried setting value to NULL.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @param[out] size the size of the array
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_string_array(clingo_ast_t *ast,
                                                                     clingo_ast_attribute_t attribute,
                                                                     char const **value, size_t *size);

//! Get the value of an ast attribute..
//!
//! The value will be set to NULL if an optional AST does not have a value.
//!
//! @note The resulting ast has to be freed using clingo_ast_free.
//!
//! @param[in] ast the target AST
//! @param[in] attribute the target attribute
//! @param[out] value the resulting value
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime
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
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_attribute_get_ast_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                                  clingo_ast_t ***value, size_t *size);

//! Get a description of the AST structure in form of a YAML document.
//!
//! @return A YAML string.
CLINGO_VISIBILITY_DEFAULT char const *clingo_ast_type_info_yaml();

//! @}

/*
//! @name Functions to construct ASTs from strings
//! @{

//! Callback function to intercept AST nodes.
//!
//! @param[in] ast the AST
//! @param[in] data a user data pointer
//! @return whether the call was successful
typedef bool (*clingo_ast_callback_t)(clingo_ast_t *ast, void *data);
//! Parse the given program and return an abstract syntax tree for each statement via a callback.
//!
//! @note The control object can be set to a NULL to disable reading input in aspif format.
//!
//! @param[in] program the program in gringo syntax
//! @param[in] callback the callback reporting statements
//! @param[in] callback_data user data for the callback
//! @param[in] control object to add ground statements to
//! @param[in] logger callback to report messages during parsing
//! @param[in] logger_data user data for the logger
//! @param[in] message_limit the maximum number of times the logger is called
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if parsing fails
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_parse_string(char const *program, clingo_ast_callback_t callback,
                                                       void *callback_data, clingo_control_t *control,
                                                       clingo_logger_t logger, void *logger_data,
                                                       unsigned message_limit);
//! Parse the programs in the given list of files and return an abstract syntax tree for each statement via a callback.
//!
//! The function follows clingo's handling of files on the command line.
//! Filename "-" is treated as "STDIN" and if an empty list is given, then the parser will read from "STDIN".
//!
//! @note The control object can be set to a NULL to disable reading input in aspif format.
//!
//! @param[in] files the beginning of the file name array
//! @param[in] size the number of file names
//! @param[in] callback the callback reporting statements
//! @param[in] callback_data user data for the callback
//! @param[in] control object to add ground statements to
//! @param[in] logger callback to report messages during parsing
//! @param[in] logger_data user data for the logger
//! @param[in] message_limit the maximum number of times the logger is called
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if parsing fails
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_ast_parse_files(char const *const *files, size_t size,
                                                      clingo_ast_callback_t callback, void *callback_data,
                                                      clingo_control_t *control, clingo_logger_t logger,
                                                      void *logger_data, unsigned message_limit);

*/

//! @}

//! @}

// NOLINTEND(modernize-*)

#ifdef __cplusplus
}
#endif

#endif
