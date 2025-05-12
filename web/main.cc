#include <clingo/app.h>
#include <clingo/control.h>

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
int run(std::string input, std::vector<std::string> const &args) {
    static clingo_application_t app = {
        nullptr,
        nullptr,
        [](clingo_control_t *control, [[maybe_unused]] clingo_string_t const *files, [[maybe_unused]] size_t files_size,
           void *data) -> bool {
            auto &str = *static_cast<std::string *>(data);
            return clingo_control_parse_string(control, str.data(), str.size()) && clingo_control_main(control);
        },
        nullptr,
        nullptr,
        nullptr,

    };
    int code = 1;
    clingo_lib_t *lib = nullptr;
    auto c_args = std::vector<clingo_string_t>(args.size());
    std::ranges::transform(args, c_args.begin(),
                           [](auto const &str) { return clingo_string_t{str.data(), str.size()}; });
    if (clingo_lib_new(clingo_lib_flags_slotted | clingo_lib_flags_fast_release, clingo_log_level_info, nullptr,
                       nullptr, message_limit, &lib)) {

        clingo_main(lib, c_args.data(), c_args.size(), &app, static_cast<void *>(&input), &code);
    }
    clingo_lib_release(lib);
    return code;
}

EMSCRIPTEN_BINDINGS(module) {
    using namespace emscripten;
    function("run", &run);
    register_vector<std::string>("StringVec");
}
