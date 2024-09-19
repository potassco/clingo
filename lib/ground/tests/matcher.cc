#include <gringo/ground/matcher.hh>

#include <gringo/util/algorithm.hh>

#include <catch2/catch_test_macros.hpp>

namespace Gringo::Ground::Test {

// NOLINTBEGIN(readability-magic-numbers,bugprone-unchecked-optional-access)

namespace {

class NullOutputLit : public OutputLit {
  private:
    void do_lit([[maybe_unused]] Sign sign, [[maybe_unused]] Symbol sym) override {}
    void do_boolean([[maybe_unused]] bool value) override {}
    auto do_cond_lit([[maybe_unused]] std::optional<size_t> uid) -> size_t override { return 0; }
    auto do_bd_aggr([[maybe_unused]] Sign sign, [[maybe_unused]] std::optional<size_t> uid) -> size_t override {
        return 0;
    }
};

class NullOutputStm : public OutputStm {
  private:
    auto do_uid() -> size_t override { return 0; }
    void do_fact([[maybe_unused]] Symbol sym) override {}
    auto do_body() -> OutputLit & override { return lout; }
    void do_rule([[maybe_unused]] std::optional<Symbol> head) override {}
    auto do_aggr_rule([[maybe_unused]] std::optional<size_t> head) -> size_t override { return 0; }
    auto do_cond() -> OutputLit & override { return lout; }
    auto do_cond_id() -> size_t override { return 0; }
    void do_cond_lit([[maybe_unused]] size_t uid, [[maybe_unused]] CondLits elems) override {}
    void do_bd_aggr([[maybe_unused]] size_t uid, [[maybe_unused]] AggregateFunction fun, [[maybe_unused]] BdElems elems,
                    [[maybe_unused]] Guards guards) override {}
    void do_hd_aggr([[maybe_unused]] size_t uid, [[maybe_unused]] AggregateFunction fun, [[maybe_unused]] HdElems elems,
                    [[maybe_unused]] Guards guards) override {}
    void do_flush() override {}
    void do_end_step() override {}
    void do_mark([[maybe_unused]] SymbolCollector &gc) override {}

    NullOutputLit lout;
};

} // namespace

TEST_CASE("ground_matcher") {
    Logger log;
    auto store = make_symbol_store(true, true);
    auto ass = Assignment{};
    auto out = NullOutputStm{};
    auto ctx = InstantiationContext{log, out, *store, ass};
    auto mbr = std::pmr::monotonic_buffer_resource{};

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
        ass = {std::nullopt, std::make_optional(store->num_ref(1))};
        std::vector<bool> const bound = {false, true};
        auto lhs = std::make_unique<TermVariable>(0);
        auto lower = std::make_unique<TermVariable>(1);
        auto upper = std::make_unique<TermSymbol>(store->num_ref(3));
        auto matcher = make_interval_matcher(bound, *lhs, *lower, *upper);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(1));
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(2));
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(3));
        REQUIRE(!matcher->next(ctx));
        ass[1] = store->num_ref(3);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(3));
        REQUIRE(!matcher->next(ctx));
        ass[1] = store->num_ref(4);
        matcher->match(ctx);
        REQUIRE(!matcher->next(ctx));
    }

    SECTION("comp") {
        ass = {std::make_optional(store->num_ref(1))};
        std::vector<bool> const bound = {true};
        auto lower = std::make_unique<TermVariable>(0);
        auto upper = std::make_unique<TermSymbol>(store->num_ref(2));
        auto matcher = make_comp_matcher(bound, *lower, Relation::less, *upper);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(!matcher->next(ctx));
        ass[0] = store->num_ref(2);
        matcher->match(ctx);
        REQUIRE(!matcher->next(ctx));
    }

    SECTION("assign") {
        ass = {std::nullopt, std::make_optional(store->num_ref(1))};
        std::vector<bool> const bound = {false, true};
        auto lower = std::make_unique<TermVariable>(0);
        auto upper = std::make_unique<TermVariable>(1);
        auto matcher = make_comp_matcher(bound, *lower, Relation::equal, *upper);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(1));
        REQUIRE(!matcher->next(ctx));
        ass[1] = store->num_ref(2);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(ass[0] == store->num_ref(2));
        REQUIRE(!matcher->next(ctx));
    }

    SECTION("nonfact") {
        auto name = store->string_ref("f");
        auto sym = [&](auto num) { return store->fun_ref(name, SymbolVec{store->num_ref(num)}, false); };
        ass = {std::nullopt};
        auto base = Base{};
        base.add(sym(1), StateAtom::fact);
        base.add(sym(2), StateAtom::derived);
        auto a1 = std::make_unique<TermVariable>(0);
        auto term = std::make_unique<TermFunction>(name, Util::make_vec<UTerm>(std::move(a1)));
        auto symbol = Symbol{};
        auto matcher = make_non_fact_matcher(base, *term, symbol);
        matcher->init(*store, 0);
        base.add(sym(3), StateAtom::derived);
        base.add(sym(4), StateAtom::fact);
        ass[0] = store->num_ref(0);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(symbol == store->fun_ref(name, SymbolVec{*ass[0]}, false));
        REQUIRE(!matcher->next(ctx));
        ass[0] = store->num_ref(1);
        matcher->match(ctx);
        REQUIRE(!matcher->next(ctx));
        ass[0] = store->num_ref(2);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(symbol == store->fun_ref(name, SymbolVec{*ass[0]}, false));
        REQUIRE(!matcher->next(ctx));
        ass[0] = store->num_ref(3);
        matcher->match(ctx);
        REQUIRE(matcher->next(ctx));
        REQUIRE(symbol == store->fun_ref(name, SymbolVec{*ass[0]}, false));
        REQUIRE(!matcher->next(ctx));
        ass[0] = store->num_ref(4);
        matcher->match(ctx);
        REQUIRE(!matcher->next(ctx));
    }

    SECTION("full matcher") {
        auto name = store->string_ref("f");
        auto sym = [&](auto a, auto b) {
            return store->fun_ref(name, SymbolVec{store->num_ref(a), store->num_ref(b)}, false);
        };
        ass = {std::nullopt};
        auto base = Base{};
        base.add(sym(1, 1), StateAtom::derived);
        base.add(sym(2, 2), StateAtom::derived);
        base.add(sym(1, 3), StateAtom::derived);
        auto a1 = std::make_unique<TermSymbol>(store->num_ref(1));
        auto a2 = std::make_unique<TermVariable>(0);
        auto term = std::make_unique<TermFunction>(name, Util::make_vec<UTerm>(std::move(a1), std::move(a2)));
        std::vector<bool> const bound = {false};
        // match all
        auto o1 = size_t{0};
        auto m1 = make_atom_matcher(mbr, bound, base, *term, MatcherType::all_atoms, o1);
        m1->init(*store, 0);
        m1->match(ctx);
        base.add(sym(1, 4), StateAtom::derived);
        base.add(sym(2, 5), StateAtom::derived);
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(1));
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(3));
        REQUIRE(!m1->next(ctx));
        m1->init(*store, 1);
        m1->match(ctx);
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(1));
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(3));
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(4));
        REQUIRE(!m1->next(ctx));
        base.add(sym(1, 6), StateAtom::derived);
        // match old
        m1 = make_atom_matcher(mbr, bound, base, *term, MatcherType::old_atoms, o1);
        m1->init(*store, 1);
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(1));
        REQUIRE(m1->next(ctx));
        REQUIRE(ass[0] == store->num_ref(3));
        REQUIRE(!m1->next(ctx));
        // match new
        m1 = make_atom_matcher(mbr, bound, base, *term, MatcherType::new_atoms, o1);
        m1->init(*store, 1);
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
        ass[0] = store->num_ref(1);
        auto base = Base{};
        // join: X=1, f(1,X,Y), f(1,Y,Z)
        auto a1 = std::make_unique<TermSymbol>(store->num_ref(1));
        auto a2 = std::make_unique<TermVariable>(0);
        auto a3 = std::make_unique<TermVariable>(1);
        auto b1 = std::make_unique<TermSymbol>(store->num_ref(1));
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
        base.add(sym(1, 1, 1), StateAtom::derived);
        base.add(sym(1, 1, 2), StateAtom::derived);
        base.add(sym(2, 2, 2), StateAtom::derived);
        base.add(sym(1, 3, 4), StateAtom::derived);
        base.add(sym(1, 1, 3), StateAtom::derived);
        // gen 0
        m1->init(*store, 0);
        m2->init(*store, 0);
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
        base.add(sym(1, 1, 5), StateAtom::derived);
        base.add(sym(1, 5, 6), StateAtom::derived);
        m1->init(*store, 1);
        m2->init(*store, 1);
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

} // namespace Gringo::Ground::Test
