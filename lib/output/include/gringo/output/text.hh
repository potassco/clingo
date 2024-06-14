#pragma once

#include <gringo/core/output.hh>

namespace Gringo::Output {

//! @addtogroup output
//! @{

//! Create a text output.
auto make_text_output(std::ostream &out) -> UOutputStm;

//! @}

} // namespace Gringo::Output
