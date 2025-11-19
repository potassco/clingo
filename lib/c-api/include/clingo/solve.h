#ifndef CLINGO_SOLVE_H
#define CLINGO_SOLVE_H

#include <clingo/core.h>
#include <clingo/model.h>
#include <clingo/stats.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clingo_control clingo_control_t;

//! @example solve.c
//! The example shows how to solve in the background.
//!
//! ## Output (approximately) ##
//!
//! ~~~~~~~~~~~~
//! ./solve 0
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

//! @addtogroup c_solve Solving
//! Interact with a running search.
//!
//! ::clingo_solve_handle_t objects can be used for both synchronous and asynchronous search,
//! as well as iteratively receiving models and solve results.
//!
//! For an example showing how to solve asynchronously, see @ref solve.c.
//!
//! @{

//! @enum clingo_solve_result_e
//! Enumeration of bit masks for solve call results.
//!
//! @note Neither ::clingo_solve_result_satisfiable nor
//! ::clingo_solve_result_exhausted is set if the search is interrupted and no
//! model was found.
//!
//! @see clingo_control_interrupt()
enum clingo_solve_result_e {
    clingo_solve_result_empty = 0,         //!< An uninitialized solve result.
    clingo_solve_result_satisfiable = 1,   //!< The last solve call found a solution.
    clingo_solve_result_unsatisfiable = 2, //!< The last solve call did not find a solution.
    clingo_solve_result_exhausted = 4,     //!< The last solve call completely exhausted the search space.
    clingo_solve_result_interrupted = 8    //!< The last solve call was interrupted.
};
//! Corresponding type to ::clingo_solve_result_e.
typedef unsigned clingo_solve_result_bitset_t;

//! Enumeration of solve modes.
enum clingo_solve_mode_e {
    clingo_solve_mode_default = 0, //!< The defalut solve mode.
    clingo_solve_mode_async = 1,   //!< Enable non-blocking search.
    clingo_solve_mode_yield = 2,   //!< Yield models in calls to clingo_solve_handle_model.
    clingo_solve_mode_lock = 4,    //!< Ensure callbacks are executed in lock-step.
};
//! Corresponding type to ::clingo_solve_mode_e.
typedef unsigned clingo_solve_mode_bitset_t;

//! The solve event handler interface.
typedef struct clingo_solve_event_handler {
    //! Call back to intercept models.
    //!
    //! @param model the model
    //! @param data the user data
    //! @param goon whether to stop or continue
    //! @return whether the call was successful
    bool (*model)(clingo_model_t *model, void *data, bool *goon);
    //! Callback to interept lower bounds.
    //!
    //! @param values the values of the current lower bound
    //! @param size the number of values in the lower bound
    //! @param data the user data
    //! @return whether the call was successful
    bool (*unsat)(int64_t const *values, size_t size, void *data);
    //! Callback to extend statistics.
    //!
    //! @param stats the stats object
    //! @param data the user data
    //! @return whether the call was successful
    bool (*stats)(clingo_stats_t *stats, void *data);
    //! Callback to notify that the search has finished.
    //!
    //! This can be used for thread synchronization as the function is not
    //! called in the main thread when solving asynchronously.
    //!
    //! @param result the solve result
    //! @param data the user data
    void (*finish)(clingo_solve_result_bitset_t result, void *data);
    //! Callback to free the userdata of the handler.
    //!
    //! @param data the user data
    void (*free)(void *data);
} clingo_solve_event_handler_t;

//! Search handle to a solve call.
//!
//! @see clingo_control_solve()
typedef struct clingo_solve_handle clingo_solve_handle_t;

//! Get the next solve result.
//!
//! Blocks until the result is ready.
//! When yielding partial solve results can be obtained, i.e.,
//! when a model is ready, the result will be satisfiable but neither the search exhausted nor the optimality proven.
//!
//! @param[in] handle the target
//! @param[out] result the solve result
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_handle_get(clingo_solve_handle_t *handle,
                                                       clingo_solve_result_bitset_t *result);
//! Wait for the specified amount of time to check if the next result is ready.
//!
//! If the time is set to zero, this function can be used to poll if the search is still active.
//! If the time is negative, the function blocks until the search is finished.
//!
//! @param[in] handle the target
//! @param[in] timeout the maximum time to wait
//! @param[out] result whether the search has finished
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_handle_wait(clingo_solve_handle_t *handle, double timeout, bool *result);
//! Get the next model (or zero if there are no more models).
//!
//! @param[in] handle the target
//! @param[out] model the model (it is NULL if there are no more models)
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_handle_model(clingo_solve_handle_t *handle, clingo_model_t const **model);
//! When a problem is unsatisfiable, get a subset of the assumptions that made the problem unsatisfiable.
//!
//! If the program is not unsatisfiable, an empty core is returned.
//!
//! @param[in] handle the target
//! @param[out] literals array of literals in the core
//! @param[out] size the size of the core
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_handle_core(clingo_solve_handle_t *handle,
                                                        clingo_literal_t const **literals, size_t *size);
//! When a problem is satisfiable and the search is finished, get the last
//! computed model.
//!
//! If the program is unsatisfiable or the search is not finished, model is set to NULL.
//!
//! @param[in] handle the target
//! @param[out] model the last computed model (or NULL if the program is unsatisfiable or the search is still ongoing)
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_handle_last(clingo_solve_handle_t *handle, clingo_model_t const **model);
//! Discards the last model and starts the search for the next one.
//!
//! If the search has been started asynchronously, this function continues the
//! search in the background.
//!
//! @note This function does not block.
//!
//! @param[in] handle the target
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_handle_resume(clingo_solve_handle_t *handle);
//! Stop the running search and block until done.
//!
//! @param[in] handle the target
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_handle_cancel(clingo_solve_handle_t *handle);
//! Stops the running search and releases the handle.
//!
//! Blocks until the search is stopped (as if an implicit cancel was called before the handle is released).
//!
//! @param[in] handle the target
CLINGO_VISIBILITY_DEFAULT void clingo_solve_handle_close(clingo_solve_handle_t *handle);

//! Solve the currently grounded logic program enumerating its models.
//!
//! See the @ref c_solve module for more information.
//!
//! @param[in] control the target
//! @param[in] mode configures the search mode
//! @param[in] assumptions array of assumptions to solve under
//! @param[in] assumptions_size number of assumptions
//! @param[in] handler the event handler to register
//! @param[in] data the user data for the event handler
//! @param[out] handle handle to the current search to enumerate models
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_solve(clingo_control_t *control, clingo_solve_mode_bitset_t mode,
                                                    clingo_literal_t const *assumptions, size_t assumptions_size,
                                                    clingo_solve_event_handler_t const *handler, void *data,
                                                    clingo_solve_handle_t **handle);

//! @}

#ifdef __cplusplus
}
#endif

#endif
