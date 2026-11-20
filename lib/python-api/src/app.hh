#pragma once

#include <Python.h>

#include <clingo/app.h>

namespace PyClingo {

auto py_app_options_type() -> PyObject *;
auto py_app_options_check(PyObject *type, PyObject *obj) -> clingo_options_t *;

//! Create the clingo.app module with its own types and functions (first registration phase).
auto create_app(PyObject *parent) -> PyObject *;
//! Store the types of other modules in the state of the clingo.app module (second registration phase).
auto link_app(PyObject *mod) -> bool;

} // namespace PyClingo
