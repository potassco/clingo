#pragma once

#include <gringo/control/context.hh>

#include <gringo/input/body_literal.hh>
#include <gringo/input/head_literal.hh>

namespace Gringo::Control {

//! Translate head aggregates.
void build_hd_lit(BuildContext &ctx, Input::HdLitAggregate const &lit);

//! Translate body aggregates.
void build_bd_lit(BuildContext &ctx, Input::BdLitAggregate const &lit);

} // namespace Gringo::Control
