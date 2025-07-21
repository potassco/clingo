#include <clingo/observe.hh>
#include <clingo/propagate.hh>
#include <clingo/script.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <sstream>

namespace Clingo::Test {

namespace {

class TestPropagator : public Heuristic {
  public:
    explicit TestPropagator(std::string throw_str) : throw_{std::move(throw_str)} {}

  private:
    void do_init([[maybe_unused]] Assignment assignment, PropagateInit init) override {
        if (throw_.find('i') != std::string::npos) {
            throw std::runtime_error("prop: init");
        }
        init.check_mode(PropagatorCheckMode::total);
        init.add_watch(init.add_literal());
    }

    void do_propagate([[maybe_unused]] Assignment assignment, [[maybe_unused]] PropagateControl control,
                      [[maybe_unused]] SolverLiteralSpan changes) override {
        if (throw_.find('p') != std::string::npos) {
            throw std::runtime_error("prop: propagate");
        }
    }

    auto do_decide([[maybe_unused]] Assignment const assignment, SolverLiteral fallback) -> SolverLiteral override {
        if (throw_.find('d') != std::string::npos) {
            throw std::runtime_error("prop: decide");
        }
        return fallback;
    }

    std::string throw_;
};

class TestObserver : public Observer {
  private:
    void do_rule(ProgramAtomSpan head, SolverLiteralSpan body, bool choice) override {
        auto oss = std::ostringstream{};
        oss << "rule: [ ";
        for (auto h : head) {
            oss << h << " ";
        }
        oss << "] :- [ ";
        for (auto b : body) {
            oss << b << " ";
        }
        oss << "]. [ " << choice << " ]";
        throw std::runtime_error(oss.str());
    }
};

class TestScript : public Script {
  public:
    void do_execute([[maybe_unused]] std::string_view code) override {}
    auto do_call([[maybe_unused]] Library &lib, std::string_view name, SymbolSpan arguments) -> SymbolVector override {
        auto oss = std::ostringstream{};
        oss << name << " called with ";
        for (auto const &arg : arguments) {
            oss << arg << ", ";
        }
        throw std::runtime_error(oss.str());
    }
    auto do_callable(std::string_view name, [[maybe_unused]] size_t arguments) -> bool override {
        return name != "main";
    }
    void do_main([[maybe_unused]] Library &lib, [[maybe_unused]] Control const &ctl) override {}
    auto do_name() -> std::string_view override { return "test"; }
    auto do_version() -> std::string_view override { return "version"; }
};

struct Fixture {
    Library lib;
    Control ctl = Control{lib, {"0"}};
};

} // namespace

TEST_CASE_METHOD(Fixture, "error context", "[cxx][error][context]") {
    using namespace Catch::Matchers;

    ctl.parse_string("p(@fun(1)). q.");

    auto ctx = []([[maybe_unused]] std::string_view name, SymbolSpan params) -> SymbolVector {
        auto oss = std::ostringstream{};
        oss << "fun called with " << params.front();
        throw std::runtime_error{oss.str()};
    };

    REQUIRE_THROWS_MATCHES(ctl.ground(std::nullopt, ctx), std::runtime_error,
                           MessageMatches(ContainsSubstring("fun called with 1")));
}

TEST_CASE_METHOD(Fixture, "error on_model", "[cxx][error][on_model]") {
    using namespace Catch::Matchers;

    auto ctl = Control{lib};
    register_script(lib, std::make_unique<TestScript>());
    ctl.parse_string("a. b.");
    ctl.ground();

    struct EH : SolveEventHandler {
        auto do_model(Model model) -> bool override { throw std::runtime_error(model.to_string()); };
    } eh;

    REQUIRE_THROWS_MATCHES(ctl.solve(eh), std::runtime_error, MessageMatches(ContainsSubstring("a, b")));
}

TEST_CASE_METHOD(Fixture, "error script", "[cxx][error][script]") {
    using namespace Catch::Matchers;

    register_script(lib, std::make_unique<TestScript>()); // assume this registers your script
    ctl.parse_string("p(@fun(1)). q.");

    REQUIRE_THROWS_MATCHES(ctl.ground(), std::runtime_error, MessageMatches(ContainsSubstring("fun called with 1")));
    REQUIRE_THROWS_MATCHES(ctl.main(), std::runtime_error, MessageMatches(ContainsSubstring("fun called with 1")));
}

TEST_CASE_METHOD(Fixture, "error observer", "[cxx][error][observer]") {
    using namespace Catch::Matchers;

    auto obs = TestObserver{};
    ctl.parse_string("{a}.");
    ctl.ground();

    REQUIRE_THROWS_MATCHES(ctl.observe(obs), std::runtime_error,
                           MessageMatches(ContainsSubstring("rule: [ 1 ] :- [ ]. [ 1 ]")));
}

TEST_CASE_METHOD(Fixture, "error propagator", "[cxx][error][propagator]") {
    using namespace Catch::Matchers;

    auto [throw_str, msg] =
        GENERATE(std::pair{"i", "prop: init"}, std::pair{"p", "prop: propagate"}, std::pair{"d", "prop: decide"});

    SECTION("solve") {
        ctl.register_propagator(std::make_unique<TestPropagator>(throw_str));
        REQUIRE_THROWS_MATCHES(ctl.main(), std::runtime_error, MessageMatches(ContainsSubstring(msg)));
    }

    SECTION("main") {
        ctl.register_propagator(std::make_unique<TestPropagator>(throw_str));
        REQUIRE_THROWS_MATCHES(
            [&] {
                auto hnd = ctl.solve({}, SolveFlags::empty);
                std::ignore = hnd.get();
            }(),
            std::runtime_error, MessageMatches(ContainsSubstring(msg)));
    }
}

} // namespace Clingo::Test
