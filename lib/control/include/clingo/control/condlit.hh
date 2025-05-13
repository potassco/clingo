#pragma once

#include <clingo/control/context.hh>

#include <clingo/input/literal.hh>

namespace CppClingo::Control {

//! @addtogroup control
//! @{

//! Translate disjunctions.
void build_hd_lit(BuildContext &ctx, Input::HdLitDisjunction const &lit);

//! Translate conditional literals.
void build_bd_lit(BuildContext &ctx, Input::BdLitConjunction const &lit);

//! @}

} // namespace CppClingo::Control
