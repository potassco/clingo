#ifndef CLINGO_PROPAGATE_H
#define CLINGO_PROPAGATE_H

#include <clingo/base.h>
#include <clingo/core.h>
#include <clingo/shared.h>
#include <clingo/symbol.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clingo_control clingo_control_t;

//! @example propagate.c
//! The example shows how to write a simple propagator for the pigeonhole problem. For
//! a detailed description of what is implemented here and some background, take a look at the following paper:
//!
//! https://www.cs.uni-potsdam.de/wv/publications/#DBLP:conf/iclp/GebserKKOSW16x
//!
//! ## Output ##
//!
//! The output is empty because the pigeonhole problem is unsatisfiable.
//!
//! ## Code ##

//! @addtogroup c_propagate
//! Extend the search with propagators for arbitrary theories.
//!
//! For an example, see @ref propagate.c.
//! @{

//! Represents a (partial) assignment of a particular solver.
//!
//! An assignment assigns truth values to a set of literals. A literal is
//! assigned to either @link clingo_assignment_truth_value() true or false, or
//! is unassigned@endlink. Furthermore, each assigned literal is associated
//! with a @link clingo_assignment_level() decision level@endlink. There is
//! exactly one @link clingo_assignment_decision() decision literal@endlink for
//! each decision level greater than zero. Assignments to all other literals on
//! the same level are consequences implied by the current and possibly
//! previous decisions. Assignments on level zero are immediate consequences of
//! the current program. Decision levels are consecutive numbers starting with
//! zero up to and including the @link clingo_assignment_decision_level()
//! current decision level@endlink.
typedef struct clingo_assignment clingo_assignment_t;

//! Represents three-valued truth values.
enum clingo_truth_value_e {
    clingo_truth_value_free = 0, //!< no truth value
    clingo_truth_value_true = 1, //!< true
    clingo_truth_value_false = 2 //!< false
};
//! Corresponding type to ::clingo_truth_value_e.
typedef int clingo_truth_value_t;

//! @name Assignment Functions
//! @{

//! Get the current decision level.
//!
//! @param[in] assignment the target assignment
//! @param[out] level the decision level
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_decision_level(clingo_assignment_t const *assignment, uint32_t *level);
//! Get the current root level.
//!
//! Decisions levels smaller or equal to the root level are not backtracked during solving.
//!
//! @param[in] assignment the target assignment
//! @param[out] level the decision level
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_root_level(clingo_assignment_t const *assignment, uint32_t *level);
//! Check if the given assignment is conflicting.
//!
//! @param[in] assignment the target assignment
//! @param[out] is_conflicting whether the assignment is conflicting
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_has_conflict(clingo_assignment_t const *assignment,
                                                              bool *is_conflicting);
//! Check if the given literal is part of a (partial) assignment.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @param[out] is_valid whether the literal is valid
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_has_literal(clingo_assignment_t const *assignment,
                                                             clingo_literal_t literal, bool *is_valid);
//! Determine the decision level of a given literal.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @param[out] level the resulting level
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_level(clingo_assignment_t const *assignment, clingo_literal_t literal,
                                                       uint32_t *level);
//! Determine the decision literal given a decision level.
//!
//! @param[in] assignment the target assignment
//! @param[in] level the level
//! @param[out] literal the resulting literal
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_decision(clingo_assignment_t const *assignment, uint32_t level,
                                                          clingo_literal_t *literal);
//! Check if a literal has a fixed truth value.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @param[out] is_fixed whether the literal is fixed
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_is_fixed(clingo_assignment_t const *assignment,
                                                          clingo_literal_t literal, bool *is_fixed);
//! Check if a literal is true.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @param[out] is_true whether the literal is true
//! @return wether the call was successful
//! @see clingo_assignment_truth_value()
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_is_true(clingo_assignment_t const *assignment,
                                                         clingo_literal_t literal, bool *is_true);
//! Check if a literal has a fixed truth value.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @param[out] is_false whether the literal is false
//! @return wether the call was successful
//! @see clingo_assignment_truth_value()
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_is_false(clingo_assignment_t const *assignment,
                                                          clingo_literal_t literal, bool *is_false);
//! Determine the truth value of a given literal.
//!
//! @param[in] assignment the target assignment
//! @param[in] literal the literal
//! @param[out] value the resulting truth value
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_truth_value(clingo_assignment_t const *assignment,
                                                             clingo_literal_t literal, clingo_truth_value_t *value);
//! The number of (positive) literals in the assignment.
//!
//! @param[in] assignment the target
//! @param[out] size the number of literals
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_size(clingo_assignment_t const *assignment, size_t *size);
//! The (positive) literal at the given offset in the assignment.
//!
//! @param[in] assignment the target
//! @param[in] offset the offset of the literal
//! @param[out] literal the literal
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_at(clingo_assignment_t const *assignment, size_t offset,
                                                    clingo_literal_t *literal);
//! Check if the assignment is total, i.e. there are no free literal.
//!
//! @param[in] assignment the target
//! @param[out] is_total whether the assignment is total
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_is_total(clingo_assignment_t const *assignment, bool *is_total);
//! Returns the number of literals in the trail, i.e., the number of assigned literals.
//!
//! @param[in] assignment the target
//! @param[out] size the number of literals in the trail
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_trail_size(clingo_assignment_t const *assignment, uint32_t *size);
//! Returns the offset of the decision literal with the given decision level in
//! the trail.
//!
//! @note Literals in the trail are ordered by decision levels, where the first
//! literal with a larger level than the previous literals is a decision; the
//! following literals with same level are implied by this decision literal.
//! Each decision level up to and including the current decision level has a
//! valid offset in the trail.
//!
//! @param[in] assignment the target
//! @param[in] level the decision level
//! @param[out] offset the offset of the decision literal
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_trail_begin(clingo_assignment_t const *assignment, uint32_t level,
                                                             uint32_t *offset);
//! Returns the offset following the last literal with the given decision level.
//!
//! @note This function is the counterpart to clingo_assignment_trail_begin().
//!
//! @param[in] assignment the target
//! @param[in] level the decision level
//! @param[out] offset the offset
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_trail_end(clingo_assignment_t const *assignment, uint32_t level,
                                                           uint32_t *offset);
//! Returns the literal at the given position in the trail.
//!
//! @param[in] assignment the target
//! @param[in] offset the offset of the literal
//! @param[out] literal the literal
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_assignment_trail_at(clingo_assignment_t const *assignment, uint32_t offset,
                                                          clingo_literal_t *literal);

//! @}

//! Supported check modes for propagators.
//!
//! Note that total checks are subject to the lock when a model is found.
//! This means that information from previously found models can be used to discard assignments in check calls.
enum clingo_propagator_check_mode_e {
    clingo_propagator_check_mode_none = 0,     //!< do not call @ref ::clingo_propagator::check() at all
    clingo_propagator_check_mode_total = 1,    //!< call @ref ::clingo_propagator::check() on total assignments
    clingo_propagator_check_mode_fixpoint = 2, //!< call @ref ::clingo_propagator::check() on propagation fixpoints
    clingo_propagator_check_mode_both =
        3, //!< call @ref ::clingo_propagator::check() on propagation fixpoints and total assignments
};
//! Corresponding type to ::clingo_propagator_check_mode_e.
typedef int clingo_propagator_check_mode_t;

//! Undo modes for propagators.
enum clingo_propagator_undo_mode_e {
    clingo_propagator_undo_mode_default = 0, //!< call @ref ::clingo_propagator::undo() for non-empty change lists
    clingo_propagator_undo_mode_always = 1,  //!< also call @ref ::clingo_propagator::check() when check has been called
};
//! Corresponding type to ::clingo_propagator_undo_mode_e.
typedef int clingo_propagator_undo_mode_t;

//! Enumeration of weight_constraint_types.
enum clingo_weight_constraint_type_e {
    clingo_weight_constraint_type_implication_left = -1, //!< the weight constraint implies the literal
    clingo_weight_constraint_type_implication_right = 1, //!< the literal implies the weight constraint
    clingo_weight_constraint_type_equivalence = 0,       //!< the weight constraint is equivalent to the literal
};
//! Corresponding type to ::clingo_weight_constraint_type_e.
typedef int clingo_weight_constraint_type_t;

//! Object to initialize a user-defined propagator before each solving step.
//!
//! Each @link c_base symbolic or theory atom@endlink is uniquely associated with an
//! aspif atom in form of a positive integer (@ref ::clingo_literal_t). Aspif literals additionally are signed to
//! represent default negation. Furthermore, there are non-zero integer solver literals (also represented using @ref
//! ::clingo_literal_t). There is a surjective mapping from program atoms to solver literals.
//!
//! All methods called during propagation use solver literals whereas clingo_symbolic_atoms_literal() and
//! clingo_theory_atoms_atom_literal() return program literals. The function clingo_propagate_init_solver_literal() can
//! be used to map program literals or @link clingo_theory_base_element_condition_id() condition ids@endlink to solver
//! literals.
typedef struct clingo_propagate_init clingo_propagate_init_t;

//! @name Initialization Functions
//! @{

//! Map the given program literal or condition id to its solver literal.
//!
//! @param[in] init the target
//! @param[in] aspif_literal the aspif literal to map
//! @param[out] solver_literal the resulting solver literal
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_solver_literal(clingo_propagate_init_t const *init,
                                                                    clingo_literal_t aspif_literal,
                                                                    clingo_literal_t *solver_literal);
//! Add a watch for the solver literal in the given phase.
//!
//! @param[in] init the target
//! @param[in] solver_literal the solver literal
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_add_watch(clingo_propagate_init_t *init,
                                                               clingo_literal_t solver_literal);
//! Add a watch for the solver literal in the given phase to the given solver thread.
//!
//! @param[in] init the target
//! @param[in] solver_literal the solver literal
//! @param[in] thread_id the id of the solver thread
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_add_watch_to_thread(clingo_propagate_init_t *init,
                                                                         clingo_literal_t solver_literal,
                                                                         clingo_id_t thread_id);
//! Remove the watch for the solver literal in the given phase.
//!
//! @param[in] init the target
//! @param[in] solver_literal the solver literal
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_remove_watch(clingo_propagate_init_t *init,
                                                                  clingo_literal_t solver_literal);
//! Remove the watch for the solver literal in the given phase from the given solver thread.
//!
//! @param[in] init the target
//! @param[in] solver_literal the solver literal
//! @param[in] thread_id the id of the solver thread
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_remove_watch_from_thread(clingo_propagate_init_t *init,
                                                                              clingo_literal_t solver_literal,
                                                                              uint32_t thread_id);
//! Freeze the given solver literal.
//!
//! Any solver literal that is not frozen is subject to simplification and might be removed in a preprocessing step
//! after propagator initialization. A propagator should freeze all literals over which it might add clauses during
//! propagation. Note that any watched literal is automatically frozen and that it does not matter which phase of the
//! literal is frozen.
//!
//! @param[in] init the target
//! @param[in] solver_literal the solver literal
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_freeze_literal(clingo_propagate_init_t *init,
                                                                    clingo_literal_t solver_literal);
//! Get the underlying library object.
//!
//! @param[in] init the target
//! @param[out] lib the resulting object
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_library(clingo_propagate_init_t const *init, clingo_lib_t **lib);
//! Get an object to inspect the base.
//!
//! @param[in] init the target
//! @param[out] base the resulting object
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_base(clingo_propagate_init_t const *init,
                                                          clingo_base_t const **base);
//! Get the number of threads used in subsequent solving.
//!
//! @param[in] init the target
//! @param[out] threads the number of threads
//! @return wether the call was successful
//! @see clingo_propagate_control_thread_id()
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_number_of_threads(clingo_propagate_init_t const *init,
                                                                       clingo_id_t *threads);
//! Configure when to call the check method of the propagator.
//!
//! @param[in] init the target
//! @param[in] mode bitmask when to call the propagator
//! @return wether the call was successful
//! @see @ref ::clingo_propagator::check()
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_set_check_mode(clingo_propagate_init_t *init,
                                                                    clingo_propagator_check_mode_t mode);
//! Get the current check mode of the propagator.
//!
//! @param[in] init the target
//! @param[out] mode the rersulting mode
//! @return wether the call was successful
//! @see clingo_propagate_init_set_check_mode()
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_get_check_mode(clingo_propagate_init_t const *init,
                                                                    clingo_propagator_check_mode_t *mode);
//! Configure when to call the undo method of the propagator.
//!
//! @param[in] init the target
//! @param[in] mode when to call the propagator
//! @return wether the call was successful
//! @see @ref ::clingo_propagator::check()
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_set_undo_mode(clingo_propagate_init_t *init,
                                                                   clingo_propagator_undo_mode_t mode);
//! Get the current undo mode of the propagator.
//!
//! @param[in] init the target
//! @param[out] mode the resulting mode
//! @return wether the call was successful
//! @see clingo_propagate_init_set_undo_mode()
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_get_undo_mode(clingo_propagate_init_t const *init,
                                                                   clingo_propagator_undo_mode_t *mode);
//! Get the top level assignment solver.
//!
//! @param[in] init the target
//! @param[out] assignment the resulting assignment
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_assignment(clingo_propagate_init_t const *init,
                                                                clingo_assignment_t const **assignment);
//! Add a literal to the solver.
//!
//! To be able to use the variable in clauses during propagation or add watches to it, it has to be frozen.
//! Otherwise, it might be removed during preprocessing.
//!
//! @attention If variables were added, subsequent calls to functions adding constraints or
//! ::clingo_propagate_init_propagate() are expensive. It is best to add variables in batches.
//!
//! @param[in] init the target
//! @param[in] freeze whether to freeze the literal
//! @param[out] solver_literal the added literal
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_add_literal(clingo_propagate_init_t *init, bool freeze,
                                                                 clingo_literal_t *solver_literal);
//! Add the given clause to the solver.
//!
//! @attention No further calls on the init object or functions on the assignment should be called when the result of
//! this method is false.
//!
//! @param[in] init the target
//! @param[in] literals the clause to add
//! @param[in] size the size of the clause
//! @param[out] result result indicating whether the problem became unsatisfiable
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_add_clause(clingo_propagate_init_t *init,
                                                                clingo_literal_t const *literals, size_t size,
                                                                bool *result);
//! Add the given weight constraint to the solver.
//!
//! This function adds a constraint of form `literal <=> { lit=weight | (lit, weight) in literals } >= bound` to the
//! solver. Depending on the type the `<=>` connective can be either a left implication, right implication, or
//! equivalence.
//!
//! @attention No further calls on the init object or functions on the assignment should be called when the result of
//! this method is false.
//!
//! @param[in] init the target
//! @param[in] solver_literal the literal of the constraint
//! @param[in] literals the weighted literals
//! @param[in] size the number of weighted literals
//! @param[in] bound the bound of the constraint
//! @param[in] type the type of the weight constraint
//! @param[in] compare_equal if true compare equal instead of less than equal
//! @param[out] result result indicating whether the problem became unsatisfiable
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_add_weight_constraint(
    clingo_propagate_init_t *init, clingo_literal_t solver_literal, clingo_weighted_literal_t const *literals,
    size_t size, clingo_weight_t bound, clingo_weight_constraint_type_t type, bool compare_equal, bool *result);
//! Add the given literal to minimize to the solver.
//!
//! This corresponds to a weak constraint of form `:~ literal. [weight@priority]`.
//!
//! @param[in] init the target
//! @param[in] solver_literal the literal to minimize
//! @param[in] weight the weight of the literal
//! @param[in] priority the priority of the literal
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_add_minimize(clingo_propagate_init_t *init,
                                                                  clingo_literal_t solver_literal,
                                                                  clingo_weight_t weight, clingo_weight_t priority);
//! Propagates consequences of the underlying problem excluding registered propagators.
//!
//! @note The function has no effect if SAT-preprocessing is enabled.
//! @attention No further calls on the init object or functions on the assignment should be called when the result of
//! this method is false.
//!
//! @param[in] init the target
//! @param[out] result result indicating whether the problem became unsatisfiable
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_init_propagate(clingo_propagate_init_t *init, bool *result);

//! @}

//! Enumeration of clause types determining the lifetime of a clause.
//!
//! Clauses in the solver are either cleaned up based on a configurable deletion policy or at the end of a solving step.
//! The values of this enumeration determine if a clause is subject to one of the above deletion strategies.
enum clingo_clause_type_e {
    clingo_clause_type_learnt = 0, //!< clause is subject to the solvers deletion policy
    clingo_clause_type_static = 1, //!< clause is not subject to the solvers deletion policy
    clingo_clause_type_volatile =
        2, //!< like ::clingo_clause_type_learnt but the clause is deleted after a solving step
    clingo_clause_type_volatile_static =
        3 //!< like ::clingo_clause_type_static but the clause is deleted after a solving step
};
//! Corresponding type to ::clingo_clause_type_e.
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
//! @param[out] thread_id the thread id
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_thread_id(clingo_propagate_control_t const *control,
                                                                  clingo_id_t *thread_id);
//! Get the assignment associated with the underlying solver.
//!
//! @param[in] control the target
//! @param[out] assignment the resulting assignment
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_assignment(clingo_propagate_control_t const *control,
                                                                   clingo_assignment_t const **assignment);
//! Adds a new volatile literal to the underlying solver thread.
//!
//! @attention The literal is only valid within the current solving step and solver thread.
//! All volatile literals and clauses involving a volatile literal are deleted after the current search.
//!
//! @param[in] control the target
//! @param[out] result the (positive) solver literal
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_add_literal(clingo_propagate_control_t *control,
                                                                    clingo_literal_t *result);
//! Add a watch for the solver literal in the given phase.
//!
//! @note Unlike @ref clingo_propagate_init_add_watch() this does not add a watch to all solver threads but just the
//! current one.
//!
//! @param[in] control the target
//! @param[in] literal the literal to watch
//! @return wether the call was successful
//! @see clingo_propagate_control_remove_watch()
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_add_watch(clingo_propagate_control_t *control,
                                                                  clingo_literal_t literal);
//! Check whether a literal is watched in the current solver thread.
//!
//! @param[in] control the target
//! @param[in] literal the literal to check
//! @param[out] has_watch whether the literal is watched
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_has_watch(clingo_propagate_control_t const *control,
                                                                  clingo_literal_t literal, bool *has_watch);
//! Removes the watch (if any) for the given solver literal.
//!
//! @note Similar to @ref clingo_propagate_init_add_watch() this just removes the watch in the current solver thread.
//!
//! @param[in] control the target
//! @param[in] literal the literal to remove
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_remove_watch(clingo_propagate_control_t *control,
                                                                     clingo_literal_t literal);
//! Add the given clause to the solver.
//!
//! This method sets its result to false if the current propagation must be stopped for the solver to backtrack.
//!
//! @attention No further calls on the control object or functions on the assignment should be called when the result of
//! this method is false.
//!
//! @param[in] control the target
//! @param[in] literals the literals of the clause
//! @param[in] size the size of the clause
//! @param[in] type the clause type determining its lifetime
//! @param[out] result result indicating whether propagation has to be stopped
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_add_clause(clingo_propagate_control_t *control,
                                                                   clingo_literal_t const *literals, size_t size,
                                                                   clingo_clause_type_t type, bool *result);
//! Propagate implied literals (resulting from added clauses).
//!
//! This method sets its result to false if the current propagation must be stopped for the solver to backtrack.
//!
//! @attention No further calls on the control object or functions on the assignment should be called when the result of
//! this method is false.
//!
//! @param[in] control the target
//! @param[out] result result indicating whether propagation has to be stopped
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_propagate_control_propagate(clingo_propagate_control_t *control, bool *result);

//! @}

//! Typedef for @ref ::clingo_propagator::init().
typedef bool (*clingo_propagator_init_callback_t)(clingo_propagate_init_t *, void *);

//! Typedef for @ref ::clingo_propagator::propagate().
typedef bool (*clingo_propagator_propagate_callback_t)(clingo_propagate_control_t *, clingo_literal_t const *, size_t,
                                                       void *);

//! Typedef for @ref ::clingo_propagator::undo().
typedef void (*clingo_propagator_undo_callback_t)(clingo_propagate_control_t const *, clingo_literal_t const *, size_t,
                                                  void *);

//! Typedef for @ref ::clingo_propagator::check().
typedef bool (*clingo_propagator_check_callback_t)(clingo_propagate_control_t *, void *);

//! An instance of this struct has to be registered with a solver to implement a custom propagator.
//!
//! Not all callbacks have to be implemented and can be set to NULL if not needed.
//! @see Propagator
typedef struct clingo_propagator {
    //! This function is called once before each solving step.
    //! It is used to map relevant program literals to solver literals, add watches for solver literals, and initialize
    //! the data structures used during propagation.
    //!
    //! @note This is the last point to access symbolic and theory atoms.
    //! Once the search has started, they are no longer accessible.
    //!
    //! @param[in] init initialization object
    //! @param[in] data user data for the callback
    //! @return wether the call was successful
    //! @see ::clingo_propagator_init_callback_t
    bool (*init)(clingo_propagate_init_t *init, void *data);
    //! Can be used to propagate solver literals given a @link clingo_assignment_t partial assignment@endlink.
    //!
    //! Called during propagation with a non-empty array of @link clingo_propagate_init_add_watch() watched solver
    //! literals@endlink that have been assigned to true since the last call to either propagate, undo, (or the start of
    //! the search) - the change set. Only watched solver literals are contained in the change set. Each literal in the
    //! change set is true w.r.t. the current @link clingo_assignment_t assignment@endlink.
    //! @ref clingo_propagate_control_add_clause() can be used to add clauses.
    //! If a clause is unit resulting, it can be propagated using @ref clingo_propagate_control_propagate().
    //! If the result of either of the two methods is false, the propagate function must return immediately.
    //!
    //! The following snippet shows how to use the methods to add clauses and propagate consequences within the
    //! callback. The important point is to return true (true to indicate there was no error) if the result of either of
    //! the methods is false.
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
    //! Each thread has its own assignment and id, which can be obtained using @ref
    //! clingo_propagate_control_thread_id().
    //!
    //! @param[in] control control object for the target solver
    //! @param[in] changes the change set
    //! @param[in] size the size of the change set
    //! @param[in] data user data for the callback
    //! @return wether the call was successful
    //! @see ::clingo_propagator_propagate_callback_t
    bool (*propagate)(clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, void *data);
    //! Called whenever a solver undoes assignments to watched solver literals.
    //!
    //! This callback is meant to update assignment dependent state in the propagator.
    //!
    //! @note No clauses must be propagated in this callback and no errors should be set.
    //!
    //! @param[in] control control object for the target solver
    //! @param[in] changes the change set
    //! @param[in] size the size of the change set
    //! @param[in] data user data for the callback
    //! @return wether the call was successful
    //! @see ::clingo_propagator_undo_callback_t
    void (*undo)(clingo_propagate_control_t const *control, clingo_literal_t const *changes, size_t size, void *data);
    //! This function is similar to @ref clingo_propagate_control_propagate() but is called without a change set on
    //! propagation fixpoints.
    //!
    //! When exactly this function is called, can be configured using the @ref clingo_propagate_init_set_check_mode()
    //! function.
    //!
    //! @note This function is called even if no watches have been added.
    //!
    //! @param[in] control control object for the target solver
    //! @param[in] data user data for the callback
    //! @return wether the call was successful
    //! @see ::clingo_propagator_check_callback_t
    bool (*check)(clingo_propagate_control_t *control, void *data);
    //! This function allows a propagator to implement domain-specific heuristics.
    //!
    //! It is called whenever propagation reaches a fixed point and
    //! should return a free solver literal that is to be assigned true.
    //! In case multiple propagators are registered,
    //! this function can return 0 to let a propagator registered later make a decision.
    //! If all propagators return 0, then the fallback literal is
    //!
    //! @param[in] thread_id the solver's thread id
    //! @param[in] assignment the assignment of the solver
    //! @param[in] fallback the literal chosen by the solver's heuristic
    //! @param[in] data user data for the callback
    //! @param[out] decision the literal to make true
    //! @return wether the call was successful
    bool (*decide)(clingo_id_t thread_id, clingo_assignment_t const *assignment, clingo_literal_t fallback, void *data,
                   clingo_literal_t *decision);
    //! Free the propagator.
    //! @param[in] data user data for the callback
    void (*free)(void *data);
} clingo_propagator_t;

//! Register a custom propagator with the control object.
//!
//! @param[in] control the target
//! @param[in] propagator the propagator
//! @param[in] data user data passed to the propagator functions
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_register_propagator(clingo_control_t *control,
                                                                  clingo_propagator_t const *propagator, void *data);
//! @}

#ifdef __cplusplus
}
#endif

#endif
