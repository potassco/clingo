#pragma once

#include <clingo/propagate.h>

#include <pybind11/pybind11.h>

#include "base.hh"
#include "symbol.hh"

namespace Clingo::Python {

void register_propagate(pybind11::module &m);

} // namespace Clingo::Python
