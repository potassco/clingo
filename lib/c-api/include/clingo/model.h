#ifndef CLINGO_MODEL_H
#define CLINGO_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/base.h>
#include <clingo/core.h>
#include <clingo/symbol.h>

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
    clingo_show_type_shown = 1,  //!< Select shown atoms and terms.
    clingo_show_type_atoms = 2,  //!< Select all atoms.
    clingo_show_type_terms = 4,  //!< Select all terms.
    clingo_show_type_theory = 8, //!< Select symbols added by theory.
    clingo_show_type_all = 15,   //!< Select everything.
};
//! Corresponding type to ::clingo_show_type_e.
typedef unsigned clingo_show_type_bitset_t;

//! Enumeration for the different consequence types.
enum clingo_consequence_e {
    clingo_consequence_false = 0,   //!< The literal is not a consequence.
    clingo_consequence_true = 1,    //!< The literal is a consequence.
    clingo_consequence_unknown = 2, //!< The literal might or might not be a consequence.
};
//! Corresponding type to ::clingo_model_type_e.
typedef int clingo_consequence_t;

//! @name Functions for Inspecting Models
//! @{

//! Get the type of the model.
//!
//! @param[in] model the target
//! @param[out] type the type of the model
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_type(clingo_model_t const *model, clingo_model_type_t *type);
//! Get the running number of the model.
//!
//! @param[in] model the target
//! @param[out] number the number of the model
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_number(clingo_model_t const *model, uint64_t *number);

//! Get the symbols of the selected types in the model.
//!
//! @param[in] model the target
//! @param[in] show which symbols to select
//! @param[out] callback callback to copy symbols
//! @param[in] data userdata for the callback
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_symbols(clingo_model_t const *model, clingo_show_type_bitset_t show,
                                                    clingo_symbol_callback_t callback, void *data);
//! Constant time lookup to test whether an atom is in a model.
//!
//! @param[in] model the target
//! @param[in] atom the atom to lookup
//! @param[out] contained whether the atom is contained
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_contains(clingo_model_t const *model, clingo_symbol_t atom,
                                                     bool *contained);
//! Check if a program literal is true in a model.
//!
//! @param[in] model the target
//! @param[in] literal the literal to lookup
//! @param[out] result whether the literal is true
//! @return wether the call was successful
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
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_is_consequence(clingo_model_t const *model, clingo_literal_t literal,
                                                           clingo_consequence_t *result);
//! Get the costs of a model.
//!
//! @attention The lifetime of the costs array is tied to the lifetime of the object.
//!
//! @param[in] model the target
//! @param[out] costs the resulting costs
//! @param[out] size the size of the costs array
//! @return wether the call was successful
//! @see clingo_model_optimality_proven()
CLINGO_VISIBILITY_DEFAULT bool clingo_model_cost(clingo_model_t const *model, int64_t const **costs, size_t *size);
//! Get the priorities of the costs.
//!
//! @attention The lifetime of the costs array is tied to the lifetime of the object.
//!
//! @param[in] model the target
//! @param[out] priorities the resulting priorities
//! @param[out] size the size of the priorities array
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_priority(clingo_model_t const *model, clingo_weight_t const **priorities,
                                                     size_t *size);
//! Whether the optimality of a model has been proven.
//!
//! @param[in] model the target
//! @param[out] proven whether the optimality has been proven
//! @return wether the call was successful
//! @see clingo_model_cost()
CLINGO_VISIBILITY_DEFAULT bool clingo_model_optimality_proven(clingo_model_t const *model, bool *proven);
//! Get the id of the solver thread that found the model.
//!
//! @param[in] model the target
//! @param[out] id the resulting thread id
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_thread_id(clingo_model_t const *model, clingo_id_t *id);
//! Add symbols to the model.
//!
//! These symbols will appear in clingo's output, which means that this
//! function is only meaningful if there is an underlying clingo application.
//! Only models passed to the ::clingo_solve_event_handler_t are extendable.
//!
//! @param[in] model the target
//! @param[in] symbols the symbols to add
//! @param[in] size the number of symbols to add
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_extend(clingo_model_t *model, clingo_symbol_t const *symbols, size_t size);
//! @}

//! @name Functions for Adding Clauses
//! @{

//! Get the associated solve control object of a model.
//!
//! This object allows for adding clauses during model enumeration.
//! @param[in] model the target
//! @param[out] control the resulting solve control object
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_model_control(clingo_model_t *model, clingo_solve_control_t **control);
//! Get an object to inspect the symbolic atoms.
//!
//! @param[in] control the target
//! @param[out] base the resulting object
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_control_base(clingo_solve_control_t const *control,
                                                         clingo_base_t const **base);
//! Add a clause that applies to the current solving step during model
//! enumeration.
//!
//! @note The @ref c_propagate module provides a more sophisticated
//! interface to add clauses - even on partial assignments.
//!
//! @param[in] control the target
//! @param[in] clause array of literals representing the clause
//! @param[in] size the size of the literal array
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_solve_control_add_clause(clingo_solve_control_t *control,
                                                               clingo_literal_t const *clause, size_t size);
//! @}

//! @}

#ifdef __cplusplus
}
#endif

#endif
