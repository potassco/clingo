#include "gringo/input/test.hh"

#include <gringo/input/algo/dependency.hh>
#include <gringo/input/algo/simplify.hh>

namespace Gringo::Input::Test {

namespace {

auto unify_terms(std::string_view a, std::string_view b) -> bool {
    ParseHelper ph;
    auto simplify_term = [&ph](std::string_view str) -> Term {
        auto term = *ph.term(str);
        auto res = simplify(SimplifyTermFlags::none, ph, term);
        return std::move(res).value.value_or(std::move(term));
    };
    return unify(ph, simplify_term(a), simplify_term(b));
}

} // namespace

TEST_CASE("dependency") {
    ParseHelper ph;
    SECTION("unify variable") {
        REQUIRE(unify_terms("X", "Y"));
        REQUIRE(unify_terms("X", "a"));
        REQUIRE(unify_terms("X", "f(Y)"));
        CHECK(!unify_terms("X", "f(X)"));
        REQUIRE(unify_terms("X", "(Y,)"));
        CHECK(!unify_terms("X", "(X,)"));
        REQUIRE(unify_terms("X", "-X"));
        REQUIRE(unify_terms("X", "|X|"));
        REQUIRE(unify_terms("X", "X+X"));
        REQUIRE(unify_terms("X", "1*X+0"));
        CHECK(!unify_terms("X", "1*X+1"));
    }
    SECTION("unify symbol") {
        REQUIRE(unify_terms("f(x)", "f(X)"));
        REQUIRE(!unify_terms("f(x)", "g(X)"));
        REQUIRE(unify_terms("(x,)", "(X,)"));
        REQUIRE(!unify_terms("(x,)", "(x,X,)"));
        REQUIRE(unify_terms("x", "x"));
        REQUIRE(!unify_terms("x", "y"));
        REQUIRE(unify_terms("1", "-X"));
        REQUIRE(unify_terms("1", "~X"));
        REQUIRE(unify_terms("-1", "-X"));
        REQUIRE(unify_terms("1", "|X|"));
        REQUIRE(!unify_terms("-1", "|X|"));
        REQUIRE(unify_terms("4", "2*X+2"));
        REQUIRE(!unify_terms("1", "2*X+2"));
        REQUIRE(unify_terms("1", "X*X"));
    }
    SECTION("unify tuple") {}
    SECTION("unify function") {}
    SECTION("unify absolute") {}
    SECTION("unify unary") {}
    SECTION("unify binary") {}
    /*
    SECTION("complex") {
        // TODO: should use program + rewrite
        std::vector<Stm> stms;
        stms.emplace_back(*ph.statement("y."));
        stms.emplace_back(*ph.statement("a :- x, y."));
        stms.emplace_back(*ph.statement("x :- a."));
        stms.emplace_back(*ph.statement("b :- not c, a."));
        stms.emplace_back(*ph.statement("c :- not b."));
        stms.emplace_back(*ph.statement("d :- e."));
        stms.emplace_back(*ph.statement("e :- d, c."));
        auto comps = analyze(ph, stms);
        std::ofstream out{"dep1.dot"};
        visualize(comps, out);
        REQUIRE(comps.size() == 1);
        REQUIRE(comps.front().size() == 1);
        REQUIRE(comps.front().front().stms.size() == 3);
    }
    */
}

} // namespace Gringo::Input::Test
