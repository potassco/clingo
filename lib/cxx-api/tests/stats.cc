#include <clingo/control.hh>
#include <clingo/stats.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "cbs.hh"

namespace Clingo::Test {

namespace {

class SCB : public MCB {
  public:
    SCB(MV &models) : MCB{models} {}

  private:
    void do_stats(Stats step, Stats accu) override {
        step.map().insert("a", StatsType::value).value(10);
        step.map().insert("b", StatsType::array).array().push(StatsType::value).value(10);
        step.map().insert("c", StatsType::map).map().insert("x", StatsType::value).value(1);

        auto test = accu.map().insert("Test", StatsType::map).map();
        auto test_x = test.insert("x", StatsType::value);
        auto test_y = test.insert("y", StatsType::array).array();

        test_y.ensure(0, StatsType::value) = 1;
        test_y.ensure(1, StatsType::value) = 2;
        test_y.ensure(2, StatsType::value) = 3;

        test_x = 10;
        test_x = *test_x + 2;

        for (auto sub : test_y) {
            sub = *sub + 1;
        }
    }
};

struct Fixture {
    Library lib;
    Control ctl = Control(lib, {"0"});
    MV models;
};

} // namespace

TEST_CASE_METHOD(Fixture, "stats solve", "[cxx][stats][solve]") {
    ctl.parse_string("1 { a; b; c; d } 1.");
    ctl.ground();
    {
#ifndef __EMSCRIPTEN__
        auto hnd = ctl.start_solve({}, SolveFlags::async | SolveFlags::yield, MCB{models});
#else
        auto hnd = ctl.start_solve({}, SolveFlags::yield, MCB{models});
#endif
        REQUIRE_THROWS_AS(ctl.stats(), std::invalid_argument);
        for ([[maybe_unused]] auto const &mdl : hnd) {
        }
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == MV{{"a"}, {"b"}, {"c"}, {"d"}});
    auto stats = ctl.stats();
    REQUIRE(*stats["solving"]["solvers"]["choices"] > 0);
#ifndef __EMSCRIPTEN__
    REQUIRE(*stats["summary"]["times"]["cpu"] >= 0);
#else
    REQUIRE(std::isnan(*stats["summary"]["times"]["cpu"]));
#endif
}

TEST_CASE_METHOD(Fixture, "stats user", "[cxx][stats][user]") {
    ctl.parse_string("1 { a; b; c; d } 1.");
    ctl.ground();
    REQUIRE(ctl.solve({}, SCB{models}).satisfiable());
    REQUIRE(models == MV{{"a"}, {"b"}, {"c"}, {"d"}});

    auto stats = ctl.stats();
    auto user_step = stats_step_root();
    auto user_accu = stats_accu_root();

    REQUIRE(*stats[user_step]["a"] == 10.0);
    REQUIRE(*stats[user_step]["b"][0] == 10.0);
    REQUIRE(*stats[user_step]["c"]["x"] == 1.0);

    REQUIRE(*stats[user_accu]["Test"]["x"] == 12.0);
    REQUIRE(*stats[user_accu]["Test"]["y"][0] == 2.0);
    REQUIRE(*stats[user_accu]["Test"]["y"][1] == 3.0);
    REQUIRE(*stats[user_accu]["Test"]["y"][2] == 4.0);
}

} // namespace Clingo::Test
