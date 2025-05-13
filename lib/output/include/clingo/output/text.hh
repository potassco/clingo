#pragma once

#include <clingo/core/output.hh>

namespace CppClingo::Output {

//! @addtogroup output
//! @{

//! Create a text output.
auto make_text_output(Util::OutputBuffer &out) -> UOutputStm;

//! @}

} // namespace CppClingo::Output
