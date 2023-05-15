#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

#include <parser/theory.hh>
#include <parser/head_literal.hh>

namespace test {

namespace grammar {

using literal = parse_root<::grammar::head_literal, '.'>;

} // namespace grammar

TEST_CASE("theory atoms") {
    REQUIRE(parse<grammar::literal>("&p(x,y;z){}<=a") == "&p(x,y;z){}<=a");
    REQUIRE(parse<grammar::literal>("&p(x,y;z)<=a") == "&p(x,y;z){}<=a");
    REQUIRE(parse<grammar::literal>("&p(x,y;z){}") == "&p(x,y;z)");
    REQUIRE(parse<grammar::literal>("&p(x,y;z)") == "&p(x,y;z)");
}

} // namespace test

