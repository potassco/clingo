#pragma once

#include <gringo/control/context.hh>

#include <gringo/input/literal.hh>

namespace Gringo::Control {

//! Translate disjunctions.
void build_hd_lit(BuildContext &ctx, Input::HdLitDisjunction const &lit);

//! Translate conditional literals.
void build_bd_lit(BuildContext &ctx, Input::BdLitConjunction const &lit);

} // namespace Gringo::Control
