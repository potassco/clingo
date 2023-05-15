#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("theory atoms") {
    REQUIRE(parse_head_literal("&p(x,y;z){}<=a") == "&p(x,y;z){}<=a");
    REQUIRE(parse_head_literal("&p(x,y;z)<=a") == "&p(x,y;z){}<=a");
    REQUIRE(parse_head_literal("&p(x,y;z){}") == "&p(x,y;z)");
    REQUIRE(parse_head_literal("&p(x,y;z)") == "&p(x,y;z)");
}

} // namespace test
