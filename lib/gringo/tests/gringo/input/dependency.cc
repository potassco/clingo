#include "gringo/input/test.hh"

#include <gringo/input/algo/dependency.hh>

// TODO
#include <iostream>

namespace Gringo::Input::Test {

TEST_CASE("dependency") {
    ParseHelper ph;
    // TODO: should use program + rewrite
    std::vector<Stm> stms;
    stms.emplace_back(*ph.statement("a(X) :- b(X), b(g(X))."));
    stms.emplace_back(*ph.statement("b(f(X)) :- a(X)."));
    analyze(ph, stms);
}

} // namespace Gringo::Input::Test
