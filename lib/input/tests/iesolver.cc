#include "test.hh"

#include <clingo/input/rewrite/iesolver.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Input::Test {

// NOLINTBEGIN(readability-magic-numbers)

TEST_CASE("iesolver") {
    auto log = Logger{};
    auto &store = default_store();
    auto x = store.string_ref("x");
    auto y = store.string_ref("y");
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
    SECTION("terminate") {
        // X >= 0
        // X - Y >= 0
        // Y - X >= 1
        auto solver = IESolver{};
        solver.add(IE{{{{1}, x}}, {0}});
        solver.add(IE{{{{1}, x}, {{-1}, y}}, {0}});
        solver.add(IE{{{{-1}, x}, {{1}, y}}, {1}});
        REQUIRE(solver.compute(log));
        REQUIRE(solver.domain().at(x).value(IEInterval::Lower) > 0);
        REQUIRE(solver.domain().at(y).value(IEInterval::Lower) > 0);
        REQUIRE(!solver.domain().at(x).has_value(IEInterval::Upper));
        REQUIRE(!solver.domain().at(y).has_value(IEInterval::Upper));
    }
};

// NOLINTEND(readability-magic-numbers)

} // namespace CppClingo::Input::Test
