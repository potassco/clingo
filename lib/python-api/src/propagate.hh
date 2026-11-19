#pragma once

#include <clingo/control.h>
#include <clingo/propagate.h>

#include <pybind11/pybind11.h>

namespace PyClingo {

//! Get the `clingo.propagate.Propagator` type (new reference).
auto py_propagator_type() -> PyObject *;

//! Register a propagator with the given control object.
//!
//! The type must be the one returned by `py_propagator_type` and the
//! propagator must be an instance of it. A handle owning the propagator and
//! the library is appended to the keeper list, which has to live as long as
//! the control object because clingo calls back into the handle. Returns
//! false with a Python error set on failure.
auto py_register_propagator(PyObject *type, clingo_control_t *ctl, PyObject *keeper, PyObject *library,
                            PyObject *propagator) -> bool;

void register_propagate(pybind11::module &m);

} // namespace PyClingo
