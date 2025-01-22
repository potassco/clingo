#include <clingo/base.h>

// Note: placeholder for later
//

/*
//! Object to inspect symbolic atoms in a program---the relevant Herbrand base
//! gringo uses to instantiate programs.
//!
//! @see clingo_control_symbolic_atoms()
typedef struct clingo_symbolic_atoms clingo_symbolic_atoms_t;
//! Object to iterate over symbolic atoms.
//!
//! Such an iterator either points to a symbolic atom within a sequence of
//! symbolic atoms or to the end of the sequence.
//!
//! @note Iterators are valid as long as the underlying sequence is not modified.
//! Operations that can change this sequence are ::clingo_control_ground(),
//! ::clingo_control_cleanup(), and functions that modify the underlying
//! non-ground program.
typedef uint64_t clingo_symbolic_atom_iterator_t;
//! Get the number of different atoms occurring in a logic program.
//!
//! @param[in] atoms the target
//! @param[out] size the number of atoms
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_size(clingo_symbolic_atoms_t const *atoms,
                                                                     size_t *size);
//! Get a forward iterator to the beginning of the sequence of all symbolic
//! atoms optionally restricted to a given signature.
//!
//! @param[in] atoms the target
//! @param[in] signature optional signature
//! @param[out] iterator the resulting iterator
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_begin(clingo_symbolic_atoms_t const *atoms,
                                                                      clingo_signature_t const *signature,
                                                                      clingo_symbolic_atom_iterator_t *iterator);
//! Iterator pointing to the end of the sequence of symbolic atoms.
//!
//! @param[in] atoms the target
//! @param[out] iterator the resulting iterator
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_end(clingo_symbolic_atoms_t const *atoms,
                                                                    clingo_symbolic_atom_iterator_t *iterator);
//! Find a symbolic atom given its symbolic representation.
//!
//! @param[in] atoms the target
//! @param[in] symbol the symbol to lookup
//! @param[out] iterator iterator pointing to the symbolic atom or to the end
//! of the sequence if no corresponding atom is found
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_find(clingo_symbolic_atoms_t const *atoms,
                                                                     clingo_symbol_t symbol,
                                                                     clingo_symbolic_atom_iterator_t *iterator);
//! Check if two iterators point to the same element (or end of the sequence).
//!
//! @param[in] atoms the target
//! @param[in] a the first iterator
//! @param[in] b the second iterator
//! @param[out] equal whether the two iterators are equal
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t
clingo_symbolic_atoms_iterator_is_equal_to(clingo_symbolic_atoms_t const *atoms, clingo_symbolic_atom_iterator_t a,
                                           clingo_symbolic_atom_iterator_t b, bool *equal);
//! Get the symbolic representation of an atom.
//!
//! @param[in] atoms the target
//! @param[in] iterator iterator to the atom
//! @param[out] symbol the resulting symbol
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_symbol(clingo_symbolic_atoms_t const *atoms,
                                                                       clingo_symbolic_atom_iterator_t iterator,
                                                                       clingo_symbol_t *symbol);
//! Check whether an atom is a fact.
//!
//! @note This does not determine if an atom is a cautious consequence. The
//! grounding or solving component's simplifications can only detect this in
//! some cases.
//!
//! @param[in] atoms the target
//! @param[in] iterator iterator to the atom
//! @param[out] fact whether the atom is a fact
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_is_fact(clingo_symbolic_atoms_t const *atoms,
                                                                        clingo_symbolic_atom_iterator_t iterator,
                                                                        bool *fact);
//! Check whether an atom is external.
//!
//! An atom is external if it has been defined using an external directive and
//! has not been released or defined by a rule.
//!
//! @param[in] atoms the target
//! @param[in] iterator iterator to the atom
//! @param[out] external whether the atom is an external
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_is_external(clingo_symbolic_atoms_t const *atoms,
                                                                            clingo_symbolic_atom_iterator_t iterator,
                                                                            bool *external);
//! Returns the (numeric) aspif literal corresponding to the given symbolic atom.
//!
//! Such a literal can be mapped to a solver literal (see the \ref Propagator
//! module) or be used in rules in aspif format (see the \ref ProgramBuilder
//! module).
//!
//! @param[in] atoms the target
//! @param[in] iterator iterator to the atom
//! @param[out] literal the associated literal
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_literal(clingo_symbolic_atoms_t const *atoms,
                                                                        clingo_symbolic_atom_iterator_t iterator,
                                                                        clingo_literal_t *literal);
//! Get the number of different predicate signatures used in the program.
//!
//! @param[in] atoms the target
//! @param[out] size the number of signatures
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_signatures_size(clingo_symbolic_atoms_t const *atoms,
                                                                                size_t *size);
//! Get the predicate signatures occurring in a logic program.
//!
//! @param[in] atoms the target
//! @param[out] signatures the resulting signatures
//! @param[in] size the number of signatures
//! @return the result code
//!
//! @see clingo_symbolic_atoms_signatures_size()
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_signatures(clingo_symbolic_atoms_t const *atoms,
                                                                           clingo_signature_t *signatures, size_t size);
//! Get an iterator to the next element in the sequence of symbolic atoms.
//!
//! @param[in] atoms the target
//! @param[in] iterator the current iterator
//! @param[out] next the succeeding iterator
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_next(clingo_symbolic_atoms_t const *atoms,
                                                                     clingo_symbolic_atom_iterator_t iterator,
                                                                     clingo_symbolic_atom_iterator_t *next);
//! Check whether the given iterator points to some element with the sequence
//! of symbolic atoms or to the end of the sequence.
//!
//! @param[in] atoms the target
//! @param[in] iterator the iterator
//! @param[out] valid whether the iterator points to some element within the
//! sequence
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_symbolic_atoms_is_valid(clingo_symbolic_atoms_t const *atoms,
                                                                         clingo_symbolic_atom_iterator_t iterator,
                                                                         bool *valid);

*/
