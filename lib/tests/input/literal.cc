#include <catch2/catch_test_macros.hpp>

#include "input/parser.hh"

namespace Gringo::Input::Test {

TEST_CASE("parse_literal") {
    REQUIRE(to_str(parse_literal("#true")) == "#true");
    REQUIRE(to_str(parse_literal("#false")) == "#false");
    REQUIRE(to_str(parse_literal("1 < 2")) == "1<2");
    REQUIRE(to_str(parse_literal("-f+1 < 2")) == "-f+1<2");
    REQUIRE(to_str(parse_literal("p(X)")) == "p(X)");
    REQUIRE(to_str(parse_literal("-p(X)")) == "-p(X)");
    REQUIRE(to_str(parse_literal("not p")) == "not p");
    REQUIRE(to_str(parse_literal("not not p")) == "not not p");
    REQUIRE(to_str(parse_literal("5")) == "<failed>");
}

TEST_CASE("unpool_literal") {
    REQUIRE(unpool_str(parse_literal("#true")) == "[#true]");
    REQUIRE(unpool_str(parse_literal("(1;2) < (3;4) < (5;6)")) ==
            "[1<3<5, 1<4<5, 1<3<6, 1<4<6, 2<3<5, 2<4<5, 2<3<6, 2<4<6]");
    REQUIRE(unpool_str(parse_literal("not f(x;y)")) == "[not f(x), not f(y)]");
}

} // namespace Gringo::Input::Test
