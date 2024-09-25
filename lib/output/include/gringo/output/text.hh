#pragma once

#include <gringo/core/output.hh>

namespace Gringo::Output {

//! @addtogroup output
//! @{

//! Create a text output.
auto make_text_output(FILE *out) -> UOutputStm;

//! Create a text output.
auto make_text_output(std::string &out) -> UOutputStm;

//! Create a text output.
auto make_text_output(std::vector<char> &out) -> UOutputStm;

//! @}

} // namespace Gringo::Output
