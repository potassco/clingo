#pragma once

#include <gringo/core/output.hh>

//! @addtogroup output
//! @{

namespace Gringo::Output {

//! Create a text output.
auto make_text_output(std::ostream &out) -> UOutputStm;

//! @}

} // namespace Gringo::Output
