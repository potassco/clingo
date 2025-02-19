#ifndef CLINGO_CORE_H
#define CLINGO_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

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
//! Set the visibility of a symbol to default.
#define CLINGO_VISIBILITY_DEFAULT
//! Set the visibility of a symbol to private.
#define CLINGO_VISIBILITY_PRIVATE
#endif
#endif
#endif

#if defined __GNUC__
#define CLINGO_DEPRECATED __attribute__((deprecated))
#elif defined _MSC_VER
#define CLINGO_DEPRECATED __declspec(deprecated)
#else
//! Mark a function as deprecated.
#define CLINGO_DEPRECATED
#endif

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

//! @addtogroup c_core
//! Core types and functions used throughout all modules and version information.
//!
//! For an example, see @ref version.c.
//! @{

//! Major version number.
#define CLINGO_VERSION_MAJOR 6
//! Minor version number.
#define CLINGO_VERSION_MINOR 0
//! Revision number.
#define CLINGO_VERSION_REVISION 0
//! String representation of version.
#define CLINGO_VERSION "6.0.0"

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
    clingo_literal_t literal; //!< the literal
    clingo_weight_t weight;   //!< the weight
} clingo_weighted_literal_t;

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
enum clingo_result_e {
    clingo_result_success = 0,   //!< successful API calls
    clingo_result_runtime = 1,   //!< errors only detectable at runtime like invalid input
    clingo_result_logic = 2,     //!< wrong usage of the clingo API
    clingo_result_bad_alloc = 3, //!< memory could not be allocated
    clingo_result_invalid = 4,   //!< invalid arguments passed to function
    clingo_result_range = 5,     //!< result out of range
    clingo_result_unknown = 6    //!< errors unrelated to clingo
};
//! Corresponding type to ::clingo_result_e.
typedef int clingo_result_t;

//! Convert the given result code into a string.
//!
//! The function returns string literals that do not have to be cleaned up.
//!
//! @param[in] code the result code
//! @return the string representation
CLINGO_VISIBILITY_DEFAULT char const *clingo_result_string(clingo_result_t code);

//! Enumeration of message codes.
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

//! Convert the giving message code into a string.
//!
//! The function returns string literals that do not have to be cleaned up.
//!
//! @param[in] code the message code
//! @return the string representation
CLINGO_VISIBILITY_DEFAULT char const *clingo_message_string(clingo_message_t code);

//! Callback to intercept messages.
//!
//! @param[in] code associated code
//! @param[in] message message
//! @param[in] data user data for callback
//!
//! @see clingo_lib_new()
typedef void (*clingo_logger_t)(clingo_message_t code, char const *message, void *data);

//! Callback to free user data.
//!
//! @param[in] data the user data to free
typedef void (*clingo_free_t)(void *data);

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
//! @param[in] flags construction flags
//! @param[in] logger callback functions for warnings and info messages
//! @param[in] logger_free callback to free the logger
//! @param[in] logger_data user data for the logger callback
//! @param[in] message_limit maximum number of times the logger callback is called
//! @param[out] lib the resulting library object
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_lib_new(clingo_lib_flags_t flags, clingo_logger_t logger,
                                                         clingo_free_t logger_free, void *logger_data,
                                                         size_t message_limit, clingo_lib_t **lib);

//! Report a message via the libraries logger.
//!
//! @param[in] lib the libary
//! @param[in] code associated code
//! @param[in] message message
CLINGO_VISIBILITY_DEFAULT void clingo_lib_report(clingo_lib_t *lib, clingo_message_t code, char const *message);

//! Free a library object created with clingo_lib_new().
//!
//! If parameter fast is set to false, the garbage collector is run to ensure
//! that all symbols are freed. If there are still referenced symbols, the
//! library is not deleted but put in a list for later cleanup. Further calls
//! to this function trigger further cleanups (the lib parameter can be set to
//! NULL to just run the cleanup).
//!
//! The flag is mainly intended for language bindings, where cleanup of all
//! symbols cannot be guaranteed due to unpredicatable garbage collection.
//! Objects using the library can still be freed after this call.
//!
//! @param[in] lib the target
//! @param[in] fast whether to perform a fast destruction
CLINGO_VISIBILITY_DEFAULT void clingo_lib_free(clingo_lib_t *lib, bool fast);

//! A builder for strings.
typedef struct clingo_string_builder clingo_string_builder_t;

//! Allocator to (re)allocate client-side memory.
//!
//! In case a new block of memory is allocated, the callback is in charge of
//! freeing the previous memory block and copying the previous data.
//!
//! @param[in] size the number of bytes to (re)allocate
//! @param[in] data userdata for the callback
//! @param[out] ptr pointer to the allocated memory
//! @return the result code
//! @todo Decide how to use.
typedef clingo_result_t (*clingo_alloc_t)(size_t size, void *data, void **ptr);

//! Create a new string builder.
//!
//! @param[out] bld the resulting builder
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_string_builder_new(clingo_string_builder_t **bld);
//! Copy the string builder.
//!
//! @param[in] src the builder to copy
//! @param[out] dst the resulting builder
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_string_builder_copy(clingo_string_builder_t const *src,
                                                                     clingo_string_builder_t **dst);
//! Free the string builder.
//!
//! @param[in] bld the builder
CLINGO_VISIBILITY_DEFAULT void clingo_string_builder_free(clingo_string_builder_t const *bld);

//! Get the (zero-terminated) string in the builder.
//!
//! @param[in] bld the builder
//! @param[out] str the resulting string
//! @param[out] size the resulting size
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_string_builder_string(clingo_string_builder_t const *bld,
                                                                       char const **str, size_t *size);
//! Clear the string in the builder.
//!
//! @param[in] bld the builder
CLINGO_VISIBILITY_DEFAULT void clingo_string_builder_clear(clingo_string_builder_t *bld);

//! Represents a cursor position in source code.
//!
//! @note Not all positions refer to physical files.
//! By convention, such locations use a name put in angular brackets as filename.
typedef struct clingo_position clingo_position_t;

//! Create a new source position object.
//!
//! @param[in] lib the library storing strings
//! @param[in] file the file of the position
//! @param[in] line the line number of the position
//! @param[in] column the column number of the position
//! @param[out] pos the resulting position
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_position_new(clingo_lib_t *lib, char const *file, size_t line,
                                                              size_t column, clingo_position_t const **pos);
//! Copy the given position.
//!
//! @param[in] src the position to copy
//! @param[out] dst the resulting position
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_position_copy(clingo_position_t const *src,
                                                               clingo_position_t const **dst);
//! Free the given position.
//!
//! @param[in] pos the position to free
CLINGO_VISIBILITY_DEFAULT void clingo_position_free(clingo_position_t const *pos);

//! Get the file name of the position.
//!
//! @param[in] pos the position
//! @return the file name
CLINGO_VISIBILITY_DEFAULT char const *clingo_position_file(clingo_position_t const *pos);
//! Get the line number of the position.
//!
//! @param[in] pos the position
//! @return the line number
CLINGO_VISIBILITY_DEFAULT size_t clingo_position_line(clingo_position_t const *pos);
//! Get the column number of the position.
//!
//! @param[in] pos the position
//! @return the column number
CLINGO_VISIBILITY_DEFAULT size_t clingo_position_column(clingo_position_t const *pos);
//! Compute a hash of the position.
//!
//! @param[in] pos the position
//! @return the resulting hash
CLINGO_VISIBILITY_DEFAULT size_t clingo_position_hash(clingo_position_t const *pos);
//! Check if two positions are equal.
//!
//! Only positions associcated with the same library may be compared.
//!
//! @param[in] a the first position
//! @param[in] b the second position
//! @return whether the positions are equal
CLINGO_VISIBILITY_DEFAULT bool clingo_position_equal(clingo_position_t const *a, clingo_position_t const *b);
//! Compare two positions.
//!
//! Only positions associcated with the same library may be compared.
//!
//! @param[in] a the first position
//! @param[in] b the second position
//! @return the comparator
CLINGO_VISIBILITY_DEFAULT int clingo_position_compare(clingo_position_t const *a, clingo_position_t const *b);
//! Convert the given position into a string.
//!
//! @param[in] pos the position
//! @param[in] str the string builder
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_position_to_string(clingo_position_t const *pos,
                                                                    clingo_string_builder_t *str);

//! Represents a source code location marking its beginning and end.
typedef struct clingo_location clingo_location_t;

//! Create a new source location object.
//!
//! @param[in] begin the position marking the beginning
//! @param[in] end the position marking the end
//! @param[out] loc the resulting location
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_location_new(clingo_position_t const *begin,
                                                              clingo_position_t const *end,
                                                              clingo_location_t const **loc);
//! Copy the given location.
//!
//! @param[in] src the location to copy
//! @param[out] dst the resulting location
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_location_copy(clingo_location_t const *src,
                                                               clingo_location_t const **dst);
//! Free the given location.
//!
//! @param[in] loc the location to free
CLINGO_VISIBILITY_DEFAULT void clingo_location_free(clingo_location_t const *loc);

//! Get the beginning of the location.
//!
//! The lifetime of the position is tied to that of the location.
//!
//! @param[in] loc the location
//! @return the beginning position
CLINGO_VISIBILITY_DEFAULT clingo_position_t const *clingo_location_begin(clingo_location_t const *loc);
//! Get the end of the location.
//!
//! The lifetime of the position is tied to that of the location.
//!
//! @param[in] loc the location
//! @return the end position
CLINGO_VISIBILITY_DEFAULT clingo_position_t const *clingo_location_end(clingo_location_t const *loc);
//! Compute a hash of the location.
//!
//! @param[in] loc the location
//! @return the resulting hash
CLINGO_VISIBILITY_DEFAULT size_t clingo_location_hash(clingo_location_t const *loc);
//! Check if two locations are equal.
//!
//! Only locations associcated with the same library may be compared.
//!
//! @param[in] a the first location
//! @param[in] b the second location
//! @return whether the location are equal
CLINGO_VISIBILITY_DEFAULT bool clingo_location_equal(clingo_location_t const *a, clingo_location_t const *b);
//! Compare two locations.
//!
//! Only locations associcated with the same library may be compared.
//!
//! @param[in] a the first location
//! @param[in] b the second location
//! @return the comparator
CLINGO_VISIBILITY_DEFAULT int clingo_location_compare(clingo_location_t const *a, clingo_location_t const *b);
//! Convert the given location into a string.
//!
//! @param[in] loc the location
//! @param[in] str the string builder
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_location_to_string(clingo_location_t const *loc,
                                                                    clingo_string_builder_t *str);

//! @}

#ifdef __cplusplus
}
#endif

#endif
