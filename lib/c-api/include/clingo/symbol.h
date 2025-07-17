#ifndef CLINGO_SYMBOL_H
#define CLINGO_SYMBOL_H

#include <clingo/core.h>

#ifdef __cplusplus
extern "C" {
#endif

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

//! @addtogroup c_symbol
//! Working with (evaluated) ground terms and related functions.
//!
//! @note Functions to create symbols are only thread-safe if library flags have been requested accordingly.
//!
//! For an example, see @ref symbol.c.
//! @{

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

//! Callback function to inject symbols.
//!
//! @param symbols array of symbols
//! @param symbols_size size of the symbol array
//! @param data user data of the callback
//! @return whether the call was successful
//! @see ::clingo_ground_callback_t
typedef bool (*clingo_symbol_callback_t)(clingo_symbol_t const *symbols, size_t symbols_size, void *data);

//! @name Symbol Construction Functions
//! @{

//! Construct a symbol representing <tt>\#inf</tt>.
//!
//! @return the resulting symbol
CLINGO_VISIBILITY_DEFAULT clingo_symbol_t clingo_symbol_create_infimum(void);

//! Construct a symbol representing \#sup.
//!
//! @return the resulting symbol
CLINGO_VISIBILITY_DEFAULT clingo_symbol_t clingo_symbol_create_supremum(void);

//! Construct a symbol representing a number.
//!
//! @param[in] number the number
//! @return the resulting symbol
CLINGO_VISIBILITY_DEFAULT clingo_symbol_t clingo_symbol_create_number(int32_t number);

//! Construct a symbol representing a number.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] number the number in form of a string
//! @param[in] size the size of the string
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_number_str(clingo_lib_t *lib, char const *number, size_t size,
                                                               clingo_symbol_t *symbol);

//! Construct a symbol representing a string.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] string the string
//! @param[in] size the size of the string
//! @param[out] symbol the resulting symbol
//! @return the result code; might return one of the following codes:
//! - ::clingo_result_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_string(clingo_lib_t *lib, char const *string, size_t size,
                                                           clingo_symbol_t *symbol);

//! Construct a symbol representing an id.
//!
//! @note This is just a shortcut for clingo_symbol_create_function() with
//! empty arguments.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] name the name
//! @param[in] size the size of the string
//! @param[in] is_positive whether the symbol is positive
//! @param[out] symbol the resulting symbol
//! @return the result code; might return one of the following codes:
//! - ::clingo_result_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_id(clingo_lib_t *lib, char const *name, size_t size,
                                                       bool is_positive, clingo_symbol_t *symbol);

//! Construct a symbol representing a tuple.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] arguments the arguments of the function
//! @param[in] arguments_size the number of arguments
//! @param[out] symbol the resulting symbol
//! @return the result code; might return one of the following codes:
//! - ::clingo_result_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_tuple(clingo_lib_t *lib, clingo_symbol_t const *arguments,
                                                          size_t arguments_size, clingo_symbol_t *symbol);

//! Construct a symbol representing a function.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] name the name of the function
//! @param[in] name_size the size of the name
//! @param[in] arguments the arguments of the function
//! @param[in] arguments_size the number of arguments
//! @param[in] is_positive whether the symbol is_positive
//! @param[out] symbol the resulting symbol
//! @return the result code; might return one of the following codes:
//! - ::clingo_result_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_create_function(clingo_lib_t *lib, char const *name, size_t name_size,
                                                             clingo_symbol_t const *arguments, size_t arguments_size,
                                                             bool is_positive, clingo_symbol_t *symbol);

//! Parse a term in string form.
//!
//! The result of this function is a symbol. The input term can contain
//! unevaluated functions, which are evaluated during parsing.
//!
//! @param[in] lib library object storing the symbol
//! @param[in] string the string to parse
//! @param[in] size the size of the string
//! @param[out] symbol the resulting symbol
//! @return the result code; might return one of the following codes:
//! - ::clingo_result_bad_alloc
//! - ::clingo_result_runtime if parsing fails
CLINGO_VISIBILITY_DEFAULT bool clingo_parse_term(clingo_lib_t *lib, char const *string, size_t size,
                                                 clingo_symbol_t *symbol);

//! Acquire ownership of the given symbol.
//!
//! Symbols not having any owners, are freed during garbage collection.
//!
//! @param[in] symbol the resulting symbol
CLINGO_VISIBILITY_DEFAULT void clingo_symbol_acquire(clingo_symbol_t symbol);

//! Release ownership of the given symbol.
//!
//! Symbols not having any owners, are freed during garbage collection.
//!
//! @param[in] symbol the resulting symbol
CLINGO_VISIBILITY_DEFAULT void clingo_symbol_release(clingo_symbol_t symbol);

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
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_name(clingo_symbol_t symbol, clingo_string_t *name);

//! Get the string of a symbol.
//!
//! @note
//! The string is internalized and valid for the duration of the process.
//!
//! @param[in] symbol the target symbol
//! @param[out] string the resulting string
//! @return the result code
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_string(clingo_symbol_t symbol, clingo_string_t *string);

//! Check whether a function or number is positive.
//!
//! @param[in] symbol the target symbol
//! @param[out] is_positive the result
//! @return the result code
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_is_positive(clingo_symbol_t symbol, bool *is_positive);

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

//! Get the string representation of a symbol.
//!
//! @param[in] symbol the target symbol
//! @param[in] builder the string builder
//! @return the result code
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_to_string(clingo_symbol_t symbol, clingo_string_builder_t *builder);

//! @}

//! @name Symbol Comparison Functions
//! @{

//! Check if two symbols are equal.
//!
//! @param[in] a first symbol
//! @param[in] b second symbol
//! @return whether a == b
CLINGO_VISIBILITY_DEFAULT bool clingo_symbol_equal(clingo_symbol_t a, clingo_symbol_t b);

//! Check if a symbol is less than another symbol.
//!
//! Symbols are first compared by type.  If the types are equal, the values are
//! compared (where strings are compared using strcmp).  Functions are first
//! compared by signature and then lexicographically by arguments.
//!
//! @param[in] a first symbol
//! @param[in] b second symbol
//! @return whether a < b
CLINGO_VISIBILITY_DEFAULT int clingo_symbol_compare(clingo_symbol_t a, clingo_symbol_t b);

//! Calculate a hash code of a symbol.
//!
//! @param[in] symbol the target symbol
//! @return the hash code of the symbol
CLINGO_VISIBILITY_DEFAULT size_t clingo_symbol_hash(clingo_symbol_t symbol);

//! @}

//! @}

#ifdef __cplusplus
}
#endif

#endif
