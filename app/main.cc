#include <clingo/app.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <span>
#include <vector>

#ifndef CLINGO_PYTHON_ENABLED
#define CLINGO_PYTHON_ENABLED 0
#endif

#if CLINGO_PYTHON_ENABLED
#include <embed.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

static constexpr auto message_limit = 25;

EMSCRIPTEN_KEEPALIVE
auto main(int argc, char *argv[]) -> int {
    struct scoped_lib {
        ~scoped_lib() { clingo_lib_release(ptr); };
        clingo_lib_t *ptr = nullptr;
    } lib;
    if (!clingo_lib_new(clingo_lib_flags_slotted | clingo_lib_flags_fast_release, clingo_log_level_trace, nullptr,
                        nullptr, message_limit, &lib.ptr)) {
        return 1;
    }
#if CLINGO_PYTHON_ENABLED
    if (!clingo_register_python(lib.ptr)) {
        return 1;
    }
#endif
    auto args = std::vector<clingo_string_t>{};
    try {
        args.reserve(argc - 1);
        std::ranges::transform(std::span{argv + 1, static_cast<size_t>(argc - 1)}, std::back_inserter(args),
                               [](auto const &x) { return clingo_string_t{x, strlen(x)}; });
    } catch (std::exception const &e) {
        clingo_lib_report(lib.ptr, clingo_message_error, e.what(), std::strlen(e.what()));
        return 1;
    }
    int code = 0;
    if (!clingo_main(lib.ptr, args.data(), args.size(), nullptr, nullptr, &code)) {
        return 1;
    }
    return code;
}
