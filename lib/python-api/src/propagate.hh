#pragma once

#include <clingo/propagate.h>

#include <pybind11/pybind11.h>

namespace Clingo::Python {

class Propagator;

void register_propagate(pybind11::module &m);

} // namespace Clingo::Python
