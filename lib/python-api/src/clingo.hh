#pragma once

#include <pybind11/pybind11.h>

namespace Clingo::Python {

void register_clingo(pybind11::module &m);

}
