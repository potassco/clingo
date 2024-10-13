#pragma once

#include <pybind11/pybind11.h>

namespace Clingo::Python {

namespace py = pybind11;

void register_ast(pybind11::module &m);

} // namespace Clingo::Python
