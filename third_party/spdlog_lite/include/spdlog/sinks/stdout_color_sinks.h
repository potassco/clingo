#pragma once

#include <memory>

#include <spdlog/spdlog.h>

namespace spdlog {

auto stderr_color_st(char const *name) -> std::shared_ptr<logger> {
    static_cast<void>(name);
    return std::make_shared<logger>();
}

auto stderr_color_mt(char const *name) -> std::shared_ptr<logger> { return stderr_color_st(name); }

} // namespace spdlog
