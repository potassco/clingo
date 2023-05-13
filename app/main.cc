#include <lexy/action/scan.hpp>

#include <parser/statement.hh>

auto main() -> int {
    auto input = ::grammar::input{std::cin};
    auto scanner = lexy::scan<::grammar::control>(input, report_error);
    // Note: skip leading whitespace
    scanner.parse(lexy::dsl::whitespace(::grammar::control::whitespace));
    while (scanner && !scanner.branch(lexy::dsl::eof)) {
        auto stm = scanner.parse<grammar::statement>();
        if (stm.has_value()) {
            std::cout << *stm.value() << "\n";
        }
    }
}
