#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <logger.hh>

namespace Gringo {

static std::shared_ptr<spdlog::logger> g_logger = nullptr;

auto setup_logger(bool shared) -> spdlog::logger & {
    if (shared) {
        g_logger = spdlog::stderr_color_mt("clingo");
    } else {
        g_logger = spdlog::stderr_color_st("clingo");
    }
    return *g_logger;
}

void destroy_logger() noexcept { g_logger = nullptr; }

[[nodiscard]] auto logger() -> spdlog::logger & {
    if (g_logger == nullptr) {
        setup_logger(true);
    }
    return *g_logger;
}

} // namespace Gringo
