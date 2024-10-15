#include <clingo.h>
#include <embed.h>

static constexpr auto message_limit = 25;

auto main(int argc, char *argv[]) -> int {
    clingo_result_t res = clingo_result_success;
    clingo_lib_t *lib = nullptr;
    res = clingo_lib_new(clingo_lib_flags_slotted, nullptr, nullptr, nullptr, message_limit, &lib);
    if (res != clingo_result_success) {
        return 1;
    }
    res = clingo_register_python(lib);
    if (res != clingo_result_success) {
        clingo_lib_free(lib, true);
        return 1;
    }
    res = clingo_main(lib, argv + 1, argc - 1);
    if (res != clingo_result_success) {
        clingo_lib_free(lib, true);
        return 1;
    }
    clingo_lib_free(lib, true);
}
