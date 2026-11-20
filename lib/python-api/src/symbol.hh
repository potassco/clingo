#pragma once

#include "core.hh"

#include <clingo/symbol.h>

namespace PyClingo {

//! Create the clingo.symbol module with its own types and functions (first registration phase).
auto create_symbol(PyObject *parent) -> PyObject *;
//! Store the types of other modules in the state of the clingo.symbol module (second registration phase).
auto link_symbol(PyObject *mod) -> bool;

auto py_symbol_type() -> PyObject *;
auto py_symbol_new(PyObject *type, clingo_symbol_t sym, bool acquire) -> PyObject *;
auto py_symbol_get(PyObject *type, PyObject *obj, clingo_symbol_t *sym) -> bool;
auto py_symbol_check(PyObject *type, PyObject *obj, clingo_symbol_t *sym) -> int;

} // namespace PyClingo
