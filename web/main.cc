#ifndef __clang_analyzer__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include <iostream>

#include <logger.hh>

#include <input/program.hh>

#include <input/algo/parse.hh>
#include <input/algo/print.hh>

using namespace Gringo::Input;

EMSCRIPTEN_KEEPALIVE
extern "C" auto run(char const *program, int project_mode, bool project_anonymous) -> bool {
    auto log = Gringo::Logger{};
    try {
        if (project_mode < 0 || project_mode > 2) {
            GRINGO_REPORT(log, error) << "invalid projection mode";
            return false;
        }
        UnprocessedProgram uprg;
        auto opts = RewriteOptions{static_cast<ProjectionMode>(project_mode), project_anonymous};
        auto store = Gringo::make_symbol_store(false, false);
        auto scanner = scan_string(log, *store, program);
        for (auto stm = scanner.scan(); stm.has_value(); stm = scanner.scan()) {
            uprg.add(*store, std::move(stm).value());
        }
        Program prg{opts};
        prg.join(log, *store, std::move(uprg));
        prg.visit_stms(*store, [](auto const &stm) { std::cout << stm << "\n"; });
    } catch (std::exception const &e) {
        fprintf(stderr, "%s: %s\n", log.message_prefix(Gringo::MessageCode::error), e.what());
        fflush(stderr);
        return false;
    }
    return !log.has_error();
}
