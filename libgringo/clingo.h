// {{{ GPL License

// This file is part of gringo---a grounder for logic programs.
// Copyright Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

//! @file clingo.h
//! Single header containing the whole clingo API.
//!
//! @author Roland Kaminski

//! @mainpage Clingo C API
//! This API provides functions to ground and solve logic programs.
//!
//! The documentation is structured into different modules.
//! To get an overview, checkout the [Modules](modules.html) page.
//! To get started, take a look at the documentation of the @ref Control module.
//!
//! The source code of clingo is available on [github.com/potassco/clingo](https://github.com/potassco/clingo).
//!
//! For information about the syntax and semantics of the clingo language,
//! take a look the [Potassco Guide](http://sourceforge.net/projects/potassco/files/guide/).
//!
//! @note Each module comes with an example highlighting key functionality.
//! The example should be studied along with the module documentation.

#ifndef CLINGO_H
#define CLINGO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if defined _WIN32 || defined __CYGWIN__
#   define CLINGO_WIN
#endif
#ifdef CLINGO_WIN
#   ifdef CLINGO_BUILD_LIBRARY
#       define CLINGO_VISIBILITY_DEFAULT __declspec (dllexport)
#   else
#       define CLINGO_VISIBILITY_DEFAULT __declspec (dllimport)
#   endif
#   define CLINGO_VISIBILITY_PRIVATE
#else
#   if __GNUC__ >= 4
#       define CLINGO_VISIBILITY_DEFAULT  __attribute__ ((visibility ("default")))
#       define CLINGO_VISIBILITY_PRIVATE __attribute__ ((visibility ("hidden")))
#   else
#       define CLINGO_VISIBILITY_DEFAULT
#       define CLINGO_VISIBILITY_PRIVATE
#   endif
#endif

// {{{1 basic types and error/warning handling

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

//! @defgroup BasicTypes Basic Data Types and Functions
//! Data types and functions used throughout all modules and version information.
//!
//! For an example, see @ref version.c.

//! @addtogroup BasicTypes
//! @{

//! Major version number.
#define CLINGO_VERSION_MAJOR 5
//! Minor version number.
#define CLINGO_VERSION_MINOR 0
//! Revision number.
#define CLINGO_VERSION_REVISION 0
//! String representation of version.
#define CLINGO_VERSION #CLINGO_VERSION_MAJOR "." #CLINGO_VERSION_MINOR "." #CLINGO_VERSION_REVISION

//! Signed integer type used for aspif and solver literals.
typedef int32_t clingo_literal_t;
//! Unsigned integer type used for aspif atoms.
typedef uint32_t clingo_atom_t;
//! Unsigned integer type used in various places.
typedef uint32_t clingo_id_t;
//! Signed integer type for weights in sum aggregates and minimize constraints.
typedef int32_t clingo_weight_t;

//! Enumeration of error codes.
//!
//! @note Errors can only be recovered from if explicitly mentioned; most
//! functions do not provide strong exception guarantees.  This means that in
//! case of errors associated objects cannot be used further.  If such an
//! object has a free function, this function can and should still be called.
enum clingo_error {
    clingo_error_success   = 0, //!< successful API calls
    clingo_error_runtime   = 1, //!< errors only detectable at runtime like invalid input
    clingo_error_logic     = 2, //!< wrong usage of the clingo API
    clingo_error_bad_alloc = 3, //!< memory could not be allocated
    clingo_error_unknown   = 4  //!< errors unrelated to clingo
};
//! Corresponding type to ::clingo_error.
typedef int clingo_error_t;
//! Convert error code into string.
CLINGO_VISIBILITY_DEFAULT char const *clingo_error_string(clingo_error_t code);
//! Get the last error code set by a clingo API call.
//! @note Each thread has its own local error code.
//! @return error code
CLINGO_VISIBILITY_DEFAULT clingo_error_t clingo_error_code();
//! Get the last error message set if an API call fails.
//! @note Each thread has its own local error message.
//! @return error message or NULL
CLINGO_VISIBILITY_DEFAULT char const *clingo_error_message();
//! Set a custom error code and message in the active thread.
//! @param[in] code the error code
//! @param[in] message the error message
CLINGO_VISIBILITY_DEFAULT void clingo_set_error(clingo_error_t code, char const *message);

//! Enumeration of warning codes.
enum clingo_warning {
    clingo_warning_operation_undefined = 0, //!< undefined arithmetic operation or weight of aggregate
    clingo_warning_runtime_error       = 1, //!< to report multiple errors; a corresponding runtime error is raised later
    clingo_warning_atom_undefined      = 2, //!< undefined atom in program
    clingo_warning_file_included       = 3, //!< same file included multiple times
    clingo_warning_variable_unbounded  = 4, //!< CSP variable with unbounded domain
    clingo_warning_global_variable     = 5, //!< global variable in tuple of aggregate element
    clingo_warning_other               = 6, //!< other kinds of warnings
};
//! Corresponding type to ::clingo_warning.
typedef int clingo_warning_t;
//! Convert warning code into string.
CLINGO_VISIBILITY_DEFAULT char const *clingo_warning_string(clingo_warning_t code);
//! Callback to intercept warning messages.
//!
//! @param[in] code associated warning code
//! @param[in] message warning message
//! @param[in] data user data for callback
//!
//! @see clingo_control_new()
//! @see clingo_parse_term()
//! @see clingo_parse_program()
typedef void clingo_logger_t(clingo_warning_t code, char const *message, void *data);

//! Obtain the clingo version.
//!
//! @param[out] major major version number
//! @param[out] minor minor version number
//! @param[out] revision revision number
CLINGO_VISIBILITY_DEFAULT void clingo_version(int *major, int *minor, int *revision);

//! Represents three-valued truth values.
enum clingo_truth_value {
    clingo_truth_value_free  = 0, //!< no truth value
    clingo_truth_value_true  = 1, //!< true
    clingo_truth_value_false = 2  //!< false
};
//! Corresponding type to ::clingo_truth_value.
typedef int clingo_truth_value_t;

//! Represents a source code location marking its beginnig and end.
//!
//! @note Not all locations refer to physical files.
//! By convention, such locations use a name put in angular brackets as filename.
typedef struct clingo_location {
    char const *begin_file; //!< the file where the location begins
    char const *end_file;   //!< the file where the location ends
    size_t begin_line;      //!< the line where the location begins
    size_t end_line;        //!< the line where the location ends
    size_t begin_column;    //!< the column where the location begins
    size_t end_column;      //!< the column where the location ends
} clingo_location_t;

//! @}

// {{{1 signature and symbols

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
//! @note All functions in this module are thread-safe.
//!
//! For an example, see @ref symbol.c.

//! @addtogroup Symbols
//! @{

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
CLINGO_VISIBILITY_DEFAULT bool clingo_signature_create(char const *name, uint32_t arity, bool positive, clingo_signature_t *signature);
//! Get the name of a signature.
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

//! Enumeration of available symbol types.
enum clingo_symbol_type {
    clingo_symbol_type_infimum  = 0, //!< the <tt>\#inf</tt> symbol
    clingo_symbol_type_number   = 1, //!< a numeric symbol, e.g., `1`
    clingo_symbol_type_string   = 4, //!< a string symbol, e.g., `"a"`
    clingo_symbol_type_function = 5, //!< a numeric symbol, e.g., `c`, `(1, "a")`, or `f(1,"a")`
    clingo_symbol_type_supremum = 7  //!< the <tt>\#sup</tt> symbol
};
//! Corresponding type to ::clingo_symbol_type.
typedef int clingo_symbol_type_t;

//! Represents a symbol.
//!
//! This includes numbers, strings, functions (including constants when
//! arguments are empty and tuples when the name is empty), <tt>\#inf</tt> and <tt>\#sup</tt>.
typedef uint64_t clingo_symbol_t;

//! Represents a symbolic literal.
typedef struct clingo_symbolic_literal {
    clingo_symbol_t symbol; //!< the associated symbol (must be a function)
    bool positive;          //!< whether the literal has a sign
} clingo_symbolic_literal_t;

//! @name Symbol Construction Functions
//! @{

//! Construct a symbol representing a number.
//!
//! @param[in] number the number
//! @param[out] symbol the resulting symbol
CLINGO_VISIBILITY_DEFAULT void clingo_symbol_create_number(int number, clingo_symbol_t *symbol);
//! Construct a symbol representing \#sup.
//!
//! @param[out] symbol the resulting symbol
CLINGO_VISIBILITY_DEFAULT void clingo_symbol_create_supremum(clingo_symbol_t *symbol);
//! Construct a symbol representing <tt>\#inf</tt>.
//!
//! @param[out] symbol the resulting symbol
CLINGO_VISIBILITY_DEFAULT void clingo_symbol_create_infimum(clingo_symbol_t *symbol);
//! Construct a symbol representing a string.
//!
//! @param[in] string the string
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_string(char const *string, clingo_symbol_t *symbol);
//! Construct a symbol representing an id.
//!
//! @note This is just a shortcut for clingo_symbol_create_function() with
//! empty arguments.
//!
//! @param[in] name the name
//! @param[in] positive whether the symbol has a classical negation sign
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_id(char const *name, bool positive, clingo_symbol_t *symbol);
//! Construct a symbol representing a function or tuple.
//!
//! @note To create tuples, the empty string has to be used as name.
//!
//! @param[in] name the name of the function
//! @param[in] arguments the arguments of the function
//! @param[in] arguments_size the number of arguments
//! @param[in] positive whether the symbol has a classical negation sign
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_function(char const *name, clingo_symbol_t const *arguments, size_t arguments_size, bool positive, clingo_symbol_t *symbol);

//! @}

//! @name Symbol Inspection Functions
//! @{

//! Get the number of a symbol.
//!
//! @param[in] symbol the target symbol
//! @param[out] number the resulting number
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if symbol is not of type ::clingo_symbol_type_number
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_number(clingo_symbol_t symbol, int *number);
//! Get the name of a symbol.
//!
//! @param[in] symbol the target symbol
//! @param[out] name the resulting name
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if symbol is not of type ::clingo_symbol_type_function
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_name(clingo_symbol_t symbol, char const **name);
//! Get the string of a symbol.
//!
//! @param[in] symbol the target symbol
//! @param[out] string the resulting string
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if symbol is not of type ::clingo_symbol_type_string
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_string(clingo_symbol_t symbol, char const **string);
//! Check if a function is positive (does not have a sign).
//!
//! @param[in] symbol the target symbol
//! @param[out] positive the result
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if symbol is not of type ::clingo_symbol_type_function
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_is_positive(clingo_symbol_t symbol, bool *positive);
//! Check if a function is negative (has a sign).
//!
//! @param[in] symbol the target symbol
//! @param[out] negative the result
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if symbol is not of type ::clingo_symbol_type_function
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_is_negative(clingo_symbol_t symbol, bool *negative);
//! Get the arguments of a symbol.
//!
//! @param[in] symbol the target symbol
//! @param[out] arguments the resulting arguments
//! @param[out] arguments_size the number of arguments
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if symbol is not of type ::clingo_symbol_type_function
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_arguments(clingo_symbol_t symbol, clingo_symbol_t const **arguments, size_t *arguments_size);
//! Get the type of a symbol.
//!
//! @param[in] symbol the target symbol
//! @return the type of the symbol
CLINGO_VISIBILITY_DEFAULT clingo_symbol_type_t clingo_symbol_type(clingo_symbol_t symbol);
//! Get the size of the string representation of a symbol (including the terminating 0).
//!
//! @param[in] symbol the target symbol
//! @param[out] size the resulting size
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_to_string_size(clingo_symbol_t symbol, size_t *size);
//! Get the string representation of a symbol.
//!
//! @param[in] symbol the target symbol
//! @param[out] string the resulting string
//! @param[in] size the size of the string
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//!
//! @see clingo_symbol_to_string_size()
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

//! Internalize a string.
//!
//! This functions takes a string as input and returns an equal unique string
//! that is (at the moment) not freed until the program is closed.  All strings
//! returned from clingo API functions are internalized and must not be freed.
//!
//! @param[in] string the string to internalize
//! @param[out] result the internalized string
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_add_string(char const *string, char const **result);
//! Parse a term in string form.
//!
//! The result of this function is a symbol. The input term can contain
//! unevaluated functions, which are evaluated during parsing.
//!
//! @param[in] string the string to parse
//! @param[in] logger optional logger to report warnings during parsing
//! @param[in] logger_data user data for the logger
//! @param[in] message_limit maximum number of times to call the logger
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if parsing fails
CLINGO_VISIBILITY_DEFAULT bool clingo_parse_term(char const *string, clingo_logger_t *logger, void *logger_data, unsigned message_limit, clingo_symbol_t *symbol);

//! @}

// {{{1 model and solve control

//! @example model.c
//! The example shows how to inspect a model.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! $ ./model 0
//! Stable model:
//!   shown: c
//!   atoms: b
//!   terms: c
//!  ~atoms: a
//! Stable model:
//!   shown: a
//!   atoms: a
//!   terms:
//!  ~atoms: b
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @defgroup Model Model Inspection
//! Inspection of models and a high-level interface to add constraints during solving.
//!
//! For an example, see @ref model.c.
//! @ingroup Control

//! @addtogroup Model
//! @{

//! Object to add clauses during search.
typedef struct clingo_solve_control clingo_solve_control_t;

//! Object representing a model.
typedef struct clingo_model clingo_model_t;

//! Enumeration for the different model types.
enum clingo_model_type {
    clingo_model_type_stable_model          = 0, //!< The model represents a stable model.
    clingo_model_type_brave_consequences    = 1, //!< The model represents a set of brave consequences.
    clingo_model_type_cautious_consequences = 2  //!< The model represents a set of cautious consequences.
};
//! Corresponding type to ::clingo_model_type.
typedef int clingo_model_type_t;

//! Enumeration of bit flags to select symbols in models.
enum clingo_show_type {
    clingo_show_type_csp        = 1,  //!< Select CSP assignments.
    clingo_show_type_shown      = 2,  //!< Select shown atoms and terms.
    clingo_show_type_atoms      = 4,  //!< Select all atoms.
    clingo_show_type_terms      = 8,  //!< Select all terms.
    clingo_show_type_extra      = 16, //!< Select symbols added by extensions.
    clingo_show_type_all        = 31, //!< Select everything.
    clingo_show_type_complement = 32  //!< Select false instead of true atoms (::clingo_show_type_atoms) or terms (::clingo_show_type_terms).
};
//! Corresponding type to ::clingo_show_type.
typedef unsigned clingo_show_type_bitset_t;

//! @name Functions for Inspecting Models
//! @{

//! Get the type of the model.
//!
//! @param[in] model the target
//! @param[out] type the type of the model
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_type(clingo_model_t *model, clingo_model_type_t *type);
//! Get the running number of the model.
//!
//! @param[in] model the target
//! @param[out] number the number of the model
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_number(clingo_model_t *model, uint64_t *number);
//! Get the number of symbols of the selected types in the model.
//!
//! @param[in] model the target
//! @param[in] show which symbols to select
//! @param[out] size the number symbols
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_model_symbols_size(clingo_model_t *model, clingo_show_type_bitset_t show, size_t *size);
//! Get the symbols of the selected types in the model.
//!
//! @note CSP assignments are represented using functions with name "$"
//! where the first argument is the name of the CSP variable and the second one its
//! value.
//!
//! @param[in] model the target
//! @param[in] show which symbols to select
//! @param[out] symbols the resulting symbols
//! @param[in] size the number of selected symbols
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if the size is too small
//!
//! @see clingo_model_symbols_size()
CLINGO_VISIBILITY_DEFAULT bool clingo_model_symbols(clingo_model_t *model, clingo_show_type_bitset_t show, clingo_symbol_t *symbols, size_t size);
//! Constant time lookup to test whether an atom is in a model.
//!
//! @param[in] model the target
//! @param[in] atom the atom to lookup
//! @param[out] contained whether the atom is contained
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_contains(clingo_model_t *model, clingo_symbol_t atom, bool *contained);
//! Get the number of cost values of a model.
//!
//! @param[in] model the target
//! @param[out] size the number of costs
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_cost_size(clingo_model_t *model, size_t *size);
//! Get the cost vector of a model.
//!
//! @param[in] model the target
//! @param[out] costs the resulting costs
//! @param[in] size the number of costs
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if the size is too small
//!
//! @see clingo_model_cost_size()
//! @see clingo_model_optimality_proven()
CLINGO_VISIBILITY_DEFAULT bool clingo_model_cost(clingo_model_t *model, int64_t *costs, size_t size);
//! Whether the optimality of a model has been proven.
//!
//! @param[in] model the target
//! @param[out] proven whether the optimality has been proven
//! @return whether the call was successful
//!
//! @see clingo_model_cost()
CLINGO_VISIBILITY_DEFAULT bool clingo_model_optimality_proven(clingo_model_t *model, bool *proven);
//! @}

//! @name Functions for Adding Clauses
//! @{

//! Get the associated solve control object of a model.
//!
//! This object allows for adding clauses during model enumeration.
//! @param[in] model the target
//! @param[out] control the resulting solve control object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_context(clingo_model_t *model, clingo_solve_control_t **control);
//! Get the id of the solver thread that found the model.
//!
//! @param[in] control the target
//! @param[out] id the resulting thread id
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_control_thread_id(clingo_solve_control_t *control, clingo_id_t *id);
//! Add a clause that applies to the current solving step during model
//! enumeration.
//!
//! @note The @ref Propagator module provides a more sophisticated
//! interface to add clauses - even on partial assignments.
//!
//! @param[in] control the target
//! @param[in] clause array of literals representing the clause
//! @param[in] size the size of the literal array
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if adding the clause fails
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_control_add_clause(clingo_solve_control_t *control, clingo_symbolic_literal_t const *clause, size_t size);
//! @}

//! @}

// {{{1 solve result

// NOTE: documented in Control Module
enum clingo_solve_result {
    clingo_solve_result_satisfiable   = 1,
    clingo_solve_result_unsatisfiable = 2,
    clingo_solve_result_exhausted     = 4,
    clingo_solve_result_interrupted   = 8
};
typedef unsigned clingo_solve_result_bitset_t;


// {{{1 solve iter

//! @example solve-iteratively.c
//! The example shows how to iteratively enumerate models.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! ./solve-iteratively 0
//! Model: a
//! Model: b
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @defgroup SolveIter Iterative Solving
//! Iterative enumeration of models (without using callbacks).
//!
//! Clingo provides two additional ways to handle solving: @ref SolveIter and @ref SolveAsync.
//! With iterative solving, models are enumerated one by one by calling @ref clingo_solve_iteratively_next() and @ref clingo_solve_iteratively_get() in a loop.
//!
//! For an example, see @ref solve-iteratively.c.
//! @ingroup Control

//! @addtogroup SolveIter
//! @{

//! Search handle to enumerate models iteratively.
//!
//! @see clingo_control_solve_iteratively()
typedef struct clingo_solve_iteratively clingo_solve_iteratively_t;
//! Get the next model.
//!
//! @param[in] handle the target
//! @param[out] model the next model
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if solving fails
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_iteratively_next(clingo_solve_iteratively_t *handle, clingo_model_t **model);
//! Get the solve result.
//!
//! @param[in] handle the target
//! @param[out] result the solve result
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if solving fails
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_iteratively_get(clingo_solve_iteratively_t *handle, clingo_solve_result_bitset_t *result);
//! Closes an active search.
//!
//! There must be no function calls on the associated control object until this function has been called.
//!
//! @param[in] handle the target
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if solving fails
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_iteratively_close(clingo_solve_iteratively_t *handle);

//! @}

// {{{1 solve async

//! @example solve-async.c
//! The example shows how to solve in the background.
//!
//! ## Output (approximately) ##
//!
//! ~~~~~~~~~~~~
//! ./solve-async 0
//! pi = 3.
//! 1415926535 8979323846 2643383279 5028841971 6939937510 5820974944
//! 5923078164 0628620899 8628034825 3421170679 8214808651 3282306647
//! 0938446095 5058223172 5359408128 4811174502 8410270193 8521105559
//! 6446229489 5493038196 4428810975 6659334461 2847564823 3786783165
//! 2712019091 4564856692 3460348610 4543266482 1339360726 0249141273
//! 7245870066 0631558817 4881520920 9628292540 9171536436 7892590360
//! 0113305305 4882046652 1384146951 9415116094 3305727036 5759591953
//! 0921861173 8193261179 3105118548 0744623799 6274956735 1885752724
//! 8912279381 8301194912 ...
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @defgroup SolveAsync Asynchronous Solving
//! Solve in the background and notify about new models.
//!
//! Clingo provides two additional ways to handle solving: @ref SolveIter and @ref SolveAsync.
//! With asynchronous solving, the solving process is executed in the background,
//! and each newly found model is signalled via a @link ::clingo_model_callback_t model callback@endlink.
//!
//! For an example, see @ref solve-async.c.
//! @ingroup Control

//! @addtogroup SolveAsync
//! @{

//! Search handle to an asynchronous solve call.
//!
//! @see clingo_control_solve_async()
typedef struct clingo_solve_async clingo_solve_async_t;
//! Get the solve result.
//!
//! Blocks until the search is completed.
//!
//! @param[in] handle the target
//! @param[out] result the solve result
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if solving fails
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_async_get(clingo_solve_async_t *handle, clingo_solve_result_bitset_t *result);
//! Wait for the specified amount of time to check if the search has finished.
//!
//! If the time is set to zero, this function can be used to poll if the search
//! is still running.
//!
//! @param[in] handle the target
//! @param[in] timeout the maximum time to wait
//! @param[out] result whether the search is still running
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if solving fails
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_async_wait(clingo_solve_async_t *handle, double timeout, bool *result);
//! Stop the running search.
//!
//! Blocks until the search is stopped.
//!
//! @param[in] handle the target
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if solving fails
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_async_cancel(clingo_solve_async_t *handle);

//! @}

// {{{1 symbolic atoms

//! @example symbolic-atoms.c
//! The example shows how to iterate over symbolic atoms.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! ./symbolic-atoms 0
//! Symbolic atoms:
//!   b
//!   c, external
//!   a, fact
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @defgroup SymbolicAtoms Symbolic Atom Inspection
//! Inspection of atoms occurring in ground logic programs.
//!
//! For an example, see @ref symbolic-atoms.c.
//! @ingroup Control

//! @addtogroup SymbolicAtoms
//! @{

//! Object to inspect symbolic atoms in a program---the relevant Herbrand base
//! gringo uses to instantiate programs.
//!
//! @see clingo_control_symbolic_atoms()
typedef struct clingo_symbolic_atoms clingo_symbolic_atoms_t;
//! Object to iterate over symbolic atoms.
//!
//! Such an iterator either points to a symbolic atom within a sequence of
//! symbolic atoms or to the end of the sequence.
//!
//! @note Iterators are valid as long as the underlying sequence is not modified.
//! Operations that can change this sequence are ::clingo_control_ground(),
//! ::clingo_control_cleanup(), and functions that modify the underlying
//! non-ground program.
typedef uint64_t clingo_symbolic_atom_iterator_t;
//! Get the number of different atoms occurring in a logic program.
//!
//! @param[in] atoms the target
//! @param[out] size the number of atoms
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_size(clingo_symbolic_atoms_t *atoms, size_t *size);
//! Get a forward iterator to the beginning of the sequence of all symbolic
//! atoms optionally restricted to a given signature.
//!
//! @param[in] atoms the target
//! @param[in] signature optional signature
//! @param[out] iterator the resulting iterator
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_begin(clingo_symbolic_atoms_t *atoms, clingo_signature_t const *signature, clingo_symbolic_atom_iterator_t *iterator);
//! Iterator pointing to the end of the sequence of symbolic atoms.
//!
//! @param[in] atoms the target
//! @param[out] iterator the resulting iterator
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_end(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iterator_t *iterator);
//! Find a symbolic atom given its symbolic representation.
//!
//! @param[in] atoms the target
//! @param[in] symbol the symbol to lookup
//! @param[out] iterator iterator pointing to the symbolic atom or to the end
//! of the sequence if no corresponding atom is found
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_find(clingo_symbolic_atoms_t *atoms, clingo_symbol_t symbol, clingo_symbolic_atom_iterator_t *iterator);
//! Check if two iterators point to the same element (or end of the sequence).
//!
//! @param[in] atoms the target
//! @param[in] a the first iterator
//! @param[in] b the second iterator
//! @param[out] equal whether the two iterators are equal
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_iterator_is_equal_to(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iterator_t a, clingo_symbolic_atom_iterator_t b, bool *equal);
//! Get the symbolic representation of an atom.
//!
//! @param[in] atoms the target
//! @param[in] iterator iterator to the atom
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_symbol(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iterator_t iterator, clingo_symbol_t *symbol);
//! Check whether an atom is a fact.
//!
//! @note This does not determine if an atom is a cautious consequence. The
//! grounding or solving component's simplifications can only detect this in
//! some cases.
//!
//! @param[in] atoms the target
//! @param[in] iterator iterator to the atom
//! @param[out] fact whether the atom is a fact
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_is_fact(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iterator_t iterator, bool *fact);
//! Check whether an atom is external.
//!
//! An atom is external if it has been defined using an external directive and
//! has not been released or defined by a rule.
//!
//! @param[in] atoms the target
//! @param[in] iterator iterator to the atom
//! @param[out] external whether the atom is a external
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_is_external(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iterator_t iterator, bool *external);
//! Returns the (numeric) aspif literal corresponding to the given symbolic atom.
//!
//! Such a literal can be mapped to a solver literal (see the \ref Propagator
//! module) or be used in rules in aspif format (see the \ref ProgramBuilder
//! module).
//!
//! @param[in] atoms the target
//! @param[in] iterator iterator to the atom
//! @param[out] literal the associated literal
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_literal(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iterator_t iterator, clingo_literal_t *literal);
//! Get the number of different predicate signatures used in the program.
//!
//! @param[in] atoms the target
//! @param[out] size the number of signatures
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_signatures_size(clingo_symbolic_atoms_t *atoms, size_t *size);
//! Get the predicate signatures occurring in a logic program.
//!
//! @param[in] atoms the target
//! @param[out] signatures the resulting signatures
//! @param[in] size the number of signatures
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if the size is too small
//!
//! @see clingo_symbolic_atoms_signatures_size()
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_signatures(clingo_symbolic_atoms_t *atoms, clingo_signature_t *signatures, size_t size);
//! Get an iterator to the next element in the sequence of symbolic atoms.
//!
//! @param[in] atoms the target
//! @param[in] iterator the current iterator
//! @param[out] next the succeeding iterator
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_next(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iterator_t iterator, clingo_symbolic_atom_iterator_t *next);
//! Check whether the given iterator points to some element with the sequence
//! of symbolic atoms or to the end of the sequence.
//!
//! @param[in] atoms the target
//! @param[in] iterator the iterator
//! @param[out] valid whether the iterator points to some element within the
//! sequence
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbolic_atoms_is_valid(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iterator_t iterator, bool *valid);

//! @}

// {{{1 theory atoms

//! @example theory-atoms.c
//! The example shows how to inspect and use theory atoms.
//!
//! @verbatim@endverbatim
//!
//! This is a very simple example that uses the @link ProgramBuilder backend@endlink to let theory atoms affect answer sets.
//! In general, the backend can be used to implement a custom theory by translating it to a logic program.
//! On the other hand, a @link Propagator propagator@endlink can be used to implement a custom theory without adding any constraints in advance.
//! Or both approaches can be combined.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! ./theory-atoms 0
//! number of grounded theory atoms: 2
//! theory atom b/1 has a guard: true
//! Model: y
//! Model: x y
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @defgroup TheoryAtoms Theory Atom Inspection
//! Inspection of theory atoms occurring in ground logic programs.
//! @ingroup Control
//!
//! During grounding, theory atoms get consecutive numbers starting with zero.
//! The total number of theory atoms can be obtained using clingo_theory_atoms_size().
//!
//! @attention
//! All structural information about theory atoms, elements, and terms is reset after @link clingo_control_solve() solving@endlink.
//! If afterward fresh theory atoms are @link clingo_control_ground() grounded@endlink, previously used ids are reused.
//!
//! For an example, see @ref theory-atoms.c.

//! @addtogroup TheoryAtoms
//! @{

//! Enumeration of theory term types.
enum clingo_theory_term_type {
    clingo_theory_term_type_tuple,    //!< a tuple term, e.g., `(1,2,3)`
    clingo_theory_term_type_list,     //!< a list term, e.g., `[1,2,3]`
    clingo_theory_term_type_set,      //!< a set term, e.g., `{1,2,3}`
    clingo_theory_term_type_function, //!< a function term, e.g., `f(1,2,3)`
    clingo_theory_term_type_number,   //!< a number term, e.g., `42`
    clingo_theory_term_type_symbol    //!< a symbol term, e.g., `c`
};
//! Corresponding type to ::clingo_theory_term_type.
typedef int clingo_theory_term_type_t;

//! Container that stores theory atoms, elements, and terms (see @ref clingo_control_theory_atoms()).
typedef struct clingo_theory_atoms clingo_theory_atoms_t;

//! @name Theory Term Inspection
//! @{

//! Get the type of the given theory term.
//!
//! @param[in] atoms container where the term is stored
//! @param[in] term id of the term
//! @param[out] type the resulting type
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_term_type(clingo_theory_atoms_t *atoms, clingo_id_t term, clingo_theory_term_type_t *type);
//! Get the number of the given numeric theory term.
//!
//! @pre The term must be of type ::clingo_theory_term_type_number.
//! @param[in] atoms container where the term is stored
//! @param[in] term id of the term
//! @param[out] number the resulting number
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_term_number(clingo_theory_atoms_t *atoms, clingo_id_t term, int *number);
//! Get the name of the given constant or function theory term.
//!
//! @pre The term must be of type ::clingo_theory_term_type_function or ::clingo_theory_term_type_symbol.
//! @param[in] atoms container where the term is stored
//! @param[in] term id of the term
//! @param[out] name the resulting name
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_term_name(clingo_theory_atoms_t *atoms, clingo_id_t term, char const **name);
//! Get the arguments of the given function theory term.
//!
//! @pre The term must be of type ::clingo_theory_term_type_function.
//! @param[in] atoms container where the term is stored
//! @param[in] term id of the term
//! @param[out] arguments the resulting arguments in form of an array of term ids
//! @param[out] size the number of arguments
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_term_arguments(clingo_theory_atoms_t *atoms, clingo_id_t term, clingo_id_t const **arguments, size_t *size);
//! Get the size of the string representation of the given theory term (including the terminating 0).
//!
//! @param[in] atoms container where the term is stored
//! @param[in] term id of the term
//! @param[out] size the resulting size
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_term_to_string_size(clingo_theory_atoms_t *atoms, clingo_id_t term, size_t *size);
//! Get the string representation of the given theory term.
//!
//! @param[in] atoms container where the term is stored
//! @param[in] term id of the term
//! @param[out] string the resulting string
//! @param[in] size the size of the string
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if the size is too small
//! - ::clingo_error_bad_alloc
//!
//! @see clingo_theory_atoms_term_to_string_size()
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_term_to_string(clingo_theory_atoms_t *atoms, clingo_id_t term, char *string, size_t size);
//! @}

//! @name Theory Element Inspection
//! @{

//! Get the tuple (array of theory terms) of the given theory element.
//!
//! @param[in] atoms container where the element is stored
//! @param[in] element id of the element
//! @param[out] tuple the resulting array of term ids
//! @param[out] size the number of term ids
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_element_tuple(clingo_theory_atoms_t *atoms, clingo_id_t element, clingo_id_t const **tuple, size_t *size);
//! Get the condition (array of aspif literals) of the given theory element.
//!
//! @param[in] atoms container where the element is stored
//! @param[in] element id of the element
//! @param[out] condition the resulting array of aspif literals
//! @param[out] size the number of term literals
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_element_condition(clingo_theory_atoms_t *atoms, clingo_id_t element, clingo_literal_t const **condition, size_t *size);
//! Get the id of the condition of the given theory element.
//!
//! @note
//! This id can be mapped to a solver literal using clingo_propagate_init_solver_literal().
//! This id is not (necessarily) an aspif literal;
//! to get aspif literals use clingo_theory_atoms_element_condition().
//!
//! @param[in] atoms container where the element is stored
//! @param[in] element id of the element
//! @param[out] condition the resulting condition id
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_element_condition_id(clingo_theory_atoms_t *atoms, clingo_id_t element, clingo_literal_t *condition);
//! Get the size of the string representation of the given theory element (including the terminating 0).
//!
//! @param[in] atoms container where the element is stored
//! @param[in] element id of the element
//! @param[out] size the resulting size
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_element_to_string_size(clingo_theory_atoms_t *atoms, clingo_id_t element, size_t *size);
//! Get the string representation of the given theory element.
//!
//! @param[in] atoms container where the element is stored
//! @param[in] element id of the element
//! @param[out] string the resulting string
//! @param[in] size the size of the string
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if the size is too small
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_element_to_string(clingo_theory_atoms_t *atoms, clingo_id_t element, char *string, size_t size);
//! @}

//! @name Theory Atom Inspection
//! @{

//! Get the total number of theory atoms.
//!
//! @param[in] atoms the target
//! @param[out] size the resulting number
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_size(clingo_theory_atoms_t *atoms, size_t *size);
//! Get the theory term associated with the theory atom.
//!
//! @param[in] atoms container where the atom is stored
//! @param[in] atom id of the atom
//! @param[out] term the resulting term id
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_atom_term(clingo_theory_atoms_t *atoms, clingo_id_t atom, clingo_id_t *term);
//! Get the theory elements associated with the theory atom.
//!
//! @param[in] atoms container where the atom is stored
//! @param[in] atom id of the atom
//! @param[out] elements the resulting array of elements
//! @param[out] size the number of elements
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_atom_elements(clingo_theory_atoms_t *atoms, clingo_id_t atom, clingo_id_t const **elements, size_t *size);
//! Whether the theory atom has a guard.
//!
//! @param[in] atoms container where the atom is stored
//! @param[in] atom id of the atom
//! @param[out] has_guard whether the theory atom has a guard
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_atom_has_guard(clingo_theory_atoms_t *atoms, clingo_id_t atom, bool *has_guard);
//! Get the guard consisting of a theory operator and a theory term of the given theory atom.
//!
//! @param[in] atoms container where the atom is stored
//! @param[in] atom id of the atom
//! @param[out] connective the resulting theory operator
//! @param[out] term the resulting term
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_atom_guard(clingo_theory_atoms_t *atoms, clingo_id_t atom, char const **connective, clingo_id_t *term);
//! Get the aspif literal associated with the given theory atom.
//!
//! @param[in] atoms container where the atom is stored
//! @param[in] atom id of the atom
//! @param[out] literal the resulting literal
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_atom_literal(clingo_theory_atoms_t *atoms, clingo_id_t atom, clingo_literal_t *literal);
//! Get the size of the string representation of the given theory atom (including the terminating 0).
//!
//! @param[in] atoms container where the atom is stored
//! @param[in] atom id of the element
//! @param[out] size the resulting size
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_atom_to_string_size(clingo_theory_atoms_t *atoms, clingo_id_t atom, size_t *size);
//! Get the string representation of the given theory atom.
//!
//! @param[in] atoms container where the atom is stored
//! @param[in] atom id of the element
//! @param[out] string the resulting string
//! @param[in] size the size of the string
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if the size is too small
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_atoms_atom_to_string(clingo_theory_atoms_t *atoms, clingo_id_t atom, char *string, size_t size);
//! @}

//! @}

// {{{1 propagator

//! @example propagator.c
//! The example shows how to write a simple propagator for the pigeon hole problem. For
//! a detailed description of what is implemented here and some background, take a look at the following paper:
//!
//! http://www.cs.uni-potsdam.de/wv/pdfformat/gekakaosscwa16b.pdf
//!
//! ## Output ##
//!
//! The output is empty because the pigeon hole problem is unsatisfiable.
//!
//! ## Code ##

//! @defgroup Propagator Theory Propagation
//! Extend the search with propagators for arbitrary theories.
//!
//! For an example, see @ref propagator.c.
//! @ingroup Control

//! @addtogroup Propagator
//! @{

//! Object to initialize a used-defined propagator before each solving step.
//!
//! Each @link SymbolicAtoms symbolic@endlink or @link TheoryAtoms theory atom@endlink is uniquely associated with an aspif atom in form of a positive integer (@ref ::clingo_literal_t).
//! Aspif literals additionally are signed to represent default negation.
//! Furthermore, there are non-zero integer solver literals (also represented using @ref ::clingo_literal_t).
//! There is a surjective mapping from program atoms to solver literals.
//!
//! All methods called during propagation use solver literals whereas clingo_symbolic_atoms_literal() and clingo_theory_atoms_atom_literal() return program literals.
//! The function clingo_propagate_init_solver_literal() can be used to map program literals or @link clingo_theory_atoms_element_condition_id() condition ids@endlink to solver literals.
typedef struct clingo_propagate_init clingo_propagate_init_t;

//! @name Initialization Functions
//! @{

//! Map the given program literal or condition id to its solver literal.
//!
//! @param[in] init the target
//! @param[in] aspif_literal the aspif literal to map
//! @param[out] solver_literal the resulting solver literal
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_solver_literal(clingo_propagate_init_t *init, clingo_literal_t aspif_literal, clingo_literal_t *solver_literal);
//! Add a watch for the solver literal in the given phase.
//!
//! @param[in] init the target
//! @param[out] solver_literal the solver literal
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_add_watch(clingo_propagate_init_t *init, clingo_literal_t solver_literal);
//! Get an object to inspect the symbolic atoms.
//!
//! @param[in] init the target
//! @param[out] atoms the resulting object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_symbolic_atoms(clingo_propagate_init_t *init, clingo_symbolic_atoms_t **atoms);
//! Get an object to inspect the theory atoms.
//!
//! @param[in] init the target
//! @param[out] atoms the resulting object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_theory_atoms(clingo_propagate_init_t *init, clingo_theory_atoms_t **atoms);
//! Get the number of threads used in subsequent solving.
//!
//! @param[in] init the target
//! @return the number of threads
//! @see clingo_propagate_control_thread_id()
CLINGO_VISIBILITY_DEFAULT int clingo_propagate_init_number_of_threads(clingo_propagate_init_t *init);

//! @}

//! Represents a (partial) assignment of a particular solver.
//!
//! An assignment assigns truth values to a set of literals.
//! A literal is assigned to either @link clingo_assignment_truth_value() true or false, or is unassigned@endlink.
//! Furthermore, each assigned literal is associated with a @link clingo_assignment_level() decision level@endlink.
//! There is exactly one @link clingo_assignment_decision() decision literal@endlink for each decision level greater than zero.
//! Assignments to all other literals on the same level are consequences implied by the current and possibly previous decisions.
//! Assignments on level zero are immediate consequences of the current program.
//! Decision levels are consecutive numbers starting with zero up to and including the @link clingo_assignment_decision_level() current decision level@endlink.
typedef struct clingo_assignment clingo_assignment_t;

//! @name Assignment Functions
//! @{

//! Get the current decision level.
//!
//! @param[in] assignment the target assignment
//! @return the decision level
CLINGO_VISIBILITY_DEFAULT uint32_t clingo_assignment_decision_level(clingo_assignment_t *assignment);
//! Check if the given assignment is conflicting.
//!
//! @param[in] assignment the target assignment
//! @return whether the assignment is conflicting
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_has_conflict(clingo_assignment_t *assignment);
//! Check if the given literal is part of a (partial) assignment.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @return whether the literal is valid
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_has_literal(clingo_assignment_t *assignment, clingo_literal_t literal);
//! Determine the decision level of a given literal.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @param[out] level the resulting level
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_level(clingo_assignment_t *assignment, clingo_literal_t literal, uint32_t *level);
//! Determine the decision literal given a decision level.
//!
//! @param[in] assignment the target assignment
//! @param[in] level the level
//! @param[out] literal the resulting literal
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_decision(clingo_assignment_t *assignment, uint32_t level, clingo_literal_t *literal);
//! Check if a literal has a fixed truth value.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @param[out] is_fixed whether the literal is fixed
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_is_fixed(clingo_assignment_t *assignment, clingo_literal_t literal, bool *is_fixed);
//! Check if a literal is true.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @param[out] is_true whether the literal is true
//! @return whether the call was successful
//! @see clingo_assignment_truth_value()
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_is_true(clingo_assignment_t *assignment, clingo_literal_t literal, bool *is_true);
//! Check if a literal has a fixed truth value.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @param[out] is_false whether the literal is false
//! @return whether the call was successful
//! @see clingo_assignment_truth_value()
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_is_false(clingo_assignment_t *assignment, clingo_literal_t literal, bool *is_false);
//! Determine the truth value of a given literal.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @param[out] value the resulting truth value
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_truth_value(clingo_assignment_t *assignment, clingo_literal_t literal, clingo_truth_value_t *value);

//! @}

//! Enumeration of clause types determining the lifetime of a clause.
//!
//! Clauses in the solver are either cleaned up based on a configurable deletion policy or at the end of a solving step.
//! The values of this enumeration determine if a clause is subject to one of the above deletion strategies.
enum clingo_clause_type {
    clingo_clause_type_learnt          = 0, //!< clause is subject to the solvers deletion policy
    clingo_clause_type_static          = 1, //!< clause is not subject to the solvers deletion policy
    clingo_clause_type_volatile        = 2, //!< like ::clingo_clause_type_learnt but the clause is deleted after a solving step
    clingo_clause_type_volatile_static = 3  //!< like ::clingo_clause_type_static but the clause is deleted after a solving step
};
//! Corresponding type to ::clingo_clause_type.
typedef int clingo_clause_type_t;

//! This object can be used to add clauses and propagate literals while solving.
typedef struct clingo_propagate_control clingo_propagate_control_t;

//! @name Propagation Functions
//! @{

//! Get the id of the underlying solver thread.
//!
//! Thread ids are consecutive numbers starting with zero.
//!
//! @param[in] control the target
//! @return the thread id
CLINGO_VISIBILITY_DEFAULT clingo_id_t clingo_propagate_control_thread_id(clingo_propagate_control_t *control);
//! Get the assignment associated with the underlying solver.
//!
//! @param[in] control the target
//! @return the assignment
CLINGO_VISIBILITY_DEFAULT clingo_assignment_t *clingo_propagate_control_assignment(clingo_propagate_control_t *control);
//! Adds a new volatile literal to the underlying solver thread.
//!
//! @attention The literal is only valid within the current solving step and solver thread.
//! All volatile literals and clauses involving a volatile literal are deleted after the current search.
//!
//! @param[in] control the target
//! @param[out] result the (positive) solver literal
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_logic if the assignment is conflicting
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_add_literal(clingo_propagate_control_t *control, clingo_literal_t *result);
//! Add a watch for the solver literal in the given phase.
//!
//! @note Unlike @ref clingo_propagate_init_add_watch() this does not add a watch to all solver threads but just the current one.
//!
//! @param[in] control the target
//! @param[in] literal the literal to watch
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_logic if the literal is invalid
//! @see clingo_propagate_control_remove_watch()
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_add_watch(clingo_propagate_control_t *control, clingo_literal_t literal);
//! Check whether a literal is watched in the current solver thread.
//!
//! @param[in] control the target
//! @param[in] literal the literal to check
//!
//! @return whether the literal is watched
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_has_watch(clingo_propagate_control_t *control, clingo_literal_t literal);
//! Removes the watch (if any) for the given solver literal.
//!
//! @note Similar to @ref clingo_propagate_init_add_watch() this just removes the watch in the current solver thread.
//!
//! @param[in] control the target
//! @param[in] literal the literal to remove
CLINGO_VISIBILITY_DEFAULT void clingo_propagate_control_remove_watch(clingo_propagate_control_t *control, clingo_literal_t literal);
//! Add the given clause to the solver.
//!
//! This method sets its result to false if the current propagation must be stopped for the solver to backtrack.
//!
//! @attention No further calls on the control object or functions on the assignment should be called when the result of this method is false.
//!
//! @param[in] control the target
//! @param[in] clause the clause to add
//! @param[in] size the size of the clause
//! @param[in] type the clause type determining its lifetime
//! @param[out] result result indicating whether propagation has to be stopped
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_add_clause(clingo_propagate_control_t *control, clingo_literal_t const *clause, size_t size, clingo_clause_type_t type, bool *result);
//! Propagate implied literals (resulting from added clauses).
//!
//! This method sets its result to false if the current propagation must be stopped for the solver to backtrack.
//!
//! @attention No further calls on the control object or functions on the assignment should be called when the result of this method is false.
//!
//! @param[in] control the target
//! @param[out] result result indicating whether propagation has to be stopped
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_propagate(clingo_propagate_control_t *control, bool *result);

//! @}

//! Typedef for @ref ::clingo_propagator::init().
typedef bool (*clingo_propagator_init_callback_t) (clingo_propagate_init_t *, void *);

//! Typedef for @ref ::clingo_propagator::propagate().
typedef bool (*clingo_propagator_propagate_callback_t) (clingo_propagate_control_t *, clingo_literal_t const *, size_t, void *);

//! Typedef for @ref ::clingo_propagator::undo().
typedef bool (*clingo_propagator_undo_callback_t) (clingo_propagate_control_t *, clingo_literal_t const *, size_t, void *);

//! Typedef for @ref ::clingo_propagator::check().
typedef bool (*clingo_propagator_check_callback_t) (clingo_propagate_control_t *, void *);

//! An instance of this struct has to be registered with a solver to implement a custom propagator.
//!
//! Not all callbacks have to be implemented and can be set to NULL if not needed.
//! @see Propagator
typedef struct clingo_propagator {
    //! This function is called once before each solving step.
    //! It is used to map relevant program literals to solver literals, add watches for solver literals, and initialize the data structures used during propagation.
    //!
    //! @note This is the last point to access symbolic and theory atoms.
    //! Once the search has started, they are no longer accessible.
    //!
    //! @param[in] control control object for the target solver
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    //! @see ::clingo_propagator_init_callback_t
    bool (*init) (clingo_propagate_init_t *control, void *data);
    //! Can be used to propagate solver literals given a @link clingo_assignment_t partial assignment@endlink.
    //!
    //! Called during propagation with a non-empty array of @link clingo_propagate_init_add_watch() watched solver literals@endlink
    //! that have been assigned to true since the last call to either propagate, undo, (or the start of the search) - the change set.
    //! Only watched solver literals are contained in the change set.
    //! Each literal in the change set is true w.r.t. the current @link clingo_assignment_t assignment@endlink.
    //! @ref clingo_propagate_control_add_clause() can be used to add clauses.
    //! If a clause is unit resulting, it can be propagated using @ref clingo_propagate_control_propagate().
    //! If the result of either of the two methods is false, the propagate function must return immediately.
    //!
    //! The following snippet shows how to use the methods to add clauses and propagate consequences within the callback.
    //! The important point is to return true (true to indicate there was no error) if the result of either of the methods is false.
    //! ~~~~~~~~~~~~~~~{.c}
    //! bool result;
    //! clingo_literal_t clause[] = { ... };
    //!
    //! // add a clause
    //! if (!clingo_propagate_control_add_clause(control, clause, clingo_clause_type_learnt, &result) { return false; }
    //! if (!result) { return true; }
    //! // propagate its consequences
    //! if (!clingo_propagate_control_propagate(control, &result) { return false; }
    //! if (!result) { return true; }
    //!
    //! // add further clauses and propagate them
    //! ...
    //!
    //! return true;
    //! ~~~~~~~~~~~~~~~
    //!
    //! @note
    //! This function can be called from different solving threads.
    //! Each thread has its own assignment and id, which can be obtained using @ref clingo_propagate_control_thread_id().
    //!
    //! @param[in] control control object for the target solver
    //! @param[in] changes the change set
    //! @param[in] size the size of the change set
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    //! @see ::clingo_propagator_propagate_callback_t
    bool (*propagate) (clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, void *data);
    //! Called whenever a solver undoes assignments to watched solver literals.
    //!
    //! This callback is meant to update assignment dependent state in the propagator.
    //!
    //! @note No clauses must be propagated in this callback.
    //!
    //! @param[in] control control object for the target solver
    //! @param[in] changes the change set
    //! @param[in] size the size of the change set
    //! @param[in] data user data for the callback
    //! @see ::clingo_propagator_undo_callback_t
    bool (*undo) (clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, void *data);
    //! This function is similar to @ref clingo_propagate_control_propagate() but is only called on total assignments without a change set.
    //!
    //! @note This function is called even if no watches have been added.
    //! @param[in] control control object for the target solver
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    //! @see ::clingo_propagator_check_callback_t
    bool (*check) (clingo_propagate_control_t *control, void *data);
} clingo_propagator_t;

//! @}

// {{{1 backend

//! @example backend.c
//! The example shows how to used the backend to extend a grounded program.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! ./backend 0
//! Model: a b
//! Model: a b c
//! Model:
//! Model: a
//! Model: b
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @defgroup ProgramBuilder Program Building
//! Add non-ground program representations (ASTs) to logic programs or extend the ground (aspif) program.
//! @ingroup Control
//!
//! For an example about ground logic programs, see @ref backend.c.
//! For an example about non-ground logic programs, see @ref ast.c and the @ref AST module.

//! @addtogroup ProgramBuilder
//! @{

//! Enumeration of different heuristic modifiers.
//! @ingroup ProgramInspection
enum clingo_heuristic_type {
    clingo_heuristic_type_level  = 0, //!< set the level of an atom
    clingo_heuristic_type_sign   = 1, //!< configure which sign to chose for an atom
    clingo_heuristic_type_factor = 2, //!< modify VSIDS factor of an atom
    clingo_heuristic_type_init   = 3, //!< modify the initial VSIDS score of an atom
    clingo_heuristic_type_true   = 4, //!< set the level of an atom and choose a positive sign
    clingo_heuristic_type_false  = 5  //!< set the level of an atom and choose a negative sign
};
//! Corresponding type to ::clingo_heuristic_type.
//! @ingroup ProgramInspection
typedef int clingo_heuristic_type_t;

//! Enumeration of different external statements.
//! @ingroup ProgramInspection
enum clingo_external_type {
    clingo_external_type_free    = 0, //!< allow an external to be assigned freely
    clingo_external_type_true    = 1, //!< assign an external to true
    clingo_external_type_false   = 2, //!< assign an external to false
    clingo_external_type_release = 3, //!< no longer treat an atom as external
};
//! Corresponding type to ::clingo_external_type.
//! @ingroup ProgramInspection
typedef int clingo_external_type_t;

//! A Literal with an associated weight.
//! @ingroup ProgramInspection
typedef struct clingo_weighted_literal {
    clingo_literal_t literal;
    clingo_weight_t weight;
} clingo_weighted_literal_t;

//! Handle to the backend to add directives in aspif format.
typedef struct clingo_backend clingo_backend_t;

//! Add a rule to the program.
//!
//! @param[in] backend the target backend
//! @param[in] choice determines if the head is a choice or a disjunction
//! @param[in] head the head atoms
//! @param[in] head_size the number of atoms in the head
//! @param[in] body the body literals
//! @param[in] body_size the number of literals in the body
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_size, clingo_literal_t const *body, size_t body_size);
//! Add a weight rule to the program.
//!
//! @attention All weights and the lower bound must be positive.
//! @param[in] backend the target backend
//! @param[in] choice determines if the head is a choice or a disjunction
//! @param[in] head the head atoms
//! @param[in] head_size the number of atoms in the head
//! @param[in] lower_bound the lower bound of the weight rule
//! @param[in] body the weighted body literals
//! @param[in] body_size the number of weighted literals in the body
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_weight_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_size, clingo_weight_t lower_bound, clingo_weighted_literal_t const *body, size_t body_size);
//! Add a minimize constraint (or weak constraint) to the program.
//!
//! @param[in] backend the target backend
//! @param[in] priority the priority of the constraint
//! @param[in] literals the weighted literals whose sum to minimize
//! @param[in] size the number of weighted literals
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_minimize(clingo_backend_t *backend, clingo_weight_t priority, clingo_weighted_literal_t const* literals, size_t size);
//! Add a projection directive.
//!
//! @param[in] backend the target backend
//! @param[in] atoms the atoms to project on
//! @param[in] size the number of atoms
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_project(clingo_backend_t *backend, clingo_atom_t const *atoms, size_t size);
//! Add an external statement.
//!
//! @param[in] backend the target backend
//! @param[in] atom the external atom
//! @param[in] type the type of the external statement
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_external(clingo_backend_t *backend, clingo_atom_t atom, clingo_external_type_t type);
//! Add an assumption directive.
//!
//! @param[in] backend the target backend
//! @param[in] literals the literals to assume (positive literals are true and negative literals false for the next solve call)
//! @param[in] size the number of atoms
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_assume(clingo_backend_t *backend, clingo_literal_t const *literals, size_t size);
//! Add an heuristic directive.
//!
//! @param[in] backend the target backend
//! @param[in] atom the target atom
//! @param[in] type the type of the heuristic modification
//! @param[in] bias the heuristic bias
//! @param[in] priority the heuristic priority
//! @param[in] condition the condition under which to apply the heuristic modification
//! @param[in] size the number of atoms in the condition
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_heuristic(clingo_backend_t *backend, clingo_atom_t atom, clingo_heuristic_type_t type, int bias, unsigned priority, clingo_literal_t const *condition, size_t size);
//! Add an edge directive.
//!
//! @param[in] backend the target backend
//! @param[in] node_u the start vertex of the edge
//! @param[in] node_v the end vertex of the edge
//! @param[in] condition the condition under which the edge is part of the graph
//! @param[in] size the number of atoms in the condition
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_acyc_edge(clingo_backend_t *backend, int node_u, int node_v, clingo_literal_t const *condition, size_t size);
//! Get a fresh atom to be used in aspif directives.
//!
//! @param[in] backend the target backend
//! @param[out] atom the resulting atom
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_add_atom(clingo_backend_t *backend, clingo_atom_t *atom);

//! @}

// {{{1 configuration

//! @example configuration.c
//! The example shows how to configure the solver.
//!
//! @note It is also possible to loop over all configuration entries.
//! This can be done in a similar fashion as in the @ref statistics.c example.
//! But note that, unlike with statistics entries, a configuration entry can have more than one type.
//!
//! ## Output ##
//!
//! ~~~~~~~~
//! ./configuration
//! Model: a
//! Model: b
//! ~~~~~~~~
//!
//! ## Code ##

//! @defgroup Configuration Solver Configuration
//! Configuration of search and enumeration algorithms.
//!
//! Entries in a configuration are organized hierarchically.
//! Subentries are either accessed by name for map entries or by offset for array entries.
//! Value entries have a string value that can be inspected or modified.
//!
//! For an example, see @ref configuration.c.
//! @ingroup Control

//! @addtogroup Configuration
//! @{

//! Enumeration for entries of the configuration.
enum clingo_configuration_type {
    clingo_configuration_type_value = 1, //!< the entry is a (string) value
    clingo_configuration_type_array = 2, //!< the entry is an array
    clingo_configuration_type_map   = 4  //!< the entry is a map
};
//! Bitset for values of type ::clingo_configuration_type.
typedef unsigned clingo_configuration_type_bitset_t;

//! Handle for to the solver configuration.
typedef struct clingo_configuration clingo_configuration_t;

//! Get the root key of the configuration.
//!
//! @param[in] configuration the target configuration
//! @param[out] key the root key
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_root(clingo_configuration_t *configuration, clingo_id_t *key);
//! Get the type of a key.
//!
//! @note The type is bitset, an entry can have multiple (but at least one) type.
//!
//! @param[in] configuration the target configuration
//! @param[in] key the key
//! @param[out] type the resulting type
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_type(clingo_configuration_t *configuration, clingo_id_t key, clingo_configuration_type_bitset_t *type);
//! Get the description of an entry.
//!
//! @param[in] configuration the target configuration
//! @param[in] key the key
//! @param[out] description the description
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_description(clingo_configuration_t *configuration, clingo_id_t key, char const **description);

//! @name Functions to access arrays
//! @{

//! Get the size of an array entry.
//!
//! @pre The @link clingo_configuration_type() type@endlink of the entry must be @ref ::clingo_configuration_type_array.
//! @param[in] configuration the target configuration
//! @param[in] key the key
//! @param[out] size the resulting size
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_array_size(clingo_configuration_t *configuration, clingo_id_t key, size_t *size);
//! Get the subkey at the given offset of an array entry.
//!
//! @note Some array entries, like fore example the solver configuration, can be accessed past there actual size to add subentries.
//! @pre The @link clingo_configuration_type() type@endlink of the entry must be @ref ::clingo_configuration_type_array.
//! @param[in] configuration the target configuration
//! @param[in] key the key
//! @param[in] offset the offset in the array
//! @param[out] subkey the resulting subkey
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_array_at(clingo_configuration_t *configuration, clingo_id_t key, size_t offset, clingo_id_t *subkey);
//! @}

//! @name Functions to access maps
//! @{

//! Get the number of subkeys of a map entry.
//!
//! @pre The @link clingo_configuration_type() type@endlink of the entry must be @ref ::clingo_configuration_type_map.
//! @param[in] configuration the target configuration
//! @param[in] key the key
//! @param[out] size the resulting number
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_map_size(clingo_configuration_t *configuration, clingo_id_t key, size_t* size);
//! Get the name associated with the offset-th subkey.
//!
//! @pre The @link clingo_configuration_type() type@endlink of the entry must be @ref ::clingo_configuration_type_map.
//! @param[in] configuration the target configuration
//! @param[in] key the key
//! @param[in] offset the offset of the name
//! @param[out] name the resulting name
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_map_subkey_name(clingo_configuration_t *configuration, clingo_id_t key, size_t offset, char const **name);
//! Lookup a subkey under the given name.
//!
//! @pre The @link clingo_configuration_type() type@endlink of the entry must be @ref ::clingo_configuration_type_map.
//! @note Multiple levels can be looked up by concatenating keys with a period.
//! @param[in] configuration the target configuration
//! @param[in] key the key
//! @param[in] name the name to lookup the subkey
//! @param[out] subkey the resulting subkey
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_map_at(clingo_configuration_t *configuration, clingo_id_t key, char const *name, clingo_id_t* subkey);
//! @}

//! @name Functions to access values
//! @{

//! Check whether a entry has a value.
//!
//! @pre The @link clingo_configuration_type() type@endlink of the entry must be @ref ::clingo_configuration_type_value.
//! @param[in] configuration the target configuration
//! @param[in] key the key
//! @param[out] assigned whether the entry has a value
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_value_is_assigned(clingo_configuration_t *configuration, clingo_id_t key, bool *assigned);
//! Get the size of the string value of the given entry.
//!
//! @pre The @link clingo_configuration_type() type@endlink of the entry must be @ref ::clingo_configuration_type_value.
//! @param[in] configuration the target configuration
//! @param[in] key the key
//! @param[out] size the resulting size
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_value_get_size(clingo_configuration_t *configuration, clingo_id_t key, size_t *size);
//! Get the string value of the given entry.
//!
//! @pre The @link clingo_configuration_type() type@endlink of the entry must be @ref ::clingo_configuration_type_value.
//! @pre The given size must be larger or equal to size of the value.
//! @param[in] configuration the target configuration
//! @param[in] key the key
//! @param[out] value the resulting string value
//! @param[in] size the size of the given char array
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_value_get(clingo_configuration_t *configuration, clingo_id_t key, char *value, size_t size);
//! Set the value of an entry.
//!
//! @pre The @link clingo_configuration_type() type@endlink of the entry must be @ref ::clingo_configuration_type_value.
//! @param[in] configuration the target configuration
//! @param[in] key the key
//! @param[in] value the value to set
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_configuration_value_set(clingo_configuration_t *configuration, clingo_id_t key, char const *value);
//! @}

//! @}

// {{{1 statistics

//! @example statistics.c
//! The example shows how to inspect statistics.
//!
//! ## Output ##
//!
//! ~~~~~~~~
//! ./statistics 0
//! Model: a
//! Model: b
//! problem:
//!   lp:
//!     atoms:
//!       2
//!     atoms_aux:
//!       0
//!     ...
//! solving:
//!   ...
//! summary:
//!   ...
//! accu:
//!   times:
//!     ...
//!   models:
//!     ...
//!   solving:
//!     ...
//! ~~~~~~~~
//!
//! ## Code ##

//! @defgroup Statistics Statistics
//! Inspect search and problem statistics.
//!
//! @ingroup Control
//! For an example, see @ref statistics.c.

//! @addtogroup Statistics
//! @{

//! Enumeration for entries of the statistics.
enum clingo_statistics_type {
    clingo_statistics_type_empty = 0, //!< the entry is invalid (has neither of the types below)
    clingo_statistics_type_value = 1, //!< the entry is a (string) value
    clingo_statistics_type_array = 2, //!< the entry is an array
    clingo_statistics_type_map   = 3  //!< the entry is a map
};
//! Corresponding type to ::clingo_solve_result.
typedef int clingo_statistics_type_t;

//! Handle for to the solver statistics.
typedef struct clingo_statistic clingo_statistics_t;

//! Get the root key of the statistics.
//!
//! @param[in] statistics the target statistics
//! @param[out] key the root key
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_statistics_root(clingo_statistics_t *statistics, uint64_t *key);
//! Get the type of a key.
//!
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[out] type the resulting type
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_statistics_type(clingo_statistics_t *statistics, uint64_t key, clingo_statistics_type_t *type);

//! @name Functions to access arrays
//! @{

//! Get the size of an array entry.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_array.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[out] size the resulting size
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_statistics_array_size(clingo_statistics_t *statistics, uint64_t key, size_t *size);
//! Get the subkey at the given offset of an array entry.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_array.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[in] offset the offset in the array
//! @param[out] subkey the resulting subkey
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_statistics_array_at(clingo_statistics_t *statistics, uint64_t key, size_t offset, uint64_t *subkey);
//! @}

//! @name Functions to access maps
//! @{

//! Get the number of subkeys of a map entry.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_map.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[out] size the resulting number
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_statistics_map_size(clingo_statistics_t *statistics, uint64_t key, size_t *size);
//! Get the name associated with the offset-th subkey.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_map.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[in] offset the offset of the name
//! @param[out] name the resulting name
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_statistics_map_subkey_name(clingo_statistics_t *statistics, uint64_t key, size_t offset, char const **name);
//! Lookup a subkey under the given name.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_map.
//! @note Multiple levels can be looked up by concatenating keys with a period.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[in] name the name to lookup the subkey
//! @param[out] subkey the resulting subkey
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_statistics_map_at(clingo_statistics_t *statistics, uint64_t key, char const *name, uint64_t *subkey);
//! @}

//! @name Functions to inspect values
//! @{

//! Get the value of the given entry.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_value.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[out] value the resulting value
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_statistics_value_get(clingo_statistics_t *statistics, uint64_t key, double *value);
//! @}

//! @}

// {{{1 ast

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
//! @warning There might still be changes to this part of the API and there is not much documentation yet.
//! In its current form the interface is rather large
//! but has the advantage that the structure of a logic program is (hopefully) self-explanatory.
//! The API could also be much reduced by, for example, just providing one node type as done in [clingo's python API](https://potassco.org/clingo/python-api/current/clingo.html).
//! [Feedback would be very welcome!](https://github.com/potassco/clingo/issues)
//!
//! For an example, see @ref ast.c.

//! @addtogroup AST
//! @{

enum clingo_ast_comparison_operator {
    clingo_ast_comparison_operator_greater_than  = 0,
    clingo_ast_comparison_operator_less_than     = 1,
    clingo_ast_comparison_operator_less_equal    = 2,
    clingo_ast_comparison_operator_greater_equal = 3,
    clingo_ast_comparison_operator_not_equal     = 4,
    clingo_ast_comparison_operator_equal         = 5
};
typedef int clingo_ast_comparison_operator_t;

enum clingo_ast_sign {
    clingo_ast_sign_none            = 0,
    clingo_ast_sign_negation        = 1,
    clingo_ast_sign_double_negation = 2
};
typedef int clingo_ast_sign_t;

// {{{2 terms

enum clingo_ast_term_type {
    clingo_ast_term_type_symbol            = 0,
    clingo_ast_term_type_variable          = 1,
    clingo_ast_term_type_unary_operation   = 2,
    clingo_ast_term_type_binary_operation  = 3,
    clingo_ast_term_type_interval          = 4,
    clingo_ast_term_type_function          = 5,
    clingo_ast_term_type_external_function = 6,
    clingo_ast_term_type_pool              = 7
};
typedef int clingo_ast_term_type_t;

typedef struct clingo_ast_unary_operation clingo_ast_unary_operation_t;
typedef struct clingo_ast_binary_operation clingo_ast_binary_operation_t;
typedef struct clingo_ast_interval clingo_ast_interval_t;
typedef struct clingo_ast_function clingo_ast_function_t;
typedef struct clingo_ast_pool clingo_ast_pool_t;
typedef struct clingo_ast_term {
    clingo_location_t location;
    clingo_ast_term_type_t type;
    union {
        clingo_symbol_t symbol;
        char const *variable;
        clingo_ast_unary_operation_t const *unary_operation;
        clingo_ast_binary_operation_t const *binary_operation;
        clingo_ast_interval_t const *interval;
        clingo_ast_function_t const *function;
        clingo_ast_function_t const *external_function;
        clingo_ast_pool_t const *pool;
    };
} clingo_ast_term_t;

// unary operation

enum clingo_ast_unary_operator {
    clingo_ast_unary_operator_minus    = 0,
    clingo_ast_unary_operator_negation = 1,
    clingo_ast_unary_operator_absolute = 2
};
typedef int clingo_ast_unary_operator_t;

struct clingo_ast_unary_operation {
    clingo_ast_unary_operator_t unary_operator;
    clingo_ast_term_t argument;
};

// binary operation

enum clingo_ast_binary_operator {
    clingo_ast_binary_operator_xor            = 0,
    clingo_ast_binary_operator_or             = 1,
    clingo_ast_binary_operator_and            = 2,
    clingo_ast_binary_operator_plus           = 3,
    clingo_ast_binary_operator_minus          = 4,
    clingo_ast_binary_operator_multiplication = 5,
    clingo_ast_binary_operator_division       = 6,
    clingo_ast_binary_operator_modulo         = 7

};
typedef int clingo_ast_binary_operator_t;

struct clingo_ast_binary_operation {
    clingo_ast_binary_operator_t binary_operator;
    clingo_ast_term_t left;
    clingo_ast_term_t right;
};

// interval

struct clingo_ast_interval {
    clingo_ast_term_t left;
    clingo_ast_term_t right;
};

// function

struct clingo_ast_function {
    char const *name;
    clingo_ast_term_t *arguments;
    size_t size;
};

// pool

struct clingo_ast_pool {
    clingo_ast_term_t *arguments;
    size_t size;
};

// {{{2 csp

typedef struct clingo_ast_csp_product_term {
    clingo_location_t location;
    clingo_ast_term_t coefficient;
    clingo_ast_term_t const *variable;
} clingo_ast_csp_product_term_t;

typedef struct clingo_ast_csp_sum_term {
    clingo_location_t location;
    clingo_ast_csp_product_term_t *terms;
    size_t size;
} clingo_ast_csp_sum_term_t;

typedef struct clingo_ast_csp_guard {
    clingo_ast_comparison_operator_t comparison;
    clingo_ast_csp_sum_term_t term;
} clingo_ast_csp_guard_t;

typedef struct clingo_ast_csp_literal {
    clingo_ast_csp_sum_term_t term;
    clingo_ast_csp_guard_t const *guards;
    // NOTE: size must be at least one
    size_t size;
} clingo_ast_csp_literal_t;

// {{{2 ids

typedef struct clingo_ast_id {
    clingo_location_t location;
    char const *id;
} clingo_ast_id_t;

// {{{2 literals

typedef struct clingo_ast_comparison {
    clingo_ast_comparison_operator_t comparison;
    clingo_ast_term_t left;
    clingo_ast_term_t right;
} clingo_ast_comparison_t;

enum clingo_ast_literal_type {
    clingo_ast_literal_type_boolean    = 0,
    clingo_ast_literal_type_symbolic   = 1,
    clingo_ast_literal_type_comparison = 2,
    clingo_ast_literal_type_csp        = 3
};
typedef int clingo_ast_literal_type_t;

typedef struct clingo_ast_literal {
    clingo_location_t location;
    clingo_ast_sign_t sign;
    clingo_ast_literal_type_t type;
    union {
        bool boolean;
        clingo_ast_term_t const *symbol;
        clingo_ast_comparison_t const *comparison;
        clingo_ast_csp_literal_t const *csp_literal;
    };
} clingo_ast_literal_t;

// {{{2 aggregates

enum clingo_ast_aggregate_function {
    clingo_ast_aggregate_function_count = 0,
    clingo_ast_aggregate_function_sum   = 1,
    clingo_ast_aggregate_function_sump  = 2,
    clingo_ast_aggregate_function_min   = 3,
    clingo_ast_aggregate_function_max   = 4
};
typedef int clingo_ast_aggregate_function_t;

typedef struct clingo_ast_aggregate_guard {
    clingo_ast_comparison_operator_t comparison;
    clingo_ast_term_t term;
} clingo_ast_aggregate_guard_t;

typedef struct clingo_ast_conditional_literal {
    clingo_ast_literal_t literal;
    clingo_ast_literal_t const *condition;
    size_t size;
} clingo_ast_conditional_literal_t;

// lparse-style aggregate

typedef struct clingo_ast_aggregate {
    clingo_ast_conditional_literal_t const *elements;
    size_t size;
    clingo_ast_aggregate_guard_t const *left_guard;
    clingo_ast_aggregate_guard_t const *right_guard;
} clingo_ast_aggregate_t;

// body aggregate

typedef struct clingo_ast_body_aggregate_element {
    clingo_ast_term_t *tuple;
    size_t tuple_size;
    clingo_ast_literal_t const *condition;
    size_t condition_size;
} clingo_ast_body_aggregate_element_t;

typedef struct clingo_ast_body_aggregate {
    clingo_ast_aggregate_function_t function;
    clingo_ast_body_aggregate_element_t const *elements;
    size_t size;
    clingo_ast_aggregate_guard_t const *left_guard;
    clingo_ast_aggregate_guard_t const *right_guard;
} clingo_ast_body_aggregate_t;

// head aggregate

typedef struct clingo_ast_head_aggregate_element {
    clingo_ast_term_t const *tuple;
    size_t tuple_size;
    clingo_ast_conditional_literal_t conditional_literal;
} clingo_ast_head_aggregate_element_t;

typedef struct clingo_ast_head_aggregate {
    clingo_ast_aggregate_function_t function;
    clingo_ast_head_aggregate_element_t const *elements;
    size_t size;
    clingo_ast_aggregate_guard_t const *left_guard;
    clingo_ast_aggregate_guard_t const *right_guard;
} clingo_ast_head_aggregate_t;

// disjunction

typedef struct clingo_ast_disjunction {
    clingo_ast_conditional_literal_t const *elements;
    size_t size;
} clingo_ast_disjunction_t;

// disjoint

typedef struct clingo_ast_disjoint_element {
    clingo_location_t location;
    clingo_ast_term_t const *tuple;
    size_t tuple_size;
    clingo_ast_csp_sum_term_t term;
    clingo_ast_literal_t const *condition;
    size_t condition_size;
} clingo_ast_disjoint_element_t;

typedef struct clingo_ast_disjoint {
    clingo_ast_disjoint_element_t const *elements;
    size_t size;
} clingo_ast_disjoint_t;

// {{{2 theory atom

enum clingo_ast_theory_term_type {
    clingo_ast_theory_term_type_symbol        = 0,
    clingo_ast_theory_term_type_variable      = 1,
    clingo_ast_theory_term_type_tuple         = 2,
    clingo_ast_theory_term_type_list          = 3,
    clingo_ast_theory_term_type_set           = 4,
    clingo_ast_theory_term_type_function      = 5,
    clingo_ast_theory_term_type_unparsed_term = 6
};
typedef int clingo_ast_theory_term_type_t;

typedef struct clingo_ast_theory_function clingo_ast_theory_function_t;
typedef struct clingo_ast_theory_term_array clingo_ast_theory_term_array_t;
typedef struct clingo_ast_theory_unparsed_term clingo_ast_theory_unparsed_term_t;

typedef struct clingo_ast_theory_term {
    clingo_location_t location;
    clingo_ast_theory_term_type_t type;
    union {
        clingo_symbol_t symbol;
        char const *variable;
        clingo_ast_theory_term_array_t const *tuple;
        clingo_ast_theory_term_array_t const *list;
        clingo_ast_theory_term_array_t const *set;
        clingo_ast_theory_function_t const *function;
        clingo_ast_theory_unparsed_term_t const *unparsed_term;
    };
} clingo_ast_theory_term_t;

struct clingo_ast_theory_term_array {
    clingo_ast_theory_term_t const *terms;
    size_t size;
};

struct clingo_ast_theory_function {
    char const *name;
    clingo_ast_theory_term_t const *arguments;
    size_t size;
};

typedef struct clingo_ast_theory_unparsed_term_element {
    char const *const *operators;
    size_t size;
    clingo_ast_theory_term_t term;
} clingo_ast_theory_unparsed_term_element_t;

struct clingo_ast_theory_unparsed_term {
    clingo_ast_theory_unparsed_term_element_t const *elements;
    size_t size;
};

typedef struct clingo_ast_theory_atom_element {
    clingo_ast_theory_term_t const *tuple;
    size_t tuple_size;
    clingo_ast_literal_t const *condition;
    size_t condition_size;
} clingo_ast_theory_atom_element_t;

typedef struct clingo_ast_theory_guard {
    char const *operator_name;
    clingo_ast_theory_term_t term;
} clingo_ast_theory_guard_t;

typedef struct clingo_ast_theory_atom {
    clingo_ast_term_t term;
    clingo_ast_theory_atom_element_t const *elements;
    size_t size;
    clingo_ast_theory_guard_t const *guard;
} clingo_ast_theory_atom_t;

// {{{2 head literals

enum clingo_ast_head_literal_type {
    clingo_ast_head_literal_type_literal        = 0,
    clingo_ast_head_literal_type_disjunction    = 1,
    clingo_ast_head_literal_type_aggregate      = 2,
    clingo_ast_head_literal_type_head_aggregate = 3,
    clingo_ast_head_literal_type_theory_atom    = 4
};
typedef int clingo_ast_head_literal_type_t;

typedef struct clingo_ast_head_literal {
    clingo_location_t location;
    clingo_ast_head_literal_type_t type;
    union {
        clingo_ast_literal_t const *literal;
        clingo_ast_disjunction_t const *disjunction;
        clingo_ast_aggregate_t const *aggregate;
        clingo_ast_head_aggregate_t const *head_aggregate;
        clingo_ast_theory_atom_t const *theory_atom;
    };
} clingo_ast_head_literal_t;

// {{{2 body literals

enum clingo_ast_body_literal_type {
    clingo_ast_body_literal_type_literal        = 0,
    clingo_ast_body_literal_type_conditional    = 1,
    clingo_ast_body_literal_type_aggregate      = 2,
    clingo_ast_body_literal_type_body_aggregate = 3,
    clingo_ast_body_literal_type_theory_atom    = 4,
    clingo_ast_body_literal_type_disjoint       = 5
};
typedef int clingo_ast_body_literal_type_t;

typedef struct clingo_ast_body_literal {
    clingo_location_t location;
    clingo_ast_sign_t sign;
    clingo_ast_body_literal_type_t type;
    union {
        clingo_ast_literal_t const *literal;
        // Note: conditional literals must not have signs!!!
        clingo_ast_conditional_literal_t const *conditional;
        clingo_ast_aggregate_t const *aggregate;
        clingo_ast_body_aggregate_t const *body_aggregate;
        clingo_ast_theory_atom_t const *theory_atom;
        clingo_ast_disjoint_t const *disjoint;
    };
} clingo_ast_body_literal_t;

// {{{2 theory definitions

enum clingo_ast_theory_operator_type {
     clingo_ast_theory_operator_type_unary        = 0,
     clingo_ast_theory_operator_type_binary_left  = 1,
     clingo_ast_theory_operator_type_binary_right = 2
};
typedef int clingo_ast_theory_operator_type_t;

typedef struct clingo_ast_theory_operator_definition {
    clingo_location_t location;
    char const *name;
    unsigned priority;
    clingo_ast_theory_operator_type_t type;
} clingo_ast_theory_operator_definition_t;

typedef struct clingo_ast_theory_term_definition {
    clingo_location_t location;
    char const *name;
    clingo_ast_theory_operator_definition_t const *operators;
    size_t size;
} clingo_ast_theory_term_definition_t;

typedef struct clingo_ast_theory_guard_definition {
    char const *term;
    char const *const *operators;
    size_t size;
} clingo_ast_theory_guard_definition_t;

enum clingo_ast_theory_atom_definition_type {
    clingo_ast_theory_atom_definition_type_head      = 0,
    clingo_ast_theory_atom_definition_type_body      = 1,
    clingo_ast_theory_atom_definition_type_any       = 2,
    clingo_ast_theory_atom_definition_type_directive = 3,
};
typedef int clingo_ast_theory_atom_definition_type_t;

typedef struct clingo_ast_theory_atom_definition {
    clingo_location_t location;
    clingo_ast_theory_atom_definition_type_t type;
    char const *name;
    unsigned arity;
    char const *elements;
    clingo_ast_theory_guard_definition_t const *guard;
} clingo_ast_theory_atom_definition_t;

typedef struct clingo_ast_theory_definition {
    char const *name;
    clingo_ast_theory_term_definition_t const *terms;
    size_t terms_size;
    clingo_ast_theory_atom_definition_t const *atoms;
    size_t atoms_size;
} clingo_ast_theory_definition_t;

// {{{2 statements

// rule

typedef struct clingo_ast_rule {
    clingo_ast_head_literal_t head;
    clingo_ast_body_literal_t const *body;
    size_t size;
} clingo_ast_rule_t;

// definition

typedef struct clingo_ast_definition {
    char const *name;
    clingo_ast_term_t value;
    bool is_default;
} clingo_ast_definition_t;

// show

typedef struct clingo_ast_show_signature {
    clingo_signature_t signature;
    bool csp;
} clingo_ast_show_signature_t;

typedef struct clingo_ast_show_term {
    clingo_ast_term_t term;
    clingo_ast_body_literal_t const *body;
    size_t size;
    bool csp;
} clingo_ast_show_term_t;

// minimize

typedef struct clingo_ast_minimize {
    clingo_ast_term_t weight;
    clingo_ast_term_t priority;
    clingo_ast_term_t const *tuple;
    size_t tuple_size;
    clingo_ast_body_literal_t const *body;
    size_t body_size;
} clingo_ast_minimize_t;

// script

enum clingo_ast_script_type {
    clingo_ast_script_type_lua    = 0,
    clingo_ast_script_type_python = 1
};
typedef int clingo_ast_script_type_t;

typedef struct clingo_ast_script {
    clingo_ast_script_type_t type;
    char const *code;
} clingo_ast_script_t;

// program

typedef struct clingo_ast_program {
    char const *name;
    clingo_ast_id_t const *parameters;
    size_t size;
} clingo_ast_program_t;

// external

typedef struct clingo_ast_external {
    clingo_ast_term_t atom;
    clingo_ast_body_literal_t const *body;
    size_t size;
} clingo_ast_external_t;

// edge

typedef struct clingo_ast_edge {
    clingo_ast_term_t u;
    clingo_ast_term_t v;
    clingo_ast_body_literal_t const *body;
    size_t size;
} clingo_ast_edge_t;

// heuristic

typedef struct clingo_ast_heuristic {
    clingo_ast_term_t atom;
    clingo_ast_body_literal_t const *body;
    size_t size;
    clingo_ast_term_t bias;
    clingo_ast_term_t priority;
    clingo_ast_term_t modifier;
} clingo_ast_heuristic_t;

// project

typedef struct clingo_ast_project {
    clingo_ast_term_t atom;
    clingo_ast_body_literal_t const *body;
    size_t size;
} clingo_ast_project_t;

// statement

enum clingo_ast_statement_type {
    clingo_ast_statement_type_rule                   = 0,
    clingo_ast_statement_type_const                  = 1,
    clingo_ast_statement_type_show_signature         = 2,
    clingo_ast_statement_type_show_term              = 3,
    clingo_ast_statement_type_minimize               = 4,
    clingo_ast_statement_type_script                 = 5,
    clingo_ast_statement_type_program                = 6,
    clingo_ast_statement_type_external               = 7,
    clingo_ast_statement_type_edge                   = 8,
    clingo_ast_statement_type_heuristic              = 9,
    clingo_ast_statement_type_project_atom           = 10,
    clingo_ast_statement_type_project_atom_signature = 11,
    clingo_ast_statement_type_theory_definition      = 12
};
typedef int clingo_ast_statement_type_t;

typedef struct clingo_ast_statement {
    clingo_location_t location;
    clingo_ast_statement_type_t type;
    union {
        clingo_ast_rule_t const *rule;
        clingo_ast_definition_t const *definition;
        clingo_ast_show_signature_t const *show_signature;
        clingo_ast_show_term_t const *show_term;
        clingo_ast_minimize_t const *minimize;
        clingo_ast_script_t const *script;
        clingo_ast_program_t const *program;
        clingo_ast_external_t const *external;
        clingo_ast_edge_t const *edge;
        clingo_ast_heuristic_t const *heuristic;
        clingo_ast_project_t const *project_atom;
        clingo_signature_t project_signature;
        clingo_ast_theory_definition_t const *theory_definition;
    };
} clingo_ast_statement_t;

// }}}2

typedef bool clingo_ast_callback_t (clingo_ast_statement_t const *, void *);
//! Parse the given program and return an abstract syntax tree for each statement via a callback.
//!
//! @param[in] program the program in gringo syntax
//! @param[in] callback the callback reporting statements
//! @param[in] callback_data user data for the callback
//! @param[in] logger callback to report messages during parsing
//! @param[in] logger_data user data for the logger
//! @param[in] message_limit the maximum number of times the logger is called
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if parsing fails
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_parse_program(char const *program, clingo_ast_callback_t *callback, void *callback_data, clingo_logger_t *logger, void *logger_data, unsigned message_limit);

//! @}

// {{{1 program builder

//! @addtogroup ProgramBuilder
//! @{

//! Object to build non-ground programs.
typedef struct clingo_program_builder clingo_program_builder_t;

//! Begin building a program.
//!
//! @param builder the target program builder
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_program_builder_begin(clingo_program_builder_t *builder);
//! Adds a statement to the program.
//!
//! @attention @ref clingo_program_builder_begin() must be called before adding statements and @ref clingo_program_builder_end() must be called after all statements have been added.
//! @param builder the target program builder
//! @param statement the statement to add
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime for statements of invalid form
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_program_builder_add(clingo_program_builder_t *builder, clingo_ast_statement_t const *statement);
//! End building a program.
//!
//! @param builder the target program builder
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_program_builder_end(clingo_program_builder_t *builder);

//! @}

// {{{1 ground program observer

//! @defgroup ProgramInspection Program Inspection
//! Functions and data structures to inspect programs.
//! @ingroup Control

//! @addtogroup ProgramInspection
//! @{

//! An instance of this struct has to be registered with a solver to observe ground directives as they are passed to the solver.
//!
//! @note This interface is closely modeled after the aspif format.
//! For more information please refer to the specification of the aspif format.
//!
//! Not all callbacks have to be implemented and can be set to NULL if not needed.
//! If one of the callbacks in the struct fails, grounding is stopped.
//! If a non-recoverable clingo API call fails, a callback must return false.
//! Otherwise ::clingo_error_unknown should be set and false returned.
//!
//! @see clingo_control_register_observer()
typedef struct clingo_ground_program_observer {
    //! Called once in the beginning.
    //!
    //! If the incremental flag is true, there can be multiple calls to @ref clingo_control_solve().
    //!
    //! @param[in] incremental whether the program is incremental
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*init_program)(bool incremental, void *data);
    //! Marks the beginning of a block of directives passed to the solver.
    //!
    //! @see @ref end_step
    //!
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*begin_step)(void *data);
    //! Marks the end of a block of directives passed to the solver.
    //!
    //! This function is called before solving starts.
    //!
    //! @see @ref begin_step
    //!
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*end_step)(void *data);

    //! Observe rules passed to the solver.
    //!
    //! @param[in] choice determines if the head is a choice or a disjunction
    //! @param[in] head the head atoms
    //! @param[in] head_size the number of atoms in the head
    //! @param[in] body the body literals
    //! @param[in] body_size the number of literals in the body
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*rule)(bool choice, clingo_atom_t const *head, size_t head_size, clingo_literal_t const *body, size_t body_size, void *data);
    //! Observe weight rules passed to the solver.
    //!
    //! @param[in] choice determines if the head is a choice or a disjunction
    //! @param[in] head the head atoms
    //! @param[in] head_size the number of atoms in the head
    //! @param[in] lower_bound the lower bound of the weight rule
    //! @param[in] body the weighted body literals
    //! @param[in] body_size the number of weighted literals in the body
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*weight_rule)(bool choice, clingo_atom_t const *head, size_t head_size, clingo_weight_t lower_bound, clingo_weighted_literal_t const *body, size_t body_size, void *data);
    //! Observe minimize constraints (or weak constraints) passed to the soler.
    //!
    //! @param[in] priority the priority of the constraint
    //! @param[in] literals the weighted literals whose sum to minimize
    //! @param[in] size the number of weighted literals
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*minimize)(clingo_weight_t priority, clingo_weighted_literal_t const* literals, size_t size, void *data);
    //! Observe projection directives passed to the solver.
    //!
    //! @param[in] atoms the atoms to project on
    //! @param[in] size the number of atoms
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*project)(clingo_atom_t const *atoms, size_t size, void *data);
    //! Observe external statements passed to the solver.
    //!
    //! @param[in] atom the external atom
    //! @param[in] type the type of the external statement
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*external)(clingo_atom_t atom, clingo_external_type_t type, void *data);
    //! Observe assumption directives passed to the solver.
    //!
    //! @param[in] literals the literals to assume (positive literals are true and negative literals false for the next solve call)
    //! @param[in] size the number of atoms
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*assume)(clingo_literal_t const *literals, size_t size, void *data);
    //! Observe heuristic directives passed to the solver.
    //!
    //! @param[in] atom the target atom
    //! @param[in] type the type of the heuristic modification
    //! @param[in] bias the heuristic bias
    //! @param[in] priority the heuristic priority
    //! @param[in] condition the condition under which to apply the heuristic modification
    //! @param[in] size the number of atoms in the condition
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*heuristic)(clingo_atom_t atom, clingo_heuristic_type_t type, int bias, unsigned priority, clingo_literal_t const *condition, size_t size, void *data);
    //! Observe edge directives passed to the solver.
    //!
    //! @param[in] node_u the start vertex of the edge
    //! @param[in] node_v the end vertex of the edge
    //! @param[in] condition the condition under which the edge is part of the graph
    //! @param[in] size the number of atoms in the condition
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*acyc_edge)(int node_u, int node_v, clingo_literal_t const *condition, size_t size, void *data);

    //! Observe numeric theory terms.
    //!
    //! @param[in] term_id the id of the term
    //! @param[in] number the value of the term
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*theory_term_number)(clingo_id_t term_id, int number, void *data);
    //! Observe string theory terms.
    //!
    //! @param[in] term_id the id of the term
    //! @param[in] name the value of the term
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*theory_term_string)(clingo_id_t term_id, char const *name, void *data);
    //! Observe compound theory terms.
    //!
    //! The name_id_or_type gives the type of the compound term:
    //! - if it is -1, then it is a tuple
    //! - if it is -2, then it is a set
    //! - if it is -3, then it is a list
    //! - otherwise, it is a function and name_id_or_type refers to the id of the name (in form of a string term)
    //!
    //! @param[in] term_id the id of the term
    //! @param[in] name_id_or_type the name or type of the term
    //! @param[in] arguments the arguments of the term
    //! @param[in] size the number of arguments
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*theory_term_compound)(clingo_id_t term_id, int name_id_or_type, clingo_id_t const *arguments, size_t size, void *data);
    //! Observe theory elements.
    //!
    //! @param element_id the id of the element
    //! @param terms the term tuple of the element
    //! @param terms_size the number of terms in the tuple
    //! @param condition the condition of the elemnt
    //! @param condition_size the number of literals in the condition
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*theory_element)(clingo_id_t element_id, clingo_id_t const *terms, size_t terms_size, clingo_literal_t const *condition, size_t condition_size, void *data);
    //! Observe theory atoms without guard.
    //!
    //! @param[in] atom_id_or_zero the id of the atom or zero for directives
    //! @param[in] term_id the term associated with the atom
    //! @param[in] elements the elements of the atom
    //! @param[in] size the number of elements
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*theory_atom)(clingo_id_t atom_id_or_zero, clingo_id_t term_id, clingo_id_t const *elements, size_t size, void *data);
    //! Observe theory atoms with guard.
    //!
    //! @param[in] atom_id_or_zero the id of the atom or zero for directives
    //! @param[in] term_id the term associated with the atom
    //! @param[in] elements the elements of the atom
    //! @param[in] size the number of elements
    //! @param[in] operator_id the id of the operator (a string term)
    //! @param[in] right_hand_side_id the id of the term on the right hand side of the atom
    //! @param[in] data user data for the callback
    //! @return whether the call was successful
    bool (*theory_atom_with_guard)(clingo_id_t atom_id_or_zero, clingo_id_t term_id, clingo_id_t const *elements, size_t size, clingo_id_t operator_id, clingo_id_t right_hand_side_id, void *data);
} clingo_ground_program_observer_t;

// @}

// {{{1 control

//! @example control.c
//! The example shows how to ground and solve a simple logic program, and print
//! its answer sets.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! ./control 0
//! Model: a
//! Model: b
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @defgroup Control Grounding and Solving
//! Functions to control the grounding and solving process.
//!
//! For an example, see @ref control.c.

//! @addtogroup Control
//! @{

//! @enum clingo_solve_result
//! Enumeration of bit masks for solve call results.
//!
//! @note Neither ::clingo_solve_result_satisfiable nor
//! ::clingo_solve_result_exhausted is set if the search is interrupted and no
//! model was found.
//!
//! @var clingo_solve_result::clingo_solve_result_satisfiable
//! The last solve call found a solution.
//! @var clingo_solve_result::clingo_solve_result_unsatisfiable
//! The last solve call did not find a solution.
//! @var clingo_solve_result::clingo_solve_result_exhausted
//! The last solve call completely exhausted the search space.
//! @var clingo_solve_result::clingo_solve_result_interrupted
//! The last solve call was interrupted.
//!
//! @see clingo_control_interrupt()

//! @typedef clingo_solve_result_bitset_t
//! Corresponding type to ::clingo_solve_result.

//! Control object holding grounding and solving state.
typedef struct clingo_control clingo_control_t;

//! Struct used to specify the program parts that have to be grounded.
//!
//! Programs may be structured into parts, which can be grounded independently with ::clingo_control_ground.
//! Program parts are mainly interesting for incremental grounding and multi-shot solving.
//! For single-shot solving, program parts are not needed.
//!
//! @note Parts of a logic program without an explicit <tt>\#program</tt>
//! specification are by default put into a program called `base` without
//! arguments.
//!
//! @see clingo_control_ground()
typedef struct clingo_part {
    char const *name;              //!< name of the program part
    clingo_symbol_t const *params; //!< array of parameters
    size_t size;                   //!< number of parameters
} clingo_part_t;

//! Callback function to inject symbols.
//!
//! @param symbols array of symbols
//! @param symbols_size size of the symbol array
//! @param data user data of the callback
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! @see ::clingo_ground_callback_t
typedef bool clingo_symbol_callback_t (clingo_symbol_t const *symbols, size_t symbols_size, void *data);

//! Callback function to implement external functions.
//!
//! If an external function of form <tt>\@name(parameters)</tt> occurs in a logic program,
//! then this function is called with its location, name, parameters, and a callback to inject symbols as arguments.
//! The callback can be called multiple times; all symbols passed are injected.
//!
//! If a (non-recoverable) clingo API function fails in this callback, for example, the symbol callback, the callback must return false.
//! In case of errors not related to clingo, this function can set error ::clingo_error_unknown and return false to stop grounding with an error.
//!
//! @param[in] location location from which the external function was called
//! @param[in] name name of the called external function
//! @param[in] arguments arguments of the called external function
//! @param[in] arguments_size number of arguments
//! @param[in] data user data of the callback
//! @param[in] symbol_callback function to inject symbols
//! @param[in] symbol_callback_data user data for the symbol callback
//!            (must be passed untouched)
//! @return whether the call was successful
//! @see clingo_control_ground()
//!
//! The following example implements the external function <tt>\@f()</tt> returning 42.
//! ~~~~~~~~~~~~~~~{.c}
//! bool
//! ground_callback(clingo_location_t location,
//!                 char const *name,
//!                 clingo_symbol_t const *arguments,
//!                 size_t arguments_size,
//!                 void *data,
//!                 clingo_symbol_callback_t *symbol_callback,
//!                 void *symbol_callback_data) {
//!   if (strcmp(name, "f") == 0 && arguments_size == 0) {
//!     clingo_symbol_t sym;
//!     clingo_symbol_create_number(42, &sym);
//!     return symbol_callback(&sym, 1, symbol_callback_data);
//!   }
//!   clingo_set_error(clingo_error_runtime, "function not found");
//!   return false;
//! }
//! ~~~~~~~~~~~~~~~
typedef bool clingo_ground_callback_t (clingo_location_t location, char const *name, clingo_symbol_t const *arguments, size_t arguments_size, void *data, clingo_symbol_callback_t *symbol_callback, void *symbol_callback_data);

//! Callback function to intercept models.
//!
//! The model callback is invoked once for each model found by clingo (@link SolveAsync asynchronously@endlink).
//!
//! If a (non-recoverable) clingo API function fails in this callback, the function must return false.
//! In case of errors not related to clingo, the function can set error code ::clingo_error_unknown can be set and return false to stop solving with an error.
//!
//! @param[in] model the current model
//! @param[in] data user data of the callback
//! @param[out] goon whether to continue search
//! @return whether the call was successful
//!
//! @see clingo_control_solve()
//! @see clingo_control_solve_async()
typedef bool clingo_model_callback_t (clingo_model_t *model, void *data, bool *goon);

//! Callback function called at the end of an asynchronous solve operation.
//!
//! If a (non-recoverable) clingo API function fails in this callback, it must return false.
//! In case of errors not related to clingo, set error code ::clingo_error_unknown and return false to stop solving with an error.
//!
//! @param[in] result result of the solve call
//! @param[in] data user data of the callback
//! @return whether the call was successful
//!
//! @see clingo_control_solve_async()
typedef bool clingo_finish_callback_t (clingo_solve_result_bitset_t result, void *data);

//! Create a new control object.
//!
//! A control object has to be freed using clingo_control_free().
//!
//! @note Only gringo options (without <code>\-\-output</code>) and clasp's options are supported as arguments,
//! except basic options such as <code>\-\-help</code>.
//! Furthermore, a control object is blocked while a search call is active;
//! you must not call any member function during search.
//!
//! If the logger is NULL, messages are printed to stderr.
//!
//! @param[in] arguments C string array of command line arguments
//! @param[in] arguments_size size of the arguments array
//! @param[in] logger callback functions for warnings and info messages
//! @param[in] logger_data user data for the logger callback
//! @param[in] message_limit maximum number of times the logger callback is called
//! @param[out] control resulting control object
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if argument parsing fails
CLINGO_VISIBILITY_DEFAULT bool clingo_control_new(char const *const * arguments, size_t arguments_size, clingo_logger_t *logger, void *logger_data, unsigned message_limit, clingo_control_t **control);

//! Free a control object created with clingo_control_new().
//! @param[in] control the target
CLINGO_VISIBILITY_DEFAULT void clingo_control_free(clingo_control_t *control);

//! @name Grounding Functions
//! @{

//! Extend the logic program with a program in a file.
//!
//! @param[in] control the target
//! @param[in] file path to the file
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if parsing or checking fails
CLINGO_VISIBILITY_DEFAULT bool clingo_control_load(clingo_control_t *control, char const *file);

//! Extend the logic program with the given non-ground logic program in string form.
//!
//! This function puts the given program into a block of form: <tt>\#program name(parameters).</tt>
//!
//! After extending the logic program, the corresponding program parts are typically grounded with ::clingo_control_ground.
//!
//! @param[in] control the target
//! @param[in] name name of the program block
//! @param[in] parameters string array of parameters of the program block
//! @param[in] parameters_size number of parameters
//! @param[in] program string representation of the program
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if parsing fails
CLINGO_VISIBILITY_DEFAULT bool clingo_control_add(clingo_control_t *control, char const *name, char const * const * parameters, size_t parameters_size, char const *program);

//! Ground the selected @link ::clingo_part parts @endlink of the current (non-ground) logic program.
//!
//! After grounding, logic programs can be solved with ::clingo_control_solve.
//!
//! @note Parts of a logic program without an explicit <tt>\#program</tt>
//! specification are by default put into a program called `base` without
//! arguments.
//!
//! @param[in] control the target
//! @param[in] parts array of parts to ground
//! @param[in] parts_size size of the parts array
//! @param[in] ground_callback callback to implement external functions
//! @param[in] ground_callback_data user data for ground_callback
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - error code of ground callback
//!
//! @see clingo_part
CLINGO_VISIBILITY_DEFAULT bool clingo_control_ground(clingo_control_t *control, clingo_part_t const *parts, size_t parts_size, clingo_ground_callback_t *ground_callback, void *ground_callback_data);

//! @}

//! @name Solving Functions
//! @{

//! Solve the currently @link ::clingo_control_ground grounded @endlink logic program.
//!
//! @param[in] control the target
//! @param[in] model_callback optional callback to intercept models
//! @param[in] model_callback_data user data for model callback
//! @param[in] assumptions array of assumptions to solve under
//! @param[in] assumptions_size number of assumptions
//! @param[out] result the result of the search
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if solving fails
//! - error code of model callback
CLINGO_VISIBILITY_DEFAULT bool clingo_control_solve(clingo_control_t *control, clingo_model_callback_t *model_callback, void *model_callback_data, clingo_symbolic_literal_t const * assumptions, size_t assumptions_size, clingo_solve_result_bitset_t *result);
//! Solve the currently @link ::clingo_control_ground grounded @endlink logic program enumerating models iteratively.
//!
//! See the @ref SolveIter module for more information.
//!
//! @param[in] control the target
//! @param[in] assumptions array of assumptions to solve under
//! @param[in] assumptions_size number of assumptions
//! @param[out] handle handle to the current search to enumerate models
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if solving could not be started
CLINGO_VISIBILITY_DEFAULT bool clingo_control_solve_iteratively(clingo_control_t *control, clingo_symbolic_literal_t const *assumptions, size_t assumptions_size, clingo_solve_iteratively_t **handle);
//! Solve the currently @link ::clingo_control_ground grounded @endlink logic program asynchronously in the background.
//!
//! See the @ref SolveAsync module for more information.
//!
//! @param[in] control the target
//! @param[in] model_callback optional callback to intercept models
//! @param[in] model_callback_data user data for model callback
//! @param[in] finish_callback optional callback called just before the end of the search
//! @param[in] finish_callback_data user data for finish callback
//! @param[in] assumptions array of assumptions to solve under
//! @param[in] assumptions_size number of assumptions
//! @param[out] handle handle to the current search
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if solving could not be started
CLINGO_VISIBILITY_DEFAULT bool clingo_control_solve_async(clingo_control_t *control, clingo_model_callback_t *model_callback, void *model_callback_data, clingo_finish_callback_t *finish_callback, void *finish_callback_data, clingo_symbolic_literal_t const * assumptions, size_t assumptions_size, clingo_solve_async_t **handle);
//! Clean up the domains of clingo's grounding component using the solving
//! component's top level assignment.
//!
//! This function removes atoms from domains that are false and marks atoms as
//! facts that are true.  With multi-shot solving, this can result in smaller
//! groundings because less rules have to be instantiated and more
//! simplifications can be applied.
//!
//! @param[in] control the target
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_control_cleanup(clingo_control_t *control);
//! Assign a truth value to an external atom.
//!
//! If the atom does not exist or is not external, this is a noop.
//!
//! @param[in] control the target
//! @param[in] atom atom to assign
//! @param[in] value the truth value
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_control_assign_external(clingo_control_t *control, clingo_symbol_t atom, clingo_truth_value_t value);
//! Release an external atom.
//!
//! After this call, an external atom is no longer external and subject to
//! program simplifications.  If the atom does not exist or is not external,
//! this is a noop.
//!
//! @param[in] control the target
//! @param[in] atom atom to release
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_control_release_external(clingo_control_t *control, clingo_symbol_t atom);
//! Register a custom propagator with the control object.
//!
//! If the sequential flag is set to true, the propagator is called
//! sequentially when solving with multiple threads.
//!
//! See the @ref Propagator module for more information.
//!
//! @param[in] control the target
//! @param[in] propagator the propagator
//! @param[in] data user data passed to the propagator functions
//! @param[in] sequential whether the propagator should be called sequentially
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_control_register_propagator(clingo_control_t *control, clingo_propagator_t propagator, void *data, bool sequential);
//! Get a statistics object to inspect solver statistics.
//!
//! Statistics are updated after a solve call.
//!
//! See the @ref Statistics module for more information.
//!
//! @attention
//! The level of detail of the statistics depends on the stats option
//! (which can be set using @ref Configuration module or passed as an option when @link clingo_control_new creating the control object@endlink).
//! The default level zero only provides basic statistics,
//! level one provides extended and accumulated statistics,
//! and level two provides per-thread statistics.
//!
//! @param[in] control the target
//! @param[out] statistics the statistics object
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_control_statistics(clingo_control_t *control, clingo_statistics_t **statistics);
//! Interrupt the active solve call (or the following solve call right at the beginning).
//!
//! @param[in] control the target
CLINGO_VISIBILITY_DEFAULT void clingo_control_interrupt(clingo_control_t *control);
//! Get low-level access to clasp.
//!
//! @attention
//! This function is intended for experimental use only and not part of the stable API.
//!
//! This function may return a <code>nullptr</code>.
//! Otherwise, the returned pointer can be casted to a ClaspFacade pointer.
//!
//! @param[in] control the target
//! @param[out] clasp pointer to the ClaspFacade object (may be <code>nullptr</code>)
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_clasp_facade(clingo_control_t *control, void **clasp);

//! @}

//! @name Configuration Functions
//! @{

//! Get a configuration object to change the solver configuration.
//!
//! See the @ref Configuration module for more information.
//!
//! @param[in] control the target
//! @param[out] configuration the configuration object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_configuration(clingo_control_t *control, clingo_configuration_t **configuration);
//! Configure how learnt constraints are handled during enumeration.
//!
//! If the enumeration assumption is enabled, then all information learnt from
//! the solver's various enumeration modes is removed after a solve call. This
//! includes enumeration of cautious or brave consequences, enumeration of
//! answer sets with or without projection, or finding optimal models, as well
//! as clauses added with clingo_solve_control_add_clause().
//!
//! @attention For practical purposes, this option is only interesting for single-shot solving
//! or before the last solve call to squeeze out a tiny bit of performance.
//! Initially, the enumeration assumption is enabled.
//!
//! @param[in] control the target
//! @param[in] enable whether to enable the assumption
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_use_enumeration_assumption(clingo_control_t *control, bool enable);
//! @}

//! @name Program Inspection Functions
//! @{

//! Return the symbol for a constant definition of form: <tt>\#const name = symbol</tt>.
//!
//! @param[in] control the target
//! @param[in] name the name of the constant
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_get_const(clingo_control_t *control, char const *name, clingo_symbol_t *symbol);
//! Check if there is a constant definition for the given constant.
//!
//! @param[in] control the target
//! @param[in] name the name of the constant
//! @param[out] exists whether a matching constant definition exists
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_runtime if constant definition does not exist
//!
//! @see clingo_control_get_const()
CLINGO_VISIBILITY_DEFAULT bool clingo_control_has_const(clingo_control_t *control, char const *name, bool *exists);
//! Get an object to inspect symbolic atoms (the relevant Herbrand base) used
//! for grounding.
//!
//! See the @ref SymbolicAtoms module for more information.
//!
//! @param[in] control the target
//! @param[out] atoms the symbolic atoms object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_symbolic_atoms(clingo_control_t *control, clingo_symbolic_atoms_t **atoms);
//! Get an object to inspect theory atoms that occur in the grounding.
//!
//! See the @ref TheoryAtoms module for more information.
//!
//! @param[in] control the target
//! @param[out] atoms the theory atoms object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_theory_atoms(clingo_control_t *control, clingo_theory_atoms_t **atoms);
//! Register a program observer with the control object.
//!
//! @param[in] control the target
//! @param[in] observer the observer to register
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_register_observer(clingo_control_t *control, clingo_ground_program_observer_t observer, void *data);
//! @}

//! @name Program Modification Functions
//! @{

//! Get an object to add ground directives to the program.
//!
//! See the @ref ProgramBuilder module for more information.
//!
//! @param[in] control the target
//! @param[out] backend the backend object
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_control_backend(clingo_control_t *control, clingo_backend_t **backend);
//! Get an object to add non-ground directives to the program.
//!
//! See the @ref ProgramBuilder module for more information.
//!
//! @param[in] control the target
//! @param[out] builder the program builder object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_program_builder(clingo_control_t *control, clingo_program_builder_t **builder);
//! @}

//! @}

// }}}1

#ifdef __cplusplus
}
#endif

#endif
