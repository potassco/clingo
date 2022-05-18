#include <catch2/catch.hpp>
#include "gringo/input/iesolver.hh"
#include "gringo/term.hh"

namespace Gringo { namespace Input { namespace Test {

TEST_CASE("iesolver", "[iesolver]") {
    SECTION("base") {
        auto valX = std::make_shared<Symbol>(Symbol::createNum(0));
        auto valY = std::make_shared<Symbol>(Symbol::createNum(0));
        Location loc{"", 0, 0, "", 0, 0};
        auto X = make_locatable<VarTerm>(loc, "X", valX);
        auto Y = make_locatable<VarTerm>(loc, "Y", valY);
        IESolver slv;
        slv.add({{{1, X.get()}}, 2});
        slv.add({{{1, Y.get()}, {-1, X.get()}}, 1});
        slv.add({{{-1, X.get()}, {-1, Y.get()}}, -100});

        // TODO
        // REQUIRE(slv.compute_bounds());
    }
}

} } } // namespace Test Input Gringo
