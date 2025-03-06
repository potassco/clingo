#include <clingo/app.h>

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
    clingo_lib_t *lib = nullptr;
    res = clingo_lib_new(clingo_lib_flags_slotted, nullptr, nullptr, nullptr, message_limit, &lib);
    if (res != clingo_result_success) {
        return 1;
    }
#if CLINGO_PYTHON_ENABLED
    res = clingo_register_python(lib);
    if (res != clingo_result_success) {
        clingo_lib_free(lib, true);
        return 1;
    }
#endif
    res = clingo_main(lib, argv + 1, argc - 1, nullptr, nullptr);
    clingo_lib_free(lib, true);
    return res;
}
