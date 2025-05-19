#include <clingo/control.hh>
#include <clingo/observe.hh>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace Clingo::Test {

namespace {
class ExampleObserver : public Observer {
  public:
    void do_init_program(bool incremental) override {
        if (this->incremental.has_value()) {
            throw std::runtime_error("multiple calls to init_program");
        }
        this->incremental = incremental;
    }

    void do_begin_step() override { ++begin_steps; }

    void do_end_step(Base base) override {
        std::vector<Symbol> tmp;
        for (const auto &[_, atoms] : base) {
            for (const auto &[_, atom] : atoms) {
                if (base.is_current(atom.literal())) {
                    tmp.push_back(atom.symbol());
                }
            }
        }
        std::ranges::sort(tmp);
        symbols.clear();
        symbols.reserve(tmp.size());
        for (const auto &sym : tmp) {
            symbols.push_back(sym.to_string());
        }
        ++end_steps;
    }

    void do_assume(ProgramLiteralSpan literals) override { assumptions.emplace_back(literals.begin(), literals.end()); }

    void do_rule(ProgramAtomSpan head, ProgramLiteralSpan body, bool choice) override {
        rules.emplace_back(std::vector<int>(head.begin(), head.end()), std::vector<int>(body.begin(), body.end()),
                           choice);
    }

    void do_weight_rule(ProgramAtomSpan head, Weight lower, WeightedLiteralSpan body, bool choice) override {
        std::vector<std::pair<int, int>> body_vec;
        for (const auto &wl : body) {
            body_vec.emplace_back(wl.literal, wl.weight);
        }
        weight_rules.emplace_back(std::vector<int>(head.begin(), head.end()), lower, body_vec, choice);
    }

    void do_project(ProgramAtomSpan atoms) override { projections.emplace_back(atoms.begin(), atoms.end()); }

    void do_external(ProgramAtom atom, ExternalType type) override { externals.emplace_back(atom, type); }

    void do_heuristic(ProgramAtom atom, HeuristicType type, int bias, unsigned priority,
                      ProgramLiteralSpan condition) override {
        heuristics.emplace_back(atom, type, bias, priority, std::vector<int>(condition.begin(), condition.end()));
    }

    void do_edge(int node_u, int node_v, ProgramLiteralSpan condition) override {
        edges.emplace_back(node_u, node_v, std::vector<int>(condition.begin(), condition.end()));
    }

    void do_minimize(WeightedLiteralSpan literals, Weight priority) override {
        std::vector<std::pair<int, int>> lit_vec;
        for (const auto &wl : literals) {
            lit_vec.emplace_back(wl.literal, wl.weight);
        }
        minimizes.emplace_back(lit_vec, priority);
    }

    std::vector<std::vector<int>> assumptions;
    std::vector<std::tuple<int, int, std::vector<int>>> edges;
    std::vector<std::tuple<int, ExternalType>> externals;
    std::vector<std::tuple<int, HeuristicType, int, unsigned, std::vector<int>>> heuristics;
    std::optional<bool> incremental;
    std::vector<std::tuple<std::vector<std::pair<int, int>>, int>> minimizes;
    std::vector<std::vector<int>> projections;
    std::vector<std::tuple<std::vector<int>, std::vector<int>, bool>> rules;
    std::vector<std::tuple<std::vector<int>, int, std::vector<std::pair<int, int>>, bool>> weight_rules;
    std::vector<std::string> symbols;
    int begin_steps = 0;
    int end_steps = 0;
};

struct Fixture {
    Library lib;
    Control ctl{lib};

    static auto lit(ProgramAtom atom) -> ProgramLiteral { return static_cast<ProgramLiteral>(atom); }
};

} // namespace

TEST_CASE_METHOD(Fixture, "observe", "[cxx][observe]") {
    auto obs = ExampleObserver{};

    ctl.parse_string("#program a. {a}. {c}. b :- a, c.");
    ctl.ground({{"a", {}}});
    ctl.observe(obs);
    REQUIRE(obs.incremental.value_or(false)); // Should be true
    REQUIRE(obs.begin_steps == 1);
    REQUIRE(obs.end_steps == 1);
    REQUIRE(obs.rules.size() == 2);
    REQUIRE(std::count_if(obs.rules.begin(), obs.rules.end(), [](const auto &r) { return std::get<2>(r); }) == 1);
    REQUIRE(std::count_if(obs.rules.begin(), obs.rules.end(), [](const auto &r) { return !std::get<2>(r); }) == 1);
    REQUIRE(obs.symbols == std::vector<std::string>{"a", "b", "c"});

    ctl.parse_string("#program b. {p(1..20)}. :- #sum { X: p(X) } >= 40.");
    ctl.ground({{"b", {}}});
    ctl.observe(obs);
    REQUIRE(obs.begin_steps == 2);
    REQUIRE(obs.end_steps == 2);
    REQUIRE(obs.weight_rules.size() == 1);
    REQUIRE(std::get<0>(obs.weight_rules[0]).empty());
    REQUIRE(std::get<1>(obs.weight_rules[0]) == 40);
    REQUIRE(std::get<2>(obs.weight_rules[0]).size() == 20);
    REQUIRE(!std::get<3>(obs.weight_rules[0]));
    REQUIRE(obs.symbols.size() == 20);

    ctl.parse_string("#program c. #project p/1.");
    ctl.ground({{"c", {}}});
    ctl.observe(obs);
    REQUIRE(obs.begin_steps == 3);
    REQUIRE(obs.end_steps == 3);
    REQUIRE(obs.projections.size() == 1);
    REQUIRE(obs.projections[0].size() == 20);
    REQUIRE(obs.symbols.empty());

    {
        auto backend = ctl.backend();
        auto a = backend.atom(Function(lib, "a"));
        auto b = backend.atom(Function(lib, "b"));
        auto c = backend.atom(Function(lib, "c"));
        backend.assume(std::array{lit(a), lit(b), lit(c)});
    }
    ctl.observe(obs);
    REQUIRE(obs.begin_steps == 4);
    REQUIRE(obs.end_steps == 4);
    REQUIRE(obs.assumptions.size() == 1);
    REQUIRE(obs.assumptions[0].size() == 3);
    REQUIRE(obs.symbols.empty());

    ctl.parse_string("#program d. #external a. [true] #external d. [true]");
    ctl.ground({{"d", {}}});
    ctl.observe(obs);
    REQUIRE(obs.begin_steps == 5);
    REQUIRE(obs.end_steps == 5);
    REQUIRE(obs.externals.size() == 1);
    REQUIRE(std::get<1>(obs.externals[0]) == ExternalType::true_);
    REQUIRE(obs.symbols.size() == 1);

    ctl.parse_string("#program e. #heuristic a. [1@2,sign]");
    ctl.ground({{"e", {}}});
    ctl.observe(obs);
    REQUIRE(obs.begin_steps == 6);
    REQUIRE(obs.end_steps == 6);
    REQUIRE(obs.heuristics.size() == 1);
    REQUIRE(std::get<1>(obs.heuristics[0]) == HeuristicType::sign);
    REQUIRE(std::get<2>(obs.heuristics[0]) == 1);
    REQUIRE(std::get<3>(obs.heuristics[0]) == 2);
    REQUIRE(std::get<4>(obs.heuristics[0]).empty());
    REQUIRE(obs.symbols.empty());

    ctl.parse_string("#program f. #edge (1, 2) : a, c.");
    ctl.ground({{"f", {}}});
    ctl.observe(obs);
    REQUIRE(obs.edges.size() == 1);
    REQUIRE(std::get<0>(obs.edges[0]) != std::get<1>(obs.edges[0]));
    REQUIRE(std::get<2>(obs.edges[0]).size() == 2);
    REQUIRE(obs.symbols.empty());

    ctl.parse_string("#program g. #minimize{ 1@2: a; 2@3: b }.");
    ctl.ground({{"g", {}}});
    ctl.observe(obs);
    REQUIRE(obs.minimizes.size() == 2);
    auto mins = obs.minimizes;
    std::ranges::sort(mins, [](const auto &a, const auto &b) { return std::get<1>(a) < std::get<1>(b); });
    REQUIRE(std::get<1>(mins[0]) == 2);
    REQUIRE(std::get<1>(mins[1]) == 3);
    REQUIRE(obs.symbols.empty());
}

TEST_CASE_METHOD(Fixture, "observe preprocess", "[cxx][observe][preprocess]") {
    ctl.parse_string("{a;c}. b :- a. :- a.");
    ctl.ground();

    {
        auto obs = ExampleObserver{};
        ctl.observe(obs, false);
        REQUIRE(obs.incremental.value_or(false));
        REQUIRE(obs.begin_steps == 1);
        REQUIRE(obs.end_steps == 1);
        REQUIRE(obs.symbols == std::vector<std::string>{"a", "b", "c"});
        REQUIRE(obs.rules.size() == 3);
    }

    {
        auto obs = ExampleObserver{};
        ctl.observe(obs, true);
        REQUIRE(obs.incremental.value_or(false));
        REQUIRE(obs.begin_steps == 1);
        REQUIRE(obs.end_steps == 1);
        REQUIRE(obs.symbols == std::vector<std::string>{"a", "b", "c"});
        REQUIRE(obs.rules.size() == 1);
        REQUIRE(std::get<0>(obs.rules[0]) == std::vector<int>{2});
        REQUIRE(std::get<1>(obs.rules[0]).empty());
        REQUIRE(std::get<2>(obs.rules[0]));
    }

    std::ignore = ctl.solve().get();
    {
        auto obs = ExampleObserver{};
        ctl.observe(obs);
        REQUIRE(obs.begin_steps == 1);
        REQUIRE(obs.end_steps == 1);
        REQUIRE(obs.symbols == std::vector<std::string>{"c"});
        REQUIRE(obs.rules.empty());
    }
}

} // namespace Clingo::Test
