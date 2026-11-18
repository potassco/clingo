#pragma once

#include <pybind11/pybind11.h>

#include "symbol.hh"
#include <clingo/base.h>

namespace PyClingo {

namespace py = pybind11;

void register_base(py::module &m);

auto py_base_type() -> PyObject *;
//! Create a `clingo.base.Base` object (new reference).
//!
//! The owner is kept alive by the base object. If valid is not nullptr, it is
//! the address of a pointer of the owner that is set to null once the owner is
//! no longer valid; accessing the base then raises an error.
auto py_base_new(PyObject *owner, void *const *valid, PyObject *type, clingo_base_t const *base) -> PyObject *;
auto py_base_get(PyObject *type, PyObject *obj) -> clingo_base_t const *;

class Base;

using MixedLitSpan = std::span<std::variant<std::pair<Symbol, bool>, Lit_t>>;
auto convert(TypeHint<"clingo.base.Base"> base, MixedLitSpan const &lits, bool flip, bool disjunctive) -> LitVec;
auto convert(PyObject *base_type, PyObject *base, PyObject *lits, bool flip, bool disjunctive,
             PyBuffer<clingo_literal_t> &out) -> bool;

} // namespace PyClingo
