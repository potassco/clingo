#ifndef CLINGO_BASE_H
#define CLINGO_BASE_H

#include <clingo/core.h>
#include <clingo/symbol.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clingo_control clingo_control_t;

//! @example base-atoms.c
//! The example shows how to iterate over symbolic atoms.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! ./base 0
//! Symbolic atoms:
//!   b
//!   c, external
//!   a, fact
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @example base-theory.c
//! The example shows how to inspect and use theory atoms.
//!
//! This is a very simple example that uses the @link c_backend ProgramBuilder@endlink to let theory atoms affect answer
//! sets. In general, the backend can be used to implement a custom theory by translating it to a logic program. On the
//! other hand, a @link c_propagate Propagator@endlink can be used to implement a custom theory without adding any
//! constraints in advance. Or both approaches can be combined.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! ./theory-atoms 0
//! number of grounded theory atoms: 2
//! theory atom b/1 has a guard: true
//! Model: y
//! Model: x y
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @addtogroup c_base
//! Inspection of atoms occurring in ground logic programs.
//!
//! For an examples, see @ref base-atoms.c and base-theory.c.
//! @{

//! Represents a predicate signature.
//!
//! Signatures have a name and an arity, and can be positive or negative (to
//! represent classical negation).
typedef struct clingo_signature {
    char const *name; //!< the name
    size_t size;      //!< the size of the name
    size_t arity;     //!< the arity
    bool is_positive; //!< whether the signature is positive
} clingo_signature_t;

//! Object to inspect symbolic atoms in a program---the relevant Herbrand base
//! gringo uses to instantiate programs and the terms that occur in the heads
//! of `#show` statements.
//!
//! @see clingo_control_base()
typedef struct clingo_base clingo_base_t;

//! Object to inspect the symbolic atoms in a program.
//!
//! Atom bases capture atoms over the same signature.
typedef struct clingo_atom_base clingo_atom_base_t;

//! Object to inspect the shown terms in a program.
typedef struct clingo_term_base clingo_term_base_t;

//! Enumeration of theory term types.
enum clingo_theory_term_type_e {
    clingo_theory_term_type_tuple = 0,    //!< a tuple term, e.g., `(1,2,3)`
    clingo_theory_term_type_list = 1,     //!< a list term, e.g., `[1,2,3]`
    clingo_theory_term_type_set = 2,      //!< a set term, e.g., `{1,2,3}`
    clingo_theory_term_type_function = 3, //!< a function term, e.g., `f(1,2,3)`
    clingo_theory_term_type_number = 4,   //!< a number term, e.g., `42`
    clingo_theory_term_type_symbol = 5    //!< a symbol term, e.g., `c`
};
//! Corresponding type to ::clingo_theory_term_type_e.
typedef int clingo_theory_term_type_t;

//! Object to inspect theory atoms.
typedef struct clingo_theory_base clingo_theory_base_t;

//! Check whether the given literal is a fact.
//!
//! @note This does not determine if a literal is a cautious consequence. The
//! grounding or solving component's simplifications can only detect this in
//! some cases.
//!
//! @param[in] atoms the atom base
//! @param[in] literal the index of the literal
//! @param[out] is_fact whether the literal is a fact
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_base_is_fact(clingo_base_t const *atoms, clingo_literal_t literal, bool *is_fact);

//! Check whether a literal is external.
//!
//! A literal is external if it has been defined using an external directive and
//! has not been released or defined by a rule.
//!
//! @param[in] atoms the target
//! @param[in] literal the index of the literal
//! @param[out] is_external whether the literal is an external
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_base_is_external(clingo_base_t const *atoms, clingo_literal_t literal,
                                                       bool *is_external);

//! Check whether a literal is shown.
//!
//! A literal is shown if it has been shown by a show directive.
//!
//! @param[in] atoms the target
//! @param[in] literal the index of the tom
//! @param[out] is_shown whether the tom is shown
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_base_is_shown(clingo_base_t const *atoms, clingo_literal_t literal,
                                                    bool *is_shown);

//! Check whether a literal is subject to projection.
//!
//! A literal is subject to projection if it occurred in a project directive.
//!
//! @param[in] atoms the target
//! @param[in] literal the index of the literal
//! @param[out] is_projected whether the literal is subject to projection
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_base_is_projected(clingo_base_t const *atoms, clingo_literal_t literal,
                                                        bool *is_projected);

//! Check whether a literal has been introduced in the current step.
//!
//! Note that all literals introduced before the last solve call are considered
//! from a previous step.
//!
//! @param[in] atoms the target
//! @param[in] literal the index of the literal
//! @param[out] is_current whether the literal was introduced in the current step
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_base_is_current(clingo_base_t const *atoms, clingo_literal_t literal,
                                                      bool *is_current);

//! Get the number of atom bases in the program.
//!
//! Each atom base is associated with a signature.
//!
//! @param[in] base the target
//! @param[out] size the number of atoms
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_base_atoms_size(clingo_base_t const *base, size_t *size);

//! Get the signature and atom base at the given index.
//!
//! The index must be smaller than the size reported by clingo_base_atoms_size().
//! Either of the two target pointers can be null if the value is not important.
//!
//! @param[in] base the base
//! @param[in] index the index of the atom base
//! @param[out] signature the target signature
//! @param[out] atoms the target atom base
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_base_atoms_at(clingo_base_t const *base, size_t index,
                                                    clingo_signature_t *signature, clingo_atom_base_t const **atoms);

//! Find the atom base wit the given signature.
//!
//! @param[in] base the base
//! @param[in] signature the signature to lookup
//! @param[out] atoms the target atom base
//! @param[out] found whether a base has been found
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_base_atoms_find(clingo_base_t const *base, clingo_signature_t const *signature,
                                                      clingo_atom_base_t const **atoms, bool *found);

//! Get the size of the given atom base.
//!
//! @param[in] atoms the atom base
//! @param[out] size the target size
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_atom_base_size(clingo_atom_base_t const *atoms, size_t *size);

//! Find the index of the atom with the given symbol in the atom base.
//!
//! If the symbol is not found, return the size of the atom base.
//!
//! @param atoms the atom base
//! @param symbol the symbol to lookup
//! @param index the target index
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_atom_base_find(clingo_atom_base_t const *atoms, clingo_symbol_t symbol,
                                                     size_t *index);

//! Get the symbolic representation of an atom.
//!
//! @param[in] atoms the atom base
//! @param[in] index the index of the atom
//! @param[out] symbol the resulting symbol
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_atom_base_symbol(clingo_atom_base_t const *atoms, size_t index,
                                                       clingo_symbol_t *symbol);

//! Returns the (numeric) program literal corresponding to the given symbolic atom.
//!
//! Such a literal can be mapped to a solver literal (see the @ref c_propagate
//! module) or be used in rules in aspif format (see the @ref c_ast module).
//!
//! @param[in] atoms the atom base
//! @param[in] index the index of the atom
//! @param[out] literal the resulting literal
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_atom_base_literal(clingo_atom_base_t const *atoms, size_t index,
                                                        clingo_literal_t *literal);

//! Get the term base capturing show term directives.
//!
//! @param base the base
//! @param terms the term base
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_base_terms(clingo_base_t const *base, clingo_term_base_t const **terms);

//! Get the theory base capturing theory terms, elements, and atoms.
//!
//! During grounding, theory atoms get consecutive numbers starting with zero.
//! The total number of theory atoms can be obtained using clingo_theory_base_size().
//!
//! @attention
//! All structural information about theory atoms, elements, and terms is reset after @link clingo_control_solve()
//! solving@endlink. If afterward fresh theory atoms are @link clingo_control_ground() grounded@endlink, previously used
//! ids are reused.
//!
//! For an example, see @ref base-theory.c.
//!
//! @param base the base
//! @param theory the theory base
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_base_theory(clingo_base_t const *base, clingo_theory_base_t const **theory);

//! During grounding, theory atoms get consecutive numbers starting with zero.
//! The total number of theory atoms can be obtained using clingo_theory_base_size().
//!
//! @attention
//! All structural information about theory atoms, elements, and terms is reset after @link clingo_control_solve()
//! solving@endlink. If afterward fresh theory atoms are @link clingo_control_ground() grounded@endlink, previously used
//! ids are reused.
//!
//! For an example, see @ref base-theory.c.
//!
//! Get the number of shown terms in a program.
//!
//! @param[in] terms the term base
//! @param[out] size the number of terms in the base
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_term_base_size(clingo_term_base_t const *terms, size_t *size);

//! Get the symbol of the show directive with the given index.
//!
//! @param[in] terms the term base
//! @param[in] index the index of the show directive
//! @param[out] term the resulting term
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_term_base_symbol(clingo_term_base_t const *terms, size_t index,
                                                       clingo_symbol_t *term);

//! Get the conditions of a show directive.
//!
//! Returns the conditions under which a term is shown in form of a disjunction
//! of conjunctions.
//!
//! @param[in] terms the term base
//! @param[in] index the index of the show directive
//! @param[out] sizes the sizes of the conjunctions
//! @param[out] literals the target literals
//! @param[out] size the size of the disjunction
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_term_base_condition(clingo_term_base_t const *terms, size_t index,
                                                          size_t const **sizes,
                                                          clingo_literal_t const *const **literals, size_t *size);

//! Get the index of the given symbol.
//!
//! If the symbol is not found, the index is set to the size of the base.
//!
//! @param[in] terms the term base
//! @param[in] symbol the symbol to lookup
//! @param[out] index the target index
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_term_base_find(clingo_term_base_t const *terms, clingo_symbol_t symbol,
                                                     size_t *index);

//! @name Theory Term Inspection
//! @{

//! Get the type of the given theory term.
//!
//! @param[in] theory container where the term is stored
//! @param[in] term id of the term
//! @param[out] type the resulting type
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_term_type(clingo_theory_base_t const *theory, clingo_id_t term,
                                                            clingo_theory_term_type_t *type);

//! Get the number of the given numeric theory term.
//!
//! @pre The term must be of type ::clingo_theory_term_type_number.
//! @param[in] theory container where the term is stored
//! @param[in] term id of the term
//! @param[out] number the resulting number
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_term_number(clingo_theory_base_t const *theory, clingo_id_t term,
                                                              int *number);
//! Get the name of the given constant or function theory term.
//!
//! @pre The term must be of type ::clingo_theory_term_type_function or ::clingo_theory_term_type_symbol.
//! @param[in] theory container where the term is stored
//! @param[in] term id of the term
//! @param[out] name the resulting name
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_term_name(clingo_theory_base_t const *theory, clingo_id_t term,
                                                            clingo_string_t *name);
//! Get the arguments of the given function theory term.
//!
//! @pre The term must be of type ::clingo_theory_term_type_function.
//! @param[in] theory container where the term is stored
//! @param[in] term id of the term
//! @param[out] arguments the resulting arguments in form of an array of term ids
//! @param[out] size the number of arguments
//! @return whether the call was successful

CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_term_arguments(clingo_theory_base_t const *theory, clingo_id_t term,
                                                                 clingo_id_t const **arguments, size_t *size);

//! Get the string representation of the given theory term.
//!
//! @param[in] theory container where the term is stored
//! @param[in] term id of the term
//! @param[in] builder the string builder
//! @return whether the call was successful
//! @see clingo_theory_base_term_to_string_size()
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_term_to_string(clingo_theory_base_t const *theory, clingo_id_t term,
                                                                 clingo_string_builder_t *builder);

//! @}

//! @name Theory Element Inspection
//! @{

//! Get the tuple (array of theory terms) of the given theory element.
//!
//! @param[in] theory container where the element is stored
//! @param[in] element id of the element
//! @param[out] tuple the resulting array of term ids
//! @param[out] size the number of term ids
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_element_tuple(clingo_theory_base_t const *theory, clingo_id_t element,
                                                                clingo_id_t const **tuple, size_t *size);

//! Get the condition (array of aspif literals) of the given theory element.
//!
//! @param[in] theory container where the element is stored
//! @param[in] element id of the element
//! @param[out] condition the resulting array of aspif literals
//! @param[out] size the number of term literals
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_element_condition(clingo_theory_base_t const *theory,
                                                                    clingo_id_t element,
                                                                    clingo_literal_t const **condition, size_t *size);

//! Get a program literal for the condition of the given theory element.
//!
//! This returns zero for empty conditons (which is technically not a program
//! literal because it cannot be negated). This special program literal is only
//! valid for the current step.
//!
//! @note
//! This id can be mapped to a solver literal using
//! clingo_propagate_init_solver_literal(). To get the (persistent) aspif
//! literals as grounded use clingo_theory_base_element_condition().
//!
//! @param[in] theory container where the element is stored
//! @param[in] element id of the element
//! @param[out] condition the resulting condition id
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_element_condition_id(clingo_theory_base_t const *theory,
                                                                       clingo_id_t element,
                                                                       clingo_literal_t *condition);

//! Get the string representation of the given theory element.
//!
//! @param[in] theory container where the element is stored
//! @param[in] element id of the element
//! @param[in] builder the builder
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_element_to_string(clingo_theory_base_t const *theory,
                                                                    clingo_id_t element,
                                                                    clingo_string_builder_t *builder);

//! @}

//! @name Theory Atom Inspection
//! @{

//! Get the total number of theory atoms.
//!
//! @param[in] theory the target
//! @param[out] size the resulting number
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_size(clingo_theory_base_t const *theory, size_t *size);

//! Get the theory term associated with the theory atom.
//!
//! @param[in] theory container where the atom is stored
//! @param[in] atom id of the atom
//! @param[out] term the resulting term id
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_atom_term(clingo_theory_base_t const *theory, clingo_id_t atom,
                                                            clingo_id_t *term);

//! Get the theory elements associated with the theory atom.
//!
//! @param[in] theory container where the atom is stored
//! @param[in] atom id of the atom
//! @param[out] elements the resulting array of elements
//! @param[out] size the number of elements
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_atom_elements(clingo_theory_base_t const *theory, clingo_id_t atom,
                                                                clingo_id_t const **elements, size_t *size);

//! Whether the theory atom has a guard.
//!
//! @param[in] theory container where the atom is stored
//! @param[in] atom id of the atom
//! @param[out] has_guard whether the theory atom has a guard
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_atom_has_guard(clingo_theory_base_t const *theory, clingo_id_t atom,
                                                                 bool *has_guard);

//! Get the guard consisting of a theory operator and a theory term of the given theory atom.
//!
//! @param[in] theory container where the atom is stored
//! @param[in] atom id of the atom
//! @param[out] connective the resulting theory operator
//! @param[out] term the resulting term
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_atom_guard(clingo_theory_base_t const *theory, clingo_id_t atom,
                                                             clingo_string_t *connective, clingo_id_t *term);

//! Get the aspif literal associated with the given theory atom.
//!
//! @param[in] theory container where the atom is stored
//! @param[in] atom id of the atom
//! @param[out] literal the resulting literal
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_atom_literal(clingo_theory_base_t const *theory, clingo_id_t atom,
                                                               clingo_literal_t *literal);

//! Get the string representation of the given theory atom.
//!
//! @param[in] theory container where the atom is stored
//! @param[in] atom id of the element
//! @param[in] builder the builder
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_theory_base_atom_to_string(clingo_theory_base_t const *theory, clingo_id_t atom,
                                                                 clingo_string_builder_t *builder);
//! @}

//! Get the base associated with the control object.
//!
//! The base can be used to query the atoms and terms occurring in a program.
//!
//! The function initializes the base struct.
//!
//! @param[in] control the target
//! @param[in] base the base to obtain
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_base(clingo_control_t const *control, clingo_base_t const **base);

//! @}

#ifdef __cplusplus
}
#endif

#endif
