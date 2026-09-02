#include <algorithm>
#include <clingo/control.hh>
#include <clingo/propagate.hh>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "cbs.hh"

#include <array>
#include <barrier>
#include <mutex>
#include <set>

namespace Clingo::Test {

namespace {

class AIFFBPropagator : public Propagator {
  public:
    void do_init([[maybe_unused]] Assignment assignment, PropagateInit init) override {
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
        attached.clear();
        n_threads = init.number_of_threads();
        if (fail_thread < n_threads) {
            barrier.emplace(n_threads);
        } else {
            barrier.reset();
        }
    }
    void do_attach([[maybe_unused]] Assignment assignment, [[maybe_unused]] PropagateControl ctl) override {
        auto thread_id = assignment.thread_id();
        auto lock = std::scoped_lock{mut};
        REQUIRE(thread_id < n_threads);
        REQUIRE(attached.insert(thread_id).second);
    }

    void do_check(Assignment assignment, PropagateControl control) override {
        auto thread_id = assignment.thread_id();
        for (int p : {slit_a, slit_b}) {
            if (!control.has_watch(p)) {
                auto lock = std::scoped_lock{mut};
                errors.push_back("solver " + std::to_string(thread_id) + " misses watch " + std::to_string(p));
            }
        }
        {
            auto lock = std::scoped_lock{mut};
            REQUIRE(attached.contains(thread_id));
        }
        if (barrier) {
            barrier->arrive_and_wait();
            {
                auto lock = std::scoped_lock{mut};
                if (barrier) {
                    barrier.reset();
                }
            }
            if (thread_id == fail_thread) {
                throw std::runtime_error("Forcing error on solver " + std::to_string(thread_id));
            }
        }
    }

    void do_propagate([[maybe_unused]] Assignment assignment, PropagateControl control,
                      SolverLiteralSpan changes) override {
        auto propagate = [&](auto p, auto q) {
            if (std::ranges::find(changes, p) != changes.end()) {
                assert(assignment.is_true(p));
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
    std::set<ProgramId> attached;
    std::mutex mut;
};

class AssertingPropagator : public Heuristic {
  public:
    AssertingPropagator(bool lock = false) : lock_(lock) {}

    void do_init([[maybe_unused]] Assignment assignment, PropagateInit init) override {
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

    void do_propagate(Assignment ass, PropagateControl control, SolverLiteralSpan changes) override {
        REQUIRE(!changes.empty());
        if (ass.is_false(value_lit_) && ass.is_false(end_lit_)) {
            auto nogood = std::array{start_lit_, -end_lit_, -value_lit_};
            auto dl = ass.decision_level();
            REQUIRE(!control.add_nogood(nogood, lock_ ? ClauseFlags::lock : ClauseFlags::none));
            REQUIRE(ass.decision_level() == dl);
        }
    }

    auto do_decide(Assignment assignment, SolverLiteral fallback) -> int override {
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

class InitPropagator : public Propagator {
  public:
    void do_init(Assignment ass, PropagateInit init) override {
        auto lib = init.library();
        auto a = init.base().get(Function(lib, "a"));
        auto b = init.base().get(Function(lib, "b"));
        auto c = init.base().get(Function(lib, "c"));
        REQUIRE((a && b && c));

        int lit_a = init.solver_literal(a->literal());
        int lit_b = init.solver_literal(b->literal());
        int lit_c = init.solver_literal(c->literal());

        int lit = init.add_literal(false);

        // a <=> b
        std::ignore = init.add_clause(std::array{lit_a, -lit});
        std::ignore = init.add_clause(std::array{lit, -lit_a});
        std::ignore = init.add_clause(std::array{lit_b, -lit});
        std::ignore = init.add_clause(std::array{lit, -lit_b});

        // c <=> {a, b} >= 2
        std::ignore = init.add_weight_constraint(lit_c, std::to_array<WeightedLiteral>({{lit_a, 1}, {lit_b, 1}}), 2,
                                                 WeightConstraintType::equivalence);

        init.add_minimize(lit_a, -1, 0);

        REQUIRE(init.propagate());
        REQUIRE(init.base().size() == 3);

        // Assignment checks
        REQUIRE(ass.value(lit_a) == std::nullopt);
        REQUIRE(!ass.is_true(lit_a));
        REQUIRE(!ass.is_false(lit_a));
        REQUIRE(!ass.is_fixed(lit_a));
        REQUIRE(ass.decision_level() == 0);
        REQUIRE(ass.contains(lit_a));
        REQUIRE(!ass.has_conflict());
        REQUIRE(!ass.is_total());
        REQUIRE(ass.root_level() == 0);
        REQUIRE(ass.size() == 5);
        REQUIRE(std::distance(ass.begin(), ass.end()) == 5);
    }
};

class AddLiteralPropagator : public Propagator {
  public:
    void do_check([[maybe_unused]] Assignment assignment, PropagateControl control) override {
        if (!added_) {
            added_ = true;
            int lit = control.add_literal();
            REQUIRE(!control.has_watch(lit));
            control.add_watch(lit);
            REQUIRE(control.has_watch(lit));
            control.remove_watch(lit);
            REQUIRE(!control.has_watch(lit));
        }
    }

  private:
    bool added_ = false;
};

class HeuristicPropagator : public Heuristic {
  public:
    void do_init([[maybe_unused]] Assignment assignment, PropagateInit init) override {
        auto lib = init.library();
        auto a = init.base().get(Function(lib, "a"));
        auto b = init.base().get(Function(lib, "b"));
        REQUIRE((a && b));
        lit_a_ = init.solver_literal(a->literal());
        lit_b_ = init.solver_literal(b->literal());
    }

    auto do_decide(Assignment assignment, SolverLiteral fallback) -> int override {
        if (assignment.is_free(lit_a_)) {
            return lit_a_;
        }
        if (assignment.is_free(lit_b_)) {
            return -lit_b_;
        }
        return fallback;
    }

  private:
    SolverLiteral lit_a_ = 0;
    SolverLiteral lit_b_ = 0;
};

class PropagateControlPropagator : public Propagator {
  public:
    void do_init(Assignment ass, PropagateInit init) override {
        auto lib = init.library();
        init.check_mode(PropagatorCheckMode::none);
        REQUIRE(init.check_mode() == PropagatorCheckMode::none);
        REQUIRE(init.number_of_threads() == 1);
        REQUIRE(ass.size() >= 1);

        auto a = init.base().get(Function(lib, "a"));
        REQUIRE(a);
        lit_a_ = init.solver_literal(a->literal());
        REQUIRE(ass.contains(lit_a_));
        REQUIRE(ass.contains(-lit_a_));
        init.add_watch(-lit_a_);
    }

    void do_propagate(Assignment ass, PropagateControl control, SolverLiteralSpan changes) override {
        auto trail = ass.trail();
        auto lvl = ass.decision_level();

        REQUIRE(std::ranges::find(changes, -lit_a_) != changes.end());
        REQUIRE(lvl >= 1);
        REQUIRE(ass.level(lit_a_) >= 1);
        REQUIRE(trail.size() >= 1);
        REQUIRE(std::distance(trail.begin(), trail.end()) >= 1);
        REQUIRE(*(trail.begin(lvl)) == -lit_a_);
        REQUIRE(std::vector<SolverLiteral>{trail.begin(lvl), trail.end(lvl)} == std::vector<SolverLiteral>{-lit_a_});
        REQUIRE(ass.decision(lvl) == -lit_a_);
        REQUIRE(ass.thread_id() == 0);
        REQUIRE(control.has_watch(-lit_a_));
        REQUIRE(control.propagate());
        REQUIRE(!control.add_clause(std::array{lit_a_}));
    }

    void do_undo(Assignment assignment, SolverLiteralSpan changes) override {
        REQUIRE(assignment.size() > 0);
        REQUIRE(std::ranges::find(changes, -lit_a_) != changes.end());
    }

  private:
    SolverLiteral lit_a_ = 0;
};

class ModePropagator : public Propagator {
  public:
    void do_init([[maybe_unused]] Assignment assignment, PropagateInit init) override {
        init.check_mode(PropagatorCheckMode::fixpoint);
        init.undo_mode(PropagatorUndoMode::always);

        REQUIRE(init.check_mode() == PropagatorCheckMode::fixpoint);
        REQUIRE(init.undo_mode() == PropagatorUndoMode::always);
    }

    void do_check(Assignment assignment, [[maybe_unused]] PropagateControl control) override {
        ++num_check;
        auto dl = assignment.decision_level();
        REQUIRE(level.back() <= dl);
        if (level.back() != dl) {
            level.emplace_back(dl);
        }
    }

    void do_undo(Assignment assignment, SolverLiteralSpan changes) override {
        REQUIRE(assignment.size() > 0);
        REQUIRE(changes.empty());
        ++num_undo;
        REQUIRE(level.size() >= 2);
        level.pop_back();
    }

    SolverLiteral num_check = 0;
    SolverLiteral num_undo = 0;
    std::vector<ProgramId> level = {0};
};

class TheoryProgagator : public Clingo::Propagator {
  public:
    TheoryProgagator(ProgramLiteral lit) : lit_{lit} {}
    auto do_init(Assignment assign, PropagateInit init) -> void override {
        auto lit = init.solver_literal(lit_);
        REQUIRE(assign.value(lit) == false);
    }

  private:
    ProgramLiteral lit_;
};

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

    static auto lit(ProgramAtom atom) -> ProgramLiteral { return static_cast<ProgramLiteral>(atom); }
};

} // namespace

TEST_CASE_METHOD(Fixture, "propagate a iff b", "[cxx][propagate]") {
    auto prop = std::make_unique<AIFFBPropagator>();
    auto &ref = ctl.register_propagator(std::move(prop));
    ctl.parse_string("1 { a; b }.");
    ctl.ground();

    for (ProgramId n : {1U, 3U}) {
#ifdef __EMSCRIPTEN__
        if (n > 1) {
            continue;
        }
#endif
        ctl.config()["solve"]["parallel_mode"].value(std::to_string(n));

        auto models = MV{};
        REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
        REQUIRE(ref.n_threads == n);
        REQUIRE(ref.errors.empty());
        REQUIRE(models == MV{{"a", "b"}});
    }
}

TEST_CASE_METHOD(Fixture, "propagate exception", "[cxx][propagate][exception]") {
    using namespace Catch::Matchers;

    auto prop = std::make_unique<AIFFBPropagator>();
    auto &ref = ctl.register_propagator(std::move(prop));
    ctl.parse_string("1 { a; b }.");
    ctl.ground();

    for (ProgramId n : {8U, 1U}) {
#ifdef __EMSCRIPTEN__
        if (n > 1) {
            continue;
        }
#endif
        ctl.config()["solve"]["parallel_mode"].value(std::to_string(n));
        ref.fail_thread = n - 1;
        auto models = MV{};
        REQUIRE_THROWS_MATCHES(ctl.solve({}, MCB{models}), std::runtime_error,
                               MessageMatches(ContainsSubstring("solver " + std::to_string(n - 1))));
        REQUIRE(ref.n_threads == n);
        REQUIRE(ref.attached.size() == n);
    }
}

TEST_CASE_METHOD(Fixture, "propagate asserting", "[cxx][propagate][asserting]") {
    bool locked = GENERATE(false, true);

    auto prop = std::make_unique<AssertingPropagator>(locked);
    ctl.register_propagator(std::move(prop));
    ctl.parse_string("start. {value}. {end}.");
    ctl.ground();

    REQUIRE(ctl.solve().satisfiable());
}

TEST_CASE_METHOD(Fixture, "propagate init", "[cxx][propagate][init]") {
    ctl.config()["solve"]["opt_mode"] = "optN";
    ctl.parse_string("{a; b; c}.");
    ctl.ground();

    ctl.register_propagator(std::make_unique<InitPropagator>());

    auto models = MV{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == MV{{"a", "b", "c"}});
}

TEST_CASE_METHOD(Fixture, "propagate add_literal", "[cxx][propagate][add_literal]") {
    ctl.register_propagator(std::make_unique<AddLiteralPropagator>());

    auto models = MV{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == MV{{}, {}});
}

TEST_CASE_METHOD(Fixture, "propagate heuristic", "[cxx][propagate][heuristic]") {
    ctl.config()["solve"]["models"] = "1";
    ctl.parse_string("{a;b}.");
    ctl.ground();

    ctl.register_propagator(std::make_unique<HeuristicPropagator>());

    auto models = MV{};
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == MV{{"a"}});
}

TEST_CASE_METHOD(Fixture, "propagate control", "[cxx][propagate][control]") {
    ctl.parse_string("{a}.");
    ctl.ground();

    ctl.register_propagator(std::make_unique<PropagateControlPropagator>());

    MV models;
    REQUIRE(ctl.solve({}, MCB{models}).satisfiable());
    REQUIRE(models == MV{{"a"}});
}

TEST_CASE_METHOD(Fixture, "propagate mode", "[cxx][propagate][mode]") {
    auto prop = std::make_unique<ModePropagator>();
    auto &ref = ctl.register_propagator(std::move(prop));

    ctl.parse_string("{a; b; c}.");
    ctl.main();

    REQUIRE(ref.level == std::vector<ProgramId>{0});
    REQUIRE(ref.num_check >= 16);
    REQUIRE(ref.num_undo >= 8);
}

TEST_CASE_METHOD(Fixture, "backend theory condition", "[cxx][propagate][theory]") {
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
    REQUIRE(lit == elem.condition_id());
    auto prp = TheoryProgagator{lit};
    ctl.register_propagator(std::ref(prp));
    auto res = ctl.solve();
    REQUIRE(res.satisfiable());
}

} // namespace Clingo::Test
