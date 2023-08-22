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

void process(Gringo::SymbolStore &store, RewriteOptions opts, auto &&scanner, auto &&output) {
    for (auto stm = scanner.scan(); stm.has_value(); stm = scanner.scan()) {
        StatementVec stms;
        rewrite(store, std::move(stm).value(), opts, stms);
        for (auto const &stm : stms) {
            output << stm << "\n";
        }
    }
}

EMSCRIPTEN_KEEPALIVE
extern "C" void run(char const *program, int level, int project_mode, bool project_anonymous) {
    try {
        Gringo::with_logger wl{false};
        if (level < 0 || level > 3) {
            std::cerr << "invalid rewrite level" << std::endl;
            return;
        }
        if (project_mode < 0 || project_mode > 2) {
            std::cerr << "invalid projection mode" << std::endl;
            return;
        }
        auto opts = RewriteOptions{static_cast<RewriteLevel>(level), static_cast<ProjectionMode>(project_mode),
                                   project_anonymous};
        auto store = Gringo::make_symbol_store(true, false);
        process(*store, opts, scan_string(*store, program), std::cout);
    } catch (std::exception const &e) {
        std::cerr << "error: " << e.what() << std::endl;
    }
}
