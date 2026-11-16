#pragma once

#include <clingo/control.h>
#include <clingo/ground.h>

#include <pybind11/pybind11.h>

namespace PyClingo {

auto py_ground_handle_type() -> PyObject *;
auto py_ground(PyObject *type, clingo_control_t *ctl, clingo_part_t const *parts, size_t size, PyObject *ctx) -> bool;
auto py_ground_start(PyObject *type, clingo_control_t *ctl, PyObject *owner, clingo_part_t const *parts, size_t size,
                     PyObject *ctx, PyObject *finish) -> PyObject *;

void register_ground(pybind11::module &m);

} // namespace PyClingo
