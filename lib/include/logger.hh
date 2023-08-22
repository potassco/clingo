#pragma once

#include <spdlog/spdlog.h>
// Note: must be included after spdlog.h.
#include <spdlog/fmt/ostr.h>

namespace Gringo {

auto setup_logger(bool shared) -> spdlog::logger &;
void destroy_logger() noexcept;

[[nodiscard]] auto logger() -> spdlog::logger &;

struct with_logger {
    with_logger(bool shared) { setup_logger(shared); }
    ~with_logger() noexcept { destroy_logger(); }
};

} // namespace Gringo
