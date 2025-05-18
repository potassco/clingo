#include <clingo/core.hh>

#include <catch2/catch_test_macros.hpp>

namespace Clingo::Test {

TEST_CASE("core version", "[cxx][core][version]") {
    REQUIRE(version() >= std::tuple{6, 0, 0});
    REQUIRE(version() == std::tuple{Version::major, Version::minor, Version::revision});
}

} // namespace Clingo::Test
