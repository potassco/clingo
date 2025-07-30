#include <clingo/base.hh>
#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

#include "cbs.hh"

namespace Clingo::Test {

namespace {

struct Fixture {
    Library lib;
    Control ctl{lib};
    Config cfg = ctl.config();
};

} // namespace

TEST_CASE_METHOD(Fixture, "config base", "[cxx][config][base]") {
    REQUIRE(cfg.type() == ConfigType::map);
    auto root_map = cfg.map();
    REQUIRE(std::ranges::find(root_map, "solve", [](auto const &x) { return x.first; }) != root_map.end());
    REQUIRE(cfg.to_string().find("solve:") != std::string::npos);
    auto solve_map = root_map.get("solve").map();
    REQUIRE(std::ranges::find(solve_map, "models", [](auto const &x) { return x.first; }) != solve_map.end());
    REQUIRE(solve_map.get("models").type() == ConfigType::value);
    REQUIRE(solve_map.get("models").description().starts_with("Compute"));
    REQUIRE(solve_map.get("models").description().find("%A") == std::string::npos);
    solve_map.get("models").value("-1");
    REQUIRE(solve_map.get("models").to_string() == "\"-1\"");
    REQUIRE(solve_map.get("models").value() == "-1");
    REQUIRE(std::ranges::find(root_map, "solver", [](auto const &x) { return x.first; }) != root_map.end());
    REQUIRE(root_map.get("solver").type() == (ConfigType::array | ConfigType::map));
    REQUIRE(root_map.get("solver").array().size() >= 1);
    auto solver_map = root_map.get("solver").array().at(0).map();
    REQUIRE(std::ranges::find(solver_map, "heuristic", [](auto const &x) { return x.first; }) != solver_map.end());
    cfg["solve"]["models"] = "3";
    REQUIRE(*cfg["solve"]["models"] == "3");
}

TEST_CASE_METHOD(Fixture, "config solve", "[cxx][config][solve]") {
    auto res = std::vector<std::vector<std::string>>{{"a"}, {"b"}, {"c"}, {"d"}};
    auto models = std::vector<std::vector<std::string>>{};

    ctl.parse_string("1 { a; b; c; d } 1.");
    ctl.ground();

    auto models_value = cfg.map().get("solve").map().get("models");

    models_value.value("0");
    {
        auto mcb = MCB{models};
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == res);

    models_value.value("2");
    {
        auto mcb = MCB{models};
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models.size() == 2);

    models_value.value("0");
    {
        auto mcb = MCB{models};
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == res);
}

} // namespace Clingo::Test
