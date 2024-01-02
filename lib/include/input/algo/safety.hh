#pragma once

#include <input/program.hh>

namespace Gringo::Input {

[[nodiscard]] auto check_safety(Statement const &stm) -> bool;

}
