#pragma once

#include <atomic>

namespace CppClingo::Util {

//! Helper class to signal stopping of grounding.
//!
//! Ideally, std::stop_token would be used instead. Unfortunately, clang 18 and
//! 19 do not support it yet.
class StopFlag {
  public:
    StopFlag() noexcept = default;
    StopFlag(StopFlag const &other) = delete;
    StopFlag(StopFlag &&other) noexcept = delete;
    auto operator=(StopFlag const &other) -> StopFlag & = delete;
    auto operator=(StopFlag &&other) noexcept -> StopFlag & = delete;

    void request_stop() noexcept { state_.store(true, std::memory_order_relaxed); }
    auto stop_requested() noexcept -> bool { return state_.load(std::memory_order_relaxed); }

  private:
    std::atomic<bool> state_ = false;
};

} // namespace CppClingo::Util
