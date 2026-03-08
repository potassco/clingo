#ifndef CLINGO_THEORY_H
#define CLINGO_THEORY_H

#include <clingo/app.h>
#include <clingo/ast.h>
#include <clingo/core.h>
#include <clingo/stats.h>
#include <clingo/symbol.h>

#ifdef __cplusplus
extern "C" {
#endif

//! @example theory.c
//! The example shows a partial theory implementation.
//!
//! ## Code ##

//! @addtogroup c_theory
//!
//! This module defines a well-specified C interface that must be implemented
//! by external theory plugins. The interface is designed to allow efficient
//! interoperation between the core solver and external theory implementations.
//! It facilitates integration across language barriers (for example, bridging
//! C++ and Python) and across modular boundaries within the system.
//!
//! Among its responsibilities, the theory interface includes: Providing
//! essential theory callbacks (e.g., rewriting ASTs, model handling, preparing
//! theory data, and statistics collection). Defining the lifecycle of a theory
//! object, including optional cleanup via a destroy callback.
//!
//! For an example, refer to @ref theory.c.
//! @{

//! The available value types.
enum clingo_theory_value_type_e {
    //! An integer value.
    clingo_theory_value_type_int = 0,
    //! A double value.
    clingo_theory_value_type_double = 1,
    //! A symbol value.
    clingo_theory_value_type_symbol = 2
};
//! Corresponding type to clingo_theory_value_type.
typedef int clingo_theory_value_type_t;
//! A struct to hold values assigned by a theory.
typedef struct clingo_theory_value {
    //! The type of the value.
    clingo_theory_value_type_t type;
    union {
        //! The integer value.
        int int_number;
        //! The double value.
        double double_number;
        //! The symbolic value.
        clingo_symbol_t symbol;
    };
} clingo_theory_value_t;

//! A callback to rewrite asts.
typedef bool (*clingo_theory_ast_callback_t)(clingo_ast_t *ast, void *data);

//! A theory object to extend solving.
typedef struct clingo_theory {
    //! Get the name and version of the theory.
    //!
    //! @param[in] self the self pointer
    //! @param[out] name the name of the theory (optional)
    //! @param[out] major the major version component (optional)
    //! @param[out] minor the minor version component (optional)
    //! @param[out] revision the revision version component (optional)
    //! @return whether the call was successful
    bool (*info)(void *self, clingo_string_t *name, int *major, int *minor, int *revision);
    //! Destroy the theory.
    //!
    //! @param[in] self the self pointer
    void (*destroy)(void *self);
    //! Register the theory with the given control object.
    //!
    //! A theory might register propagators or add theory definitions here.
    //!
    //! @param[in] self the self pointer
    //! @param[in] control the control object
    //! @return whether the call was successful
    bool (*register_theory)(void *self, clingo_control_t *control);
    //! Rewrite the given ast for the theory.
    //!
    //! A theory might rewrite theory atoms.
    //!
    //! @param[in] self the self pointer
    //! @param[in] statement the statement to rewrite
    //! @param[in] callback the callback to pass rewritten statements to
    //! @param[in] data the user data for the callback
    //! @return whether the call was successful
    bool (*rewrite_ast)(void *self, clingo_ast_t *statement, clingo_theory_ast_callback_t callback, void *data);
    //! Prepare the theory.
    //!
    //! Must be called between ground and solve.
    //!
    //! @param[in] self the self pointer
    //! @param[in] control the control object
    //! @return whether the call was successful
    bool (*prepare)(void *self, clingo_control_t *control);
    //! Register the theory's options with the given application options object.
    //!
    //! @param[in] self the self pointer
    //! @param[in] control the options object
    //! @return whether the call was successful
    bool (*register_options)(void *self, clingo_options_t *options);
    //! Validate the options of the theory.
    //!
    //! @param[in] self the self pointer
    //! @return whether the call was successful
    bool (*validate_options)(void *self);
    //! Inform the theory that a model has been found.
    //!
    //! @param[in] self the self pointer
    //! @param[in] model the current model
    //! @return whether the call was successful
    bool (*on_model)(void *self, clingo_model_t *model);
    //! Add the theory's statistics to the given maps.
    //!
    //! @param[in] self the self pointer
    //! @param[in] stats the statistics object
    //! @return whether the call was successful
    bool (*on_stats)(void *self, clingo_stats_t *stats);
    //! Get the integer index of a symbol assigned by the theory when a model is found.
    //!
    //! Using indices allows for efficient retrieval of values.
    //!
    //! @param[in] self the self pointer
    //! @param[in] symbol the symbol to lookup
    //! @param[out] index the resulting index (optional)
    //! @param[out] found whether the symbol has been found (optional)
    //! @return whether the call was successful
    bool (*lookup_symbol)(void *self, clingo_symbol_t symbol, size_t *index, bool *found);
    //! Get the next index that has a value.
    //!
    //! If init is true, an index to the first value in the assignment is
    //! returned and the value of init is set to false.
    //!
    //! The returned index has a value if has_value is true. Otherwise,
    //! iteration must be stopped.
    //!
    //! @param[in] self the self pointer
    //! @param[in] thread_id the thread that holds the assignment
    //! @param[inout] init whether to advance or initialize the index
    //! @param[out] index the resulting index
    //! @param[out] has_value whether the index has a value
    //! @return whether the call was successful
    bool (*assignment_next)(void *self, uint32_t thread_id, bool *init, size_t *index, bool *has_value);
    //! Get the value assigned to the given index.
    //!
    //! Note that the caller of this function is responsible to release symbols.
    //!
    //! @param[in] self the self pointer
    //! @param[in] thread_id the thread that holds the assignment
    //! @param[in] index the index to lookup
    //! @param[out] symbol the resulting symbol (optional)
    //! @param[out] value the resulting value (optional)
    //! @param[out] has_value whether the index has a value (optional)
    //! @return whether the call was successful
    bool (*assignment_get_value)(void *self, uint32_t thread_id, size_t index, clingo_symbol_t *symbol,
                                 clingo_theory_value_t *value, bool *has_value);

    //! The userdata for the first value to the callbacks in this struct.
    void *self;
} clingo_theory_t;

//! @}

#ifdef __cplusplus
}
#endif

#endif
