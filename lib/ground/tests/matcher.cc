#include <clingo/ground/matcher.hh>

#include <clingo/util/algorithm.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Ground::Test {

// NOLINTBEGIN(readability-magic-numbers,bugprone-unchecked-optional-access)

namespace {

class NullOutputLit : public OutputLit {
  private:
    void do_lit([[maybe_unused]] Sign sign, [[maybe_unused]] Symbol sym, [[maybe_unused]] size_t uid) override {}
    void do_boolean([[maybe_unused]] bool value) override {}
    auto do_cond_lit([[maybe_unused]] std::optional<size_t> uid) -> size_t override { return 0; }
    auto do_bd_aggr([[maybe_unused]] Sign sign, [[maybe_unused]] std::optional<size_t> uid) -> size_t override {
        return 0;
    }
    auto do_bd_theory([[maybe_unused]] Sign sign, [[maybe_unused]] std::optional<size_t> uid) -> size_t override {
        return 0;
    }
};

class NullOutputStm : public OutputStm {
  private:
    auto do_uid([[maybe_unused]] bool fact) -> size_t override { return 0; }
    void do_fact([[maybe_unused]] Symbol sym, [[maybe_unused]] size_t uid) override {}
    void do_project_atom([[maybe_unused]] size_t p_atom, [[maybe_unused]] size_t atom) override {}
    auto do_body() -> OutputLit & override { return lout; }
    void do_rule([[maybe_unused]] std::optional<std::tuple<Symbol, size_t, bool>> head) override {}
    void do_external([[maybe_unused]] Symbol atom, [[maybe_unused]] size_t uid,
                     [[maybe_unused]] ExternalType type) override {}
    void do_project([[maybe_unused]] Symbol atom, [[maybe_unused]] size_t uid) override {}
    void do_show_atom([[maybe_unused]] Symbol atom, [[maybe_unused]] size_t uid) override {}
    void do_show_term([[maybe_unused]] Symbol term) override {}
    auto do_aggr_rule([[maybe_unused]] std::optional<size_t> head) -> size_t override { return 0; }
    auto do_disjunctive_rule([[maybe_unused]] std::optional<size_t> head) -> size_t override { return 0; }
    auto do_theory_rule([[maybe_unused]] std::optional<size_t> head) -> size_t override { return 0; }
    void do_weak_constraint([[maybe_unused]] Number const &weight, [[maybe_unused]] Number const *prio,
                            [[maybe_unused]] SymbolSpan terms) override {};
    auto do_cond() -> OutputLit & override { return lout; }
    auto do_cond_id() -> size_t override { return 0; }
    void do_cond_lit([[maybe_unused]] size_t uid, [[maybe_unused]] CondLitSpan elems) override {}
    void do_bd_aggr([[maybe_unused]] size_t uid, [[maybe_unused]] AggregateFunction fun,
                    [[maybe_unused]] BdElemSpan elems, [[maybe_unused]] GuardSpan guards) override {}
    void do_hd_aggr([[maybe_unused]] size_t uid, [[maybe_unused]] AggregateFunction fun,
                    [[maybe_unused]] HdElemSpan elems, [[maybe_unused]] GuardSpan guards) override {}
    void do_disjunction([[maybe_unused]] size_t uid, [[maybe_unused]] DisjElemSpan elems) override {}
    auto do_theory() -> OutputTheory & override { throw std::logic_error("not implemented"); }
    void do_heuristic([[maybe_unused]] Symbol atom, [[maybe_unused]] size_t uid, [[maybe_unused]] Number const &weight,
                      [[maybe_unused]] Number const *prio, [[maybe_unused]] HeuristicType type) override {}
    void do_edge([[maybe_unused]] Symbol src, [[maybe_unused]] Symbol dst) override {}
    void do_flush() override {}
    void do_classical_negation([[maybe_unused]] size_t atom_a, [[maybe_unused]] size_t atom_b) override {}
    void do_end_step() override {}
    void do_mark([[maybe_unused]] SymbolCollector &gc) override {}
    void do_simplify([[maybe_unused]] std::function<TruthValue(prg_lit_t)> const &pred) override {}

    NullOutputLit lout;
};

} // namespace

TEST_CASE("ground_matcher") {
    auto log = Logger{};
    auto store = make_symbol_store(true, true);
    auto ass = Assignment{};
    auto out = NullOutputStm{};
    auto ctx = EvalContext{log, *store, out, ass};
    auto mbr = std::pmr::monotonic_buffer_resource{};
    auto gen = []() { return 1; };

    SECTION("once") {
        auto matcher = make_once_matcher();
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(!matcher->next(ctx));
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(!matcher->next(ctx));
    }

    SECTION("interval") {
        ass = {std::nullopt, std::make_optional(CppClingo::SymbolStore::num_ref(1))};
        std::vector<bool> const bound = {false, true};
        auto lhs = std::make_unique<TermVariable>(0);
        auto lower = std::make_unique<TermVariable>(1);
        auto upper = std::make_unique<TermSymbol>(CppClingo::SymbolStore::num_ref(3));
        auto matcher = make_interval_matcher(bound, *lhs, *lower, *upper);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(1));
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(2));
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(3));
        REQUIRE(!matcher->next(ctx));
        ass[1] = CppClingo::SymbolStore::num_ref(3);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(3));
        REQUIRE(!matcher->next(ctx));
        ass[1] = CppClingo::SymbolStore::num_ref(4);
        matcher->match(ctx);
        REQUIRE(!matcher->next(ctx));
    }

    SECTION("comp") {
        ass = {std::make_optional(CppClingo::SymbolStore::num_ref(1))};
        std::vector<bool> const bound = {true};
        auto lower = std::make_unique<TermVariable>(0);
        auto upper = std::make_unique<TermSymbol>(CppClingo::SymbolStore::num_ref(2));
        auto matcher = make_comp_matcher(bound, *lower, Relation::less, *upper);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(!matcher->next(ctx));
        ass[0] = CppClingo::SymbolStore::num_ref(2);
        matcher->match(ctx);
        REQUIRE(!matcher->next(ctx));
    }

    SECTION("assign") {
        ass = {std::nullopt, std::make_optional(CppClingo::SymbolStore::num_ref(1))};
        std::vector<bool> const bound = {false, true};
        auto lower = std::make_unique<TermVariable>(0);
        auto upper = std::make_unique<TermVariable>(1);
        auto matcher = make_comp_matcher(bound, *lower, Relation::equal, *upper);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(1));
        REQUIRE(!matcher->next(ctx));
        ass[1] = CppClingo::SymbolStore::num_ref(2);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(2));
        REQUIRE(!matcher->next(ctx));
    }

    SECTION("nonfact") {
        auto name = store->string_ref("f");
        auto sym = [&](auto num) { return store->fun_ref(name, SymbolVec{store->num_ref(num)}, false); };
        ass = {std::nullopt};
        auto base = AtomBase{};
        base.add(sym(1), StateAtom::fact, gen);
        base.add(sym(2), StateAtom::derived, gen);
        auto a1 = std::make_unique<TermVariable>(0);
        auto term = std::make_unique<TermFunction>(name, Util::make_vec<UTerm>(std::move(a1)));
        auto symbol = Symbol{};
        auto matcher = make_non_fact_matcher(base, *term, symbol);
        matcher->init(ctx, 0);
        base.add(sym(3), StateAtom::derived, gen);
        base.add(sym(4), StateAtom::fact, gen);
        ass[0] = CppClingo::SymbolStore::num_ref(0);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(symbol == store->fun_ref(name, SymbolVec{*ass[0]}, false));
        REQUIRE(!matcher->next(ctx));
        ass[0] = CppClingo::SymbolStore::num_ref(1);
        matcher->match(ctx);
        REQUIRE(!matcher->next(ctx));
        ass[0] = CppClingo::SymbolStore::num_ref(2);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(symbol == store->fun_ref(name, SymbolVec{*ass[0]}, false));
        REQUIRE(!matcher->next(ctx));
        ass[0] = CppClingo::SymbolStore::num_ref(3);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(symbol == store->fun_ref(name, SymbolVec{*ass[0]}, false));
        REQUIRE(!matcher->next(ctx));
        ass[0] = CppClingo::SymbolStore::num_ref(4);
        matcher->match(ctx);
        REQUIRE(!matcher->next(ctx));
    }

    SECTION("full matcher") {
        auto name = store->string_ref("f");
        auto sym = [&](auto a, auto b) {
            return store->fun_ref(name, SymbolVec{store->num_ref(a), store->num_ref(b)}, false);
        };
        ass = {std::nullopt};
        auto base = AtomBase{};
        base.add(sym(1, 1), StateAtom::derived, gen);
        base.add(sym(2, 2), StateAtom::derived, gen);
        base.add(sym(1, 3), StateAtom::derived, gen);
        auto a1 = std::make_unique<TermSymbol>(CppClingo::SymbolStore::num_ref(1));
        auto a2 = std::make_unique<TermVariable>(0);
        auto term = std::make_unique<TermFunction>(name, Util::make_vec<UTerm>(std::move(a1), std::move(a2)));
        std::vector<bool> const bound = {false};
        // match all
        auto o1 = size_t{0};
        auto m1 = make_atom_matcher(mbr, bound, base, *term, MatcherType::all_atoms, o1);
        m1->init(ctx, 0);
        m1->match(ctx);
        base.add(sym(1, 4), StateAtom::derived, gen);
        base.add(sym(2, 5), StateAtom::derived, gen);
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(1));
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(3));
        REQUIRE(!m1->next(ctx));
        m1->init(ctx, 1);
        m1->match(ctx);
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(1));
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(3));
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(4));
        REQUIRE(!m1->next(ctx));
        base.add(sym(1, 6), StateAtom::derived, gen);
        // match old
        m1 = make_atom_matcher(mbr, bound, base, *term, MatcherType::old_atoms, o1);
        m1->init(ctx, 1);
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(1));
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(3));
        REQUIRE(!m1->next(ctx));
        // match new
        m1 = make_atom_matcher(mbr, bound, base, *term, MatcherType::new_atoms, o1);
        m1->init(ctx, 1);
        m1->match(ctx);
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(4));
        REQUIRE(!m1->next(ctx));
    }
    SECTION("hash matcher") {
        auto name = store->string_ref("f");
        auto sym = [&](auto a, auto b, auto c) {
            return store->fun_ref(name, SymbolVec{store->num_ref(a), store->num_ref(b), store->num_ref(c)}, false);
        };
        ass = {std::nullopt, std::nullopt, std::nullopt};
        ass[0] = CppClingo::SymbolStore::num_ref(1);
        auto base = AtomBase{};
        // join: X=1, f(1,X,Y), f(1,Y,Z)
        auto a1 = std::make_unique<TermSymbol>(CppClingo::SymbolStore::num_ref(1));
        auto a2 = std::make_unique<TermVariable>(0);
        auto a3 = std::make_unique<TermVariable>(1);
        auto b1 = std::make_unique<TermSymbol>(CppClingo::SymbolStore::num_ref(1));
        auto b2 = std::make_unique<TermVariable>(1);
        auto b3 = std::make_unique<TermVariable>(2);
        auto t1 =
            std::make_unique<TermFunction>(name, Util::make_vec<UTerm>(std::move(a1), std::move(a2), std::move(a3)));
        auto t2 =
            std::make_unique<TermFunction>(name, Util::make_vec<UTerm>(std::move(b1), std::move(b2), std::move(b3)));
        std::vector<bool> const v1 = {true, false, false};
        std::vector<bool> const v2 = {true, true, false};
        auto o1 = size_t{0};
        auto m1 = make_atom_matcher(mbr, v1, base, *t1, MatcherType::new_atoms, o1);
        auto o2 = size_t{0};
        auto m2 = make_atom_matcher(mbr, v2, base, *t2, MatcherType::new_atoms, o2);
        base.add(sym(1, 1, 1), StateAtom::derived, gen);
        base.add(sym(1, 1, 2), StateAtom::derived, gen);
        base.add(sym(2, 2, 2), StateAtom::derived, gen);
        base.add(sym(1, 3, 4), StateAtom::derived, gen);
        base.add(sym(1, 1, 3), StateAtom::derived, gen);
        // gen 0
        m1->init(ctx, 0);
        m2->init(ctx, 0);
        m1->match(ctx);
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[1] == store->num_ref(1));
        m2->match(ctx);
        REQUIRE(m2->next(ctx));
        REQUIRE(ass[2] == store->num_ref(1));
        REQUIRE(m2->next(ctx));
        REQUIRE(ass[2] == store->num_ref(2));
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[1] == store->num_ref(2));
        m2->match(ctx);
        REQUIRE(!m2->next(ctx));
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[1] == store->num_ref(3));
        m2->match(ctx);
        REQUIRE(m2->next(ctx));
        REQUIRE(ass[2] == store->num_ref(4));
        REQUIRE(!m2->next(ctx));
        REQUIRE(!m1->next(ctx));
        // gen 1
        base.add(sym(1, 1, 5), StateAtom::derived, gen);
        base.add(sym(1, 5, 6), StateAtom::derived, gen);
        m1->init(ctx, 1);
        m2->init(ctx, 1);
        m1->match(ctx);
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[1] == store->num_ref(5));
        m2->match(ctx);
        REQUIRE(m2->next(ctx));
        REQUIRE(ass[2] == store->num_ref(6));
        REQUIRE(!m2->next(ctx));
        REQUIRE(!m1->next(ctx));
    }
}

// NOLINTEND(readability-magic-numbers,bugprone-unchecked-optional-access)

} // namespace CppClingo::Ground::Test
