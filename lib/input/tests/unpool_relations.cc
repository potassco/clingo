#include "test.hh"

#include <clingo/input/rewrite/unpool_relations.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Input::Test {

namespace {

template <class T> auto unpool_str(ParseHelper &ph, std::optional<T> value, char const *sep = ", ") -> std::string {
    if (value) {
        auto unpooled = unpool_relations(ph, value.value());
        ph.check();
        if (!unpooled.has_value()) {
            unpooled = Util::make_vec<T>(value.value());
        }
        return to_str(unpooled.value(), sep);
    }
    return "<failed>";
}

auto unpool_statement(std::string const &str) -> std::string {
    ParseHelper ph;
    return unpool_str(ph, ph.statement(str), " ");
}

} // namespace

TEST_CASE("unpool_relations_head") {
    REQUIRE(unpool_statement("not p.") == "[ :- not not p.]");
    REQUIRE(unpool_statement("1<=X<=Y.") == "[ :- 1>X.  :- X>Y.]");
    REQUIRE(unpool_statement("not 1<=X<=Y.") == "[ :- 1<=X; X<=Y.]");
    REQUIRE(unpool_statement("not p | 1<=X<=Y | not 1<=A<=B.") == "[ :- not not p; 1>X; 1<=A; A<=B."
                                                                  "  :- not not p; X>Y; 1<=A; A<=B.]");
    REQUIRE(unpool_statement("1<Y<3: not 4<X<6.") == "[ :- #false: 4>=X, 1<Y, Y<3."
                                                     "  :- #false: X>=6, 1<Y, Y<3.]");
    REQUIRE(unpool_statement("not 1<Y<3: 4<X<6.") == "[ :- #false: 4<X, X<6, 1>=Y."
                                                     "  :- #false: 4<X, X<6, Y>=3.]");
    REQUIRE(unpool_statement("1<Y<3: not 4<X<6; not 1<A<3: 4<B<6.") ==
            "[ :- #false: 4>=X, 1<Y, Y<3; #false: 4<B, B<6, 1>=A."
            "  :- #false: X>=6, 1<Y, Y<3; #false: 4<B, B<6, 1>=A."
            "  :- #false: 4>=X, 1<Y, Y<3; #false: 4<B, B<6, A>=3."
            "  :- #false: X>=6, 1<Y, Y<3; #false: 4<B, B<6, A>=3.]");
    REQUIRE(unpool_statement("&f{ : 1<=X<=Y }.") == "[&f { : 1<=X, X<=Y }.]");
    REQUIRE(unpool_statement("&f{ a,b,c : not 1<=X<=Y }.") == "[&f { a,b,c: 1>X; a,b,c: X>Y }.]");
    REQUIRE(unpool_statement("&f{ a,b,c : not 1<=X<=Y, 2<=A<=B }.") ==
            "[&f { a,b,c: 1>X, 2<=A, A<=B; a,b,c: X>Y, 2<=A, A<=B }.]");
    REQUIRE(unpool_statement("#sum { 1,X: 1<=X<=Y } >= 2.") == "[#sum { 1,X: #true: 1<=X, X<=Y } >= 2.]");
    REQUIRE(unpool_statement("#sum { 1,X: not 1<=X<=Y } >= 2.") == "[#sum { 1,X: #true: 1>X; 1,X: #true: X>Y } >= 2.]");
    REQUIRE(unpool_statement("#sum { 1,X: x: 1<=X<=Y } >= 2.") == "[#sum { 1,X: x: 1<=X, X<=Y } >= 2.]");
    REQUIRE(unpool_statement("#sum { 1,X: x: not 1<=X<=Y } >= 2.") == "[#sum { 1,X: x: 1>X; 1,X: x: X>Y } >= 2.]");
    REQUIRE(unpool_statement("#sum { 1,X: #true: 1<=X<=Y } >= 2.") == "[#sum { 1,X: #true: 1<=X, X<=Y } >= 2.]");
    REQUIRE(unpool_statement("#sum { 1,X: #true: not 1<=X<=Y } >= 2.") ==
            "[#sum { 1,X: #true: 1>X; 1,X: #true: X>Y } >= 2.]");
    REQUIRE(unpool_statement("#sum { 1,X: 1<=X<=Y: not 1<=A<=B } >= 2.") ==
            "[#sum { 1,X: #true: 1>A, 1<=X, X<=Y; 1,X: #true: A>B, 1<=X, X<=Y } >= 2.]");
    REQUIRE(unpool_statement("#sum { 1,X: not 1<=X<=Y: 1<=A<=B } >= 2.") ==
            "[#sum { 1,X: #true: 1<=A, A<=B, 1>X; 1,X: #true: 1<=A, A<=B, X>Y } >= 2.]");
}

TEST_CASE("unpool_relations_body") {
    REQUIRE(unpool_statement("h :- 1<=X<=Y.") == "[h :- 1<=X; X<=Y.]");
    REQUIRE(unpool_statement("h :- a, 1<=X<=Y, b.") == "[h :- a; 1<=X; X<=Y; b.]");
    REQUIRE(unpool_statement("h :- not 1<=X<=Y.") == "[h :- 1>X. h :- X>Y.]");
    REQUIRE(unpool_statement("h :- a, not 1<=X<=Y, b.") == "[h :- a; 1>X; b. h :- a; X>Y; b.]");
    REQUIRE(unpool_statement("p :- not p:; 1<=X<=Y:; not 1<=A<=B:.") ==
            "[p :- #false: not not p; #false: 1>X; #false: 1<=A, A<=B."
            " p :- #false: not not p; #false: X>Y; #false: 1<=A, A<=B.]");
    REQUIRE(unpool_statement("h :- 1<2<3: not 4<5<6; not a<b<c: d<e<f.") ==
            "[h :- #false: 4>=5, 1>=2; #false: d<e, e<f, a<b, b<c."
            " h :- #false: 5>=6, 1>=2; #false: d<e, e<f, a<b, b<c."
            " h :- #false: 4>=5, 2>=3; #false: d<e, e<f, a<b, b<c."
            " h :- #false: 5>=6, 2>=3; #false: d<e, e<f, a<b, b<c.]");
    REQUIRE(unpool_statement("h :- &f{ : 1<=X<=Y }.") == "[h :- &f { : 1<=X, X<=Y }.]");
    REQUIRE(unpool_statement("h :- &f{ a,b,c : not 1<=X<=Y }.") == "[h :- &f { a,b,c: 1>X; a,b,c: X>Y }.]");
    REQUIRE(unpool_statement("h :- &f{ a,b,c : not 1<=X<=Y, 2<=A<=B }.") ==
            "[h :- &f { a,b,c: 1>X, 2<=A, A<=B; a,b,c: X>Y, 2<=A, A<=B }.]");
    REQUIRE(unpool_statement("h :- #sum { 1,X: 1<=X<=Y } >= 2.") == "[h :- #sum { 1,X: 1<=X, X<=Y } >= 2.]");
    REQUIRE(unpool_statement("h :- #sum { 1,X: not 1<=X<=Y } >= 2.") == "[h :- #sum { 1,X: 1>X; 1,X: X>Y } >= 2.]");
    REQUIRE(unpool_statement("h :- #sum { 1,X: 1<=X<=Y, not 1<=A<=B } >= 2.") ==
            "[h :- #sum { 1,X: 1<=X, X<=Y, 1>A; 1,X: 1<=X, X<=Y, A>B } >= 2.]");
    REQUIRE(unpool_statement("h :- X = #sum { } >= 2.") == "[h :- X = #sum { }; 2 <= #sum { }.]");
    REQUIRE(unpool_statement("h :- 2 <= #sum { } = X.") == "[h :- 2 <= #sum { }; X = #sum { }.]");
    REQUIRE(unpool_statement("h :- #sum { } = X.") == "[h :- X = #sum { }.]");
    REQUIRE(unpool_statement("h :- X = #sum { } = Y.") == "[h :- X = #sum { }; Y = #sum { }.]");
    REQUIRE(unpool_statement("h :- X = #sum { Y: not 1 <= Y <= 3 } >= 2.") ==
            "[h :- X = #sum { Y: 1>Y; Y: Y>3 }; 2 <= #sum { Y: 1>Y; Y: Y>3 }.]");
    REQUIRE(unpool_statement("h :- not X != #sum { } >= 2.") == "[h :- not X != #sum { } >= 2.]");
}

TEST_CASE("unpool_relations_stms") {
    REQUIRE(unpool_statement(":~ 1<=X<=Y; not 1<=A<=B. [1@2,3]") ==
            "[ :~ 1<=X; X<=Y; 1>A. [1@2,3]  :~ 1<=X; X<=Y; A>B. [1@2,3]]");
    REQUIRE(unpool_statement("#show X: 1<=X<=Y; not 1<=A<=B.") ==
            "[#show X: 1<=X; X<=Y; 1>A. #show X: 1<=X; X<=Y; A>B.]");
    REQUIRE(unpool_statement("#project p(X): 1<=X<=Y; not 1<=A<=B.") ==
            "[#project p(X): 1<=X; X<=Y; 1>A. #project p(X): 1<=X; X<=Y; A>B.]");
    REQUIRE(unpool_statement("#external p(X): 1<=X<=Y; not 1<=A<=B.") ==
            "[#external p(X): 1<=X; X<=Y; 1>A. #external p(X): 1<=X; X<=Y; A>B.]");
    REQUIRE(unpool_statement("#edge (X,Y): 1<=X<=Y; not 1<=A<=B.") ==
            "[#edge (X,Y): 1<=X; X<=Y; 1>A. #edge (X,Y): 1<=X; X<=Y; A>B.]");
    REQUIRE(unpool_statement("#heuristic p(X): 1<=X<=Y; not 1<=A<=B. [1@2,3]") ==
            "[#heuristic p(X): 1<=X; X<=Y; 1>A. [1@2,3] #heuristic p(X): 1<=X; X<=Y; A>B. [1@2,3]]");
    REQUIRE_THROWS(unpool_statement(":- not X<Y<Z; #false: X<Y<Z."));
}

} // namespace CppClingo::Input::Test
