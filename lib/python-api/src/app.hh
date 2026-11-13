#pragma once

#include <clingo/app.h>

#include <pybind11/pybind11.h>

namespace PyClingo {

namespace py = pybind11;

auto py_app_options_type_ref() -> PyObject *;
auto py_app_options_check(PyObject *type, PyObject *obj) -> clingo_options_t *;

void register_app(pybind11::module &m);

} // namespace PyClingo
