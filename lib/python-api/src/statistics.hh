#pragma once

#include "clingo/statistics.h"

#include <pybind11/pybind11.h>

namespace Clingo::Python {

auto as_dict(clingo_statistics_t *stats) -> pybind11::dict;

void register_statistics(pybind11::module &m);

} // namespace Clingo::Python
