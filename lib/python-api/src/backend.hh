#pragma once

#include <clingo/backend.h>
#include <clingo/observe.h>

#include <pybind11/pybind11.h>

namespace PyClingo {

auto py_backend_manager_type_ref() -> PyObject *;
auto py_backend_manager_new(PyObject *type, clingo_control_t *ctl, PyObject *owner) -> PyObject *;
auto py_observe(PyObject *module, clingo_control_t *ctl, PyObject *observer, bool preprocess) -> bool;
void register_backend(pybind11::module &m);

} // namespace PyClingo
