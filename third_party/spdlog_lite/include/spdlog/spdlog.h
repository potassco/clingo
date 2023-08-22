#pragma once

namespace spdlog {
// This is just a stub for the web target. It should be extended to at least
// print something to stdout. Another route could be to just roll an own logger
// because the requirements for clingo are very basic.
struct logger {
    template <class... Args> void trace(Args const &...args) { (static_cast<void>(args), ...); }

    template <class... Args> void debug(Args const &...args) { (static_cast<void>(args), ...); }

    template <class... Args> void info(Args const &...args) { (static_cast<void>(args), ...); }
};

} // namespace spdlog
