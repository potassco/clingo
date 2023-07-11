#include <catch2/catch_test_macros.hpp>

#include "input/parser.hh"

namespace Gringo::Input::Test {

TEST_CASE("program") {
    Parser parser{"a.b.c"};
    auto stm = parser.scan();
    REQUIRE(stm.has_value());
    REQUIRE(stm.value() == "a.");
    stm = parser.scan();
    REQUIRE(stm.has_value());
    REQUIRE(stm.value() == "b.");
    stm = parser.scan();
    REQUIRE(!stm.has_value());
}

} // namespace Gringo::Input::Test
