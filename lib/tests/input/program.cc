#include <catch2/catch_test_macros.hpp>

#include "input/parser.hh"

namespace Gringo::Input::Test {

TEST_CASE("program") {
    std::istringstream in{"%p\na.%a\nb%b\n.%c\nc%d\n"};
    auto scanner = scan_stream(in);
    REQUIRE(to_str(scanner.scan()) == "%p");
    REQUIRE(to_str(scanner.scan()) == "a.");
    REQUIRE(to_str(scanner.scan()) == "%a");
    REQUIRE(to_str(scanner.scan()) == "%b");
    REQUIRE(to_str(scanner.scan()) == "b.");
    REQUIRE(to_str(scanner.scan()) == "%c");
    REQUIRE(to_str(scanner.scan()) == "%d");
    REQUIRE(to_str(scanner.scan()) == "<failed>");
}

} // namespace Gringo::Input::Test
