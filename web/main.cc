#include <clingo/app.h>

#include <emscripten.h>
#include <emscripten/bind.h>

#include <string>
#include <vector>

static constexpr auto message_limit = 25;

//! This function is meant to run clingo in a web worker context.
//!
//! It behaves just like clingo on the command line; it reads from stdin if no
//! files are given and writes to stdout.
//!
//! @param[in] args the command line arguments
void run(std::vector<std::string> const &args) {
    clingo_lib_t *lib = nullptr;
    auto c_args = std::vector<const char *>(args.size());
    std::ranges::transform(args, c_args.begin(), [](auto const &str) { return str.c_str(); });
    if (clingo_lib_new(clingo_lib_flags_slotted, nullptr, nullptr, nullptr, message_limit, &lib) ==
        clingo_result_success) {
        std::ignore = clingo_main(lib, c_args.data(), c_args.size());
    }
    clingo_lib_free(lib, true);
}

EMSCRIPTEN_BINDINGS(module) {
    using namespace emscripten;
    function("run", &run);
    register_vector<std::string>("StringVec");
}
