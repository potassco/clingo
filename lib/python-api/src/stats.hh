#pragma once

#include <clingo/stats.h>

#include "iterable.hh"

#include <pybind11/pybind11.h>

#include <cstdint>

namespace PyClingo {

using Stats = TypeHint<"clingo.stats.Stats">;
using StatsView = TypeHint<"clingo.stats.StatsView">;

enum class StatsType : uint8_t {
    value = clingo_stats_type_value,
    array = clingo_stats_type_array,
    map = clingo_stats_type_map,
};

void register_stats(pybind11::module &m);

auto py_stats_type() -> PyObject *;
auto py_stats_view_type() -> PyObject *;
//! Create a stats object (new reference).
//!
//! The owner is kept alive by the stats object. If valid is not nullptr, it is
//! the address of a pointer of the owner that is set to null once the owner is
//! no longer valid; accessing the stats then raises an error.
auto py_stats_new(PyObject *owner, void const *valid, PyObject *type, clingo_stats_t *stats, uint64_t key)
    -> PyObject *;
auto py_stats_get(PyObject *type, PyObject *obj, clingo_stats_t **stats, uint64_t *key) -> bool;

} // namespace PyClingo
