#include <iostream>

#include <lexy/action/scan.hpp>
#include <lexy/input/file.hpp>

#include <input/print.hh>

#include <input/parser/statement.hh>

using namespace Gringo::Input;

template <typename Scanner> auto recover(Scanner &scanner) {
    auto recovery = scanner.error_recovery();
    while (!scanner.branch(lexy::dsl::period)) {
        if (!scanner.discard(lexy::dsl::code_point)) {
            LEXY_MOV(recovery).cancel();
            return;
        }
    }
    if (scanner.branch(LEXY_LIT("["))) {
        while (!scanner.branch(LEXY_LIT("]"))) {
            if (!scanner.discard(lexy::dsl::code_point)) {
                LEXY_MOV(recovery).cancel();
                return;
            }
        }
    }
    std::move(recovery).finish();
}

template <typename Input, typename Scanner> void discard(Input &input, Scanner &scanner) {
    static_cast<void>(input);
    static_cast<void>(scanner);
}

template <typename Encoding, typename Counting, typename Scanner>
void discard(Gringo::Util::StreamInput<Encoding, Counting> &input, Scanner &scanner) {
    input.discard_before(scanner.position());
}

void parse(RewriteOptions opts, auto &&input, auto &&output) {
    Comments comments;
    auto stateful_input = StatefulInput{input, comments};
    auto scanner = lexy::scan<Grammar::control>(stateful_input, Gringo::Util::report_error);
    // skip leading whitespace
    scanner.parse(lexy::dsl::whitespace(Grammar::control::whitespace));
    while (scanner && !scanner.is_at_eof()) {
        discard(input, scanner);
        lexy::scan_result<SStatement> res_stm = scanner.template parse<Grammar::statement>();
        if (res_stm.has_value()) {
            // output comments before end of statement
            for (auto &comment : comments) {
                output << comment << "\n";
            }
            comments.clear();
            // rewrite statements
            SStatementVec stms;
            rewrite(std::move(res_stm.value()), opts, stms);
            for (auto const &stm : stms) {
                output << *stm << "\n";
            }
        }
        if (!scanner) {
            recover(scanner);
        }
    }
    // print remaining comments
    comments.mark();
    for (auto &comment : comments) {
        output << comment << "\n";
    }
    comments.clear();
};

auto main(int argc, char **argv) -> int {
    auto opts = RewriteOptions{};
    if (argc == 1) {
        auto input = Gringo::Util::StreamInput<Grammar::encoding>{std::cin};
        parse(opts, input, std::cout);
    } else {
        for (int i = 1; i < argc; ++i) {
            auto file = lexy::read_file<Grammar::encoding>(argv[i]);
            parse(opts, file.buffer(), std::cout);
        }
    }
}
