#pragma once

#include <pybind11/pybind11.h>

#include "symbol.hh"
#include <clingo/base.h>

namespace PyClingo {

namespace py = pybind11;

void register_base(py::module &m);

auto py_base_type() -> PyTypeObject *;
auto py_base_new(PyTypeObject *type, const clingo_base_t *base) -> PyObject *;

class Base;

using MixedLitSpan = std::span<std::variant<std::pair<Symbol, bool>, Lit_t>>;
auto convert(TypeHint<"clingo.base.Base"> base, MixedLitSpan const &lits, bool flip, bool disjunctive) -> LitVec;

} // namespace PyClingo
