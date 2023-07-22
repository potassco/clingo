#include <input/algo/unpool.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

namespace {

template <class T> auto unpool_str(std::optional<T> value, char const *sep = ", ") -> std::string {
    if (value) {
        auto unpooled = unpool(value.value());
        if (!unpooled.has_value()) {
            unpooled = Util::make_vec<T>(value.value());
        }
        return to_str(unpooled.value(), sep);
    }
    return "<failed>";
}

} // namespace

TEST_CASE("unpool_term") {
    REQUIRE(unpool_str(parse_term("x")) == "[x]");
    REQUIRE(unpool_str(parse_term("42")) == "[42]");
    REQUIRE(unpool_str(parse_term("(1;2)")) == "[1, 2]");
    REQUIRE(unpool_str(parse_term("f(1;2)")) == "[f(1), f(2)]");
    REQUIRE(unpool_str(parse_term("-(1;2)")) == "[-1, -2]");
    REQUIRE(unpool_str(parse_term("((1;2);(3;4))")) == "[1, 2, 3, 4]");
    REQUIRE(unpool_str(parse_term("((1;2),(3;4))")) == "[(1,3), (2,3), (1,4), (2,4)]");
    REQUIRE(unpool_str(parse_term("(1;2)+(3;4)")) == "[1+3, 1+4, 2+3, 2+4]");
    REQUIRE(unpool_str(parse_term("|(1;2);3|")) == "[|1|, |2|, |3|]");
    REQUIRE(unpool_str(parse_term("|1;2;3|")) == "[|1|, |2|, |3|]");
}

TEST_CASE("unpool_literal") {
    REQUIRE(unpool_str(parse_literal("#true")) == "[#true]");
    REQUIRE(unpool_str(parse_literal("(1;2) < (3;4) < (5;6)")) ==
            "[1<3<5, 1<4<5, 1<3<6, 1<4<6, 2<3<5, 2<4<5, 2<3<6, 2<4<6]");
    REQUIRE(unpool_str(parse_literal("not f(x;y)")) == "[not f(x), not f(y)]");
}

TEST_CASE("unpool_head_literal") {
    REQUIRE(unpool_str(parse_head_literal("x: y, z; a: b, c")) == "[x: y, z; a: b, c]");
    REQUIRE(unpool_str(parse_head_literal("p(1;2):p(3;4)"), ". ") ==
            "[p(1): p(3); p(1): p(4). p(2): p(3); p(2): p(4)]");
    REQUIRE(unpool_str(parse_head_literal("p(1;2):p(3)"), ". ") == "[p(1): p(3). p(2): p(3)]");
    REQUIRE(unpool_str(parse_head_literal("p(1):p(2;3)"), ". ") == "[p(1): p(2); p(1): p(3)]");
    REQUIRE(unpool_str(parse_head_literal("(1;2) #count {} (3;4)"), ". ") ==
            "[1 <= #count { } <= 3. 1 <= #count { } <= 4. 2 <= #count { } <= 3. 2 <= #count { } <= 4]");
    REQUIRE(unpool_str(parse_head_literal("1 #count {} 2"), ". ") == "[1 <= #count { } <= 2]");
    REQUIRE(unpool_str(parse_head_literal("#count { (1;2):a(3;4): b(5;6) }"), ". ") ==
            "[#count { "
            "1: a(3): b(5); 1: a(3): b(6); 1: a(4): b(5); 1: a(4): b(6); "
            "2: a(3): b(5); 2: a(3): b(6); 2: a(4): b(5); 2: a(4): b(6) }]");
    REQUIRE(unpool_str(parse_head_literal("(1;2) #count {(3;4):a}"), ". ") ==
            "[1 <= #count { 3: a; 4: a }. 2 <= #count { 3: a; 4: a }]");
    REQUIRE(unpool_str(parse_head_literal("(1;2) {} (3;4)"), ". ") ==
            "[1 <= { } <= 3. 1 <= { } <= 4. 2 <= { } <= 3. 2 <= { } <= 4]");
    REQUIRE(unpool_str(parse_head_literal("#count { a(1;2): b(3;4) }"), ". ") ==
            "[#count { a(1): b(3); a(1): b(4); a(2): b(3); a(2): b(4) }]");
    REQUIRE(unpool_str(parse_head_literal("&p(1;2)"), ". ") == "[&p(1). &p(2)]");
    REQUIRE(unpool_str(parse_head_literal("&p { : a(1;2) }"), ". ") == "[&p { : a(1); : a(2) }]");
    REQUIRE(unpool_str(parse_head_literal("&p(1;2) { : a(1;2) }"), ". ") ==
            "[&p(1) { : a(1); : a(2) }. &p(2) { : a(1); : a(2) }]");
}

TEST_CASE("unpool_body_literal") {
    REQUIRE(unpool_str(parse_body_literal("x: y, z")) == "[x: y, z]");
    REQUIRE(unpool_str(parse_body_literal("p(1;2):p(3;4)"), ". ") ==
            "[p(1): p(3); p(1): p(4). p(2): p(3); p(2): p(4)]");
    REQUIRE(unpool_str(parse_body_literal("p(1;2):p(3)"), ". ") == "[p(1): p(3). p(2): p(3)]");
    REQUIRE(unpool_str(parse_body_literal("p(1):p(2;3)"), ". ") == "[p(1): p(2); p(1): p(3)]");
    REQUIRE(unpool_str(parse_body_literal("(1;2) #count {} (3;4)"), ". ") ==
            "[1 <= #count { } <= 3. 1 <= #count { } <= 4. 2 <= #count { } <= 3. 2 <= #count { } <= 4]");
    REQUIRE(unpool_str(parse_body_literal("#count { a(1;2),b(3;4): c(5;6) }"), ". ") ==
            "[#count { "
            "a(1),b(3): c(5); a(1),b(3): c(6); "
            "a(2),b(3): c(5); a(2),b(3): c(6); "
            "a(1),b(4): c(5); a(1),b(4): c(6); "
            "a(2),b(4): c(5); a(2),b(4): c(6) }]");
    REQUIRE(unpool_str(parse_body_literal("(1;2) #count {(3;4):a}"), ". ") ==
            "[1 <= #count { 3: a; 4: a }. 2 <= #count { 3: a; 4: a }]");
    REQUIRE(unpool_str(parse_body_literal("(1;2) {} (3;4)"), ". ") ==
            "[1 <= { } <= 3. 1 <= { } <= 4. 2 <= { } <= 3. 2 <= { } <= 4]");
    REQUIRE(unpool_str(parse_body_literal("#count { a(1;2): b(3;4) }"), ". ") ==
            "[#count { a(1): b(3); a(1): b(4); a(2): b(3); a(2): b(4) }]");
    REQUIRE(unpool_str(parse_body_literal("&p(1;2)"), ". ") == "[&p(1). &p(2)]");
    REQUIRE(unpool_str(parse_body_literal("&p { : a(1;2) }"), ". ") == "[&p { : a(1); : a(2) }]");
    REQUIRE(unpool_str(parse_body_literal("&p(1;2) { : a(1;2) }"), ". ") ==
            "[&p(1) { : a(1); : a(2) }. &p(2) { : a(1); : a(2) }]");
    REQUIRE(unpool_str(parse_body_literal("p(X): q(X;Y)")) == "[p(X): q(X); p(X): q(Y)]");
    REQUIRE(unpool_str(parse_body_literal("#and { p(X;Y,B) : p(A) }"), "; ") == "[p(X): p(A); p(Y,B): p(A)]");
}

TEST_CASE("unpool_statement") {
    // rule
    REQUIRE(unpool_str(parse_statement("a(1;2) :- b(3;4), b(5,6)."), " ") == "[a(1) :- b(3); b(5,6)."
                                                                             " a(1) :- b(4); b(5,6)."
                                                                             " a(2) :- b(3); b(5,6)."
                                                                             " a(2) :- b(4); b(5,6).]");
    REQUIRE(unpool_str(parse_statement("a(1;2) :- b(3;4), b(5;6)."), " ") == "[a(1) :- b(3); b(5)."
                                                                             " a(1) :- b(4); b(5)."
                                                                             " a(1) :- b(3); b(6)."
                                                                             " a(1) :- b(4); b(6)."
                                                                             " a(2) :- b(3); b(5)."
                                                                             " a(2) :- b(4); b(5)."
                                                                             " a(2) :- b(3); b(6)."
                                                                             " a(2) :- b(4); b(6).]");
    // minimize
    REQUIRE(unpool_str(parse_statement("#minimize { (1;2)@(2;3),(4;5): p(1;2) }.")) ==
            "[#minimize { "
            "1@2,4: p(1); 1@2,4: p(2); 1@2,5: p(1); 1@2,5: p(2); "
            "1@3,4: p(1); 1@3,4: p(2); 1@3,5: p(1); 1@3,5: p(2); "
            "2@2,4: p(1); 2@2,4: p(2); 2@2,5: p(1); 2@2,5: p(2); "
            "2@3,4: p(1); 2@3,4: p(2); 2@3,5: p(1); 2@3,5: p(2) "
            "}.]");
    // weak constraint
    REQUIRE(unpool_str(parse_statement(":~ p(1;2). [(1;2)@(2;3),(4;5)]"), " ") ==
            "[ :~ p(1). [1@2,4]  :~ p(1). [1@2,5]  :~ p(1). [1@3,4]  :~ p(1). [1@3,5]"
            "  :~ p(1). [2@2,4]  :~ p(1). [2@2,5]  :~ p(1). [2@3,4]  :~ p(1). [2@3,5]"
            "  :~ p(2). [1@2,4]  :~ p(2). [1@2,5]  :~ p(2). [1@3,4]  :~ p(2). [1@3,5]"
            "  :~ p(2). [2@2,4]  :~ p(2). [2@2,5]  :~ p(2). [2@3,4]  :~ p(2). [2@3,5]]");
    // show
    REQUIRE(unpool_str(parse_statement("#show (1;2): p(1;2)."), " ") == "[#show 1: p(1). #show 1: p(2)."
                                                                        " #show 2: p(1). #show 2: p(2).]");
    // project
    REQUIRE(unpool_str(parse_statement("#project q(1;2): p(1;2)."), " ") ==
            "[#project q(1): p(1). #project q(1): p(2)."
            " #project q(2): p(1). #project q(2): p(2).]");
    // edge
    REQUIRE(unpool_str(parse_statement("#edge ((a;b),(c;d)) : p(1;2)."), " ") ==
            "[#edge (a,c): p(1). #edge (a,d): p(1)."
            " #edge (b,c): p(1). #edge (b,d): p(1)."
            " #edge (a,c): p(2). #edge (a,d): p(2)."
            " #edge (b,c): p(2). #edge (b,d): p(2).]");
    REQUIRE(unpool_str(parse_statement("#heuristic p(1;2). [(1;2)@(3;4),(5;6)]"), " ") ==
            "[#heuristic p(1). [1@3,5] #heuristic p(1). [1@3,6] #heuristic p(1). [1@4,5] #heuristic p(1). [1@4,6]"
            " #heuristic p(1). [2@3,5] #heuristic p(1). [2@3,6] #heuristic p(1). [2@4,5] #heuristic p(1). [2@4,6]"
            " #heuristic p(2). [1@3,5] #heuristic p(2). [1@3,6] #heuristic p(2). [1@4,5] #heuristic p(2). [1@4,6]"
            " #heuristic p(2). [2@3,5] #heuristic p(2). [2@3,6] #heuristic p(2). [2@4,5] #heuristic p(2). [2@4,6]]");
    REQUIRE(unpool_str(parse_statement("#const x=(1)."), "") == "[#const x=1. [default]]");

    // local <-> global
    REQUIRE(unpool_str(parse_statement(":- p(X): q(X;Y)."), " ") == "[ :- p(X): q(X); p(X): q(Y).]");
    REQUIRE_THROWS(unpool_str(parse_statement(":- p(X;Y): q(X)."), " "));
    REQUIRE_THROWS(unpool_str(parse_statement(":- p(X;Y): q(X;Y)."), " "));
    REQUIRE_THROWS(unpool_str(parse_statement(":- p(X): q(Y); r(X;Y)."), " "));
}

} // namespace Gringo::Input::Test
