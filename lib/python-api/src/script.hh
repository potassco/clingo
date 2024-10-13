#pragma once

#include <pybind11/pybind11.h>

namespace Clingo::Python {

void register_script(pybind11::module &m);

}
