#ifndef CLINGO_BACKEND_H
#define CLINGO_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/core.h>
#include <clingo/shared.h>
#include <clingo/symbol.h>

typedef struct clingo_control clingo_control_t;

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

//! @addtogroup c_backend
//! Add non-ground program representations (ASTs) to logic programs or extend the ground (aspif) program.
//!
//! For an example about ground logic programs, see @ref backend.c.
//! For an example about non-ground logic programs, see @ref ast.c and the @ref c_ast module.
//! @{

//! Enumeration of theory sequence types.
enum clingo_theory_sequence_type_e {
    clingo_theory_sequence_type_tuple = 0, //!< Theory tuples "(t1,...,tn)".
    clingo_theory_sequence_type_set = 1,   //!< Theory sets "{t1,...,tn}".
    clingo_theory_sequence_type_list = 2   //!< Theory lists "[t1,...,tn]".
};
//! Corresponding type to ::clingo_theory_sequence_type_e.
typedef int clingo_theory_sequence_type_t;

//! Handle to the backend to add directives in aspif format.
typedef struct clingo_backend clingo_backend_t;

//! Get a backend object to extend the ground program.
//!
//! The control object itself should not be used until the backend is closed.
//!
//! @param[in] control the control object
//! @param[out] backend the resulting backend
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_backend(clingo_control_t *control, clingo_backend_t **backend);

//! Finalize the backend after using it.
//!
//! @param[in] backend the target backend
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_close(clingo_backend_t *backend);

//! Add a rule to the program.
//!
//! @param[in] backend the target backend
//! @param[in] choice determines if the head is a choice or a disjunction
//! @param[in] head the head atoms
//! @param[in] head_size the number of atoms in the head
//! @param[in] body the body literals
//! @param[in] body_size the number of literals in the body
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head,
                                                   size_t head_size, clingo_literal_t const *body, size_t body_size);
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
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_weight_rule(clingo_backend_t *backend, bool choice,
                                                          clingo_atom_t const *head, size_t head_size,
                                                          clingo_weight_t lower_bound,
                                                          clingo_weighted_literal_t const *body, size_t body_size);
//! Add a minimize constraint (or weak constraint) to the program.
//!
//! @param[in] backend the target backend
//! @param[in] priority the priority of the constraint
//! @param[in] literals the weighted literals whose sum to minimize
//! @param[in] size the number of weighted literals
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_minimize(clingo_backend_t *backend, clingo_weight_t priority,
                                                       clingo_weighted_literal_t const *literals, size_t size);
//! Add a projection directive.
//!
//! @param[in] backend the target backend
//! @param[in] atoms the atoms to project on
//! @param[in] size the number of atoms
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_project(clingo_backend_t *backend, clingo_atom_t const *atoms,
                                                      size_t size);
//! Add an external statement.
//!
//! @param[in] backend the target backend
//! @param[in] atom the external atom
//! @param[in] type the type of the external statement
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_external(clingo_backend_t *backend, clingo_atom_t atom,
                                                       clingo_external_type_t type);
//! Add an assumption directive.
//!
//! @param[in] backend the target backend
//! @param[in] literals the literals to assume (positive literals are true and negative literals false for the next
//! solve call)
//! @param[in] size the number of atoms
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_assume(clingo_backend_t *backend, clingo_literal_t const *literals,
                                                     size_t size);
//! Add a heuristic directive.
//!
//! @param[in] backend the target backend
//! @param[in] atom the target atom
//! @param[in] type the type of the heuristic modification
//! @param[in] bias the heuristic bias
//! @param[in] priority the heuristic priority
//! @param[in] condition the condition under which to apply the heuristic modification
//! @param[in] size the number of atoms in the condition
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_heuristic(clingo_backend_t *backend, clingo_atom_t atom,
                                                        clingo_heuristic_type_t type, int bias, unsigned priority,
                                                        clingo_literal_t const *condition, size_t size);
//! Add an edge directive.
//!
//! @param[in] backend the target backend
//! @param[in] node_u the start vertex of the edge
//! @param[in] node_v the end vertex of the edge
//! @param[in] condition the condition under which the edge is part of the graph
//! @param[in] size the number of atoms in the condition
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_acyc_edge(clingo_backend_t *backend, int node_u, int node_v,
                                                        clingo_literal_t const *condition, size_t size);
//! Get a fresh atom to be used in aspif directives.
//!
//! @param[in] backend the target backend
//! @param[in] symbol optional symbol to associate the atom with
//! @param[out] atom the resulting atom
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_add_atom(clingo_backend_t *backend, clingo_symbol_t const *symbol,
                                                       clingo_atom_t *atom);
//! Add a numeric theory term.
//!
//! @param[in] backend the target backend
//! @param[in] number the value of the term
//! @param[out] term_id the resulting term id
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_theory_term_number(clingo_backend_t *backend, int number,
                                                                 clingo_id_t *term_id);
//! Add a theory term representing a string.
//!
//! @param[in] backend the target backend
//! @param[in] string the value of the term
//! @param[in] size the size of the string
//! @param[out] term_id the resulting term id
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_theory_term_string(clingo_backend_t *backend, char const *string,
                                                                 size_t size, clingo_id_t *term_id);
//! Add a theory term representing a sequence of theory terms.
//!
//! @param[in] backend the target backend
//! @param[in] type the type of the sequence
//! @param[in] arguments the term ids of the terms in the sequence
//! @param[in] size the number of elements of the sequence
//! @param[out] term_id the resulting term id
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_theory_term_sequence(clingo_backend_t *backend,
                                                                   clingo_theory_sequence_type_t type,
                                                                   clingo_id_t const *arguments, size_t size,
                                                                   clingo_id_t *term_id);
//! Add a theory term representing a function.
//!
//! @param[in] backend the target backend
//! @param[in] name the name of the function
//! @param[in] name_size the size of the name
//! @param[in] arguments an array of term ids for the theory terms in the arguments
//! @param[in] arguments_size the number of arguments
//! @param[out] term_id the resulting term id
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_theory_term_function(clingo_backend_t *backend, char const *name,
                                                                   size_t name_size, clingo_id_t const *arguments,
                                                                   size_t arguments_size, clingo_id_t *term_id);
//! Convert the given symbol into a theory term.
//!
//! @param[in] backend the target backend
//! @param[in] symbol the symbol to convert
//! @param[out] term_id the resulting term id
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_theory_term_symbol(clingo_backend_t *backend, clingo_symbol_t symbol,
                                                                 clingo_id_t *term_id);
//! Add a theory atom element.
//!
//! @param[in] backend the target backend
//! @param[in] tuple the array of term ids representing the tuple
//! @param[in] tuple_size the size of the tuple
//! @param[in] condition an array of program literals representing the condition
//! @param[in] condition_size the size of the condition
//! @param[out] element_id the resulting element id
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_theory_element(clingo_backend_t *backend, clingo_id_t const *tuple,
                                                             size_t tuple_size, clingo_literal_t const *condition,
                                                             size_t condition_size, clingo_id_t *element_id);

//! Add a theory atom without a guard.
//!
//! If the theory atom does not exist yet, the atom receives a fresh program
//! atom, if atom_id is NULL; or *atom_in, otherwise. Note that value zero can
//! be used to mark program directives. The ouput paramater atom_out is set to
//! the final value.
//!
//! @param[in] backend the target backend
//! @param[in] name the symbol representing the name
//! @param[in] elements an array of element ids for the theory atom's elements
//! @param[in] size the number of elements
//! @param[in] operator_name the (optional) right-hand-side operator
//! @param[in] right_hand_side_id the term id of the right-hand-side
//! @param[in] atom_in the atom as described above
//! @param[out] atom_out the final program atom of the theory atom
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_backend_theory_atom(clingo_backend_t *backend, clingo_symbol_t name,
                                                          clingo_id_t const *elements, size_t size,
                                                          clingo_string_t *operator_name,
                                                          clingo_id_t right_hand_side_id, clingo_atom_t const *atom_in,
                                                          clingo_atom_t *atom_out);

//! @}
#ifdef __cplusplus
}
#endif

#endif
