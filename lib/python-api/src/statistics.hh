#pragma once

#include "clingo/statistics.h"

#include <pybind11/pybind11.h>

namespace Clingo::Python {

namespace py = pybind11;

class Statistics {
  public:
    Statistics(clingo_statistics_t *stats, uint64_t key) : stats_{stats}, key_{key} {}

    auto as_dict() -> py::dict;

  private:
    clingo_statistics_t *stats_;
    uint64_t key_;
};

void register_statistics(pybind11::module &m);

} // namespace Clingo::Python
