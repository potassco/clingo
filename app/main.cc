#include <iostream>

#include <input/algo/parse.hh>
#include <input/algo/print.hh>
#include <input/algo/rewrite.hh>

using namespace Gringo::Input;

void process(RewriteOptions opts, auto &&scanner, auto &&output) {
    for (auto stm = scanner.scan(); stm.has_value(); stm = scanner.scan()) {
        StatementVec stms;
        rewrite(std::move(stm).value(), opts, stms);
        for (auto const &stm : stms) {
            output << stm << "\n";
        }
    }
}

auto main(int argc, char **argv) -> int {
    auto opts = RewriteOptions{};
    if (argc == 1) {
        process(opts, parse_stream(std::cin), std::cout);
    } else {
        for (int i = 1; i < argc; ++i) {
            process(opts, parse_file(argv[i]), std::cout);
        }
    }
}
