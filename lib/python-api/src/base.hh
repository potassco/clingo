#pragma once

#include <clingo/base.h>

#include <pybind11/pybind11.h>

namespace Clingo::Python {

class AtomBase {
  public:
    AtomBase(clingo_atom_base_t base) : base_{base} {}

    auto size() -> size_t;

  private:
    clingo_atom_base_t base_;
};

class Base {
  public:
    using value_type = std::pair<std::tuple<std::string, size_t, bool>, AtomBase>;

    Base(clingo_base_t base) : base_{base} {}

    auto size() -> size_t;
    auto at(size_t index) -> value_type;
    auto at(std::tuple<char const *, size_t, bool> sig) -> AtomBase;

  private:
    clingo_base_t base_;
};

void register_base(pybind11::module &m);

} // namespace Clingo::Python
