#include <gringo/ground/matcher.hh>

#include <gringo/util/algorithm.hh>

#include <catch2/catch_test_macros.hpp>

namespace Gringo::Ground::Test {

// NOLINTBEGIN(readability-magic-numbers)

TEST_CASE("ground_matcher") {
    auto store = make_symbol_store(true, true);
    auto ass = Assignment{};

    SECTION("once") {
        auto matcher = make_once_matcher();
        matcher->match(*store, ass);
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(!matcher->next(*store, ass));
        matcher->match(*store, ass);
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(!matcher->next(*store, ass));
    }

    SECTION("interval") {
        ass = {std::nullopt, std::make_optional(store->num(1))};
        std::vector<bool> const bound = {false, true};
        auto lhs = std::make_unique<TermVariable>(0);
        auto lower = std::make_unique<TermVariable>(1);
        auto upper = std::make_unique<TermSymbol>(store->num(3));
        auto matcher = make_interval_matcher(bound, *lhs, *lower, *upper);
        matcher->match(*store, ass);
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(ass[0] == store->num(1));
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(ass[0] == store->num(2));
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(ass[0] == store->num(3));
        REQUIRE(!matcher->next(*store, ass));
        ass[1] = store->num(3);
        matcher->match(*store, ass);
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(ass[0] == store->num(3));
        REQUIRE(!matcher->next(*store, ass));
        ass[1] = store->num(4);
        matcher->match(*store, ass);
        REQUIRE(!matcher->next(*store, ass));
    }

    SECTION("comp") {
        ass = {std::make_optional(store->num(1))};
        std::vector<bool> const bound = {true};
        auto lower = std::make_unique<TermVariable>(0);
        auto upper = std::make_unique<TermSymbol>(store->num(2));
        auto matcher = make_comp_matcher(bound, *lower, Relation::less, *upper);
        matcher->match(*store, ass);
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(!matcher->next(*store, ass));
        ass[0] = store->num(2);
        matcher->match(*store, ass);
        REQUIRE(!matcher->next(*store, ass));
    }

    SECTION("assign") {
        ass = {std::nullopt, std::make_optional(store->num(1))};
        std::vector<bool> const bound = {false, true};
        auto lower = std::make_unique<TermVariable>(0);
        auto upper = std::make_unique<TermVariable>(1);
        auto matcher = make_comp_matcher(bound, *lower, Relation::equal, *upper);
        matcher->match(*store, ass);
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(ass[0] == store->num(1));
        REQUIRE(!matcher->next(*store, ass));
        ass[1] = store->num(2);
        matcher->match(*store, ass);
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(ass[0] == store->num(2));
        REQUIRE(!matcher->next(*store, ass));
    }

    SECTION("nonfact") {
        auto name = store->string("f");
        auto sym = [&](auto num) { return store->fun(name, SymbolVec{store->num(num)}, false); };
        ass = {std::nullopt};
        auto base = Base{};
        base.add(sym(1), AtomState::fact);
        base.add(sym(2), AtomState::unknown);
        auto a1 = std::make_unique<TermVariable>(0);
        auto term = std::make_unique<TermFunction>(name, Util::make_vec<UTerm>(std::move(a1)));
        auto matcher = make_non_fact_matcher(base, *term);
        matcher->init(0);
        base.add(sym(3), AtomState::unknown);
        base.add(sym(4), AtomState::fact);
        ass[0] = store->num(0);
        matcher->match(*store, ass);
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(!matcher->next(*store, ass));
        ass[0] = store->num(1);
        matcher->match(*store, ass);
        REQUIRE(!matcher->next(*store, ass));
        ass[0] = store->num(2);
        matcher->match(*store, ass);
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(!matcher->next(*store, ass));
        ass[0] = store->num(3);
        matcher->match(*store, ass);
        REQUIRE(matcher->next(*store, ass));
        REQUIRE(!matcher->next(*store, ass));
        ass[0] = store->num(4);
        matcher->match(*store, ass);
        REQUIRE(!matcher->next(*store, ass));
    }

    SECTION("full matcher") {
        auto name = store->string("f");
        auto sym = [&](auto a, auto b) { return store->fun(name, SymbolVec{store->num(a), store->num(b)}, false); };
        ass = {std::nullopt};
        auto base = Base{};
        base.add(sym(1, 1), AtomState::unknown);
        base.add(sym(2, 2), AtomState::unknown);
        base.add(sym(1, 3), AtomState::unknown);
        auto a1 = std::make_unique<TermSymbol>(store->num(1));
        auto a2 = std::make_unique<TermVariable>(0);
        auto term = std::make_unique<TermFunction>(name, Util::make_vec<UTerm>(std::move(a1), std::move(a2)));
        std::vector<bool> const bound = {false};
        // match all
        auto m1 = make_atom_matcher(bound, base, *term, MatcherType::all_atoms);
        m1->init(0);
        m1->match(*store, ass);
        base.add(sym(1, 4), AtomState::unknown);
        base.add(sym(2, 5), AtomState::unknown);
        REQUIRE(m1->next(*store, ass));
        REQUIRE(ass[0] == store->num(1));
        REQUIRE(m1->next(*store, ass));
        REQUIRE(ass[0] == store->num(3));
        REQUIRE(!m1->next(*store, ass));
        m1->init(1);
        m1->match(*store, ass);
        REQUIRE(m1->next(*store, ass));
        REQUIRE(ass[0] == store->num(1));
        REQUIRE(m1->next(*store, ass));
        REQUIRE(ass[0] == store->num(3));
        REQUIRE(m1->next(*store, ass));
        REQUIRE(ass[0] == store->num(4));
        REQUIRE(!m1->next(*store, ass));
        base.add(sym(1, 6), AtomState::unknown);
        // match old
        m1 = make_atom_matcher(bound, base, *term, MatcherType::old_atoms);
        m1->init(1);
        REQUIRE(m1->next(*store, ass));
        REQUIRE(ass[0] == store->num(1));
        REQUIRE(m1->next(*store, ass));
        REQUIRE(ass[0] == store->num(3));
        REQUIRE(!m1->next(*store, ass));
        // match new
        m1 = make_atom_matcher(bound, base, *term, MatcherType::new_atoms);
        m1->init(1);
        m1->match(*store, ass);
        REQUIRE(m1->next(*store, ass));
        REQUIRE(ass[0] == store->num(4));
        REQUIRE(!m1->next(*store, ass));
    }
    // TODO
    // - hash matcher
}

// NOLINTEND(readability-magic-numbers)

} // namespace Gringo::Ground::Test
