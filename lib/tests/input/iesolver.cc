#include <input/algo/compute_bounds.hh>
#include <input/iesolver.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

namespace {}

// NOLINTBEGIN(readability-magic-numbers)

TEST_CASE("iesolver") {
    auto log = Logger{};
    log.set_level(LogLevel::trace);
    auto &store = default_store();
    auto x = store.string("x");
    auto y = store.string("y");
    auto solver = IESolver{};

    SECTION("sat") {
        solver.add(IE{{{{3}, x}}, {5}});
        solver.add(IE{{{{-5}, x}}, {-17}});
        REQUIRE(solver.compute(log));
        REQUIRE(solver.domain().size() == 1);
        REQUIRE(solver.domain().front() == std::make_pair(x, IEInterval{Number{2}, Number{3}}));
    }
    SECTION("unsat") {
        auto solver = IESolver{};
        solver.add(IE{{{{3}, x}}, {5}});
        solver.add(IE{{{{-5}, x}}, {-9}});
        REQUIRE(!solver.compute(log));
        REQUIRE(solver.domain().empty());
    }
    SECTION("complex") {
        auto solver = IESolver{};
        solver.add(IE{{{{1}, x}}, {0}});
        solver.add(IE{{{{1}, x}, {{1}, y}}, {6}});
        solver.add(IE{{{{-1}, x}, {{-1}, y}}, {-6}});
        solver.add(IE{{{{-3}, x}, {{1}, y}}, {2}});
        solver.add(IE{{{{3}, x}, {{-1}, y}}, {-2}});
        REQUIRE(solver.compute(log));
        REQUIRE(solver.domain().size() == 2);
        REQUIRE(solver.domain().at(x) == IEInterval{Number{1}, Number{1}});
        REQUIRE(solver.domain().at(y) == IEInterval{Number{5}, Number{5}});
    }
};

TEST_CASE("compute_bounds") {
    // TODO: move into separate file
    ParseHelper ph;
    ph.logger() = Logger{};
    ph.logger().set_level(LogLevel::debug);
    ConstMap const_map;
    ParamMap param_map;
    RewriteContext ctx{ph, ph, param_map, const_map, {}, "__A_"};

    REQUIRE(compute_bounds(ctx, *ph.statement("h :- X>=1; X<= 5.")).state);
}

// NOLINTEND(readability-magic-numbers)

} // namespace Gringo::Input::Test
