#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("statement") {
    // rule
    REQUIRE(parse_statement(":-.") == " :- .");
    REQUIRE(parse_statement("a.") == "a.");
    REQUIRE(parse_statement("a:-.") == "a.");
    REQUIRE(parse_statement("a:-b.") == "a :- b.");
    REQUIRE(parse_statement("a:-b,c.") == "a :- b; c.");
    REQUIRE(parse_statement("a:-b;c.") == "a :- b; c.");
    REQUIRE(parse_statement("a:-a:b,c;d.") == "a :- a: b, c; d.");

    // theory
    REQUIRE(parse_statement("#theory{}.") == "#theory { ... }.");
    REQUIRE(parse_statement("#theory{a { }; b { - : 10, unary }; p/0: a, {+, -}, b, head }.") == "#theory { ... }.");
}

} // namespace test
