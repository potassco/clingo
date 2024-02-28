#pragma once

#include <pybind11/pybind11.h>

namespace Clingo::AST {

namespace py = pybind11;

void register_module(pybind11::module &m);

} // namespace Clingo::AST
