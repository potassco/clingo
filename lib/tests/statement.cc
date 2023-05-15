#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("statement") {
    // TODO
    // 1. ensure `:` is never followed by `-`
    // 2. print with spaces
    REQUIRE(parse_statement(":-.") == ":-.");
    REQUIRE(parse_statement("a.") == "a.");
    REQUIRE(parse_statement("a:-.") == "a.");
    REQUIRE(parse_statement("a:-b.") == "a:-b.");
    REQUIRE(parse_statement("a:-b,c.") == "a:-b;c.");
    REQUIRE(parse_statement("a:-b;c.") == "a:-b;c.");
    REQUIRE(parse_statement("a:-a:b,c;d.") == "a:-a:b,c;d.");
    REQUIRE(parse_statement(":-.") == ":-.");
}

} // namespace test
