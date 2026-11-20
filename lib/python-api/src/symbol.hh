#pragma once

#include "core.hh"

#include <clingo/symbol.h>

#include <pybind11/pybind11.h>

namespace PyClingo {

void register_symbol(pybind11::module &m);

auto py_symbol_type() -> PyObject *;
auto py_symbol_new(PyObject *type, clingo_symbol_t sym, bool acquire) -> PyObject *;
auto py_symbol_get(PyObject *type, PyObject *obj, clingo_symbol_t *sym) -> bool;
auto py_symbol_check(PyObject *type, PyObject *obj, clingo_symbol_t *sym) -> int;

} // namespace PyClingo
