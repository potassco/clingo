#pragma once

#include <Python.h>

#include <clingo/backend.h>
#include <clingo/observe.h>

namespace PyClingo {

auto py_backend_manager_type() -> PyObject *;
auto py_backend_manager_new(PyObject *type, clingo_control_t *ctl, PyObject *owner) -> PyObject *;
auto py_observe(PyObject *module, clingo_control_t *ctl, PyObject *observer, bool preprocess) -> bool;
//! Create the clingo.backend module with its own types and functions (first registration phase).
auto create_backend(PyObject *parent) -> PyObject *;
//! Store the types of other modules in the state of the clingo.backend module (second registration phase).
auto link_backend(PyObject *mod) -> bool;

} // namespace PyClingo
