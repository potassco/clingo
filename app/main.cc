#include <clingo/app.h>
#include <cstdio>

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
    clingo_result_t res = clingo_result_success;
    struct scoped_lib {
        ~scoped_lib() { clingo_lib_release(ptr); };
        clingo_lib_t *ptr = nullptr;
    } lib;
    res = clingo_lib_new(clingo_lib_flags_slotted | clingo_lib_flags_fast_release, clingo_log_level_trace, nullptr,
                         nullptr, message_limit, &lib.ptr);
    if (res != clingo_result_success) {
        return 1;
    }
#if CLINGO_PYTHON_ENABLED
    res = clingo_register_python(lib.ptr);
    if (res != clingo_result_success) {
        return 1;
    }
#endif
    int code = 0;
    clingo_main(lib.ptr, argv + 1, argc - 1, nullptr, nullptr, &code);
    return code;
}
