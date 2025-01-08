#ifndef CLINGO_CONTROL_H
#define CLINGO_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/core.h>
#include <clingo/symbol.h>

// NOLINTBEGIN(modernize-*,cppcoreguidelines-macro-usage,performance-enum-size)

// Note: forward declaration
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

//! Callback function to implement external functions.
//!
//! If an external function of form <tt>\@name(parameters)</tt> occurs in a logic program,
//! then this function is called with its location, name, parameters, and a callback to inject symbols as arguments.
//! The callback can be called multiple times; all symbols passed are injected.
//!
//! If a (non-recoverable) clingo API function fails in this callback, for example, the symbol callback, the callback
//! must return its return code. In case of errors not related to clingo, this function can return
//! ::clingo_result_unknown to stop grounding with an error.
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
//!                 clingo_symbol_t const *arguments,
//!                 size_t arguments_size,
//!                 void *data,
//!                 clingo_symbol_callback_t symbol_callback,
//!                 void *symbol_callback_data) {
//!   if (strcmp(name, "f") == 0 && arguments_size == 0) {
//!     clingo_symbol_t sym;
//!     sym = clingo_symbol_create_number(42);
//!     return symbol_callback(&sym, 1, symbol_callback_data);
//!   }
//!   clingo_lib_report(lib, clingo_result_runtime, "function not found");
//!   return clingo_result_runtime;
//! }
//! ~~~~~~~~~~~~~~~
typedef clingo_result_t (*clingo_ground_callback_t)(clingo_lib_t *lib, clingo_location_t const *location,
                                                    char const *name, clingo_symbol_t const *arguments,
                                                    size_t arguments_size, void *data,
                                                    clingo_symbol_callback_t symbol_callback,
                                                    void *symbol_callback_data);

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
//! @param[in] arguments C string array of command line arguments
//! @param[in] arguments_size size of the arguments array
//! @param[out] control resulting control object
//! @return the result code; might return one of the following codes:
//! - ::clingo_result_bad_alloc
//! - ::clingo_result_runtime if argument parsing fails
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_control_new(clingo_lib_t *lib, char const *const *arguments,
                                                             size_t arguments_size, clingo_control_t **control);

//! Free a control object created with clingo_control_new().
//! @param[in] control the target
CLINGO_VISIBILITY_DEFAULT void clingo_control_free(clingo_control_t *control);

//! Extend the logic program with a program in a file.
//!
//! @param[in] control the target
//! @param[in] files the files to parse
//! @param[in] files_size the number of files to parse
//! @return the result code; might return one of the following codes:
//! - ::clingo_result_bad_alloc
//! - ::clingo_result_runtime if parsing or checking fails
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_control_parse_file(clingo_control_t *control, char const **files,
                                                                    size_t files_size);

//! Extend the logic program with the given non-ground logic program in string form.
//!
//! After extending the logic program, the corresponding program parts are typically grounded with
//! ::clingo_control_ground.
//!
//! @param[in] control the target
//! @param[in] program string representation of the program
//! @return the result code; might return one of the following codes:
//! - ::clingo_result_bad_alloc
//! - ::clingo_result_runtime if parsing fails
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_control_parse_string(clingo_control_t *control, char const *program);

//! Extend the control objects's program with the given one.
//!
//! After extending the logic program, the corresponding program parts are typically grounded with
//! ::clingo_control_ground.
//!
//! @param[in] control the target
//! @param[in] program the program to add
//! @return the result code; might return one of the following codes:
//! - ::clingo_result_bad_alloc
//! - ::clingo_result_runtime if rewriting fails
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_control_join(clingo_control_t *control,
                                                              clingo_program_t const *program);

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
//! @param[in] parts_size size of the parts array
//! @param[in] ground_callback callback to implement external functions
//! @param[in] ground_callback_data user data for ground_callback
//! @return the result code; might return one of the following codes:
//! - ::clingo_result_bad_alloc
//! - error code of ground callback
//!
//! @see clingo_part
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_control_ground(clingo_control_t *control, clingo_part_t const *parts,
                                                                size_t parts_size,
                                                                clingo_ground_callback_t ground_callback,
                                                                void *ground_callback_data);

//! Execute the default ground and solve flow after parsing.
//!
//! @param[in] control the target
//! @return the result code; might return one of the following codes:
//! - ::clingo_result_bad_alloc
//! - error code of ground callback
//!
//! @see clingo_part
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_control_main(clingo_control_t *control);

//! Get the output of the text output.
//!
//! @note The control object has to be created passing option `--text-buffer`.
//!
//! @param[in] control the target
//! @param[out] buffer the resulting string
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_control_buffer(clingo_control_t *control, char const **buffer);

//! Solve the currently grounded logic program enumerating its models.
//!
//! See the @ref SolveHandle module for more information.
//!
//! @param[in] control the target
//! @param[in] mode configures the search mode
//! @param[in] assumptions array of assumptions to solve under
//! @param[in] assumptions_size number of assumptions
//! @param[in] notify the event handler to register
//! @param[in] data the user data for the event handler
//! @param[out] handle handle to the current search to enumerate models
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_control_solve(clingo_control_t *control);
// CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_control_solve(
//     clingo_control_t *control, clingo_solve_mode_bitset_t mode, clingo_literal_t const *assumptions,
//     size_t assumptions_size, clingo_solve_event_callback_t notify, void *data, clingo_solve_handle_t **handle);
//! @}

// NOLINTEND(modernize-*,cppcoreguidelines-macro-usage,performance-enum-size)

#ifdef __cplusplus
}
#endif

#endif
