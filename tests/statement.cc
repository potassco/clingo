#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

#include <parser/statement.hh>

namespace test {

namespace grammar {

using statement = parse_root<::grammar::statement>;

} // namespace grammar

TEST_CASE("statement") {
    // TODO
    // 1. ensure `:` is never followed by `-`
    // 2. print with spaces
    REQUIRE(parse<grammar::statement>(":-.") == ":-.");
    REQUIRE(parse<grammar::statement>("a.") == "a.");
    REQUIRE(parse<grammar::statement>("a:-.") == "a.");
    REQUIRE(parse<grammar::statement>("a:-b.") == "a:-b.");
    REQUIRE(parse<grammar::statement>("a:-b,c.") == "a:-b;c.");
    REQUIRE(parse<grammar::statement>("a:-b;c.") == "a:-b;c.");
    REQUIRE(parse<grammar::statement>("a:-a:b,c;d.") == "a:-a:b,c;d.");
    REQUIRE(parse<grammar::statement>(":-.") == ":-.");
}

} // namespace test
