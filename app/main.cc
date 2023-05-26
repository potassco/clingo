#include <lexy/action/scan.hpp>
#include <lexy/input/file.hpp>

#include <parser/statement.hh>

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

template <typename Input, typename Scanner> void discard(Input &input, Scanner &scanner) {}

template <typename Encoding, typename Counting, typename Scanner>
void discard(StreamInput<Encoding, Counting> &input, Scanner &scanner) {
    input.discard_before(scanner.position());
}

template <typename Input> void parse(Input &input) {
    auto scanner = lexy::scan<grammar::control>(input, report_error);
    // Note: skip leading whitespace
    scanner.parse(lexy::dsl::whitespace(grammar::control::whitespace));
    while (scanner && !scanner.is_at_eof()) {
        discard(input, scanner);
        auto stm = scanner.template parse<grammar::statement>();
        if (stm.has_value()) {
            std::cout << *stm.value() << "\n";
            for (auto &comment : grammar::comment_sink()) {
                std::cout << comment << "\n";
            }
        }
        grammar::comment_sink().clear();
        if (!scanner) {
            recover(scanner);
        }
    }
};

auto main(int argc, char **argv) -> int {
    if (argc == 1) {
        auto input = StreamInput<grammar::encoding>{std::cin};
        parse(input);
    } else {
        for (int i = 1; i < argc; ++i) {
            auto file = lexy::read_file<grammar::encoding>(argv[i]);
            parse(file.buffer());
        }
    }
}
