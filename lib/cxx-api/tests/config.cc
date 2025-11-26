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
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == res);

    models_value.value("2");
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models.size() == 2);

    models_value.value("0");
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == res);
}

TEST_CASE_METHOD(Fixture, "config extend", "[cxx][config][extend]") {
    class ConfigValue {
      public:
        [[nodiscard]] auto get() const -> std::optional<std::string> { return value_; }
        void set(std::string_view value) { value_ = value; }

      private:
        std::optional<std::string> value_;
    };
    class ConfigArray {
      public:
        ConfigArray(std::vector<std::optional<std::string>> &values) : values_{&values} {}
        [[nodiscard]] auto size() const -> size_t { return values_->size(); }

      private:
        std::vector<std::optional<std::string>> *values_;
    };
    class ConfigArrayValue {
      public:
        ConfigArrayValue(std::vector<std::optional<std::string>> &values) : values_{&values} {}
        [[nodiscard]] auto get(std::optional<size_t> index) const -> std::optional<std::string> {
            return values_->at(index.value_or(0));
        }
        void set(std::optional<size_t> index, std::string_view value) {
            auto idx = index.value_or(0);
            while (values_->size() <= idx) {
                values_->emplace_back();
            }
            values_->at(idx) = value;
        }

      private:
        std::vector<std::optional<std::string>> *values_;
    };
    auto values = std::vector<std::optional<std::string>>{};
    cfg.add("test", "test group");
    auto test_cfg = cfg["test"];
    test_cfg.add("value", "test value", ConfigValue{});
    cfg.add("test.array[]", "test array", ConfigArray{values});
    cfg.add("test.array[].value", "test array.value", ConfigArrayValue{values});
    REQUIRE(cfg["test.array"].description() == "test array");
    REQUIRE(cfg["test.array[]"].description() == "test array");
    REQUIRE(cfg["test.array[0]"].description() == "test array");
    REQUIRE(cfg["test.array[99]"].description() == "test array");
    cfg["test"]["value"] = "hello";
    REQUIRE(*cfg["test.value"] == "hello");
    cfg["test.value"] = "world";
    REQUIRE(*cfg["test.value"] == "world");
    REQUIRE(cfg["test.array"].array().size() == 0);
    REQUIRE(cfg["test.array.value"].description() == "test array.value");
    cfg["test.array[0].value"] = "a";
    cfg["test.array[1].value"] = "b";
    REQUIRE(cfg["test.array"].array().size() == 2);
    REQUIRE(*cfg["test.array[0].value"] == "a");
    REQUIRE(*cfg["test.array.value"] == "a");
    REQUIRE(*cfg["test.array[1].value"] == "b");
    cfg["test.array.value"] = "c";
    REQUIRE(*cfg["test.array[0].value"] == "c");
    REQUIRE(*cfg["test.array.value"] == "c");
    cfg["test.array[3].value"] = "d";
    REQUIRE(!cfg["test.array[2].value"].value().has_value());
    REQUIRE(*cfg["test.array[3].value"] == "d");
}

} // namespace Clingo::Test
