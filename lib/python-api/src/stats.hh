#pragma once

#include "clingo/stats.h"

#include <pybind11/pybind11.h>

namespace Clingo::Python {

namespace py = pybind11;

enum class StatsType : uint8_t {
    value = clingo_stats_type_value,
    array = clingo_stats_type_array,
    map = clingo_stats_type_map,
};

class Stats;

class StatsArray {
  public:
    StatsArray(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}
    void set(size_t index, py::handle value);
    auto get(size_t index) -> Stats;
    void append(py::handle value);
    auto len() -> size_t;

  private:
    clingo_stats_t *stats_;
    uint64_t key_;
};

class StatsMap {
  public:
    StatsMap(clingo_stats_t *stats, uint64_t key) : stats_{stats}, key_{key} {}
    auto get(char const *name) -> Stats;
    void set(char const *name, py::handle value);
    auto len() -> size_t;
    auto contains(char const *name) -> bool;

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
    auto nestify() -> py::object;
    void update(py::handle value) { update_(value, false); }
    void update_(py::handle value, bool init);

  private:
    clingo_stats_t *stats_;
    uint64_t key_;
};

void register_stats(pybind11::module &m);

} // namespace Clingo::Python
