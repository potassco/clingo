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

void process(Gringo::Logger &log, Gringo::SymbolStore &store, RewriteOptions opts, auto &&scanner, auto &&output) {
    for (auto stm = scanner.scan(); stm.has_value(); stm = scanner.scan()) {
        StatementVec stms;
        rewrite(log, store, std::move(stm).value(), opts, stms);
        for (auto const &stm : stms) {
            output << stm << "\n";
        }
    }
}

EMSCRIPTEN_KEEPALIVE
extern "C" void run(char const *program, int level, int project_mode, bool project_anonymous) {
    auto log = Gringo::Logger{};
    try {
        if (level < 0 || level > 3) {
            GRINGO_REPORT(log, error) << "invalid rewrite level";
            return;
        }
        if (project_mode < 0 || project_mode > 2) {
            GRINGO_REPORT(log, error) << "invalid projection mode";
            return;
        }
        auto opts = RewriteOptions{static_cast<RewriteLevel>(level), static_cast<ProjectionMode>(project_mode),
                                   project_anonymous};
        auto store = Gringo::make_symbol_store(true, false);
        process(log, *store, opts, scan_string(*store, program), std::cout);
    } catch (std::exception const &e) {
        GRINGO_REPORT(log, error) << e.what();
    }
}
