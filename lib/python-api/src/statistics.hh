#pragma once

#include <pybind11/pybind11.h>

namespace Clingo::Python {

void register_statistics(pybind11::module &m);

} // namespace Clingo::Python
