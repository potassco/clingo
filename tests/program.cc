#include <catch2/catch_test_macros.hpp>

#include <lexy/action/scan.hpp>

#include "parser.hh"

#include <parser/statement.hh>

namespace test {

namespace grammar {

using statement = parse_root<::grammar::statement>;

} // namespace grammar

TEST_CASE("program") {
    std::istringstream in;
    in.str("a.b.c");
    auto input = ::grammar::input{in};
    auto scanner = lexy::scan<::grammar::control>(input, report_error);
    auto stm = scanner.parse<grammar::statement>();
    REQUIRE(stm.has_value());
    REQUIRE(stm.value()->to_string() == "a.");
    input.discard_before(scanner.position());
    stm = scanner.parse<grammar::statement>();
    REQUIRE(stm.has_value());
    REQUIRE(stm.value()->to_string() == "b.");
    input.discard_before(scanner.position());
    stm = scanner.parse<grammar::statement>();
    REQUIRE(!stm.has_value());
}

} // namespace test

