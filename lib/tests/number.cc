#include <iostream>
#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include <number.hh>

// NOLINTBEGIN(readability-magic-numbers)

namespace Gringo::Test {

TEST_CASE("number_construct") {
    Number a{1};
    Number b{2};
    Number c{"123456789012345678901234567890"};
    REQUIRE(a.as_int() == 1);
    REQUIRE(b.as_int() == 2);
    REQUIRE(a.as_string() == "1");
    REQUIRE(b.as_string() == "2");
    REQUIRE(c.as_string() == "123456789012345678901234567890");
}

TEST_CASE("number_add") {
    Number a{1};
    Number b{2};
    Number c{"123456789012345678901234567890"};

    // int + int
    REQUIRE(a + a == 2);
    REQUIRE(a + b == 3);
    REQUIRE(b + b == 4);
    // int + big
    REQUIRE((a + c).as_string() == "123456789012345678901234567891");
    // big + int
    REQUIRE((c + a).as_string() == "123456789012345678901234567891");
    // big + big
    REQUIRE((c + c).as_string() == "246913578024691357802469135780");

    Number x = c;
    auto repr = Number::to_repr(x);
    REQUIRE(Number::to_repr(std::move(x) + a) == repr);

    x = c;
    repr = Number::to_repr(x);
    REQUIRE(Number::to_repr(a + std::move(x)) == repr);

    x = c;
    Number y = c;
    repr = Number::to_repr(x);
    REQUIRE(Number::to_repr(std::move(x) + std::move(y)) == repr);
}

} // namespace Gringo::Test

// NOLINTEND(readability-magic-numbers)
