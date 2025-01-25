#ifndef CLINGO_BASE_H
#define CLINGO_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/core.h>
#include <clingo/symbol.h>

typedef struct clingo_control clingo_control_t;

//! @example symbolic-atoms.c
//! The example shows how to iterate over symbolic atoms.
//!
//! ## Output ##
//!
//! ~~~~~~~~~~~~
//! ./symbolic-atoms 0
//! Symbolic atoms:
//!   b
//!   c, external
//!   a, fact
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @addtogroup c_base
//! Inspection of atoms occurring in ground logic programs.
//!
//! For an example, see @ref symbolic-atoms.c.
//! @{

//! Represents a predicate signature.
//!
//! Signatures have a name and an arity, and can be positive or negative (to
//! represent classical negation).
typedef struct clingo_signature {
    char const *name; //! the name
    size_t arity;     //! the arity
    bool sign;        //! the (classical) sign
} clingo_signature_t;

//! Object to inspect symbolic atoms in a program---the relevant Herbrand base
//! gringo uses to instantiate programs and the terms that occur in the heads
//! of `#show` statements.
//!
//! @see clingo_control_base()
typedef struct clingo_base {
    uintptr_t a; //! internal data
    uintptr_t b; //! internal data
} clingo_base_t;

//! Object to inspect the symbolic atoms in a program.
//!
//! Atom bases capture atoms over the same signature.
typedef struct clingo_atom_base {
    uintptr_t a; //! internal data
    uintptr_t b; //! internal data
} clingo_atom_base_t;

//! Object to inspect the shown terms in a program.
typedef struct clingo_term_base clingo_term_base_t;

//! Get the number of atom bases in the program.
//!
//! Each atom base is associated with a signature.
//!
//! @param[in] atoms the target
//! @param[out] size the number of atoms
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_base_atoms_size(clingo_base_t const *base, size_t *size);

//! Get the signature and atom base at the given index.
//!
//! The index must be smaller than the size reported by clingo_base_atoms_size().
//! Either of the two target pointers can be null if the value is not important.
//!
//! @param[in] base the base
//! @param[in] index the index of the atom base
//! @param[out] signature the target signature
//! @param[out] atoms the target atom base
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_base_atoms_at(clingo_base_t const *base, size_t index,
                                                               clingo_signature_t *signature,
                                                               clingo_atom_base_t *atoms);

//! Find the atom base wit the given signature.
//!
//! @param[in] base the base
//! @param[in] signature the signature to lookup
//! @param[out] atoms the target atom base
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_base_atoms_find(clingo_base_t const *base,
                                                                 clingo_signature_t const *signature,
                                                                 clingo_atom_base_t *atoms);

//! Get the size of the given atom base.
//!
//! @param[in] atoms the atom base
//! @param[out] size the target size
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_atom_base_size(clingo_atom_base_t const *atoms, size_t *size);

//! Find the index of the atom with the given symbol in the atom base.
//!
//! If the symbol is not found, return the size of the atom base.
//!
//! @param atoms the atom base
//! @param symbol the symbol to lookup
//! @param index the target index
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_atom_base_find(clingo_atom_base_t const *atoms, clingo_symbol_t symbol,
                                                                size_t *index);

//! Check whether an atom is a fact.
//!
//! @note This does not determine if an atom is a cautious consequence. The
//! grounding or solving component's simplifications can only detect this in
//! some cases.
//!
//! @param[in] atoms the atom base
//! @param[in] index the index of the atom
//! @param[out] fact whether the atom is a fact
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_atom_base_is_fact(clingo_atom_base_t const *atoms, size_t index,
                                                                   bool *fact);

//! Check whether an atom is external.
//!
//! An atom is external if it has been defined using an external directive and
//! has not been released or defined by a rule.
//!
//! @param[in] atoms the target
//! @param[in] index the index of the atom
//! @param[out] external whether the atom is an external
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_atom_base_is_external(clingo_atom_base_t const *atoms, size_t index,
                                                                       bool *is_external);

//! Get the symbolic representation of an atom.
//!
//! @param[in] atoms the atom base
//! @param[in] index the index of the atom
//! @param[out] symbol the resulting symbol
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_atom_base_symbol(clingo_atom_base_t const *atoms, size_t index,
                                                                  clingo_symbol_t *symbol);

//! Returns the (numeric) program literal corresponding to the given symbolic atom.
//!
//! Such a literal can be mapped to a solver literal (see the @ref c_propagator
//! module) or be used in rules in aspif format (see the @ref c_ast module).
//!
//! @param[in] atoms the atom base
//! @param[in] index the index of the atom
//! @param[out] literal the resulting literal
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_atom_base_literal(clingo_atom_base_t const *atoms, size_t index,
                                                                   clingo_literal_t *literal);

//! Get the term base capturing show term directives.
//!
//! @param bases the base
//! @param terms the term base
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_base_terms(clingo_base_t const *base,
                                                            clingo_term_base_t const **terms);

//! Get the number of shown terms in a program.
//!
//! @param[in] terms the term base
//! @param[out] size the number of terms in the base
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_term_base_size(clingo_term_base_t const *terms, size_t *size);

//! Get the symbol of the show directive with the given index.
//!
//! @param[in] terms the term base
//! @param[in] index the index of the show directive
//! @param[out] term the resulting term
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_term_base_symbol(clingo_term_base_t const *terms, size_t index,
                                                                  clingo_symbol_t *term);

//! Get the condition of a show directive.
//!
//! Note that the literals is set to NULL if the term is shown unconditionally.
//!
//! @param[in] terms the term base
//! @param[in] index the index of the show diriective
//! @param[out] literals the target literals
//! @param[out] size the target size
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_term_base_condition(clingo_term_base_t const *terms, size_t index,
                                                                     clingo_literal_t **literals, size_t *size);

//! Get the index of the given symbol.
//!
//! If the symbol is not found, the index is set to the size of the base.
//!
//! @param[in] terms the term base
//! @param[in] symbol the symbol to lookup
//! @param[out] index the target index
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_term_base_find(clingo_term_base_t const *terms, clingo_symbol_t symbol,
                                                                size_t *index);

//! Get the base associated with the control object.
//!
//! The base can be used to query the atoms and terms occurring in a program.
//!
//! The function initializes the base struct.
//!
//! @param[in] control the target
//! @param[in] base the base to obtain
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_control_base(clingo_control_t const *control, clingo_base_t *base);
//! @}

#ifdef __cplusplus
}
#endif

#endif
