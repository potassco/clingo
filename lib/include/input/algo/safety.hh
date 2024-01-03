#pragma once

#include <util/optional.hh>

#include <input/program.hh>

namespace Gringo::Input {

[[nodiscard]] auto check_safety(Logger &log, Statement const &stm) -> Util::ResultState<Statement>;

}
