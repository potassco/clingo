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
    REQUIRE(c.as_int() == std::nullopt);
}

TEST_CASE("number_cross") {
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
    // big - big = int
    REQUIRE((c - c).as_int() == 0);

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

TEST_CASE("number_op") {
    auto big = Number{"123456789012345678901234567890"};
    // plus
    REQUIRE(Number{1} + Number(2) == 3);
    REQUIRE(big + Number{1} == Number("123456789012345678901234567891"));
    REQUIRE(Number{1} + big == Number("123456789012345678901234567891"));
    REQUIRE(big + big == Number("246913578024691357802469135780"));
    // div
    REQUIRE(Number{1} / Number(2) == 0);
    REQUIRE(Number{-1} / Number(-2) == 0);
    REQUIRE(Number{1} / Number(-2) == -1);
    REQUIRE(Number{-1} / Number(2) == -1);
    REQUIRE(Number{1} / big == 0);
    REQUIRE(Number{-1} / big == -1);
    REQUIRE(-big / (big + Number{1}) == -1);
    REQUIRE(big / (-big - Number{1}) == -1);
    REQUIRE(-big / (-big - Number{1}) == 0);
    REQUIRE(big / (big + Number{1}) == 0);
    REQUIRE_THROWS(Number{1} / Number(0));
    REQUIRE_THROWS(Number{0} / Number(0));
}

} // namespace Gringo::Test

// NOLINTEND(readability-magic-numbers)
