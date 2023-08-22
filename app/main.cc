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

auto main(int argc, char **argv) -> int {
    Gringo::with_logger wl{false};

    auto opts = RewriteOptions{};
    opts.project_anonymous = true;
    auto timestamp = false;
    auto loglevel = spdlog::level::trace;
    auto store = Gringo::make_symbol_store(false, false);
    auto &log = Gringo::logger();
    log.set_level(loglevel);
    if (timestamp) {
        log.set_pattern("[%Y-%m-%d %T.%e] %^%l%$: %v");
    } else {
        log.set_pattern("%^%l%$: %v");
    }
    log.debug("starting up");
    if (argc == 1) {
        process(*store, opts, scan_stream(*store, std::cin), std::cout);
    } else {
        for (int i = 1; i < argc; ++i) {
            process(*store, opts, scan_file(*store, argv[i]), std::cout);
        }
    }
}
