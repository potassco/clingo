#pragma once

#include <clingo/control/context.hh>

#include <clingo/input/theory.hh>

namespace CppClingo::Control {

//! @addtogroup control
//! @{

//! Translate a head theory atom.
void build_hd_lit(BuildContext &ctx, Input::HdLitTheoryAtom const &lit, Ground::ProfileNodeInternal *node);

//! Translate a body theory atom.
void build_bd_lit(BuildContext &ctx, Input::BdLitTheoryAtom const &lit, Ground::ProfileNodeInternal *node);

//! @}

} // namespace CppClingo::Control
