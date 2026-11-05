#pragma once

#include <pybind11/pybind11.h>

#include "symbol.hh"
#include <clingo/base.h>

namespace PyClingo {

namespace py = pybind11;

void register_base(py::module &m);

auto py_base_type() -> PyTypeObject *;
auto py_base_from_clingo_object(clingo_base_t const *base) -> PyObject *;

class Base;

using MixedLitSpan = std::span<std::variant<std::pair<Symbol, bool>, Lit_t>>;
auto convert(TypeHint<"Base"> base, MixedLitSpan const &lits, bool flip, bool disjunctive) -> LitVec;

} // namespace PyClingo
