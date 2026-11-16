#pragma once

#include <clingo/control.h>
#include <clingo/solve.h>

#include <pybind11/pybind11.h>

#include <span>

namespace PyClingo {

//! Get the `clingo.solve.SolveHandle` type (new reference).
auto py_solve_handle_type() -> PyObject *;

//! Get the `clingo.solve.Model` type (new reference).
auto py_model_type() -> PyObject *;

//! Create a `clingo.solve.Model` of the given type viewing the given model.
//!
//! The model is only valid as long as clingo keeps it alive (e.g., during a
//! model callback). Returns a new reference or nullptr with a Python error set.
auto py_model_new(PyObject *type, clingo_model_t const *model, bool is_mutable) -> PyObject *;

//! Get the clingo model wrapped by a `clingo.solve.Model` object.
//!
//! Returns nullptr with a Python error set if the object is not a model of the
//! given type or if the model has been cleared.
auto py_model_get(PyObject *type, PyObject *obj, bool mut) -> clingo_model_t *;

//! Detach a `clingo.solve.Model` object from its clingo model.
//!
//! Further access from Python raises a RuntimeError. The object must be a
//! model created by `py_model_new`.
void py_model_clear(PyObject *obj);

//! Start a solve call and return a new `clingo.solve.SolveHandle`.
//!
//! The type must be the one returned by `py_solve_handle_type`. The callbacks
//! can be nullptr or Py_None. The handle takes new references to the owner and
//! the callbacks. Returns nullptr with a Python error set on failure.
auto py_solve_start(PyObject *type, clingo_control_t *ctl, PyObject *owner,
                    std::span<clingo_literal_t const> assumptions, PyObject *on_model, PyObject *on_unsat,
                    PyObject *on_stats, PyObject *on_finish, bool yield, bool async) -> PyObject *;

//! Solve to completion and return a new `clingo.solve.SolveResult`.
//!
//! Equivalent to `with start_solve(...) as hnd: return hnd.get()`. The type
//! must be the one returned by `py_solve_handle_type`. Returns nullptr with a
//! Python error set on failure.
auto py_solve(PyObject *type, clingo_control_t *ctl, PyObject *owner, std::span<clingo_literal_t const> assumptions,
              PyObject *on_model, PyObject *on_unsat, PyObject *on_stats, PyObject *on_finish) -> PyObject *;

void register_solve(pybind11::module &m);

} // namespace PyClingo
