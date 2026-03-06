#ifndef CLINGO_CORE_H
#define CLINGO_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef CLINGO_NO_VISIBILITY
#define CLINGO_VISIBILITY_DEFAULT
#define CLINGO_VISIBILITY_PRIVATE
#elif defined __EMSCRIPTEN__
#include <emscripten.h>
#define CLINGO_VISIBILITY_DEFAULT EMSCRIPTEN_KEEPALIVE
#define CLINGO_VISIBILITY_PRIVATE
#elif defined _WIN32 || defined __CYGWIN__
#ifdef CLINGO_BUILD_LIBRARY
#define CLINGO_VISIBILITY_DEFAULT __declspec(dllexport)
#else
#define CLINGO_VISIBILITY_DEFAULT __declspec(dllimport)
#endif
#define CLINGO_VISIBILITY_PRIVATE
#elif __GNUC__ >= 4
#define CLINGO_VISIBILITY_DEFAULT __attribute__((visibility("default")))
#define CLINGO_VISIBILITY_PRIVATE __attribute__((visibility("hidden")))
#else
#define CLINGO_VISIBILITY_DEFAULT
#define CLINGO_VISIBILITY_PRIVATE
#endif

#ifdef __GNUC__
#define CLINGO_DEPRECATED __attribute__((deprecated))
#elif defined _MSC_VER
#define CLINGO_DEPRECATED __declspec(deprecated)
#else
//! Mark a function as deprecated.
#define CLINGO_DEPRECATED
#endif

#ifdef __cplusplus
extern "C" {
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
//! The name of the clingo executable.
#define CLINGO_EXECUTABLE "clingo"

//! Signed integer type used for aspif and solver literals.
typedef int32_t clingo_literal_t;
//! Unsigned integer type used for aspif atoms.
typedef uint32_t clingo_atom_t;
//! Unsigned integer type used in various places.
typedef uint32_t clingo_id_t;
//! Signed integer type for weights in sum aggregates and minimize constraints.
typedef int32_t clingo_weight_t;
//! A literal with an associated weight.
typedef struct clingo_weighted_literal {
    clingo_literal_t literal; //!< the literal
    clingo_weight_t weight;   //!< the weight
} clingo_weighted_literal_t;
//! Struct to capture strings that are not null-terminated.
typedef struct clingo_string {
    char const *data; //!< pointer to the beginning of the string
    size_t size;      //!< the length of the string
} clingo_string_t;

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
};
//! Corresponding type to ::clingo_result_e.
typedef int clingo_result_t;

//! Set an error in the current thread.
//!
//! @param[in] code the error code
//! @param[in] message the error message
//! @param[in] size the size of the error message
//! @return returns false
CLINGO_VISIBILITY_DEFAULT bool clingo_set_error(clingo_result_t code, char const *message, size_t size);

//! Get the error set in the current thread.
//!
//! @param[out] code the error code
//! @param[out] message the error message
CLINGO_VISIBILITY_DEFAULT void clingo_get_error(clingo_result_t *code, clingo_string_t *message);

//! Clear the current error.
CLINGO_VISIBILITY_DEFAULT void clingo_clear_error(void);

//! Enumeration of message codes.
enum clingo_message_e {
    clingo_message_trace = 0,               //!< a trace message
    clingo_message_debug = 1,               //!< a debug message
    clingo_message_info = 2,                //!< an info message
    clingo_message_operation_undefined = 3, //!< undefined operation in program
    clingo_message_atom_undefined = 4,      //!< undefined atom in program
    clingo_message_file_included = 5,       //!< same file included multiple times
    clingo_message_global_variable = 6,     //!< global variable in tuple of aggregate element
    clingo_message_warn = 7,                //!< a warning message
    clingo_message_error = 8, //!< to report multiple errors; a corresponding runtime error is raised later
};
//! Corresponding type to ::clingo_message_e.
typedef int clingo_message_t;

//! Enumeration of log levels.
enum clingo_log_level_e {
    clingo_log_level_trace = clingo_message_trace, //!< the trace level (most verbose)
    clingo_log_level_debug = clingo_message_debug, //!< the debug level
    clingo_log_level_info = clingo_message_info,   //!< the info level
    clingo_log_level_warn = clingo_message_warn,   //!< the warning level
    clingo_log_level_error = clingo_message_error, //!< the error level (least verbose)
};
//! Corresponding type to ::clingo_log_level_e.
typedef int clingo_log_level_t;

//! The clingo logger.
typedef struct clingo_logger {
    //! Callback to intercept messages.
    //!
    //! @param[in] code associated code
    //! @param[in] message the message
    //! @param[in] size the size of the message
    //! @param[in] data user data for callback
    //!
    //! @see clingo_lib_new()
    void (*log)(clingo_message_t code, char const *message, size_t size, void *data);
    //! Free the logger.
    //!
    //! @param[in] data user data for callback
    //!
    //! @see clingo_lib_new()
    void (*free)(void *data);
} clingo_logger_t;

//! A library object storing global information.
typedef struct clingo_lib clingo_lib_t;

//! Flags to create library objects.
enum clingo_lib_flags_e {
    clingo_lib_flags_slotted = 1,      //!< use custom allocator for storing symbols
    clingo_lib_flags_shared = 2,       //!< create symbols in a thread-safe manner
    clingo_lib_flags_fast_release = 4, //!< whether to enable fast release of libraries
};
//! Bitset of ::clingo_lib_flags_e.
typedef uint32_t clingo_lib_flags_t;

//! Obtain the clingo version.
//!
//! @param[out] major major version number
//! @param[out] minor minor version number
//! @param[out] revision revision number
CLINGO_VISIBILITY_DEFAULT void clingo_version(int *major, int *minor, int *revision);

//! Convert the given result code into a string.
//!
//! The function returns string literals that do not have to be cleaned up.
//!
//! @param[in] code the result code
//! @param[out] value the string
CLINGO_VISIBILITY_DEFAULT void clingo_result_string(clingo_result_t code, clingo_string_t *value);

//! Convert the giving message code into a string.
//!
//! The function returns string literals that do not have to be cleaned up.
//!
//! @param[in] code the message code
//! @param[out] value the string
CLINGO_VISIBILITY_DEFAULT void clingo_message_string(clingo_message_t code, clingo_string_t *value);

//! Create a library object.
//!
//! A library has to be freed using clingo_lib_free().
//!
//! If the logger is NULL, the default logger printing messages to stderr is used.
//!
//! @param[in] flags construction flags
//! @param[in] level the log level for the message logger
//! @param[in] logger callback functions for warnings and info messages
//! @param[in] data user data for the logger callback
//! @param[in] limit the message limit
//! @param[out] lib the resulting library object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_lib_new(clingo_lib_flags_t flags, clingo_log_level_t level,
                                              clingo_logger_t const *logger, void *data, size_t limit,
                                              clingo_lib_t **lib);

//! Increment the reference count of the given library.
//!
//! @param[in] lib the target
CLINGO_VISIBILITY_DEFAULT void clingo_lib_acquire(clingo_lib_t *lib);

//! Release a library object created with clingo_lib_new().
//!
//! If fast release has been disabled, the garbage collector is run to ensure
//! that all symbols are freed. If there are still referenced symbols, the
//! library is not deleted but put in a list for later cleanup. Further calls
//! to this function trigger further cleanups (the lib parameter can be set to
//! NULL to just run the cleanup).
//!
//! The flag is mainly intended for language bindings, where cleanup of all
//! symbols cannot be guaranteed due to unpredictable garbage collection.
//! Objects using the library can still be freed after this call.
//!
//! @param[in] lib the target
CLINGO_VISIBILITY_DEFAULT void clingo_lib_release(clingo_lib_t *lib);

//! Report a message via the library’s logger.
//!
//! @param[in] lib the library
//! @param[in] code associated code
//! @param[in] message the message
//! @param[in] size the size of the message
CLINGO_VISIBILITY_DEFAULT void clingo_lib_report(clingo_lib_t *lib, clingo_message_t code, char const *message,
                                                 size_t size);

//! A builder for strings.
typedef struct clingo_string_builder clingo_string_builder_t;

//! Create a new string builder.
//!
//! @param[out] bld the resulting builder
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_string_builder_new(clingo_string_builder_t **bld);
//! Copy the string builder.
//!
//! @param[in] src the builder to copy
//! @param[out] dst the resulting builder
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_string_builder_copy(clingo_string_builder_t const *src,
                                                          clingo_string_builder_t **dst);
//! Free the string builder.
//!
//! @param[in] bld the builder
CLINGO_VISIBILITY_DEFAULT void clingo_string_builder_free(clingo_string_builder_t const *bld);

//! Get the (zero-terminated) string in the builder.
//!
//! @param[in] bld the builder
//! @param[out] value the resulting string
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_string_builder_string(clingo_string_builder_t const *bld, clingo_string_t *value);
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
//! @param[in] size the size of the file string
//! @param[in] line the line number of the position
//! @param[in] column the column number of the position
//! @param[out] pos the resulting position
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_position_new(clingo_lib_t *lib, char const *file, size_t size, size_t line,
                                                   size_t column, clingo_position_t const **pos);
//! Copy the given position.
//!
//! @param[in] src the position to copy
//! @param[out] dst the resulting position
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_position_copy(clingo_position_t const *src, clingo_position_t const **dst);
//! Free the given position.
//!
//! @param[in] pos the position to free
CLINGO_VISIBILITY_DEFAULT void clingo_position_free(clingo_position_t const *pos);

//! Get the file name of the position.
//!
//! @param[in] pos the position
//! @param[out] value the file name
CLINGO_VISIBILITY_DEFAULT void clingo_position_file(clingo_position_t const *pos, clingo_string_t *value);
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
//! Only positions associated with the same library may be compared.
//!
//! @param[in] a the first position
//! @param[in] b the second position
//! @return whether the positions are equal
CLINGO_VISIBILITY_DEFAULT bool clingo_position_equal(clingo_position_t const *a, clingo_position_t const *b);
//! Compare two positions.
//!
//! Only positions associated with the same library may be compared.
//!
//! @param[in] a the first position
//! @param[in] b the second position
//! @return the comparator
CLINGO_VISIBILITY_DEFAULT int clingo_position_compare(clingo_position_t const *a, clingo_position_t const *b);
//! Convert the given position into a string.
//!
//! @param[in] pos the position
//! @param[in] str the string builder
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_position_to_string(clingo_position_t const *pos, clingo_string_builder_t *str);

//! Represents a source code location marking its beginning and end.
typedef struct clingo_location clingo_location_t;

//! Create a new source location object.
//!
//! @param[in] begin the position marking the beginning
//! @param[in] end the position marking the end
//! @param[out] loc the resulting location
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_location_new(clingo_position_t const *begin, clingo_position_t const *end,
                                                   clingo_location_t const **loc);
//! Copy the given location.
//!
//! @param[in] src the location to copy
//! @param[out] dst the resulting location
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_location_copy(clingo_location_t const *src, clingo_location_t const **dst);
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
//! Only locations associated with the same library may be compared.
//!
//! @param[in] a the first location
//! @param[in] b the second location
//! @return whether the location are equal
CLINGO_VISIBILITY_DEFAULT bool clingo_location_equal(clingo_location_t const *a, clingo_location_t const *b);
//! Compare two locations.
//!
//! Only locations associated with the same library may be compared.
//!
//! @param[in] a the first location
//! @param[in] b the second location
//! @return the comparator
CLINGO_VISIBILITY_DEFAULT int clingo_location_compare(clingo_location_t const *a, clingo_location_t const *b);
//! Convert the given location into a string.
//!
//! @param[in] loc the location
//! @param[in] str the string builder
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_location_to_string(clingo_location_t const *loc, clingo_string_builder_t *str);

//! @}

#ifdef __cplusplus
}
#endif

#endif
