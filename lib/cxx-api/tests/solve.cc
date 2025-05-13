#include <clingo/control.hh>
#include <clingo/core.hh>
#include <clingo/solve.hh>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace Clingo::Test {

namespace {

class MCB : public SolveEventHandler {
  public:
    MCB(std::vector<std::vector<std::string>> &models) : models_{&models} { models_->clear(); }
    ~MCB() override { std::ranges::sort(*models_); }

    using SolveEventHandler::model;

    auto model(ConstModel &model) -> bool {
        models_->emplace_back();
        for (auto &sym : model.symbols(Clingo::ShowFlags::shown)) {
            models_->back().push_back(sym.to_string());
        }
        std::ranges::sort(models_->back());
        return true;
    }

  private:
    auto do_model(Model &model) -> bool override { return MCB::model(static_cast<ConstModel &>(model)); }
    std::vector<std::vector<std::string>> *models_;
};

class Extender : public SolveEventHandler {
  public:
    Extender(Library &lib) : lib_{&lib} {}

  private:
    auto do_model(Model &model) -> bool override {
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
    {
        auto mcb = MCB{models};
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
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
        auto hnd = ctl.solve({}, SolveFlags::yield);
        for (auto &it : hnd) {
            mcb.model(it);
        }
        REQUIRE(hnd.get().satisfiable());
    }
    std::ranges::sort(models);
    REQUIRE(models == std::vector<std::vector<std::string>>{{"a"}, {"b"}});
}

TEST_CASE("solve async", "[cxx][solve][async]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("1 { a; b } 1.");
    ctl.ground({{"base", {}}});

    auto models = std::vector<std::vector<std::string>>{};
    {
        auto mcb = MCB{models};
        auto hnd = ctl.solve(mcb, {}, SolveFlags::async);
        while (!hnd.wait(0.01)) {
            // simulate doing something else
        }
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == std::vector<std::vector<std::string>>{{"a"}, {"b"}});
}

TEST_CASE("solve assume", "[cxx][solve][assume]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("{a; b; c}. :- a, b.");
    ctl.ground({{"base", {}}});

    auto lit = [&](std::string_view name) { return ctl.base().get(Function(lib, name, {})).value().literal(); };

    auto assumptions = std::vector{lit("a"), lit("b"), lit("c")};
    {
        auto hnd = ctl.solve(assumptions);
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
        auto ext = Extender{lib};
        auto hnd = ctl.solve(ext, {});
        REQUIRE(hnd.get().satisfiable());
        REQUIRE(hnd.last()->symbols(ShowFlags::theory).size() == 2);
        REQUIRE(hnd.last()->symbols(ShowFlags::shown).size() == 3);
        REQUIRE(hnd.last()->to_string() == "a, b, c");
    }
}

TEST_CASE("solve extend yield", "[solve][extend][yield]") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("a.");
    ctl.ground({{"base", {}}});
    auto models = std::vector<std::vector<std::string>>{};
    {
        auto ext = Extender{lib};
        auto mcb = MCB{models};
        auto hnd = ctl.solve(ext, {}, SolveFlags::yield);
        for (auto &&mdl : hnd) {
            mcb.model(mdl);
        }
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == std::vector<std::vector<std::string>>{{"a", "b", "c"}});
}

/*
TEST_CASE("solve consequence", "[solve][consequence]") {
    Clingo::Control ctl({"0"});
    ctl.add("base", {}, "a. b :- a.");
    ctl.ground({{"base", {}}});

    // Consequence API is a bit different in C++
    // This is a sketch; you may need to adapt for actual C++ API
    std::vector<Clingo::Symbol> atoms = {ctl.symbolic_atoms().symbol(ctl.symbolic_atoms().find("a")),
                                         ctl.symbolic_atoms().symbol(ctl.symbolic_atoms().find("b"))};
    std::vector<Clingo::Symbol> consequences;
    ctl.consequences(Clingo::LiteralSpan{}, atoms, [&](Clingo::Consequences &cons) {
        for (auto &sym : cons.added()) {
            consequences.push_back(sym);
        }
    });
    REQUIRE(std::find_if(consequences.begin(), consequences.end(),
                         [](const Clingo::Symbol &s) { return s.to_string() == "a"; }) != consequences.end());
    REQUIRE(std::find_if(consequences.begin(), consequences.end(),
                         [](const Clingo::Symbol &s) { return s.to_string() == "b"; }) != consequences.end());
}

*/

} // namespace Clingo::Test
