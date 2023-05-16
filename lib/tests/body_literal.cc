#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("body literals") {
    // negation
    REQUIRE(parse_body_literal("a") == "a");
    REQUIRE(parse_body_literal("not a") == "not a");
    REQUIRE(parse_body_literal("not not a") == "not not a");
    // theory_atom | aggregate | set_aggregate
    REQUIRE(parse_body_literal("&x{}") == "&x");
    REQUIRE(parse_body_literal("#count{}") == "#count { }");
    REQUIRE(parse_body_literal("{}") == "{ }");
    // atom_like relation aggregate
    REQUIRE(parse_body_literal("a<{}") == "a < { }");
    REQUIRE(parse_body_literal("a<#count{}") == "a < #count { }");
    // atom_like relation term ...
    REQUIRE(parse_body_literal("a<b<c") == "a<b<c");
    REQUIRE(parse_body_literal("a<a:a") == "a<a: a");
    // atom_like aggregate
    REQUIRE(parse_body_literal("a{}") == "a <= { }");
    REQUIRE(parse_body_literal("a#count{}") == "a <= #count { }");
    // term aggregate
    REQUIRE(parse_body_literal("a+1{}") == "(a+1) <= { }");
    REQUIRE(parse_body_literal("a+1#count{}") == "(a+1) <= #count { }");
    // term relation aggregate
    REQUIRE(parse_body_literal("a+1<{}") == "(a+1) < { }");
    REQUIRE(parse_body_literal("a+1<#count{}") == "(a+1) < #count { }");
    // term relation term ...
    REQUIRE(parse_body_literal("a+1<b<c") == "(a+1)<b<c");
    REQUIRE(parse_body_literal("a+1<a:a") == "(a+1)<a: a");
    // atom ...
    REQUIRE(parse_body_literal("-a") == "(-a)");
    REQUIRE(parse_body_literal("-a(X)") == "(-a(X))");
    REQUIRE(parse_body_literal("a:b,c") == "a: b, c");
    // aggregates with guards
    REQUIRE(parse_body_literal("a<{}<b") == "a < { } < b");
    REQUIRE(parse_body_literal("a{}b") == "a <= { } <= b");
    // aggregate elements
    REQUIRE(parse_body_literal("#sum{:a;1:a;1,2:a,b,c}") == "#sum { : a; 1: a; 1,2: a, b, c }");
    REQUIRE(parse_body_literal("{1<2;1<2:a;a:b;a:b,c}") == "{ 1<2; 1<2: a; a: b; a: b, c }");
}

} // namespace test
