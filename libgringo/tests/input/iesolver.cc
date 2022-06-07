#include <catch2/catch.hpp>
#include "gringo/term.hh"

namespace Gringo { namespace Input { namespace Test {

struct DummyContext : IEContext {
    void gatherIEs(IESolver &solver) const override {
        static_cast<void>(solver);
    }
    void addIEBounds(IESolver const &solver, IEBoundMap const &bounds) override {
        this->bounds = bounds;
    }
    IEBoundMap bounds;
};

TEST_CASE("iesolver", "[iesolver]") {
    SECTION("base") {
        auto valX = std::make_shared<Symbol>(Symbol::createNum(0));
        auto valY = std::make_shared<Symbol>(Symbol::createNum(0));
        Location loc{"", 0, 0, "", 0, 0};
        auto X = make_locatable<VarTerm>(loc, "X", valX);
        auto Y = make_locatable<VarTerm>(loc, "Y", valY);
        DummyContext ctx;
        IESolver slv{ctx};
        slv.add({{{1, X.get()}}, 2}, false);
        slv.add({{{1, Y.get()}, {-1, X.get()}}, 1}, false);
        slv.add({{{-1, X.get()}, {-1, Y.get()}}, -100}, false);

        slv.compute();
        auto const bounds = std::move(ctx.bounds);
        auto itX = bounds.find(X.get());
        auto itY = bounds.find(Y.get());
        REQUIRE(!bounds.empty());
        REQUIRE(itX != bounds.end());
        REQUIRE(itY != bounds.end());
        REQUIRE(itX->second.hasBound(IEBound::Lower));
        REQUIRE(itX->second.hasBound(IEBound::Upper));
        REQUIRE(itY->second.hasBound(IEBound::Lower));
        REQUIRE(itY->second.hasBound(IEBound::Upper));
        REQUIRE(itX->second.bound(IEBound::Lower) == 2);
        REQUIRE(itX->second.bound(IEBound::Upper) == 97);
        REQUIRE(itY->second.bound(IEBound::Lower) == 3);
        REQUIRE(itY->second.bound(IEBound::Upper) == 98);
    }
}

} } } // namespace Test Input Gringo
