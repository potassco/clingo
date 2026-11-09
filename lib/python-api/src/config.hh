#pragma once

#include <clingo/config.h>

#include <pybind11/pybind11.h>

namespace PyClingo {

auto py_config_type_ref() -> PyObject *;
auto py_config_new(PyObject *type, PyObject *keeper, clingo_config_t *config, clingo_id_t key) -> PyObject *;
auto py_config_get(PyObject *type, PyObject *obj, clingo_config_t **config, clingo_id_t *key) -> bool;
void register_config(pybind11::module &m);

} // namespace PyClingo
