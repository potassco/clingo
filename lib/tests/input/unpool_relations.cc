#include <input/algo/unpool_relations.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

namespace {

template <class T> auto unpool_str(ParseHelper &ph, std::optional<T> value, char const *sep = ", ") -> std::string {
    if (value) {
        ConstMap const_map;
        ParamMap param_map;
        RewriteContext ctx{ph, ph, param_map, const_map, {}, "__A_"};
        auto unpooled = unpool_relations(ctx, value.value());
        if (ph.logger().has_error()) {
            throw std::runtime_error("error while unpooling");
        }
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
    REQUIRE(unpool_statement("not p | 1<=X<=Y | not 1<=A<=B.") == "[#false :- not not p; 1>X; 1<=A; A<=B."
                                                                  " #false :- not not p; X>Y; 1<=A; A<=B.]");
    REQUIRE(unpool_statement("1<2<3: not 4<5<6; not a<b<c: d<e<f.") == "[#or { 1<2: 4>=5; a>=b, b>=c: d<e, e<f }."
                                                                       " #or { 1<2: 5>=6; a>=b, b>=c: d<e, e<f }."
                                                                       " #or { 2<3: 4>=5; a>=b, b>=c: d<e, e<f }."
                                                                       " #or { 2<3: 5>=6; a>=b, b>=c: d<e, e<f }.]");
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
    REQUIRE(unpool_statement("p :- #and { not p; 1<=X<=Y; not 1<=A<=B }.") == "[p :- not p; 1<=X; X<=Y; 1>A."
                                                                              " p :- not p; 1<=X; X<=Y; A>B.]");
    REQUIRE(unpool_statement("h :- 1<2<3: not 4<5<6; not a<b<c: d<e<f.") ==
            "[h :- #and { 1<2, 2<3: 4>=5 }; a>=b: d<e, e<f."
            " h :- #and { 1<2, 2<3: 5>=6 }; a>=b: d<e, e<f."
            " h :- #and { 1<2, 2<3: 4>=5 }; b>=c: d<e, e<f."
            " h :- #and { 1<2, 2<3: 5>=6 }; b>=c: d<e, e<f.]");
    REQUIRE(unpool_statement("h :- &f{ : 1<=X<=Y }.") == "[h :- &f { : 1<=X, X<=Y }.]");
    REQUIRE(unpool_statement("h :- &f{ a,b,c : not 1<=X<=Y }.") == "[h :- &f { a,b,c: 1>X; a,b,c: X>Y }.]");
    REQUIRE(unpool_statement("h :- &f{ a,b,c : not 1<=X<=Y, 2<=A<=B }.") ==
            "[h :- &f { a,b,c: 1>X, 2<=A, A<=B; a,b,c: X>Y, 2<=A, A<=B }.]");
    REQUIRE(unpool_statement("h :- #sum { 1,X: 1<=X<=Y } >= 2.") == "[h :- #sum { 1,X: 1<=X, X<=Y } >= 2.]");
    REQUIRE(unpool_statement("h :- #sum { 1,X: not 1<=X<=Y } >= 2.") == "[h :- #sum { 1,X: 1>X; 1,X: X>Y } >= 2.]");
    REQUIRE(unpool_statement("h :- #sum { 1,X: 1<=X<=Y, not 1<=A<=B } >= 2.") ==
            "[h :- #sum { 1,X: 1<=X, X<=Y, 1>A; 1,X: 1<=X, X<=Y, A>B } >= 2.]");
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
}

} // namespace Gringo::Input::Test
