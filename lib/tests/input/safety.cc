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
    char const *res = ", S";
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
    REQUIRE(cs_stm("h :- X=Y, not q(Y), p(X).") == "h :- p(X); X=Y; not q(Y)., S");
}

} // namespace Gringo::Input::Test
