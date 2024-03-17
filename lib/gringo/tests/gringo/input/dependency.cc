#include "gringo/input/test.hh"

#include <gringo/input/algo/dependency.hh>

namespace Gringo::Input::Test {

TEST_CASE("dependency") {
    ParseHelper ph;
    // TODO: should use program + rewrite
    std::vector<Stm> stms;
    stms.emplace_back(*ph.statement("a(X) :- b(X), b(g(X))."));
    stms.emplace_back(*ph.statement("b(f(X)) :- a(X)."));
    stms.emplace_back(*ph.statement("a(-X) :- a(-1*X+1)."));
    auto comps = analyze(ph, stms);
    REQUIRE(comps.size() == 1);
    REQUIRE(comps.front().size() == 1);
    REQUIRE(comps.front().front().stms.size() == 3);
    // TODO: refine
}

} // namespace Gringo::Input::Test
