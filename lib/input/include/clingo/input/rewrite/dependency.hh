#pragma once

#include <clingo/input/program.hh>

#include <clingo/util/enum.hh>
#include <clingo/util/ordered_map.hh>

namespace CppClingo::Input {

//! @addtogroup input_rewrite
//! @{

// examples:
// - h :- a : b, not c; d; not e.
//   - depend:
//     - positive: a, d
//     - negative: b, c, e
//   - provide: h
// - h :- { a; b } >= 1.
//   - depend:
//     - positive: a, b
//   - provide: h
// - h :- { a; b } <= 1.
//   - depend:
//     - negative: a, b
//   - provide: h
// - h :- { a; b } != 1.
//   - depend:
//     - positive: a, b
//     - negative: a, b
//   - provide: h
// - {h} :- a.
//   - depend:
//     - positive: a
//     - negative: h
//   - provide: h
// - g | h :- a.
//   - depend:
//     - positive: a
//     - negative: g, h
//   - provide: g, h

// graph:
// - n: number of statements
// - m: number of atoms in depends
// - nodes: [0,n) + [n,n+m)
// - statement depends on atoms occurring in it (either positively or negatively)
// - component graph uses both dependencies
// - refined component graph only considers positive ones

//! Analyze the given statements organizing them in components for grounding.
auto analyze(SymbolStore &store, std::vector<Stm> const &stms, SourceVec *srcs) -> Components;

//! Analyze the given statement adding provided and dependent predicates.
void analyze(Stm const &stm, SharedSigSet &provide, Util::ordered_map<SharedSig, Location> &depend);

//! Try to unify the two terms.
//!
//! The function also accepts terms with arithmetic terms,
//! which except for special cases are assumed to unify with anything that is not a function or tuple.
//! The store object is required to add auxiliary symbols during unification.
auto unify(SymbolStore &store, Term const &a, Term const &b) -> bool;

//! Output components in the dot language for visualization.
//!
//! This function is intended for debugging.
void visualize(Components const &comps, std::ostream &out);

//! @}

} // namespace CppClingo::Input
