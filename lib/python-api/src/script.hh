#pragma once

#include <clingo/core.h>

#include <pybind11/pybind11.h>

namespace Clingo::Python {

auto register_python(clingo_lib_t *lib) -> bool;

void register_script(pybind11::module &m);

} // namespace Clingo::Python
