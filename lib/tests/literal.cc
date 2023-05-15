#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("literals") {
    REQUIRE(parse_literal("#true") == "#true");
    REQUIRE(parse_literal("#false") == "#false");
    REQUIRE(parse_literal("1 < 2") == "1<2");
    REQUIRE(parse_literal("-f+1 < 2") == "((-f)+1)<2");
    REQUIRE(parse_literal("p(X)") == "p(X)");
    // TODO: get rid of parenthesis
    REQUIRE(parse_literal("-p(X)") == "(-p(X))");
    REQUIRE(parse_literal("not p") == "not p");
    REQUIRE(parse_literal("not not p") == "not not p");
    REQUIRE(parse_literal("5") == "<failed>");
}

} // namespace test
