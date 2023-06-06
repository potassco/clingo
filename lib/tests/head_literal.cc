#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("parse_head_literal") {
    // theory_atom | aggregate | set_aggregate | not disjunction
    REQUIRE(to_str(parse_head_literal("&x{}")) == "&x");
    REQUIRE(to_str(parse_head_literal("#count{}")) == "#count { }");
    REQUIRE(to_str(parse_head_literal("{}")) == "{ }");
    REQUIRE(to_str(parse_head_literal("not a")) == "not a");
    // atom_like relation aggregate
    REQUIRE(to_str(parse_head_literal("a<{}")) == "a < { }");
    REQUIRE(to_str(parse_head_literal("a<#count{}")) == "a < #count { }");
    // atom_like relation term ...
    REQUIRE(to_str(parse_head_literal("a<b<c")) == "a<b<c");
    REQUIRE(to_str(parse_head_literal("a<a:a")) == "a<a: a");
    REQUIRE(to_str(parse_head_literal("a<a:a;a")) == "a<a: a; a");
    REQUIRE(to_str(parse_head_literal("a<a,a")) == "a<a; a");
    // atom_like aggregate
    REQUIRE(to_str(parse_head_literal("a{}")) == "a <= { }");
    REQUIRE(to_str(parse_head_literal("a#count{}")) == "a <= #count { }");
    // term aggregate
    REQUIRE(to_str(parse_head_literal("a+1 { }")) == "(a+1) <= { }");
    REQUIRE(to_str(parse_head_literal("a+1#count{}")) == "(a+1) <= #count { }");
    // term relation aggregate
    REQUIRE(to_str(parse_head_literal("a+1<{}")) == "(a+1) < { }");
    REQUIRE(to_str(parse_head_literal("a+1<#count{}")) == "(a+1) < #count { }");
    // term relation term ...
    REQUIRE(to_str(parse_head_literal("a+1<b<c")) == "(a+1)<b<c");
    REQUIRE(to_str(parse_head_literal("a+1<a:a")) == "(a+1)<a: a");
    REQUIRE(to_str(parse_head_literal("a+1<a:a;a")) == "(a+1)<a: a; a");
    REQUIRE(to_str(parse_head_literal("a+1<a,a")) == "(a+1)<a; a");
    REQUIRE(to_str(parse_head_literal("a+1<>a,a")) == "<failed>");
    // atom ...
    REQUIRE(to_str(parse_head_literal("-a")) == "(-a)");
    REQUIRE(to_str(parse_head_literal("-a(X)")) == "(-a(X))");
    REQUIRE(to_str(parse_head_literal("a:a")) == "a: a");
    REQUIRE(to_str(parse_head_literal("a:a;a")) == "a: a; a");
    REQUIRE(to_str(parse_head_literal("a,b")) == "a; b");
    REQUIRE(to_str(parse_head_literal("a;b")) == "a; b");
    REQUIRE(to_str(parse_head_literal("a|b")) == "a; b");
    // aggregates with guards
    REQUIRE(to_str(parse_head_literal("a<{}<b")) == "a < { } < b");
    REQUIRE(to_str(parse_head_literal("a{}b")) == "a <= { } <= b");
    // aggregate elements
    REQUIRE(to_str(parse_head_literal("#sum{: a; 1: a; 1,2: a: b, c}")) == "#sum { : a; 1: a; 1,2: a: b, c }");
    REQUIRE(to_str(parse_head_literal("{1<2;1<2:a;a:b;a:b,c}")) == "{ 1<2; 1<2: a; a: b; a: b, c }");
    // theory atoms
    REQUIRE(to_str(parse_head_literal("&p(X){43+-Y:a} <== 7")) == "&p(X) { (43 +- Y): a } <== 7");
}

TEST_CASE("unpool_head_literal") {
    REQUIRE(unpool_str(parse_head_literal("x: y, z; a: b, c")) == "[x: y,z; a: b,c]");
    REQUIRE(unpool_str(parse_head_literal("p(1;2):p(3;4)"), ". ") ==
            "[p(1): p(3); p(1): p(4). p(2): p(3); p(2): p(4)]");
}

} // namespace test
