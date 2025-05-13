#include <clingo/base.hh>
#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

namespace Clingo::Test {

TEST_CASE("cxx-base") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    auto bse = ctl.base();
    ctl.parse_string("{p(1)}. p(2). q(3).");
    ctl.ground();
    REQUIRE(bse.size() == 2);
    REQUIRE(bse.contains(Function(lib, "p", {Number(1)})));
    REQUIRE(bse.contains(Function(lib, "p", {Number(2)})));
    REQUIRE(!bse.contains(Function(lib, "p", {Number(3)})));
    auto p1 = bse.get(Function(lib, "p", {Number(1)}));
    auto p2 = bse.get(Function(lib, "p", {Number(2)}));
    REQUIRE(p1.has_value());
    REQUIRE(p2.has_value());
    REQUIRE(!bse.is_fact(p1->literal()));
    REQUIRE(bse.is_fact(p2->literal()));
}

} // namespace Clingo::Test
