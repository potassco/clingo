#include <input/algo/compute_bounds.hh>
#include <input/iesolver.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

namespace {

auto cb_stm(char const *str) -> std::string {
    ParseHelper ph;
    ConstMap const_map;
    ParamMap param_map;
    RewriteContext ctx{ph, ph, param_map, const_map, {}, "__A_"};
    auto stm = ph.statement(str);
    if (!stm) {
        return "<failed>";
    }
    auto [state, res] = compute_bounds(ctx, *stm);
    return to_str(std::move(res).value_or(*stm));
}

} // namespace

TEST_CASE("compute_bounds") {
    // rule bodies
    REQUIRE(cb_stm("h :- X>=1; X<=3.") == "h :- X=1..3.");
    REQUIRE(cb_stm("h :- X>=1; X<=5, X>=3, X<=7.") == "h :- X=3..5.");
    REQUIRE(cb_stm("h :- X>=1; X<=5, X=3..7.") == "h :- X=3..5.");
    REQUIRE(cb_stm("h :- p(X),-0x80000000<=X, X<=0x2147483648.") ==
            "h :- p(X); -2147483648<=X; X=-2147483648..142929835592.");
    REQUIRE(cb_stm("h :- X>=0; X+Y=6; -3*X+Y=2.") == "h :- X=1; X+Y=6; -3*X+Y=2; Y=5.");

    // body literals
    REQUIRE(cb_stm("h :- X>=0; p(X): X+Y=6, -3*X+Y=2.") == "h :- X>=0; p(X): X+Y=6, -3*X+Y=2, X=1, Y=5.");
    REQUIRE(cb_stm("h :- X>=0; #count { X: X+Y=6, -3*X+Y=2 } >= 1.") ==
            "h :- X>=0; #count { X: X+Y=6, -3*X+Y=2, X=1, Y=5 } >= 1.");
    REQUIRE(cb_stm("h :- X>=0; &count { X: X+Y=6, -3*X+Y=2 } >= 1.") ==
            "h :- X>=0; &count { X: X+Y=6, -3*X+Y=2, X=1, Y=5 } >= 1.");

    // head literals
    REQUIRE(cb_stm("p(X): X+Y=6, -3*X+Y=2 :- X>=0.") == "p(X): X+Y=6, -3*X+Y=2, X=1, Y=5 :- X>=0.");
    REQUIRE(cb_stm("#count { X: p(X): X+Y=6, -3*X+Y=2 } >= 1 :- X>=0.") ==
            "#count { X: p(X): X+Y=6, -3*X+Y=2, X=1, Y=5 } >= 1 :- X>=0.");
    REQUIRE(cb_stm("&count { X: X+Y=6, -3*X+Y=2 } >= 1 :- X>=0.") ==
            "&count { X: X+Y=6, -3*X+Y=2, X=1, Y=5 } >= 1 :- X>=0.");

    // TODO: statements
}

} // namespace Gringo::Input::Test
