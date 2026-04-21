#include <algorithm>
#include <clingo/base.hh>
#include <clingo/control.hh>
#include <clingo/propagate.hh>

#include <catch2/catch_test_macros.hpp>

#include "cbs.hh"

#include <array>

namespace Clingo::Test {

namespace {

struct Fixture {
    Library lib;
    Control ctl{lib, {"0"}};

    [[nodiscard]] auto fun(std::string_view name, std::initializer_list<int> args = {}) const -> Symbol {
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
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{{"p(1)"}, {"p(1)", "p(2)", "p(3)", "p(4)"}});
}

TEST_CASE_METHOD(Fixture, "backend edge", "[cxx][backend][edge]") {
    ctl.ground();
    {
        auto bck = ctl.backend();
        auto a = bck.atom(fun("a"));
        auto b = bck.atom(fun("b"));
        bck.rule(std::array{a, b}, {}, true);
        bck.edge(1, 2, std::array{lit(a)});
        bck.edge(2, 1, std::array{lit(b)});
    }
    std::vector<std::vector<std::string>> models;
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{{}, {"a"}, {"b"}});
}

TEST_CASE_METHOD(Fixture, "backend external", "[cxx][backend][external]") {
    auto run = [&](ExternalType t) -> std::vector<std::vector<std::string>> {
        ctl.ground();
        {
            auto bck = ctl.backend();
            auto a = bck.atom(fun("a", {}));
            bck.external(a, t);
        }
        std::vector<std::vector<std::string>> models;
        REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
        return models;
    };

    REQUIRE(run(ExternalType::free) == std::vector<std::vector<std::string>>{{}, {"a"}});
    REQUIRE(run(ExternalType::false_) == std::vector<std::vector<std::string>>{{}});
    REQUIRE(run(ExternalType::true_) == std::vector<std::vector<std::string>>{{"a"}});
    REQUIRE(run(ExternalType::release) == std::vector<std::vector<std::string>>{{}});
}

TEST_CASE_METHOD(Fixture, "backend assume", "[cxx][backend][assume]") {
    ctl.ground();
    {
        auto bck = ctl.backend();
        auto a = bck.atom(fun("a", {}));
        bck.rule(std::array{a}, {}, true);
    }

    auto run = [&](bool t) -> std::vector<std::vector<std::string>> {
        ctl.ground();
        {
            auto bck = ctl.backend();
            auto a = bck.atom(fun("a", {}));
            bck.assume(std::array{t ? lit(a) : lit(-a)});
        }
        std::vector<std::vector<std::string>> models;
        REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
        return models;
    };

    REQUIRE(run(true) == std::vector<std::vector<std::string>>{{"a"}});
    REQUIRE(run(false) == std::vector<std::vector<std::string>>{{}});
}

TEST_CASE_METHOD(Fixture, "backend project", "[cxx][backend][project]") {
    ctl.config().map().get("solve").map().get("project").value("auto");

    ctl.ground();
    {
        auto bck = ctl.backend();
        auto a = bck.atom(fun("a"));
        auto b = bck.atom(fun("b"));
        auto c = bck.atom(fun("c"));
        auto d = bck.atom(fun("d"));
        bck.rule(std::array{a, b, c, d});
        bck.project(std::array{a, b});
    }
    std::vector<std::vector<std::string>> models;
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models.size() == 3);
}

TEST_CASE_METHOD(Fixture, "backend minimize", "[cxx][backend][minimize]") {
    ctl.ground();
    {
        auto bck = ctl.backend();
        auto a = bck.atom(fun("a"));
        auto b = bck.atom(fun("b"));
        auto c = bck.atom(fun("c"));
        auto d = bck.atom(fun("d"));
        bck.rule(std::array{a, b, c, d}, {}, true);
        bck.minimize(std::to_array<WeightedLiteral>({{lit(a), 1}, {lit(b), -1}, {lit(c), 1}, {lit(d), -1}}));
    }
    std::optional<std::vector<std::string>> model;
    {
        class OM : public SolveEventHandler {
          public:
            OM(std::optional<std::vector<std::string>> &model) : model_{&model} {}

          private:
            auto do_model(Model model) -> bool override {
                std::vector<std::string> syms;
                for (auto &sym : model.symbols(ShowFlags::shown)) {
                    syms.push_back(sym.to_string());
                }
                std::ranges::sort(syms);
                *this->model_ = syms;
                return true;
            }
            std::optional<std::vector<std::string>> *model_;
        };
        REQUIRE(ctl.solve({}, OM{model}).satisfiable());
    }
    REQUIRE(model.has_value());
    REQUIRE(*model == std::vector<std::string>{"b", "d"});
}

TEST_CASE_METHOD(Fixture, "backend heuristic", "[cxx][backend][heuristic]") {
    ctl.config().map().get("solver").map().get("heuristic").value("domain");
    ctl.config().map().get("solve").map().get("models").value("1");

    ctl.ground();
    {
        auto bck = ctl.backend();
        auto a = bck.atom(fun("a", {}));
        auto b = bck.atom(fun("b", {}));
        auto c = bck.atom(fun("c", {}));
        auto d = bck.atom(fun("d", {}));
        bck.rule(std::array{a, b, c, d}, {}, true);
        bck.heuristic(a, HeuristicType::true_, 1);
        bck.heuristic(b, HeuristicType::false_, 1);
        bck.heuristic(c, HeuristicType::true_, 1);
        bck.heuristic(d, HeuristicType::false_, 1);
    }
    std::vector<std::vector<std::string>> models;
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == std::vector<std::vector<std::string>>{{"a", "c"}});
}

TEST_CASE_METHOD(Fixture, "backend theory", "[cxx][backend][theory]") {
    ctl.ground();
    {
        auto bck = ctl.backend();
        auto const &thy = bck.theory();
        auto n = fun("p", {2});
        auto txt = thy.string("a");
        auto sym = thy.symbol(n);
        auto num = thy.number(1);
        auto lst = thy.sequence(TheorySequenceType::list, std::vector{txt, num});
        auto lot = thy.sequence(TheorySequenceType::set, std::vector{num, sym});
        auto tup = thy.sequence(TheorySequenceType::tuple, std::vector{sym, txt});
        auto fun_term = thy.function("f", std::vector{sym, txt, num});
        auto e = thy.element(std::vector{fun_term, lst, lot, tup}, {});
        std::ignore = thy.atom(0, n, std::vector{e});
    }
    REQUIRE(ctl.base().theory().at(0).to_string() == "&p(2) { f(p(2),a,1),[a,1],{1,p(2)},(p(2),a) }");
    REQUIRE(ctl.solve().satisfiable());

    ctl.ground();
    {
        auto bck = ctl.backend();
        auto const &thy = bck.theory();
        auto n = fun("p", {});
        auto num = thy.number(1);
        std::ignore = thy.atom(0, n, {}, std::make_pair(std::string("<="), num));
    }
    REQUIRE(ctl.base().theory().at(0).to_string() == "&p { } <= 1");
    REQUIRE(ctl.solve().satisfiable());
}

namespace {

class TestProgagator : public Clingo::Propagator {
  public:
    TestProgagator(ProgramLiteral lit) : lit_{lit} {}
    auto do_init(Assignment assign, PropagateInit init) -> void override {
        auto lit = init.solver_literal(lit_);
        //  NOTE: this fails even though -2 is a valid aspif literal
        REQUIRE(assign.value(lit) == false);
        // NOTE: interestingly the opposite works
        // auto lit = -init.solver_literal(-lit_);
        // REQUIRE(assign.value(lit) == false);
    }

  private:
    ProgramLiteral lit_;
};

} // namespace

TEST_CASE_METHOD(Fixture, "backend theory condition", "[cxx][backend][theory][condition]") {
    ctl.parse_string(R"(
#theory sum {
    sum { };
    &sum/0 : sum, {=}, sum, any
}.
{a(1..3)}.
&sum { x : not a(2) } = 0.
:- not a(2).
)");
    ctl.ground();
    auto thy = ctl.base().theory();
    REQUIRE(thy.size() == 1);
    auto atom = thy.at(0);
    REQUIRE(atom.elements().size() == 1);
    auto elem = atom.elements().front();
    auto cond = elem.condition();
    REQUIRE(cond.size() == 1);
    auto lit = cond.front();
    auto atm = ctl.base().get(fun("a", {2}));
    REQUIRE(atm.has_value());
    REQUIRE(atm->literal() == -lit);
    // NOTE: We get a negative literal here because the condition is a singleton.
    REQUIRE(lit == elem.condition_id());
    auto prp = TestProgagator{lit};
    ctl.register_propagator(std::ref(prp));
    auto res = ctl.solve();
    REQUIRE(res.satisfiable());
}

} // namespace Clingo::Test
