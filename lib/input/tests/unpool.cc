#include "test.hh"

#include <clingo/input/rewrite/unpool.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Input::Test {

namespace {

template <class T> auto unpool_str(ParseHelper &ph, std::optional<T> value, char const *sep = ", ") -> std::string {
    if (value) {
        auto unpooled = unpool(ph, value.value());
        ph.check();
        if (!unpooled.has_value()) {
            unpooled = Util::make_vec<T>(value.value());
        }
        return to_str(unpooled.value(), sep);
    }
    return "<failed>";
}

auto unpool_term(std::string const &str) -> std::string {
    ParseHelper ph;
    return unpool_str(ph, ph.term(str));
}

auto unpool_literal(std::string const &str) -> std::string {
    ParseHelper ph;
    return unpool_str(ph, ph.literal(str));
}

auto unpool_head_literal(std::string const &str) -> std::string {
    ParseHelper ph;
    return unpool_str(ph, ph.head_literal(str), ". ");
}

auto unpool_body_literal(std::string const &str) -> std::string {
    ParseHelper ph;
    return unpool_str(ph, ph.body_literal(str), ". ");
}

auto unpool_statement(std::string const &str) -> std::string {
    ParseHelper ph;
    return unpool_str(ph, ph.statement(str), " ");
}

} // namespace

TEST_CASE("unpool_term") {
    ParseHelper ph;
    REQUIRE(unpool_term("x") == "[x]");
    REQUIRE(unpool_term("42") == "[42]");
    REQUIRE(unpool_term("(1;2)") == "[1, 2]");
    REQUIRE(unpool_term("f(1;2)") == "[f(1), f(2)]");
    REQUIRE(unpool_term("-(1;2)") == "[-1, -2]");
    REQUIRE(unpool_term("((1;2);(3;4))") == "[1, 2, 3, 4]");
    REQUIRE(unpool_term("((1;2),(3;4))") == "[(1,3), (2,3), (1,4), (2,4)]");
    REQUIRE(unpool_term("(1;2)+(3;4)") == "[1+3, 1+4, 2+3, 2+4]");
    REQUIRE(unpool_term("|(1;2);3|") == "[|1|, |2|, |3|]");
    REQUIRE(unpool_term("|1;2;3|") == "[|1|, |2|, |3|]");
    REQUIRE(unpool_term(R"(f"num: {(X;-X):0= 5}")") == R"([f"num: {X:0= 5}", f"num: {-X:0= 5}"])");
}

TEST_CASE("unpool_literal") {
    ParseHelper ph;
    REQUIRE(unpool_literal("#true") == "[#true]");
    REQUIRE(unpool_literal("(1;2) < (3;4) < (5;6)") == "[1<3<5, 1<4<5, 1<3<6, 1<4<6, 2<3<5, 2<4<5, 2<3<6, 2<4<6]");
    REQUIRE(unpool_literal("not f(x;y)") == "[not f(x), not f(y)]");
}

TEST_CASE("unpool_head_literal") {
    ParseHelper ph;
    REQUIRE(unpool_head_literal("x: y, z; a: b, c") == "[x: y, z; a: b, c]");
    REQUIRE(unpool_head_literal("p(1;2):p(3;4)") == "[p(1): p(3); p(1): p(4); p(2): p(3); p(2): p(4)]");
    REQUIRE(unpool_head_literal("p(1;2):p(3)") == "[p(1): p(3); p(2): p(3)]");
    REQUIRE(unpool_head_literal("p(1):p(2;3)") == "[p(1): p(2); p(1): p(3)]");
    REQUIRE(unpool_head_literal("(1;2) #count {} (3;4)") ==
            "[1 <= #count { } <= 3. 1 <= #count { } <= 4. 2 <= #count { } <= 3. 2 <= #count { } <= 4]");
    REQUIRE(unpool_head_literal("1 #count {} 2") == "[1 <= #count { } <= 2]");
    REQUIRE(unpool_head_literal("#count { (1;2):a(3;4): b(5;6) }") ==
            "[#count { "
            "1: a(3): b(5); 1: a(3): b(6); 1: a(4): b(5); 1: a(4): b(6); "
            "2: a(3): b(5); 2: a(3): b(6); 2: a(4): b(5); 2: a(4): b(6) }]");
    REQUIRE(unpool_head_literal("(1;2) #count {(3;4):a}") ==
            "[1 <= #count { 3: a; 4: a }. 2 <= #count { 3: a; 4: a }]");
    REQUIRE(unpool_head_literal("(1;2) {} (3;4)") ==
            "[1 <= #count { } <= 3. 1 <= #count { } <= 4. 2 <= #count { } <= 3. 2 <= #count { } <= 4]");
    REQUIRE(unpool_head_literal("#count { a(1;2): b(3;4) }") ==
            "[#count { a(1): b(3); a(1): b(4); a(2): b(3); a(2): b(4) }]");
    REQUIRE(unpool_head_literal("&p(1;2)") == "[&p(1). &p(2)]");
    REQUIRE(unpool_head_literal("&p { : a(1;2) }") == "[&p { : a(1); : a(2) }]");
    REQUIRE(unpool_head_literal("&p(1;2) { : a(1;2) }") == "[&p(1) { : a(1); : a(2) }. &p(2) { : a(1); : a(2) }]");
    REQUIRE(unpool_head_literal("S = { X<(Y;X): q(X,Y); X<Y: q(X,Y); p(X;Y); not p(X;Y); not not p(X;Y) }") ==
            "[S = #count { 3,X,Y: X<Y: q(X,Y); 3,X,Y: X<X: q(X,Y)"
            "; 4,X,Y: X<Y: q(X,Y)"
            "; 0,p(X): p(X); 0,p(Y): p(Y)"
            "; 1,p(X): not p(X); 1,p(Y): not p(Y)"
            "; 2,p(X): not not p(X); 2,p(Y): not not p(Y) }]");
    REQUIRE(unpool_head_literal("S = { X<(Y;Z): q(X,Y,Z) }") ==
            "[S = #count { 3,X,Y,Z: X<Y: q(X,Y,Z); 3,X,Y,Z: X<Z: q(X,Y,Z) }]");
    REQUIRE(unpool_head_literal("S = { X+Y<(Y;X+1): q(X,Y) }") ==
            "[S = #count { 3,X,Y: X+Y<Y: q(X,Y); 3,X,Y: X+Y<1*X+1: q(X,Y) }]");
    REQUIRE(unpool_head_literal("S = { X=1..Y: q(X,Y) }") == "[S = #count { 3,X,Y: X=1..Y: q(X,Y) }]");
    REQUIRE(unpool_head_literal("S = { X=1..Y=Z: q(X,Y,Z) }") ==
            "[S = #count { 3,X,Y,Z: X=1*__A_0+0=Z: q(X,Y,Z), __A_0=1..Y }]");
    REQUIRE(unpool_head_literal("S = { not X=1..Y=Z: q(X,Y,Z) }") ==
            "[S = #count { 3,X,Y,Z: not X=__A_0=Z: q(X,Y,Z), __A_0=1..Y }]");
}

TEST_CASE("unpool_body_literal") {
    ParseHelper ph;
    REQUIRE(unpool_body_literal("x: y, z") == "[x: y, z]");
    REQUIRE(unpool_body_literal("p(1;2):p(3;4)") == "[p(1;2): p(3;4)]");
    REQUIRE(unpool_body_literal("p(1;2):p(3)") == "[p(1;2): p(3)]");
    REQUIRE(unpool_body_literal("p(1):p(2;3)") == "[p(1): p(2;3)]");
    REQUIRE(unpool_body_literal("p(X): q(X;Y)") == "[p(X): q(X;Y)]");
    REQUIRE(unpool_body_literal("p(X;Y,B) : p(A)") == "[p(X;Y,B): p(A)]");
    REQUIRE(unpool_body_literal("(1;2) #count {} (3;4)") ==
            "[1 <= #count { } <= 3. 1 <= #count { } <= 4. 2 <= #count { } <= 3. 2 <= #count { } <= 4]");
    REQUIRE(unpool_body_literal("#count { a(1;2),b(3;4): c(5;6) }") == "[#count { "
                                                                       "a(1),b(3): c(5); a(1),b(3): c(6); "
                                                                       "a(2),b(3): c(5); a(2),b(3): c(6); "
                                                                       "a(1),b(4): c(5); a(1),b(4): c(6); "
                                                                       "a(2),b(4): c(5); a(2),b(4): c(6) }]");
    REQUIRE(unpool_body_literal("(1;2) #count {(3;4):a}") ==
            "[1 <= #count { 3: a; 4: a }. 2 <= #count { 3: a; 4: a }]");
    REQUIRE(unpool_body_literal("(1;2) {} (3;4)") ==
            "[1 <= #count { } <= 3. 1 <= #count { } <= 4. 2 <= #count { } <= 3. 2 <= #count { } <= 4]");
    REQUIRE(unpool_body_literal("#count { a(1;2): b(3;4) }") ==
            "[#count { a(1): b(3); a(1): b(4); a(2): b(3); a(2): b(4) }]");
    REQUIRE(unpool_body_literal("&p(1;2)") == "[&p(1). &p(2)]");
    REQUIRE(unpool_body_literal("&p { : a(1;2) }") == "[&p { : a(1); : a(2) }]");
    REQUIRE(unpool_body_literal("&p(1;2) { : a(1;2) }") == "[&p(1) { : a(1); : a(2) }. &p(2) { : a(1); : a(2) }]");
    REQUIRE(unpool_body_literal("S = { X<(Y;X): q(X,Y); X<Y: q(X,Y); p(X;Y); not p(X;Y); not not p(X;Y) }") ==
            "[S = #count { 3,X,Y: q(X,Y), X<Y; 3,X,Y: q(X,Y), X<X"
            "; 4,X,Y: q(X,Y), X<Y"
            "; 0,p(X): p(X); 0,p(Y): p(Y)"
            "; 1,p(X): not p(X); 1,p(Y): not p(Y)"
            "; 2,p(X): not not p(X); 2,p(Y): not not p(Y) }]");
    REQUIRE(unpool_body_literal("S = { X+Y<(Y;X+1): q(X,Y) }") ==
            "[S = #count { 3,X,Y: q(X,Y), X+Y<Y; 3,X,Y: q(X,Y), X+Y<1*X+1 }]");
    REQUIRE(unpool_body_literal("S = { X=1..Y: q(X,Y) }") == "[S = #count { 3,X,Y: q(X,Y), X=1..Y }]");
    REQUIRE(unpool_body_literal("S = { X=1..Y=Z: q(X,Y,Z) }") ==
            "[S = #count { 3,X,Y,Z: q(X,Y,Z), __A_0=1..Y, X=1*__A_0+0=Z }]");
    REQUIRE(unpool_body_literal("S = { not X=1..Y=Z: q(X,Y,Z) }") ==
            "[S = #count { 3,X,Y,Z: q(X,Y,Z), __A_0=1..Y, not X=__A_0=Z }]");
}

TEST_CASE("unpool_statement") {
    // rule
    REQUIRE(unpool_statement("a(1;2) :- b(3;4), b(5,6).") == "[a(1) :- b(3); b(5,6)."
                                                             " a(1) :- b(4); b(5,6)."
                                                             " a(2) :- b(3); b(5,6)."
                                                             " a(2) :- b(4); b(5,6).]");
    REQUIRE(unpool_statement("a(1;2) :- b(3;4), b(5;6).") == "[a(1) :- b(3); b(5)."
                                                             " a(1) :- b(4); b(5)."
                                                             " a(1) :- b(3); b(6)."
                                                             " a(1) :- b(4); b(6)."
                                                             " a(2) :- b(3); b(5)."
                                                             " a(2) :- b(4); b(5)."
                                                             " a(2) :- b(3); b(6)."
                                                             " a(2) :- b(4); b(6).]");
    // minimize
    REQUIRE(unpool_statement("#minimize { (1;2)@(2;3),(4;5): p(1;2) }.") ==
            "[ :~ p(1). [1@2,4]  :~ p(1). [1@2,5]  :~ p(1). [1@3,4]  :~ p(1). [1@3,5]"
            "  :~ p(1). [2@2,4]  :~ p(1). [2@2,5]  :~ p(1). [2@3,4]  :~ p(1). [2@3,5]"
            "  :~ p(2). [1@2,4]  :~ p(2). [1@2,5]  :~ p(2). [1@3,4]  :~ p(2). [1@3,5]"
            "  :~ p(2). [2@2,4]  :~ p(2). [2@2,5]  :~ p(2). [2@3,4]  :~ p(2). [2@3,5]]");
    REQUIRE(unpool_statement("#minimize { X@Y,Z: p(X,Y,Z); A@B,C: p(A,B,C) }.") ==
            "[ :~ p(X,Y,Z). [X@Y,Z]  :~ p(A,B,C). [A@B,C]]");
    // maximize
    REQUIRE(unpool_statement("#maximize { (1;2)@(2;3),(4;5): p(1;2) }.") ==
            "[ :~ p(1). [-1@2,4]  :~ p(1). [-1@2,5]  :~ p(1). [-1@3,4]  :~ p(1). [-1@3,5]"
            "  :~ p(1). [-2@2,4]  :~ p(1). [-2@2,5]  :~ p(1). [-2@3,4]  :~ p(1). [-2@3,5]"
            "  :~ p(2). [-1@2,4]  :~ p(2). [-1@2,5]  :~ p(2). [-1@3,4]  :~ p(2). [-1@3,5]"
            "  :~ p(2). [-2@2,4]  :~ p(2). [-2@2,5]  :~ p(2). [-2@3,4]  :~ p(2). [-2@3,5]]");
    // weak constraint
    REQUIRE(unpool_statement(":~ p(1;2). [(1;2)@(2;3),(4;5)]") ==
            "[ :~ p(1). [1@2,4]  :~ p(1). [1@2,5]  :~ p(1). [1@3,4]  :~ p(1). [1@3,5]"
            "  :~ p(1). [2@2,4]  :~ p(1). [2@2,5]  :~ p(1). [2@3,4]  :~ p(1). [2@3,5]"
            "  :~ p(2). [1@2,4]  :~ p(2). [1@2,5]  :~ p(2). [1@3,4]  :~ p(2). [1@3,5]"
            "  :~ p(2). [2@2,4]  :~ p(2). [2@2,5]  :~ p(2). [2@3,4]  :~ p(2). [2@3,5]]");
    // show
    REQUIRE(unpool_statement("#show (1;2): p(1;2).") == "[#show 1: p(1). #show 1: p(2)."
                                                        " #show 2: p(1). #show 2: p(2).]");
    // project
    REQUIRE(unpool_statement("#project q(1;2): p(1;2).") == "[#project q(1): p(1). #project q(1): p(2)."
                                                            " #project q(2): p(1). #project q(2): p(2).]");
    // edge
    REQUIRE(unpool_statement("#edge ((a;b),(c;d)) : p(1;2).") == "[#edge (a,c): p(1). #edge (a,d): p(1)."
                                                                 " #edge (b,c): p(1). #edge (b,d): p(1)."
                                                                 " #edge (a,c): p(2). #edge (a,d): p(2)."
                                                                 " #edge (b,c): p(2). #edge (b,d): p(2).]");
    REQUIRE(unpool_statement("#heuristic p(1;2). [(1;2)@(3;4),(5;6)]") ==
            "[#heuristic p(1). [1@3,5] #heuristic p(1). [1@3,6] #heuristic p(1). [1@4,5] #heuristic p(1). [1@4,6]"
            " #heuristic p(1). [2@3,5] #heuristic p(1). [2@3,6] #heuristic p(1). [2@4,5] #heuristic p(1). [2@4,6]"
            " #heuristic p(2). [1@3,5] #heuristic p(2). [1@3,6] #heuristic p(2). [1@4,5] #heuristic p(2). [1@4,6]"
            " #heuristic p(2). [2@3,5] #heuristic p(2). [2@3,6] #heuristic p(2). [2@4,5] #heuristic p(2). [2@4,6]]");
    REQUIRE(unpool_statement("#const x=(1).") == "[#const x=1. [default]]");

    // local <-> global
    REQUIRE(unpool_statement(":- p(X;Y): q(X).") == "[ :- p(X): q(X); p(Y): q(X).]");
    REQUIRE(unpool_statement(":- p(X): q(X;Y).") == "[ :- p(X): q(X); p(X): q(Y).]");
    REQUIRE(unpool_statement(":- p(X;Y): q(X;Y).") == "[ :- p(X): q(X); p(X): q(Y); p(Y): q(X); p(Y): q(Y).]");
    REQUIRE_THROWS(unpool_statement(":- p(X): q(Y); r(X;Y)."));
}

} // namespace CppClingo::Input::Test
