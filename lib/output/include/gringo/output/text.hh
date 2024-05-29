#pragma once

#include <gringo/core/output.hh>

namespace Gringo::Output {

auto make_text_output(std::ostream &out) -> UOutputStm;

}
