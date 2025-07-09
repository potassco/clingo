#ifndef CLINGO_CONTROL_H
#define CLINGO_CONTROL_H

#include <clingo/core.h>
#include <clingo/symbol.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clingo_program clingo_program_t;

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

//! @addtogroup c_control
//! Functions to control the grounding and solving process.
//!
//! For an example, see @ref control.c.
//! @{

//! Control object holding grounding and solving state.
typedef struct clingo_control clingo_control_t;

//! The available application modes.
enum clingo_mode_e {
    clingo_mode_parse = 0,   //!< parse only
    clingo_mode_rewrite = 1, //!< parse and rewrite
    clingo_mode_ground = 2,  //!< parse, rewrite, ground
    clingo_mode_solve = 3,   //!< parse, rewrite, ground, and solve
};
//! The corresponding type to ::clingo_mode_e.
typedef int clingo_mode_t;

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
    size_t name_size;              //!< the size of the name
    clingo_symbol_t const *params; //!< array of parameters
    size_t params_size;            //!< number of parameters
} clingo_part_t;

//! Callback function to implement external functions.
//!
//! If an external function of form <tt>\@name(parameters)</tt> occurs in a logic program,
//! then this function is called with its location, name, parameters, and a callback to inject symbols as arguments.
//! The callback can be called multiple times; all symbols passed are injected.
//!
//! If a (non-recoverable) clingo API function fails in this callback, for example, the symbol callback, the callback
//! must return its return code. In case of errors not related to clingo, this function can use clingo_set_error().
//!
//! @param[in] lib the library object
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
//! ground_callback(clingo_lib_t *lib,
//!                 clingo_location_t const *location,
//!                 char const *name,
//!                 size_t name_size,
//!                 clingo_symbol_t const *arguments,
//!                 size_t arguments_size,
//!                 void *data,
//!                 clingo_symbol_callback_t symbol_callback,
//!                 void *symbol_callback_data) {
//!   if (size == 1 && strncmp(name, "f", 1) == 0 && arguments_size == 0) {
//!     clingo_symbol_t sym;
//!     sym = clingo_symbol_create_number(42);
//!     return symbol_callback(&sym, 1, symbol_callback_data);
//!   }
//!   return clingo_set_error(lib, clingo_result_runtime, "function not found", 18);
//! }
//! ~~~~~~~~~~~~~~~
typedef bool (*clingo_ground_callback_t)(clingo_lib_t *lib, clingo_location_t const *location, char const *name,
                                         size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size,
                                         void *data, clingo_symbol_callback_t symbol_callback,
                                         void *symbol_callback_data);

//! A map from constantns to their values.
typedef struct clingo_const_map clingo_const_map_t;

//! Get the constant with the given name.
//!
//! @param[in] map the target
//! @param[in] name the name of the constant
//! @param[in] name_size the size of the name
//! @param[out] symbol the value of the constant
//! @param[out] found whether the constant was found
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_const_map_find(clingo_const_map_t const *map, char const *name, size_t name_size,
                                                     clingo_symbol_t *symbol, bool *found);

//! Get the name and value of the contstant at the given index.
//!
//! @param[in] map the target
//! @param[in] index the index of the elemnt
//! @param[out] name the name of the constant
//! @param[out] symbol the value of the constant
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_const_map_at(clingo_const_map_t const *map, size_t index, clingo_string_t *name,
                                                   clingo_symbol_t *symbol);

//! Get the size of the constant map.
//!
//! @param[in] map the target
//! @param[out] size the size of the map
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_const_map_size(clingo_const_map_t const *map, size_t *size);

//! Create a new control object.
//!
//! A control object has to be freed using clingo_control_free().
//!
//! @note Only gringo options (without <code>\-\-output</code>) and clasp's options are supported as arguments,
//! except basic options such as <code>\-\-help</code>.
//! Furthermore, a control object is blocked while a search call is active;
//! you must not call any member function during search.
//!
//! @param[in] lib clingo library object
//! @param[in] arguments string array of command line arguments
//! @param[in] size size of the arguments array
//! @param[out] control resulting control object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_new(clingo_lib_t *lib, clingo_string_t const *arguments, size_t size,
                                                  clingo_control_t **control);

//! Increment the reference count of the given control object.
//!
//! @param[in] control the target
CLINGO_VISIBILITY_DEFAULT void clingo_control_acquire(clingo_control_t *control);

//! Decrement the reference count of the given control object and destroy if zero.
//!
//! @param[in] control the target
CLINGO_VISIBILITY_DEFAULT void clingo_control_release(clingo_control_t *control);

//! Get the configured mode.
//!
//! @param[in] control the target
//! @param[in] mode the mode
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_mode(clingo_control_t *control, clingo_mode_t *mode);

//! Extend the logic program with a program in a file.
//!
//! @param[in] control the target
//! @param[in] files the files to parse
//! @param[in] size the number of files to parse
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_parse_files(clingo_control_t *control, clingo_string_t const *files,
                                                          size_t size);

//! Extend the logic program with the given non-ground logic program in string form.
//!
//! After extending the logic program, the corresponding program parts are typically grounded with
//! ::clingo_control_ground.
//!
//! @param[in] control the target
//! @param[in] program string representation of the program
//! @param[in] size the size of the program
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_parse_string(clingo_control_t *control, char const *program, size_t size);

//! Ground the selected parts of the current (non-ground) logic program.
//!
//! After grounding, logic programs can be solved with ::clingo_control_solve().
//!
//! @note Parts of a logic program without an explicit <tt>\#program</tt>
//! specification are by default put into a program called `base` without
//! arguments.
//!
//! @param[in] control the target
//! @param[in] parts array of parts to ground
//! @param[in] size size of the parts array
//! @param[in] ground_callback callback to implement external functions
//! @param[in] data user data for ground_callback
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_ground(clingo_control_t *control, clingo_part_t const *parts, size_t size,
                                                     clingo_ground_callback_t ground_callback, void *data);

//! Execute the default ground and solve flow after parsing.
//!
//! @param[in] control the target
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_main(clingo_control_t *control);

//! Get the map of constants.
//!
//! @param[in] control the target
//! @param[out] map the map of constants
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_const_map(clingo_control_t *control, clingo_const_map_t const **map);

//! Get the output of the text output.
//!
//! @note The control object has to be created passing option `--text-buffer`.
//!
//! @param[in] control the target
//! @param[out] result the resulting string
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_buffer(clingo_control_t *control, clingo_string_t *result);

//! Get the program parts to ground.
//!
//! @param[in] control the target
//! @param[out] parts the resulting parts
//! @param[out] size the resulting parts
//! @param[out] has_value the resulting parts
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_get_parts(clingo_control_t *control, clingo_part_t const **parts,
                                                        size_t *size, bool *has_value);

//! Set the program parts to ground.
//!
//! @param[in] control the target
//! @param[in] parts the parts to set
//! @param[in] size the size of the parts
//! @param[in] has_value whether parts are available
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_set_parts(clingo_control_t *control, clingo_part_t const *parts,
                                                        size_t size, bool has_value);

//! Enumeration of discardable statements.
enum clingo_discard_type_e {
    minimize = 1,
    project = 2,
};
//! Corresponding type to clingo_discard_type_e.
typedef unsigned clingo_discard_type_t;

//! Discard the statements of the given types.
//!
//! @param[in] control the target control
//! @param[in] type what to discard
CLINGO_VISIBILITY_DEFAULT bool clingo_control_discard(clingo_control_t *control, clingo_discard_type_t type);

//! Interrupt the running search.
//!
//! It is generally better to use clingo_solve_handle_cancel(). This function
//! is thread-safe.
//!
//! @param[in] control the target
CLINGO_VISIBILITY_DEFAULT void clingo_control_interrupt(clingo_control_t *control);

//! @}

#ifdef __cplusplus
}
#endif

#endif
