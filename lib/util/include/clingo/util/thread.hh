#pragma once

#include <atomic>

namespace CppClingo::Util {

//! Helper class to signal stopping of grounding.
//!
//! Ideally, std::stop_token would be used instead. Unfortunately, clang 18 and
//! 19 do not support it yet.
class StopFlag {
  public:
    //! Construct a false stop flag.
    StopFlag() noexcept = default;
    //! Prevent copying.
    StopFlag(StopFlag const &other) = delete;
    //! Prevent moving.
    StopFlag(StopFlag &&other) noexcept = delete;
    //! Prevent copying.
    auto operator=(StopFlag const &other) -> StopFlag & = delete;
    //! Prevent moving.
    auto operator=(StopFlag &&other) noexcept -> StopFlag & = delete;

    //! Set the stop flag.
    void request_stop() noexcept { state_.store(true, std::memory_order_relaxed); }
    //! Test whether stopping has been requested.
    [[nodiscard]] auto stop_requested() noexcept -> bool { return state_.load(std::memory_order_relaxed); }

  private:
    std::atomic<bool> state_ = false;
};

} // namespace CppClingo::Util
