#pragma once

#include <clingo/app.h>

#include <pybind11/pybind11.h>

namespace PyClingo {

namespace py = pybind11;

auto convert_options(py::handle hnd) -> clingo_options_t *;

void register_app(pybind11::module &m);

} // namespace PyClingo
