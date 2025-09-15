#include "test.hh"

#include <clingo/input/rewrite/safety.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Input::Test {

namespace {

auto cs_stm(char const *str) -> std::string {
    ParseHelper ph;
    auto stm = ph.statement(str);
    if (!stm) {
        return "<failed>";
    }
    auto [res_state, res_stm] = check_safety(ph.logger(), *stm);
    if (!res_state) {
        return "<unsafe>";
    }
    char const *res = res_stm ? ", C" : ", U";
    return to_str(res_stm.value_or(*std::move(stm))) + res;
}

} // namespace

TEST_CASE("check_safety") {
    // rule bodies
    REQUIRE(cs_stm("h :- X=Y, not q(Y), p(X).") == "h :- p(X); Y=X; not q(Y)., C");
    REQUIRE(cs_stm("h :- X=Y, not q(X), p(Y).") == "h :- p(Y); X=Y; not q(X)., C");
    REQUIRE(cs_stm("h :- X=Y, not q(X,Z), p(Y).") == "<unsafe>");
    REQUIRE(cs_stm("h(Z) :- X=Y, not q(X), p(Y).") == "<unsafe>");

    // body literals
    REQUIRE(cs_stm("h :- #false: not q(X), X=Y; p(Y).") == "h :- p(Y); #false: X=Y, not q(X)., C");
    REQUIRE(cs_stm("h :- #false: not q(X,Z), X=Y; p(Y).") == "<unsafe>");
    REQUIRE(cs_stm("h :- p(Z): not q(X), X=Y; p(Y).") == "<unsafe>");
    //
    REQUIRE(cs_stm("h :- Z = #count{ X: not q(X), X=Y}; p(Y); not q(Z).") ==
            "h :- p(Y); Z = #count { X: X=Y, not q(X) }; not q(Z)., C");
    REQUIRE(cs_stm("h :- Z >= #count{ X: not q(X), X=Y}; p(Y); not q(Z).") == "<unsafe>");
    REQUIRE(cs_stm("h :- Z = #count{ X,Z: not q(X), X=Y}; p(Y); not q(Z).") == "<unsafe>");
    //
    REQUIRE(cs_stm("h :- &count(Y) { X: not q(X), X=Y} >= Y; p(Y).") ==
            "h :- p(Y); &count(Y) { X: X=Y, not q(X) } >= Y., C");
    REQUIRE(cs_stm("h :- &count(Z) { X: not q(X), X=Y} >= Y; p(Y).") == "<unsafe>");
    REQUIRE(cs_stm("h :- &count(Y) { X,Z: not q(X), X=Y} >= Y; p(Y).") == "<unsafe>");
    REQUIRE(cs_stm("h :- &count(Y) { X: not q(X), X=Y} >= Z; p(Y).") == "<unsafe>");

    // head literals
    REQUIRE(cs_stm("q(X): not q(X), X=Y; q(Y) :- p(Y).") == "q(X): X=Y, not q(X); q(Y) :- p(Y)., C");
    REQUIRE(cs_stm("q(X): not q(X), X=Y; q(Z) :- p(Y).") == "<unsafe>");
    REQUIRE(cs_stm("q(X): not q(X,Z), X=Y; q(Y) :- p(Y).") == "<unsafe>");
    REQUIRE(cs_stm("q(Z): not q(X), X=Y; q(Y) :- p(Y).") == "<unsafe>");
    //
    REQUIRE(cs_stm("#count{ X: p(X): not q(X), X=Y} >= Y :- p(Y).") ==
            "#count { X: p(X): X=Y, not q(X) } >= Y :- p(Y)., C");
    REQUIRE(cs_stm("#count{ Z: p(X): not q(X), X=Y} >= Y :- p(Y).") == "<unsafe>");
    REQUIRE(cs_stm("#count{ X: p(Z): not q(X), X=Y} >= Y :- p(Y).") == "<unsafe>");
    REQUIRE(cs_stm("#count{ X: p(X): not q(Z), X=Y} >= Y :- p(Y).") == "<unsafe>");
    REQUIRE(cs_stm("#count{ X: p(X): not q(X), X=Y} >= Z :- p(Y).") == "<unsafe>");
    //
    REQUIRE(cs_stm("&count(Y) { X: not q(X), X=Y} >= Y :- p(Y).") == "&count(Y) { X: X=Y, not q(X) } >= Y :- p(Y)., C");
    REQUIRE(cs_stm("&count(Z) { X: not q(X), X=Y} >= Y :- p(Y).") == "<unsafe>");
    REQUIRE(cs_stm("&count(Y) { X,Z: not q(X), X=Y} >= Y :- p(Y).") == "<unsafe>");
    REQUIRE(cs_stm("&count(Y) { X: not q(X), X=Y} >= Z :- p(Y).") == "<unsafe>");

    // statements
    REQUIRE(cs_stm(":~ X=Y, not q(Y), p(X). [X]") == " :~ p(X); Y=X; not q(Y). [X], C");
    REQUIRE(cs_stm(":~ X=Y, not q(Y), p(X). [Z]") == "<unsafe>");
    //
    REQUIRE(cs_stm("#show X : X=Y, not q(Y), p(X).") == "#show X: p(X); Y=X; not q(Y)., C");
    REQUIRE(cs_stm("#show Z : X=Y, not q(Y), p(X).") == "<unsafe>");
    //
    REQUIRE(cs_stm("#project p(X) : X=Y, not q(Y).") == "#project p(X): Y=X; not q(Y)., C");
    REQUIRE(cs_stm("#project p(Z) : X=Y, not q(Y).") == "<unsafe>");
    //
    REQUIRE(cs_stm("#external p(X) : X=Y, not q(Y), p(X). [X]") == "#external p(X): p(X); Y=X; not q(Y). [X], C");
    REQUIRE(cs_stm("#external p(Z) : X=Y, not q(Y), p(X). [X]") == "<unsafe>");
    REQUIRE(cs_stm("#external p(X) : X=Y, not q(Y), p(X). [Z]") == "<unsafe>");
    //
    REQUIRE(cs_stm("#edge (X,Y) : X=Y, not q(Y), p(X).") == "#edge (X,Y): p(X); Y=X; not q(Y)., C");
    REQUIRE(cs_stm("#edge (Z,Y) : X=Y, not q(Y), p(X).") == "<unsafe>");
    //
    REQUIRE(cs_stm("#heuristic p(X) : X=Y, not q(Y). [X@Y,X]") == "#heuristic p(X): Y=X; not q(Y). [X@Y,X], C");
    REQUIRE(cs_stm("#heuristic p(Z) : X=Y, not q(Y). [X@Y,X]") == "<unsafe>");
    REQUIRE(cs_stm("#heuristic p(X) : X=Y, not q(Y). [Z@Y,X]") == "<unsafe>");
    REQUIRE(cs_stm("#heuristic p(X) : X=Y, not q(Y). [X@Z,X]") == "<unsafe>");
    REQUIRE(cs_stm("#heuristic p(X) : X=Y, not q(Y). [X@Y,Z]") == "<unsafe>");

    REQUIRE(cs_stm("p(f\"{X}\").") == "<unsafe>");
    REQUIRE(cs_stm(":- p(f\"{X}\").") == "<unsafe>");
    REQUIRE(cs_stm("p(f\"{X}\") :- X=1.") == "p(f\"{X}\") :- X=1., U");
}

} // namespace CppClingo::Input::Test
