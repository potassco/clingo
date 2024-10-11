#pragma once

#include <pybind11/pybind11.h>

namespace Clingo::AST2 {

void register_module(pybind11::module &m);

} // namespace Clingo::AST2
