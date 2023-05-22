#include <lexy/action/scan.hpp>

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

auto main() -> int {
    auto input = ::grammar::input{std::cin};
    auto scanner = lexy::scan<::grammar::control>(input, report_error);
    // Note: skip leading whitespace
    scanner.parse(lexy::dsl::whitespace(::grammar::control::whitespace));
    while (scanner && !scanner.is_at_eof()) {
        input.discard_before(scanner.position());
        auto stm = scanner.parse<grammar::statement>();
        if (stm.has_value()) {
            std::cout << *stm.value() << "\n";
        }
        if (!scanner) {
            recover(scanner);
        }
    }
}
