#pragma once

#include "clingo/stats.h"

#include <pybind11/pybind11.h>

namespace Clingo::Python {

namespace py = pybind11;

enum class StatsType : uint8_t {
    value = clingo_stats_type_value,
    array = clingo_stats_type_array,
    map = clingo_stats_type_map,
    empty = clingo_stats_type_empty,
};

class Stats;

class StatsArray {
  public:
    StatsArray(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}
    auto len() -> size_t;

  private:
    clingo_stats_t *stats_;
    uint64_t key_;
};

class StatsMap {
  public:
    StatsMap(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}
    auto len() -> size_t;

  private:
    clingo_stats_t *stats_;
    uint64_t key_;
};

class Stats {
  public:
    Stats(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}

    auto type() -> StatsType;
    auto array() -> StatsArray;
    auto map() -> StatsMap;
    auto get_value() -> double;
    void set_value(double value);
    auto as_dict() -> py::dict;

  private:
    clingo_stats_t *stats_;
    uint64_t key_;
};

void register_stats(pybind11::module &m);

} // namespace Clingo::Python
