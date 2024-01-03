#include <input/algo/safety.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

namespace {

auto cs_stm(char const *str) -> std::string {
    ParseHelper ph;
    auto stm = ph.statement(str);
    if (!stm) {
        return "<failed>";
    }
    auto [res_state, res_stm] = check_safety(*stm);
    char const *res = ", C";
    if (!res_state) {
        res = ", F";
    }
    if (!res_stm) {
        res = ", U";
    }
    return to_str(res_stm.value_or(*std::move(stm))) + res;
}

} // namespace

TEST_CASE("check_safety") {
    // rule bodies
    REQUIRE(cs_stm("h :- X=Y, not q(Y), p(X).") == "h :- p(X); Y=X; not q(Y)., C");
    REQUIRE(cs_stm("h :- X=Y, not q(X), p(Y).") == "h :- p(Y); X=Y; not q(X)., C");
    REQUIRE(cs_stm("h :- #false: not q(X), X=Y; p(Y).") == "h :- p(Y); #false: X=Y, not q(X)., C");
}

} // namespace Gringo::Input::Test
