#pragma once

#include <util/optional.hh>

#include <input/program.hh>

namespace Gringo::Input {

[[nodiscard]] auto check_safety(Statement const &stm) -> Util::ResultState<Statement>;

}
