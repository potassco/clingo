#include <clingo/symbol.hh>

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN(readability-magic-numbers)

namespace Clingo::Test {

TEST_CASE("symbol") {
    char const *str_big = "1999999999999999999999999999999999999999999999991";
    auto lib = Library();
    auto num_int = Number(10);
    auto num_big = Number(lib, str_big);
    REQUIRE(num_int.to_string() == "10");
    REQUIRE(num_big.to_string() == str_big);
}

} // namespace Clingo::Test

// NOLINTEND(readability-magic-numbers)
