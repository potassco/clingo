#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN(readability-magic-numbers)

namespace Clingo::Test {

TEST_CASE("control") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("{a; b}.");
    ctl.ground();
    // TODO: solve
}

} // namespace Clingo::Test

// NOLINTEND(readability-magic-numbers)
