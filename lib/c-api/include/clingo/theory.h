#ifndef CLINGO_THEORY_H
#define CLINGO_THEORY_H

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

#include <clingo/app.h>
#include <clingo/ast.h>
#include <clingo/core.h>
#include <clingo/stats.h>
#include <clingo/symbol.h>

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
    clingo_theory_value_type_t type;
    union {
        int int_number;
        double double_number;
        clingo_symbol_t symbol;
    };
} clingo_theory_value_t;

//! A callback to rewrite asts.
typedef clingo_result_t (*clingo_theory_ast_callback_t)(clingo_ast_t *ast, void *data);

//! A theory object to extend solving.
typedef struct clingo_theory {
    //! Get the name and version of the theory.
    clingo_result_t (*info)(void *self, char const **name, int *major, int *minor, int *patch);
    //! Destroy the theory.
    void (*destroy)(void *self);
    //! Register the theory with the given control object.
    //!
    //! A theory might register propagators or add theory definitions here.
    clingo_result_t (*register_theory)(void *self, clingo_control_t *control);
    //! Rewrite the given ast for the theory.
    //!
    //! A theory might rewrite theory atoms.
    clingo_result_t (*rewrite_ast)(void *self, clingo_ast_t *ast, clingo_theory_ast_callback_t add, void *data);
    //! Prepare the theory.
    //!
    //! Must be called between ground and solve.
    clingo_result_t (*prepare)(void *self, clingo_control_t *control);
    //! Register the theory's options with the given application options object.
    clingo_result_t (*register_options)(void *self, clingo_options_t *options);
    //! Validate the options of the theory.
    clingo_result_t (*validate_options)(void *self);
    //! Configure the theory passing key value pairs.
    clingo_result_t (*configure)(void *self, char const *key, char const *value);
    //! Inform the theory that a model has been found.
    clingo_result_t (*on_model)(void *self, clingo_model_t const *model);
    //! Add the theory's statistics to the given maps.
    clingo_result_t (*on_stats)(void *self, clingo_stats_t *stats);
    //! Get the integer index of a symbol assigned by the theory when a model is found.
    //!
    //! Using indices allows for efficent retrieval of values.
    clingo_result_t (*lookup_symbol)(void *self, clingo_symbol_t symbol, size_t *index, bool *found);
    //! Get the next index that has a value.
    //!
    //! If init is true, an index to the first value in the assignment is
    //! returned and the value of init is set to false.
    //!
    //! The returned index has a value if has_value is true. Otherwise,
    //! iteration must be stopped.
    clingo_result_t (*assignment_next)(void *self, uint32_t thread_id, size_t *index, bool *init, bool *has_value);
    //! Get the value assigned to the given index.
    clingo_result_t (*assignment_get_value)(void *self, uint32_t thread_id, size_t index, clingo_symbol_t *symbol,
                                            clingo_theory_value_t *value, bool *found);

    //! The userdata that has to be passed as the first value to the callabcks in this struct.
    void *self;
} clingo_theory_t;

//! @}

#ifdef __cplusplus
}
#endif

#endif
