#pragma once

#include <pybind11/pybind11.h>

namespace PyClingo {

void register_theory(pybind11::module &m);

}
