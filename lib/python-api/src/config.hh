#pragma once

#include <Python.h>

#include <clingo/config.h>

namespace PyClingo {

auto py_config_type() -> PyObject *;
//! Create a config object (new reference).
//!
//! The owner and keeper are kept alive by the config object. Callbacks
//! registered via the config are added to the keeper. If valid is not
//! nullptr, it is the address of a pointer of the owner that is set to null
//! once the owner is no longer valid (see pointer_slot()); accessing the config
//! then raises an error.
auto py_config_new(PyObject *owner, void const *valid, PyObject *keeper, PyObject *type, clingo_config_t *config,
                   clingo_id_t key) -> PyObject *;
auto py_config_get(PyObject *type, PyObject *obj, clingo_config_t **config, clingo_id_t *key) -> bool;
//! Create the clingo.config module with its own types and functions (first registration phase).
auto create_config(PyObject *parent) -> PyObject *;

} // namespace PyClingo
