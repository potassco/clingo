#pragma once

#include <gringo/core/output.hh>

namespace Gringo::Output {

//! @addtogroup output
//! @{

//! Create a text output.
auto make_text_output(Util::OutputBuffer &out) -> UOutputStm;

//! @}

} // namespace Gringo::Output
