#ifndef CLINGO_CONTROL_H
#define CLINGO_CONTROL_H

#include <clingo/core.h>
#include <clingo/ground.h>
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

//! A map from constants to their values.
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

//! Get the name and value of the contestant at the given index.
//!
//! @param[in] map the target
//! @param[in] index the index of the element
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
//! @note Only gringo options (without <code>\-\-output</code>) and clasp's
//! options are supported as arguments, except basic options such as
//! <code>\-\-help</code>. Furthermore, a control object is blocked while a
//! search call is active; you must not call any member function during search.
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

//! Get the reference count of the given control object.
//!
//! Language bindings can use this to check how many references to the given
//! control object are held.
//!
//! @param[in] control the target
//! @return the reference count
CLINGO_VISIBILITY_DEFAULT size_t clingo_control_ref_count(clingo_control_t const *control);

//! Callback to finalize user data when an object is destroyed.
typedef void (*clingo_finalizer_t)(void *data);

//! Add a function to be called when the given control object is destroyed.
//!
//! Finalizers are called in reverse order of registration after all objects
//! registered with the control object (e.g., propagators) have been released
//! and before the library associated with the control object is released.
//! Language bindings can use this to tie the lifetime of objects to the
//! lifetime of the control object.
//!
//! @param[in] control the target
//! @param[in] finalizer the function to call
//! @param[in] data the user data passed to the finalizer
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_result_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_control_add_finalizer(clingo_control_t *control, clingo_finalizer_t finalizer,
                                                            void *data);

//! Remove a previously added finalizer from the given control object.
//!
//! @param[in] control the target
//! @param[in] finalizer the function to call
//! @param[in] data the user data passed to the finalizer
CLINGO_VISIBILITY_DEFAULT void clingo_control_remove_finalizer(clingo_control_t *control, clingo_finalizer_t finalizer,
                                                               void *data);

//! Callback to visit the finalizers of a control object.
//!
//! @param[in] finalizer the finalizer function
//! @param[in] finalizer_data the user data passed to the finalizer
//! @param[in] data the user data passed to clingo_control_visit_finalizers()
//! @return true to continue visiting and false to stop
typedef bool (*clingo_finalizer_visitor_t)(clingo_finalizer_t finalizer, void *finalizer_data, void *data);

//! Visit the finalizers added to the given control object.
//!
//! Language bindings can use this to inspect the objects kept alive by a
//! control object, e.g., to implement cycle detection in garbage collectors.
//!
//! @param[in] control the target
//! @param[in] visit the visitor
//! @param[in] data the user data passed to the visitor
CLINGO_VISIBILITY_DEFAULT bool clingo_control_visit_finalizers(clingo_control_t const *control,
                                                               clingo_finalizer_visitor_t visit, void *data);

//! Set or remove the owner of a control object registered under the given tag.
//!
//! Language bindings can use this to map control objects back to the objects
//! wrapping them. The owner is a borrowed pointer, which must be removed
//! (by passing NULL) before it becomes invalid. Each binding should use its
//! own tag, e.g., the address of a static variable.
//!
//! @param[in] control the target
//! @param[in] tag the tag identifying the binding
//! @param[in] owner the owner or NULL to remove it
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_result_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_control_set_owner(clingo_control_t *control, void const *tag, void *owner);

//! Get the owner of a control object registered under the given tag.
//!
//! @param[in] control the target
//! @param[in] tag the tag identifying the binding
//! @return the owner or NULL if there is none
CLINGO_VISIBILITY_DEFAULT void *clingo_control_get_owner(clingo_control_t const *control, void const *tag);

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
//! @see clingo_control_start_ground()
//!
//! @param[in] control the target
//! @param[in] parts array of parts to ground
//! @param[in] size size of the parts array
//! @param[in] handler handler for grounding events
//! @param[in] data user data for ground_callback
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_ground(clingo_control_t *control, clingo_part_t const *parts, size_t size,
                                                     clingo_ground_event_handler_t const *handler, void *data);

//! Start grounding a program asynchronously.
//!
//! The function returns a handle and immediately starts grounding in the
//! background. The handle can be used to wait for the grounding to finish, get
//! the result, or cancel the grounding. Stopping grounding leaves the program
//! in an unpredictable state. There is currently no way to rollback a
//! partially grounded program.
//!
//! The returned handle must be released using ::clingo_ground_handle_close().
//!
//! @see clingo_control_ground()
//!
//! @param[in] control the target
//! @param[in] parts array of parts to ground
//! @param[in] size size of the parts array
//! @param[in] handler handler for grounding events
//! @param[in] data user data for ground_callback
//! @param[out] handle the resulting ground handle
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_start_ground(clingo_control_t *control, clingo_part_t const *parts,
                                                           size_t size, clingo_ground_event_handler_t const *handler,
                                                           void *data, clingo_ground_handle_t **handle);

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
//! The buffer is only available for standalone control objects. Buffers of
//! control objects associated with application objects are regularly flushed
//! to stdout.
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
