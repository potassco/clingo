#pragma once

#include <gringo/grounder/context.hh>

#include <gringo/input/theory.hh>

namespace Gringo::Grounder {

//! Translate a head theory atom.
void build_hd_lit(BuildContext &ctx, Input::HdLitTheoryAtom const &lit);

//! Translate a body theory atom.
void build_bd_lit(BuildContext &ctx, Input::BdLitTheoryAtom const &lit);

} // namespace Gringo::Grounder
