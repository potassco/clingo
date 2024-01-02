#include <input/algo/safety.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

namespace {

auto cs_stm(char const *str) -> bool {
    ParseHelper ph;
    auto stm = ph.statement(str);
    return stm && check_safety(*stm);
}

} // namespace

TEST_CASE("check_safety") {
    // rule bodies
    REQUIRE(cs_stm("h :- X=Y, not q(Y), p(X)."));
}

} // namespace Gringo::Input::Test
