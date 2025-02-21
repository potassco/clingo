#pragma once

#include <clingo/backend.h>

#include <pybind11/pybind11.h>

#include "symbol.hh"

namespace Clingo::Python {

class Backend {
  public:
    Backend(clingo_backend_t *backend) : backend_{backend} {}
    auto atom(std::optional<Symbol> symbol) -> clingo_atom_t;
    void rule(AtomSpan head, LitSpan body, bool choice);
    void weight_rule(AtomSpan head, clingo_weight_t lower, WeightLitSpan body, bool choice);

  private:
    clingo_backend_t *backend_;
};

class BackendManager {
  public:
    BackendManager(clingo_control_t *ctl) : ctl_{ctl} {}

    auto enter() -> Backend;
    void exit(std::optional<pybind11::type> const &type, std::optional<pybind11::object> const &value,
              std::optional<pybind11::object> const &traceback);

  private:
    clingo_control_t *ctl_;
    clingo_backend_t *backend_ = nullptr;
};

void register_backend(pybind11::module &m);

} // namespace Clingo::Python
