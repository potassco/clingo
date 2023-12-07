#ifndef __clang_analyzer__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include <iostream>

#include <logger.hh>

#include <input/algo/parse.hh>
#include <input/algo/print.hh>
#include <input/algo/rewrite.hh>

using namespace Gringo::Input;

template <class Scanner> void process(Gringo::SymbolStore &store, Scanner &&scanner, UnprocessedProgram &prg) {
    for (auto stm = scanner.scan(); stm.has_value(); stm = scanner.scan()) {
        prg.add(store, std::move(stm).value());
    }
}

EMSCRIPTEN_KEEPALIVE
extern "C" void run(char const *program, int project_mode, bool project_anonymous) {
    auto log = Gringo::Logger{};
    try {
        if (project_mode < 0 || project_mode > 2) {
            GRINGO_REPORT(log, error) << "invalid projection mode";
            return;
        }
        UnprocessedProgram uprg;
        auto opts = RewriteOptions{static_cast<ProjectionMode>(project_mode), project_anonymous};
        auto store = Gringo::make_symbol_store(false, false);
        process(*store, scan_string(log, *store, program), uprg);
        Program prg{opts};
        prg.join(log, *store, std::move(uprg));
    } catch (std::exception const &e) {
        GRINGO_REPORT(log, error) << e.what();
    }
}
