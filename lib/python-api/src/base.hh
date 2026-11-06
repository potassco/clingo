#pragma once

#include <pybind11/pybind11.h>

#include "symbol.hh"
#include <clingo/base.h>

namespace PyClingo {

namespace py = pybind11;

void register_base(py::module &m);

auto py_base_type() -> PyObject *;
auto py_base_new(PyObject *type, clingo_base_t const *base) -> PyObject *;
auto py_base_get(PyObject *type, PyObject *obj) -> clingo_base_t const *;

class Base;

using MixedLitSpan = std::span<std::variant<std::pair<Symbol, bool>, Lit_t>>;
auto convert(TypeHint<"clingo.base.Base"> base, MixedLitSpan const &lits, bool flip, bool disjunctive) -> LitVec;

} // namespace PyClingo
