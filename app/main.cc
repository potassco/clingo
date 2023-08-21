#include <iostream>

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
    auto opts = RewriteOptions{};
    auto store = Gringo::make_symbol_store(false, false);
    if (argc == 1) {
        process(*store, opts, scan_stream(*store, std::cin), std::cout);
    } else {
        for (int i = 1; i < argc; ++i) {
            process(*store, opts, scan_file(*store, argv[i]), std::cout);
        }
    }
}
