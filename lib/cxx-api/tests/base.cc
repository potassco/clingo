#include <clingo/base.hh>
#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN(readability-magic-numbers)

namespace Clingo::Test {

TEST_CASE("base") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("{p(1)}. p(2). q(3).");
    ctl.ground();
}

} // namespace Clingo::Test

// NOLINTEND(readability-magic-numbers)
