#ifndef CLINGO_BACKEND_H
#define CLINGO_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/core.h>
#include <clingo/symbol.h>

//! @example backend.c
//! The example shows how to use the backend to extend a grounded program.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! ./backend 0
//! Model: a b
//! Model: a b c
//! Model:
//! Model: a
//! Model: b
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @defgroup ProgramBuilder Program Building
//! Add non-ground program representations (ASTs) to logic programs or extend the ground (aspif) program.
//! @ingroup Control
//!
//! For an example about ground logic programs, see @ref backend.c.
//! For an example about non-ground logic programs, see @ref ast.c and the @ref AST module.

//! @addtogroup ProgramBuilder
//! @{

//! Enumeration of theory sequence types.
enum clingo_theory_sequence_type_e {
    clingo_theory_sequence_type_tuple = 0, //!< Theory tuples "(t1,...,tn)".
    clingo_theory_sequence_type_set = 1,   //!< Theory sets "{t1,...,tn}".
    clingo_theory_sequence_type_list = 2   //!< Theory lists "[t1,...,tn]".
};
//! Corresponding type to ::clingo_theory_sequence_type_e.
typedef int clingo_theory_sequence_type_t;

//! Enumeration of different heuristic modifiers.
//! @ingroup ProgramInspection
enum clingo_heuristic_type_e {
    clingo_heuristic_type_level = 0,  //!< set the level of an atom
    clingo_heuristic_type_sign = 1,   //!< configure which sign to chose for an atom
    clingo_heuristic_type_factor = 2, //!< modify VSIDS factor of an atom
    clingo_heuristic_type_init = 3,   //!< modify the initial VSIDS score of an atom
    clingo_heuristic_type_true = 4,   //!< set the level of an atom and choose a positive sign
    clingo_heuristic_type_false = 5   //!< set the level of an atom and choose a negative sign
};
//! Corresponding type to ::clingo_heuristic_type_e.
typedef int clingo_heuristic_type_t;

//! Enumeration of different external statements.
enum clingo_external_type_e {
    clingo_external_type_free = 0,    //!< allow an external to be assigned freely
    clingo_external_type_true = 1,    //!< assign an external to true
    clingo_external_type_false = 2,   //!< assign an external to false
    clingo_external_type_release = 3, //!< no longer treat an atom as external
};
//! Corresponding type to ::clingo_external_type_e.
typedef int clingo_external_type_t;

//! Handle to the backend to add directives in aspif format.
typedef struct clingo_backend clingo_backend_t;

//! Finalize the backend after using it.
//!
//! @param[in] backend the target backend
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_close(clingo_backend_t *backend);
//! Add a rule to the program.
//!
//! @param[in] backend the target backend
//! @param[in] choice determines if the head is a choice or a disjunction
//! @param[in] head the head atoms
//! @param[in] head_size the number of atoms in the head
//! @param[in] body the body literals
//! @param[in] body_size the number of literals in the body
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_rule(clingo_backend_t *backend, bool choice,
                                                              clingo_atom_t const *head, size_t head_size,
                                                              clingo_literal_t const *body, size_t body_size);
//! Add a weight rule to the program.
//!
//! @attention All weights and the lower bound must be positive.
//! @param[in] backend the target backend
//! @param[in] choice determines if the head is a choice or a disjunction
//! @param[in] head the head atoms
//! @param[in] head_size the number of atoms in the head
//! @param[in] lower_bound the lower bound of the weight rule
//! @param[in] body the weighted body literals
//! @param[in] body_size the number of weighted literals in the body
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_weight_rule(clingo_backend_t *backend, bool choice,
                                                                     clingo_atom_t const *head, size_t head_size,
                                                                     clingo_weight_t lower_bound,
                                                                     clingo_weighted_literal_t const *body,
                                                                     size_t body_size);
//! Add a minimize constraint (or weak constraint) to the program.
//!
//! @param[in] backend the target backend
//! @param[in] priority the priority of the constraint
//! @param[in] literals the weighted literals whose sum to minimize
//! @param[in] size the number of weighted literals
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_minimize(clingo_backend_t *backend, clingo_weight_t priority,
                                                                  clingo_weighted_literal_t const *literals,
                                                                  size_t size);
//! Add a projection directive.
//!
//! @param[in] backend the target backend
//! @param[in] atoms the atoms to project on
//! @param[in] size the number of atoms
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_project(clingo_backend_t *backend, clingo_atom_t const *atoms,
                                                                 size_t size);
//! Add an external statement.
//!
//! @param[in] backend the target backend
//! @param[in] atom the external atom
//! @param[in] type the type of the external statement
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_external(clingo_backend_t *backend, clingo_atom_t atom,
                                                                  clingo_external_type_t type);
//! Add an assumption directive.
//!
//! @param[in] backend the target backend
//! @param[in] literals the literals to assume (positive literals are true and negative literals false for the next
//! solve call)
//! @param[in] size the number of atoms
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_assume(clingo_backend_t *backend,
                                                                clingo_literal_t const *literals, size_t size);
//! Add a heuristic directive.
//!
//! @param[in] backend the target backend
//! @param[in] atom the target atom
//! @param[in] type the type of the heuristic modification
//! @param[in] bias the heuristic bias
//! @param[in] priority the heuristic priority
//! @param[in] condition the condition under which to apply the heuristic modification
//! @param[in] size the number of atoms in the condition
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_heuristic(clingo_backend_t *backend, clingo_atom_t atom,
                                                                   clingo_heuristic_type_t type, int bias,
                                                                   unsigned priority, clingo_literal_t const *condition,
                                                                   size_t size);
//! Add an edge directive.
//!
//! @param[in] backend the target backend
//! @param[in] node_u the start vertex of the edge
//! @param[in] node_v the end vertex of the edge
//! @param[in] condition the condition under which the edge is part of the graph
//! @param[in] size the number of atoms in the condition
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_acyc_edge(clingo_backend_t *backend, int node_u, int node_v,
                                                                   clingo_literal_t const *condition, size_t size);
//! Get a fresh atom to be used in aspif directives.
//!
//! @param[in] backend the target backend
//! @param[in] symbol optional symbol to associate the atom with
//! @param[out] atom the resulting atom
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_add_atom(clingo_backend_t *backend, clingo_symbol_t *symbol,
                                                                  clingo_atom_t *atom);
//! Add a numeric theory term.
//!
//! @param[in] backend the target backend
//! @param[in] number the value of the term
//! @param[out] term_id the resulting term id
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_theory_term_number(clingo_backend_t *backend, int number,
                                                                            clingo_id_t *term_id);
//! Add a theory term representing a string.
//!
//! @param[in] backend the target backend
//! @param[in] string the value of the term
//! @param[out] term_id the resulting term id
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_theory_term_string(clingo_backend_t *backend,
                                                                            char const *string, clingo_id_t *term_id);
//! Add a theory term representing a sequence of theory terms.
//!
//! @param[in] backend the target backend
//! @param[in] type the type of the sequence
//! @param[in] arguments the term ids of the terms in the sequence
//! @param[in] size the number of elements of the sequence
//! @param[out] term_id the resulting term id
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_theory_term_sequence(clingo_backend_t *backend,
                                                                              clingo_theory_sequence_type_t type,
                                                                              clingo_id_t const *arguments, size_t size,
                                                                              clingo_id_t *term_id);
//! Add a theory term representing a function.
//!
//! @param[in] backend the target backend
//! @param[in] name the name of the function
//! @param[in] arguments an array of term ids for the theory terms in the arguments
//! @param[in] size the number of arguments
//! @param[out] term_id the resulting term id
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_theory_term_function(clingo_backend_t *backend,
                                                                              char const *name,
                                                                              clingo_id_t const *arguments, size_t size,
                                                                              clingo_id_t *term_id);
//! Convert the given symbol into a theory term.
//!
//! @param[in] backend the target backend
//! @param[in] symbol the symbol to convert
//! @param[out] term_id the resulting term id
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_theory_term_symbol(clingo_backend_t *backend,
                                                                            clingo_symbol_t symbol,
                                                                            clingo_id_t *term_id);
//! Add a theory atom element.
//!
//! @param[in] backend the target backend
//! @param[in] tuple the array of term ids representing the tuple
//! @param[in] tuple_size the size of the tuple
//! @param[in] condition an array of program literals representing the condition
//! @param[in] condition_size the size of the condition
//! @param[out] element_id the resulting element id
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_theory_element(clingo_backend_t *backend,
                                                                        clingo_id_t const *tuple, size_t tuple_size,
                                                                        clingo_literal_t const *condition,
                                                                        size_t condition_size, clingo_id_t *element_id);
//! Add a theory atom without a guard.
//!
//! If atom is set to zero, the theory atom is a directive,
//! if atom is set to UINT32_MAX, the theory atom receives a fresh atom,
//! and otherwise the theory atom receives the given atom id.
//!
//! @param[in] backend the target backend
//! @param[in] atom an undefined value, program atom, or zero for theory directives
//! @param[in] term_id the term id of the term associated with the theory atom
//! @param[in] elements an array of element ids for the theory atom's elements
//! @param[in] size the number of elements
//! @param[out] atom_id the final program atom of the theory atom
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_theory_atom(clingo_backend_t *backend, clingo_atom_t atom,
                                                                     clingo_id_t term_id, clingo_id_t const *elements,
                                                                     size_t size, clingo_atom_t *atom_id);
//! Add a theory atom with a guard.
//!
//! See the note regarding atom at clingo_backend_theory_atom().
//!
//! @param[in] backend the target backend
//! @param[in] atom an undefined value, program atom, or zero for theory directives
//! @param[in] term_id the term id of the term associated with the theory atom
//! @param[in] elements an array of element ids for the theory atom's elements
//! @param[in] size the number of elements
//! @param[in] operator_name the string representation of a theory operator
//! @param[in] right_hand_side_id the term id of the right hand side term
//! @param[out] atom_id the final program atom of the theory atom
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_backend_theory_atom_with_guard(
    clingo_backend_t *backend, clingo_atom_t atom, clingo_id_t term_id, clingo_id_t const *elements, size_t size,
    char const *operator_name, clingo_id_t right_hand_side_id, clingo_atom_t *atom_id);

//! @}
#ifdef __cplusplus
}
#endif

#endif
