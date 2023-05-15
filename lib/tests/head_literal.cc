#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("head literals") {
    // theory_atom | aggregate | set_aggregate | not disjunction
    REQUIRE(parse_head_literal("&x{}") == "&x");
    REQUIRE(parse_head_literal("#count{}") == "#count{}");
    REQUIRE(parse_head_literal("{}") == "{}");
    REQUIRE(parse_head_literal("not a") == "not a");
    // atom_like relation aggregate
    REQUIRE(parse_head_literal("a<{}") == "a<{}");
    REQUIRE(parse_head_literal("a<#count{}") == "a<#count{}");
    // atom_like relation term ...
    REQUIRE(parse_head_literal("a<b<c") == "a<b<c");
    REQUIRE(parse_head_literal("a<a:a") == "a<a:a");
    REQUIRE(parse_head_literal("a<a:a;a") == "a<a:a;a");
    REQUIRE(parse_head_literal("a<a,a") == "a<a;a");
    // atom_like aggregate
    REQUIRE(parse_head_literal("a{}") == "a<={}");
    REQUIRE(parse_head_literal("a#count{}") == "a<=#count{}");
    // term aggregate
    REQUIRE(parse_head_literal("a+1{}") == "(a+1)<={}");
    REQUIRE(parse_head_literal("a+1#count{}") == "(a+1)<=#count{}");
    // term relation aggregate
    REQUIRE(parse_head_literal("a+1<{}") == "(a+1)<{}");
    REQUIRE(parse_head_literal("a+1<#count{}") == "(a+1)<#count{}");
    // term relation term ...
    REQUIRE(parse_head_literal("a+1<b<c") == "(a+1)<b<c");
    REQUIRE(parse_head_literal("a+1<a:a") == "(a+1)<a:a");
    REQUIRE(parse_head_literal("a+1<a:a;a") == "(a+1)<a:a;a");
    REQUIRE(parse_head_literal("a+1<a,a") == "(a+1)<a;a");
    REQUIRE(parse_head_literal("a+1<>a,a") == "<failed>");
    // atom ...
    REQUIRE(parse_head_literal("-a") == "(-a)");
    REQUIRE(parse_head_literal("-a(X)") == "(-a(X))");
    REQUIRE(parse_head_literal("a:a") == "a:a");
    REQUIRE(parse_head_literal("a:a;a") == "a:a;a");
    REQUIRE(parse_head_literal("a,b") == "a;b");
    REQUIRE(parse_head_literal("a;b") == "a;b");
    REQUIRE(parse_head_literal("a|b") == "a;b");
    // aggregates with guards
    REQUIRE(parse_head_literal("a<{}<b") == "a<{}<b");
    REQUIRE(parse_head_literal("a{}b") == "a<={}<=b");
    // aggregate elements
    REQUIRE(parse_head_literal("#sum{:a;1:a;1,2:a:b,c}") == "#sum{:a;1:a;1,2:a:b,c}");
    REQUIRE(parse_head_literal("{1<2;1<2:a;a:b;a:b,c}") == "{1<2;1<2:a;a:b;a:b,c}");
    // theory atoms
    REQUIRE(parse_head_literal("&p(X){43+-Y:a} <== 7") == "&p(X){43+- Y: a}<==7");
}

} // namespace test
