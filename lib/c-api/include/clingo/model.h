#ifndef CLINGO_MODEL_H
#define CLINGO_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/base.h>
#include <clingo/core.h>
#include <clingo/symbol.h>

// NOLINTBEGIN(modernize-*,cppcoreguidelines-macro-usage,performance-enum-size)

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

//! @addtogroup c_model
//! Inspection of models and a high-level interface to add constraints during solving.
//!
//! For an example, see @ref model.c.
//! @{

//! Object to add clauses during search.
typedef struct clingo_solve_control clingo_solve_control_t;

//! Object representing a model.
typedef struct clingo_model clingo_model_t;

//! Enumeration for the different model types.
enum clingo_model_type_e {
    clingo_model_type_stable_model = 0,         //!< The model represents a stable model.
    clingo_model_type_brave_consequences = 1,   //!< The model represents a set of brave consequences.
    clingo_model_type_cautious_consequences = 2 //!< The model represents a set of cautious consequences.
};
//! Corresponding type to ::clingo_model_type_e.
typedef int clingo_model_type_t;

//! Enumeration of bit flags to select symbols in models.
enum clingo_show_type_e {
    clingo_show_type_shown = 2,   //!< Select shown atoms and terms.
    clingo_show_type_atoms = 4,   //!< Select all atoms.
    clingo_show_type_terms = 8,   //!< Select all terms.
    clingo_show_type_theory = 16, //!< Select symbols added by theory.
    clingo_show_type_all = 31,    //!< Select everything.
    clingo_show_type_complement =
        32 //!< Select false instead of true atoms (::clingo_show_type_atoms) or terms (::clingo_show_type_terms).
};
//! Corresponding type to ::clingo_show_type_e.
typedef unsigned clingo_show_type_bitset_t;

//! Enumeration for the different consequence types.
enum clingo_consequence_e {
    clingo_consequence_false = 0,   //!< The literal is not a consequence.
    clingo_consequence_true = 1,    //!< The literal is a consequence.
    clingo_consequence_unknown = 2, //!< The literal might or might not be a consequence.
};
typedef int clingo_consequence_t;

//! Corresponding type to ::clingo_model_type_e.

//! @name Functions for Inspecting Models
//! @{

//! Get the type of the model.
//!
//! @param[in] model the target
//! @param[out] type the type of the model
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_type(clingo_model_t const *model, clingo_model_type_t *type);
//! Get the running number of the model.
//!
//! @param[in] model the target
//! @param[out] number the number of the model
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_number(clingo_model_t const *model, uint64_t *number);
//! Get the number of symbols of the selected types in the model.
//!
//! @param[in] model the target
//! @param[in] show which symbols to select
//! @param[out] size the number symbols
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
CLINGO_VISIBILITY_DEFAULT bool clingo_model_symbols_size(clingo_model_t const *model, clingo_show_type_bitset_t show,
                                                         size_t *size);
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
CLINGO_VISIBILITY_DEFAULT bool clingo_model_symbols(clingo_model_t const *model, clingo_show_type_bitset_t show,
                                                    clingo_symbol_t *symbols, size_t size);
//! Constant time lookup to test whether an atom is in a model.
//!
//! @param[in] model the target
//! @param[in] atom the atom to lookup
//! @param[out] contained whether the atom is contained
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_contains(clingo_model_t const *model, clingo_symbol_t atom,
                                                     bool *contained);
//! Check if a program literal is true in a model.
//!
//! @param[in] model the target
//! @param[in] literal the literal to lookup
//! @param[out] result whether the literal is true
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_is_true(clingo_model_t const *model, clingo_literal_t literal,
                                                    bool *result);
//! Check if the given literal is a consequence.
//!
//! While enumerating cautious or brave consequences, there is partial
//! information about which literals are consequences. The current state of a
//! literal can be requested using this function. If this function is used
//! during normal model enumeration, the function just returns whether a
//! literal is true of false in the current model.
//!
//! @param[in] model the target
//! @param[in] literal the literal to lookup
//! @param[out] result whether the literal is a consequence
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_is_consequence(clingo_model_t const *model, clingo_literal_t literal,
                                                           clingo_consequence_t *result);
//! Get the number of cost values of a model.
//!
//! @param[in] model the target
//! @param[out] size the number of costs
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_cost_size(clingo_model_t const *model, size_t *size);
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
CLINGO_VISIBILITY_DEFAULT bool clingo_model_cost(clingo_model_t const *model, int64_t *costs, size_t size);
//! Get the priorities of the costs.
//!
//! The size of the array can be obtained with clingo_model_cost_size().
//!
//! @param[in] model the target
//! @param[out] priorities the resulting priorities
//! @param[in] size the number of priorities
//! @return whether the call was successful; might set one of the following error codes:
//! - ::clingo_error_bad_alloc
//! - ::clingo_error_runtime if the size is too small
//!
//! @see clingo_model_cost_size()
CLINGO_VISIBILITY_DEFAULT bool clingo_model_priority(clingo_model_t const *model, clingo_weight_t *priorities,
                                                     size_t size);
//! Whether the optimality of a model has been proven.
//!
//! @param[in] model the target
//! @param[out] proven whether the optimality has been proven
//! @return whether the call was successful
//!
//! @see clingo_model_cost()
CLINGO_VISIBILITY_DEFAULT bool clingo_model_optimality_proven(clingo_model_t const *model, bool *proven);
//! Get the id of the solver thread that found the model.
//!
//! @param[in] model the target
//! @param[out] id the resulting thread id
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_thread_id(clingo_model_t const *model, clingo_id_t *id);
//! Add symbols to the model.
//!
//! These symbols will appear in clingo's output, which means that this
//! function is only meaningful if there is an underlying clingo application.
//! Only models passed to the ::clingo_solve_event_callback_t are extendable.
//!
//! @param[in] model the target
//! @param[in] symbols the symbols to add
//! @param[in] size the number of symbols to add
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_extend(clingo_model_t *model, clingo_symbol_t const *symbols, size_t size);
//! @}

//! @name Functions for Adding Clauses
//! @{

//! Get the associated solve control object of a model.
//!
//! This object allows for adding clauses during model enumeration.
//! @param[in] model the target
//! @param[out] control the resulting solve control object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_context(clingo_model_t const *model, clingo_solve_control_t **control);
//! Get an object to inspect the symbolic atoms.
//!
//! @param[in] control the target
//! @param[out] atoms the resulting object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_control_symbolic_atoms(clingo_solve_control_t const *control,
                                                                   clingo_symbolic_atoms_t const **atoms);
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
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_control_add_clause(clingo_solve_control_t *control,
                                                               clingo_literal_t const *clause, size_t size);
//! @}

//! @}

// NOLINTEND(modernize-*,cppcoreguidelines-macro-usage,performance-enum-size)

#ifdef __cplusplus
}
#endif

#endif
