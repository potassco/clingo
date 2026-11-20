#pragma once

#include <Python.h>

#include <clingo/core.h>

namespace PyClingo {

auto register_python(clingo_lib_t *lib) -> bool;

//! Create the clingo.script module with its own types and functions (first registration phase).
auto create_script(PyObject *parent) -> PyObject *;
//! Store the types of other modules in the state of the clingo.script module (second registration phase).
auto link_script(PyObject *mod) -> bool;

} // namespace PyClingo
