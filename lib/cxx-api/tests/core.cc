#include <clingo/core.hh>

#include <catch2/catch_test_macros.hpp>

#include <unordered_set>

namespace Clingo::Test {

TEST_CASE("core version", "[cxx][core][version]") {
    REQUIRE(version() >= std::tuple{6, 0, 0});
    REQUIRE(version() == std::tuple{Version::major, Version::minor, Version::revision});
}

TEST_CASE("core location", "[cxx][core][location]") {
    auto lib = Library{};
    auto a = Position{lib, "a.lp", 1, 1};
    auto b = Position{lib, "a.lp", 1, 2};
    auto positions = std::unordered_set<Position>{a, b, a};
    REQUIRE(positions.size() == 2);
    REQUIRE(std::hash<Position>{}(a) == a.hash());
    auto locations = std::unordered_set<Location>{Location{a, b}, Location{a, a}, Location{a, b}};
    REQUIRE(locations.size() == 2);
    REQUIRE(std::hash<Location>{}(Location{a, b}) == Location{a, b}.hash());
}

} // namespace Clingo::Test
