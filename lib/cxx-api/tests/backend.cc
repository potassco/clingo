#include <clingo/base.hh>
#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

#include "cbs.hh"

namespace Clingo::Test {

namespace {

struct Fixture {
    Library lib;
    Control ctl{lib, {"0"}};

    [[nodiscard]] auto fun(std::string_view name, std::initializer_list<int> args) const -> Symbol {
        auto syms = std::vector<Symbol>{};
        for (auto arg : args) {
            syms.emplace_back(Number(arg));
        }
        return Function(lib, name, syms);
    }

    static auto lit(ProgramAtom atm) -> ProgramLiteral { return static_cast<ProgramLiteral>(atm); }
};

} // namespace

TEST_CASE_METHOD(Fixture, "backend rule", "[cxx][backend][rule]") {
    ctl.ground();
    {
        auto bck = ctl.backend();
        auto p1 = fun("p", {1});
        auto p2 = fun("p", {2});
        auto p3 = fun("p", {3});
        auto p4 = fun("p", {4});
        auto a1 = bck.atom(p1);
        auto a2 = bck.atom(p2);
        auto a3 = bck.atom(p3);
        auto a4 = bck.atom(p4);
        bck.rule(std::array{a1});
        bck.rule(std::array{a2}, {}, true);
        bck.rule(std::array{a3}, std::array{lit(a2)});
        bck.weight_rule(std::array{a4}, 4, std::to_array<WeightedLiteral>({{lit(a1), 2}, {lit(a2), 1}, {lit(a3), 1}}));
    }
    std::vector<std::vector<std::string>> models;
    {
        auto mcb = MCB{models};
        auto hnd = ctl.solve(mcb);
        REQUIRE(hnd.get().satisfiable());
    }
    REQUIRE(models == std::vector<std::vector<std::string>>{{"p(1)"}, {"p(1)", "p(2)", "p(3)", "p(4)"}});
}

} // namespace Clingo::Test
