#pragma once

#include <clingo/propagate.h>

#include <pybind11/pybind11.h>

#include "core.hh"

namespace PyClingo {

class PropagateInit;
class PropagateControl;
class Assignment;

class Propagator {
  public:
    void init(PropagateInit &init);
    void propagate(PropagateControl &ctl, LitSpan changes);
    void undo(uint32_t thread_id, Assignment &assignment, LitSpan changes);
    void check(PropagateControl &ctl);
    auto decide(uint32_t thread_id, Assignment &assignment, clingo_literal_t lit) -> clingo_literal_t;

  private:
    template <class... Args> void no_op([[maybe_unused]] Args const &...args) {}

    auto decide_([[maybe_unused]] uint32_t thread_id, [[maybe_unused]] Assignment &assignment,
                 [[maybe_unused]] clingo_literal_t lit) -> clingo_literal_t;
};

//! Register a proagator with the given control object.
//!
//! The given data reference should be stored in the Control wrapper class.
//! It's exception element is used to store exceptions thrown in C callbacks
//! and rethrow them in their calling scope.
//!
//! @param ctl the control object to register the propagator with
//! @param data an exception pointer together with a propagator implementation
void register_propagator(clingo_control_t *ctl, Propagator &propagator);

void register_propagate(pybind11::module &m);

} // namespace PyClingo
