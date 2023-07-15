#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

auto check_type(TermV2 const &term, TermCheckType type, CheckTypeResult *res = nullptr) -> bool;

}
