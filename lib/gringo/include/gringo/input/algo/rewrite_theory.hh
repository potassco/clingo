#pragma once

#include <gringo/input/program.hh>

namespace Gringo::Input {

class Theory;

auto rewrite_theory(Theory const &thy, Stm const &stm) -> std::optional<Stm>;

} // namespace Gringo::Input
