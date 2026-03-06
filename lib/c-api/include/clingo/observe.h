#ifndef CLINGO_OBSERVE_H
#define CLINGO_OBSERVE_H

#include <clingo/base.h>
#include <clingo/core.h>
#include <clingo/shared.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clingo_control clingo_control_t;

//! @addtogroup c_observe
//! Functions and data structures to inspect ground programs.
//! @{

//! Available write modes for aspif files (bitset).
//!
//! Note that the preamble flag preamble_auto toggles the
enum clingo_write_aspif_mode_e {
    clingo_write_aspif_mode_preamble = 1,      //!< Write preamble.
    clingo_write_aspif_mode_preamble_auto = 2, //!< Write preamble for newly created files.
    clingo_write_aspif_mode_append = 4,        //!< Append to an existing file (or create it).
    clingo_write_aspif_mode_preprocess = 8,    //!< Whether to preprocess the program before writing.
    clingo_write_aspif_mode_symbols = 16,      //!< Whether to write symbols in a structured format.
};
//! Corresponding type to ::clingo_write_aspif_mode_e.
using clingo_write_aspif_mode_t = unsigned;

//! An instance of this struct has to be registered with a solver to observe
//! ground directives as they are passed to the solver.
//!
//! @note This interface is closely modeled after the aspif format.
//! For more information, please refer to the specification of the aspif format.
//!
//! Not all callbacks have to be implemented and can be set to NULL if not
//! needed. If one of the callbacks in the struct fails, inspection is stopped.
//!
//! @see clingo_control_register_observer()
typedef struct clingo_observer {
    //! Called once in the beginning.
    //!
    //! If the incremental flag is true, there can be multiple calls to @ref clingo_control_solve().
    //!
    //! @param[in] incremental whether the program is incremental
    //! @param[in] data user data for the callback
    //! @return the result code
    bool (*init_program)(bool incremental, void *data);
    //! Marks the beginning of a block of directives passed to the solver.
    //!
    //! @see @ref end_step
    //!
    //! @param[in] data user data for the callback
    //! @return the result code
    bool (*begin_step)(void *data);
    //! Marks the end of a block of directives passed to the solver.
    //!
    //! This function is called before solving starts.
    //!
    //! @see @ref begin_step
    //!
    //! @param[in] base the base of the program
    //! @param[in] data user data for the callback
    //! @return the result code
    bool (*end_step)(clingo_base_t const *base, void *data);

    //! Observe rules passed to the solver.
    //!
    //! @param[in] choice determines if the head is a choice or a disjunction
    //! @param[in] head the head atoms
    //! @param[in] head_size the number of atoms in the head
    //! @param[in] body the body literals
    //! @param[in] body_size the number of literals in the body
    //! @param[in] data user data for the callback
    //! @return the result code
    bool (*rule)(bool choice, clingo_atom_t const *head, size_t head_size, clingo_literal_t const *body,
                 size_t body_size, void *data);
    //! Observe weight rules passed to the solver.
    //!
    //! @param[in] choice determines if the head is a choice or a disjunction
    //! @param[in] head the head atoms
    //! @param[in] head_size the number of atoms in the head
    //! @param[in] lower_bound the lower bound of the weight rule
    //! @param[in] body the weighted body literals
    //! @param[in] body_size the number of weighted literals in the body
    //! @param[in] data user data for the callback
    //! @return the result code
    bool (*weight_rule)(bool choice, clingo_atom_t const *head, size_t head_size, clingo_weight_t lower_bound,
                        clingo_weighted_literal_t const *body, size_t body_size, void *data);
    //! Observe minimize constraints (or weak constraints) passed to the solver.
    //!
    //! @param[in] priority the priority of the constraint
    //! @param[in] literals the weighted literals whose sum to minimize
    //! @param[in] size the number of weighted literals
    //! @param[in] data user data for the callback
    //! @return the result code
    bool (*minimize)(clingo_weight_t priority, clingo_weighted_literal_t const *literals, size_t size, void *data);
    //! Observe projection directives passed to the solver.
    //!
    //! @param[in] atoms the atoms to project on
    //! @param[in] size the number of atoms
    //! @param[in] data user data for the callback
    //! @return the result code
    bool (*project)(clingo_atom_t const *atoms, size_t size, void *data);
    //! Observe external statements passed to the solver.
    //!
    //! @param[in] atom the external atom
    //! @param[in] type the type of the external statement
    //! @param[in] data user data for the callback
    //! @return the result code
    bool (*external)(clingo_atom_t atom, clingo_external_type_t type, void *data);
    //! Observe assumption directives passed to the solver.
    //!
    //! @param[in] literals the literals to assume (positive literals are true and negative literals false for the next
    //! solve call)
    //! @param[in] size the number of atoms
    //! @param[in] data user data for the callback
    //! @return the result code
    bool (*assume)(clingo_literal_t const *literals, size_t size, void *data);
    //! Observe heuristic directives passed to the solver.
    //!
    //! @param[in] atom the target atom
    //! @param[in] type the type of the heuristic modification
    //! @param[in] bias the heuristic bias
    //! @param[in] priority the heuristic priority
    //! @param[in] condition the condition under which to apply the heuristic modification
    //! @param[in] size the number of atoms in the condition
    //! @param[in] data user data for the callback
    //! @return the result code
    bool (*heuristic)(clingo_atom_t atom, clingo_heuristic_type_t type, int bias, unsigned priority,
                      clingo_literal_t const *condition, size_t size, void *data);
    //! Observe edge directives passed to the solver.
    //!
    //! @param[in] node_u the start vertex of the edge
    //! @param[in] node_v the end vertex of the edge
    //! @param[in] condition the condition under which the edge is part of the graph
    //! @param[in] size the number of atoms in the condition
    //! @param[in] data user data for the callback
    //! @return the result code
    bool (*acyc_edge)(int node_u, int node_v, clingo_literal_t const *condition, size_t size, void *data);
} clingo_observer_t;

//! Get an observer to inspect the ground program.
//!
//! Note that the system only maintains the ground program from the current
//! grounding step. Ground programs from previous steps are discarded when
//! calling clingo_control_solve().
//!
//! @param[in] control the control object
//! @param[in] observer the observer to use
//! @param[in] data user data for the observer
//! @param[in] preprocess whether to preprocess the program first
//! @return the result code
CLINGO_VISIBILITY_DEFAULT bool clingo_control_observe(clingo_control_t *control, clingo_observer_t const *observer,
                                                      void *data, bool preprocess);

//! Write the current logic program in aspif format to a file.
//!
//! @param control the target control
//! @param path the path to the file to write to
//! @param size the size of the path
//! @param mode control how to write
//! @return the result code
CLINGO_VISIBILITY_DEFAULT bool clingo_control_write_aspif(clingo_control_t *control, char const *path, size_t size,
                                                          clingo_write_aspif_mode_t mode);

//! @}

#ifdef __cplusplus
}
#endif

#endif
