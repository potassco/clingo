#ifndef CLINGO_GROUND_H
#define CLINGO_GROUND_H

#include <clingo/core.h>
#include <clingo/model.h>
#include <clingo/stats.h>

#ifdef __cplusplus
extern "C" {
#endif

//! @example ground.c
//! The example shows how to ground in the background.
//!
//! ## Output (approximately) ##
//!
//! ~~~~~~~~~~~~
//! ./ground
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @addtogroup c_ground Grounding
//! Interact with a running ground call.
//!
//! ::clingo_ground_handle_t objects can be used for asynchronous grounding.
//! They allow for stopping the grounding process and waiting for its completion.
//!
//! For an example showing how to ground asynchronously, see @ref ground.c.
//!
//! @{

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

//! @enum clingo_ground_result_e
//! Enumeration of ground call results.
enum clingo_ground_result_e {
    clingo_ground_result_ok = 0,            //!< Grounding completed successfully.
    clingo_ground_result_unsatisfiable = 1, //!< Grounding detected unsatisfiability.
    clingo_ground_result_interrupted = 2,   //!< Grounding was interrupted.
};
//! Corresponding type to ::clingo_ground_result_e.
typedef unsigned clingo_ground_result_t;

//! The ground event handler interface.
typedef struct clingo_ground_event_handler {
    //! Check if the function with the given name and number of arguments is callable.
    //!
    //! @param[in] name name of the called external function
    //! @param[in] name_size size of the name
    //! @param[in] arguments_size number of arguments
    //! @param[in] data user data of the callback
    //! @param[out] result whether the function is callable
    //! @return whether the call was successful
    bool (*callable)(char const *name, size_t name_size, size_t arguments_size, void *data, bool *result);
    //! Callback function to implement external functions.
    //!
    //! If an external function of form <tt>\@name(parameters)</tt> occurs in a
    //! logic program, then this function is called with its location, name,
    //! parameters, and a callback to inject symbols as arguments. The callback
    //! can be called multiple times; all symbols passed are injected.
    //!
    //! If a (non-recoverable) clingo API function fails in this callback, for
    //! example, the symbol callback, the callback must return its return code.
    //! In case of errors not related to clingo, this function can use
    //! clingo_set_error().
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
    //! call(clingo_lib_t *lib,
    //!      clingo_location_t const *location,
    //!      char const *name,
    //!      size_t name_size,
    //!      clingo_symbol_t const *arguments,
    //!      size_t arguments_size,
    //!      void *data,
    //!      clingo_symbol_callback_t symbol_callback,
    //!      void *symbol_callback_data) {
    //!   if (size == 1 && strncmp(name, "f", 1) == 0 && arguments_size == 0) {
    //!     clingo_symbol_t sym;
    //!     sym = clingo_symbol_create_number(42);
    //!     return symbol_callback(&sym, 1, symbol_callback_data);
    //!   }
    //!   return clingo_set_error(lib, clingo_result_runtime, "function not found", 18);
    //! }
    //! ~~~~~~~~~~~~~~~
    bool (*call)(clingo_lib_t *lib, clingo_location_t const *location, char const *name, size_t name_size,
                 clingo_symbol_t const *arguments, size_t arguments_size, void *data,
                 clingo_symbol_callback_t symbol_callback, void *symbol_callback_data);
    //! Callback to notify that the grounding has finished.
    //!
    //! This can be used for thread synchronization as the function is not
    //! called in the main thread when grounding asynchronously.
    //!
    //! @param result the ground result
    //! @param data the user data
    void (*finish)(clingo_ground_result_t result, void *data);
    //! Callback to free the userdata of the handler.
    //!
    //! @param data the user data
    void (*free)(void *data);
} clingo_ground_event_handler_t;

//! Handle to an asynchronous ground call.
//!
//! @see clingo_control_ground()
typedef struct clingo_ground_handle clingo_ground_handle_t;

//! Get the ground result.
//!
//! Blocks until the result is ready.
//! This function propagates errors encountered during grounding.
//!
//! @param[in] handle the target
//! @param[out] result the ground result
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ground_handle_get(clingo_ground_handle_t *handle, clingo_ground_result_t *result);
//! Wait for the specified amount of time to check if the result is ready.
//!
//! If the time is set to zero, this function can be used to poll if grounding
//! is still active. If the time is negative, the function blocks until the
//! grounding is finished.
//!
//! @param[in] handle the target
//! @param[in] timeout the maximum time to wait
//! @param[out] result whether the ground call has finished
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ground_handle_wait(clingo_ground_handle_t *handle, double timeout, bool *result);
//! Stop the running ground call and block until done.
//!
//! @param[in] handle the target
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_ground_handle_cancel(clingo_ground_handle_t *handle);
//! Release the ground handle.
//!
//! This internally calls `clingo_ground_handle_cancel()` if grounding has not
//! finished yet.
//!
//! @param[in] handle the target
CLINGO_VISIBILITY_DEFAULT void clingo_ground_handle_close(clingo_ground_handle_t *handle);

//! @}

#ifdef __cplusplus
}
#endif

#endif
