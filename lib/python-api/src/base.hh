#pragma once

#include <clingo/base.h>

#include <pybind11/pybind11.h>

namespace Clingo::Python {

class Base {
  public:
    Base(clingo_base_t base) : base_{base} {}

    auto size() -> size_t;

  private:
    clingo_base_t base_;
};

void register_base(pybind11::module &m);

} // namespace Clingo::Python
