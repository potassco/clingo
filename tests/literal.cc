#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

#include <parser/literal.hh>

namespace test {

namespace grammar {

using literal = parse_root<::grammar::literal>;

} // namespace grammar

TEST_CASE("literals") {
    REQUIRE(parse<grammar::literal>("#true") == "#true");
    REQUIRE(parse<grammar::literal>("#false") == "#false");
    REQUIRE(parse<grammar::literal>("1 < 2") == "1<2");
    REQUIRE(parse<grammar::literal>("-f+1 < 2") == "((-f)+1)<2");
    REQUIRE(parse<grammar::literal>("p(X)") == "p(X)");
    // TODO: get rid of parenthesis
    REQUIRE(parse<grammar::literal>("-p(X)") == "(-p(X))");
    REQUIRE(parse<grammar::literal>("not p") == "not p");
    REQUIRE(parse<grammar::literal>("not not p") == "not not p");
    REQUIRE(parse<grammar::literal>("5") == "<failed>");
}

} // namespace test
