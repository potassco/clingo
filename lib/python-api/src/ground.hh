#pragma once

#include "util.hh"

#include "clingo/ground.h"

#include <pybind11/pybind11.h>

namespace PyClingo {

using GroundFinishCallback = std::function<void(clingo_ground_result_e)>;

class GroundHandle : public reference_keeper<GroundHandle> {
  public:
    GroundHandle() = default;
    GroundHandle(GroundHandle const &other) = delete;
    GroundHandle(GroundHandle &&other) noexcept = delete;
    auto operator=(GroundHandle const &other) -> GroundHandle & = delete;
    auto operator=(GroundHandle &&other) noexcept -> GroundHandle & = delete;
    ~GroundHandle() noexcept(false) { close(); }

    auto get() -> clingo_ground_result_e;
    void cancel();
    auto wait(std::optional<double> timeout) -> bool;
    void close();

    auto handle() -> clingo_ground_handle_t *& { return hnd_; }

  private:
    friend class Control;

    clingo_ground_handle_t *hnd_ = nullptr;
    py::handle ctx_;
    py::handle finish_;
};

void register_ground(pybind11::module &m);

} // namespace PyClingo
