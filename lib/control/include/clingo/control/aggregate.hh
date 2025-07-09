#pragma once

#include <clingo/control/context.hh>

#include <clingo/input/body_literal.hh>
#include <clingo/input/head_literal.hh>

namespace CppClingo::Control {

//! @addtogroup control
//! @{

//! Translate head aggregates.
void build_hd_lit(BuildContext &ctx, Input::HdLitAggregate const &lit, Ground::ProfileNodeInternal *node);

//! Translate body aggregates.
void build_bd_lit(BuildContext &ctx, Input::BdLitAggregate const &lit);

//! @}

} // namespace CppClingo::Control
