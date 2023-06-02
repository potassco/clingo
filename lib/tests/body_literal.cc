#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("body literals") {
    // negation
    REQUIRE(to_str(parse_body_literal("a")) == "a");
    REQUIRE(to_str(parse_body_literal("not a")) == "not a");
    REQUIRE(to_str(parse_body_literal("not not a")) == "not not a");
    // theory_atom | aggregate | set_aggregate
    REQUIRE(to_str(parse_body_literal("&x{}")) == "&x");
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
    REQUIRE(to_str(parse_body_literal("a+1{}")) == "(a+1) <= { }");
    REQUIRE(to_str(parse_body_literal("a+1#count{}")) == "(a+1) <= #count { }");
    // term relation aggregate
    REQUIRE(to_str(parse_body_literal("a+1<{}")) == "(a+1) < { }");
    REQUIRE(to_str(parse_body_literal("a+1<#count{}")) == "(a+1) < #count { }");
    // term relation term ...
    REQUIRE(to_str(parse_body_literal("a+1<b<c")) == "(a+1)<b<c");
    REQUIRE(to_str(parse_body_literal("a+1<a:a")) == "(a+1)<a: a");
    // atom ...
    REQUIRE(to_str(parse_body_literal("-a")) == "(-a)");
    REQUIRE(to_str(parse_body_literal("-a(X)")) == "(-a(X))");
    REQUIRE(to_str(parse_body_literal("a:b,c")) == "a: b, c");
    // aggregates with guards
    REQUIRE(to_str(parse_body_literal("a<{}<b")) == "a < { } < b");
    REQUIRE(to_str(parse_body_literal("a{}b")) == "a <= { } <= b");
    // aggregate elements
    REQUIRE(to_str(parse_body_literal("#sum{:a;1:a;1,2:a,b,c}")) == "#sum { : a; 1: a; 1,2: a, b, c }");
    REQUIRE(to_str(parse_body_literal("{1<2;1<2:a;a:b;a:b,c}")) == "{ 1<2; 1<2: a; a: b; a: b, c }");
}

} // namespace test
