#include <clingo/control.hh>
#include <clingo/core.hh>
#include <clingo/solve.hh>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "cbs.hh"

namespace Clingo::Test {

namespace {

class UCB : public SolveEventHandler {
  public:
    UCB(std::vector<std::vector<Sum>> &bounds) : bounds_{&bounds} { bounds_->clear(); }

  private:
    void do_unsat(SumSpan lower_bound) override { bounds_->emplace_back(lower_bound.begin(), lower_bound.end()); }
    std::vector<std::vector<Sum>> *bounds_;
};

class ECB : public SolveEventHandler {
  public:
    ECB(Library &lib) : lib_{&lib} {}

  private:
    auto do_model(Model model) -> bool override {
        auto sym = [&](std::string_view name) { return Function(*lib_, name, {}); };
        model.extend(std::array{sym("b"), sym("c")});
        return true;
    }
    Library *lib_;
};

} // namespace

TEST_CASE("solve basic", "[cxx][solve][basic]") {
    auto expected = std::vector<std::vector<std::string>>{{"a"}, {"b"}};
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};

    ctl.parse_string("1 { a; b } 1.");
    ctl.ground({{"base", {}}});

    auto models = std::vector<std::vector<std::string>>{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == expected);
}

TEST_CASE("solve yield", "[cxx][solve][yield]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("1 { a; b } 1.");
    ctl.ground({{"base", {}}});

    auto models = std::vector<std::vector<std::string>>{};
    {
        auto mcb = MCB{models};
        auto hnd = ctl.start_solve();
        for (auto &it : hnd) {
            mcb.model(it);
        }
        REQUIRE(hnd.get().satisfiable());
    }
    std::ranges::sort(models);
    REQUIRE(models == std::vector<std::vector<std::string>>{{"a"}, {"b"}});
}

#ifndef __EMSCRIPTEN__
TEST_CASE("solve async", "[cxx][solve][async]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("1 { a; b } 1.");
    ctl.ground({{"base", {}}});

    auto models = std::vector<std::vector<std::string>>{};
    {
        auto hnd = ctl.start_solve({}, SolveFlags::async, MCB{models});
        while (!hnd.wait(0.01)) {
            // simulate doing something else
        }
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == std::vector<std::vector<std::string>>{{"a"}, {"b"}});
}
#endif

TEST_CASE("solve assume", "[cxx][solve][assume]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("{a; b; c}. :- a, b.");
    ctl.ground({{"base", {}}});

    auto lit = [&](std::string_view name) { return ctl.base().get(Function(lib, name, {})).value().literal(); };

    auto assumptions = std::vector{lit("a"), lit("b"), lit("c")};
    {
        auto hnd = ctl.start_solve(assumptions);
        auto result = hnd.get();
        REQUIRE(result.unsatisfiable());
        auto core = hnd.core();
        REQUIRE(core.size() == 2);
        REQUIRE((std::ranges::find(core, assumptions[0]) != core.end() &&
                 std::ranges::find(core, assumptions[1]) != core.end()));
    }
}

TEST_CASE("solve extend base", "[cxx][solve][extend][base]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("a.");
    ctl.ground({{"base", {}}});
    {
        auto hnd = ctl.start_solve({}, SolveFlags::empty, ECB{lib});
        REQUIRE(hnd.get().satisfiable());
        REQUIRE(hnd.last()->symbols(ShowFlags::theory).size() == 2);
        REQUIRE(hnd.last()->symbols(ShowFlags::shown).size() == 3);
        REQUIRE(hnd.last()->to_string() == "a, b, c");
    }
}

TEST_CASE("solve extend yield", "[cxx][solve][extend][yield]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("a.");
    ctl.ground({{"base", {}}});
    auto models = std::vector<std::vector<std::string>>{};
    {
        auto mcb = MCB{models};
        auto hnd = ctl.start_solve({}, SolveFlags::yield, ECB{lib});
        for (auto &&mdl : hnd) {
            mcb.model(mdl);
        }
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == std::vector<std::vector<std::string>>{{"a", "b", "c"}});
}

TEST_CASE("solve unsat", "[cxx][solve][unsat]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"--opt-str=usc,oll,0", "--stats=2"}};
    ctl.parse_string("1 { p(X); q(X) } 1 :- X=1..3. #minimize { 1,p,X: p(X); 1,q,X: q(X) }.");
    ctl.ground();
    auto bounds = std::vector<std::vector<Sum>>{};
    REQUIRE(ctl.solve({}, UCB{bounds}).satisfiable());
    REQUIRE(ctl.stats()["summary"]["lower"][0].value() == 3.0);
    REQUIRE(bounds == std::vector<std::vector<Sum>>{{1}, {2}, {3}});
}

TEST_CASE("solve consequence", "[cxx][solve][consequence]") {
    auto lib = Library{};
    auto ctl = Control(lib, {"--enum-mode=brave"});
    ctl.parse_string("a. 1 {b; c} 1. {d}. :- d, b. :- d, c.");
    ctl.ground();

    auto lit = [&, base = ctl.base()](std::string_view name) {
        return base.get(Function(lib, name, {})).value().literal();
    };
    auto a = lit("a");
    auto b = lit("b");
    auto c = lit("c");
    auto d = lit("d");
    {
        auto hnd = ctl.start_solve();
        auto it = hnd.begin();
        REQUIRE(it != hnd.end());
        auto n1 = b;
        auto n2 = c;
        if (it->is_true(c)) {
            std::swap(n1, n2);
        }
        REQUIRE(it->type() == ModelType::brave_consequences);
        REQUIRE(it->is_consequence(a) == true);
        REQUIRE(it->is_consequence(n1) == true);
        REQUIRE(!it->is_consequence(n2).has_value());
        REQUIRE(!it->is_consequence(d).has_value());
        REQUIRE(++it != hnd.end());
        REQUIRE(it->type() == ModelType::brave_consequences);
        REQUIRE(it->is_consequence(a) == true);
        REQUIRE(it->is_consequence(n1) == true);
        REQUIRE(it->is_consequence(n2) == true);
        REQUIRE(!it->is_consequence(d).has_value());
        REQUIRE(++it == hnd.end());
        REQUIRE(hnd.last().has_value());
        REQUIRE(hnd.last()->type() == ModelType::brave_consequences);
        REQUIRE(hnd.last()->is_consequence(a) == true);
        REQUIRE(hnd.last()->is_consequence(n1) == true);
        REQUIRE(hnd.last()->is_consequence(n2) == true);
        REQUIRE(hnd.last()->is_consequence(d) == false);
        REQUIRE(hnd.get().satisfiable());
    }
}

} // namespace Clingo::Test
