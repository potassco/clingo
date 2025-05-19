#include <algorithm>
#include <clingo/control.hh>
#include <clingo/propagate.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "cbs.hh"

#include <barrier>

namespace Clingo::Test {

namespace {

class AIFFBPropagator : public Propagator {
  public:
    void do_init(PropagateInit init) override {
        auto watch = [&](const char *p) {
            auto plit = init.base().get(Function(init.library(), p))->literal();
            int slit = init.solver_literal(plit);
            init.add_watch(slit);
            return slit;
        };

        if (slit_a == 0) {
            slit_a = watch("a");
            slit_b = watch("b");
        }
        errors.clear();
        n_threads = init.number_of_threads();
        if (fail_thread < n_threads) {
            barrier.emplace(n_threads);
        } else {
            barrier.reset();
        }
    }

    void do_check(PropagateControl control) override {
        for (int p : {slit_a, slit_b}) {
            if (!control.has_watch(p)) {
                auto lock = std::lock_guard{mut};
                errors.push_back("solver " + std::to_string(control.thread_id()) + " misses watch " +
                                 std::to_string(p));
            }
        }
        if (barrier) {
            barrier->arrive_and_wait();
            {
                auto lock = std::lock_guard{mut};
                if (barrier) {
                    barrier.reset();
                }
            }
            if (control.thread_id() == fail_thread) {
                throw std::runtime_error("Forcing error on solver " + std::to_string(control.thread_id()));
            }
        }
    }

    void do_propagate(PropagateControl control, SolverLiteralSpan changes) override {
        auto propagate = [&](auto p, auto q) {
            if (std::ranges::find(changes, p) != changes.end()) {
                assert(control.assignment().is_true(p));
                auto clause = std::vector{-p, q};
                std::ignore = control.add_clause(clause, ClauseFlags::tag);
            }
        };
        propagate(slit_a, slit_b);
        propagate(slit_b, slit_a);
    }

    SolverLiteral slit_a = 0;
    SolverLiteral slit_b = 0;
    ProgramId n_threads = 1;
    ProgramId fail_thread = std::numeric_limits<ProgramId>::max();
    std::vector<std::string> errors;
    std::optional<std::barrier<>> barrier;
    std::mutex mut;
};

class AssertingPropagator : public Heuristic {
  public:
    AssertingPropagator(bool lock = false) : lock_(lock) {}

    void do_init(PropagateInit init) override {
        auto lit = [&](std::string_view name) {
            for (auto base = init.base().get(std::make_pair(name, 0)); auto const &[_, atom] : *base) {
                return init.solver_literal(atom.literal());
            }
            throw std::logic_error{"must not happend"};
        };
        start_lit_ = lit("start");
        end_lit_ = lit("end");
        value_lit_ = lit("value");

        auto lits = std::vector<int>{start_lit_, end_lit_, value_lit_};
        std::ranges::sort(lits);
        for (int lit : lits) {
            init.add_watch(lit);
            init.add_watch(-lit);
        }
    }

    void do_propagate(PropagateControl control, SolverLiteralSpan changes) override {
        REQUIRE(!changes.empty());
        auto ass = control.assignment();
        if (ass.is_false(value_lit_) && ass.is_false(end_lit_)) {
            auto nogood = std::array{start_lit_, -end_lit_, -value_lit_};
            auto dl = ass.decision_level();
            REQUIRE(!control.add_nogood(nogood, lock_ ? ClauseFlags::lock : ClauseFlags::none));
            REQUIRE(ass.decision_level() == dl);
        }
    }

    auto do_decide(ProgramId thread_id, Assignment assignment, SolverLiteral fallback) -> int override {
        REQUIRE(thread_id == 0);
        if (assignment.is_free(end_lit_)) {
            return -end_lit_;
        }
        if (assignment.is_free(value_lit_)) {
            return -value_lit_;
        }
        return fallback;
    }

  private:
    int start_lit_ = 0;
    int end_lit_ = 0;
    int value_lit_ = 0;
    bool lock_ = false;
};

struct Fixture {
    Library lib;
    Control ctl{lib};

    static auto lit(ProgramAtom atom) -> ProgramLiteral { return static_cast<ProgramLiteral>(atom); }
};

} // namespace

TEST_CASE_METHOD(Fixture, "propagate a iff b", "[cxx][propagator]") {
    auto prop = std::make_unique<AIFFBPropagator>();
    auto &ref = *prop;
    ctl.register_propagator(std::move(prop));
    ctl.parse_string("1 { a; b }.");
    ctl.ground();

    for (ProgramId n : {1, 3}) {
        ctl.config()["solve"]["parallel_mode"].value(std::to_string(n));
        auto models = MV{};
        {
            auto mcb = MCB{models};
            auto hnd = ctl.solve(mcb);
            REQUIRE(hnd.get().satisfiable());
            REQUIRE(ref.n_threads == n);
            REQUIRE(ref.errors.empty());
        }
        REQUIRE(models == MV{{"a", "b"}});
    }
}

TEST_CASE_METHOD(Fixture, "propagate exception", "[cxx][propagator][exception]") {
    using namespace Catch::Matchers;

    auto prop = std::make_unique<AIFFBPropagator>();
    auto &ref = *prop;
    ctl.register_propagator(std::move(prop));
    ctl.parse_string("1 { a; b }.");
    ctl.ground();

    for (ProgramId n : {8, 1}) {
        ctl.config()["solve"]["parallel_mode"].value(std::to_string(n));
        ref.fail_thread = n - 1;
        auto models = MV{};
        auto solve = [&]() {
            auto mcb = MCB{models};
            auto hnd = ctl.solve(mcb);
            REQUIRE(hnd.get().satisfiable());
        };

        REQUIRE_THROWS_MATCHES(solve(), std::runtime_error,
                               MessageMatches(ContainsSubstring("solver " + std::to_string(n - 1))));
        REQUIRE(ref.n_threads == n);
    }
}

TEST_CASE_METHOD(Fixture, "propagate asserting", "[cxx][propagator][asserting]") {
    bool locked = GENERATE(false, true);

    auto prop = std::make_unique<AssertingPropagator>(locked);
    ctl.register_heuristic(std::move(prop));
    ctl.parse_string("start. {value}. {end}.");
    ctl.ground();

    auto hnd = ctl.solve();
    REQUIRE(hnd.get().satisfiable());
}
} // namespace Clingo::Test
