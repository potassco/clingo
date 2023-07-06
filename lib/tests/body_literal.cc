#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("parse_body_literal") {
    // negation
    REQUIRE(to_str(parse_body_literal("a")) == "a");
    REQUIRE(to_str(parse_body_literal("not a")) == "not a");
    REQUIRE(to_str(parse_body_literal("not not a")) == "not not a");
    // theory_atom | aggregate | set_aggregate
    REQUIRE(to_str(parse_body_literal("&x{}")) == "&x");
    REQUIRE(to_str(parse_body_literal("not &x{}")) == "not &x");
    REQUIRE(to_str(parse_body_literal("#count{}")) == "#count { }");
    REQUIRE(to_str(parse_body_literal("{}")) == "{ }");
    // atom_like relation aggregate
    REQUIRE(to_str(parse_body_literal("a<{}")) == "a < { }");
    REQUIRE(to_str(parse_body_literal("a<#count{}")) == "a < #count { }");
    // atom_like relation term ...
    REQUIRE(to_str(parse_body_literal("a<b<c")) == "a<b<c");
    REQUIRE(to_str(parse_body_literal("a<a:a")) == "a<a: a");
    // atom_like aggregate
    REQUIRE(to_str(parse_body_literal("a{}")) == "a <= { }");
    REQUIRE(to_str(parse_body_literal("a#count{}")) == "a <= #count { }");
    // term aggregate
    REQUIRE(to_str(parse_body_literal("a+1{}")) == "a+1 <= { }");
    REQUIRE(to_str(parse_body_literal("a+1#count{}")) == "a+1 <= #count { }");
    // term relation aggregate
    REQUIRE(to_str(parse_body_literal("a+1<{}")) == "a+1 < { }");
    REQUIRE(to_str(parse_body_literal("a+1<#count{}")) == "a+1 < #count { }");
    // term relation term ...
    REQUIRE(to_str(parse_body_literal("a+1<b<c")) == "a+1<b<c");
    REQUIRE(to_str(parse_body_literal("a+1<a:a")) == "a+1<a: a");
    // atom ...
    REQUIRE(to_str(parse_body_literal("-a")) == "-a");
    REQUIRE(to_str(parse_body_literal("-a(X)")) == "-a(X)");
    REQUIRE(to_str(parse_body_literal("a:b,c")) == "a: b, c");
    // aggregates with guards
    REQUIRE(to_str(parse_body_literal("a<{}<b")) == "a < { } < b");
    REQUIRE(to_str(parse_body_literal("a{}b")) == "a <= { } <= b");
    // aggregate elements
    REQUIRE(to_str(parse_body_literal("#sum{:a;1:a;1,2:a,b,c}")) == "#sum { : a; 1: a; 1,2: a, b, c }");
    REQUIRE(to_str(parse_body_literal("{1<2;1<2:a;a:b;a:b,c}")) == "{ 1<2; 1<2: a; a: b; a: b, c }");
    // conjunction literal
    REQUIRE(to_str(parse_body_literal("#and { : q(X); p(X): q(X); p(X), q(X): r(X) }")) ==
            "#and { : q(X); p(X): q(X); p(X), q(X): r(X) }");
    REQUIRE(to_str(parse_body_literal("#and { }")) == "#and { }");
    REQUIRE(to_str(parse_body_literal("#and { }")) == "#and { }");
    REQUIRE(to_str(parse_body_literal("#and { : }")) == "#and { : }");
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

} // namespace test
