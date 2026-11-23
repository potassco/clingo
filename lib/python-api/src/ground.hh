#pragma once

#include <Python.h>

#include <clingo/control.h>
#include <clingo/ground.h>

namespace PyClingo {

auto py_ground_handle_type() -> PyObject *;
auto py_ground(PyObject *type, clingo_control_t *ctl, clingo_part_t const *parts, size_t size, PyObject *ctx) -> bool;
auto py_ground_start(PyObject *type, clingo_control_t *ctl, PyObject *owner, clingo_part_t const *parts, size_t size,
                     PyObject *ctx, PyObject *finish) -> PyObject *;
//! Start grounding in the background and return a new `clingo.ground.AsyncGroundHandle`.
//!
//! The type must be the one returned by `py_ground_handle_type`. The context
//! can be Py_None and its functions are called on the grounding thread. The
//! event loop is used to resolve the future awaited via the handle. Returns
//! nullptr with a Python error set on failure.
auto py_ground_async(PyObject *type, clingo_control_t *ctl, PyObject *owner, clingo_part_t const *parts, size_t size,
                     PyObject *ctx, PyObject *loop) -> PyObject *;

//! Create the clingo.ground module with its own types and functions (first registration phase).
auto create_ground(PyObject *parent) -> PyObject *;
//! Store the types of other modules in the state of the clingo.ground module (second registration phase).
auto link_ground(PyObject *mod) -> bool;

} // namespace PyClingo
