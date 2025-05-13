#pragma once

#include <clingo/control/context.hh>

namespace CppClingo::Control {

//! @addtogroup control
//! @{

//! Translate input statements to their ground representation.
void build_stm(BuildContext &ctx, Input::Stm const &stm);

//! @}

} // namespace CppClingo::Control
