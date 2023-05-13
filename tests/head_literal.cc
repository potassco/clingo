#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

#include <parser/head_literal.hh>

namespace test {

namespace grammar {

using head_literal = parse_root<::grammar::head_literal, '.'>;

} // namespace grammar

TEST_CASE("head literals") {
    // theory_atom | aggregate | set_aggregate | not disjunction
    REQUIRE(parse<grammar::head_literal>("&x{}") == "&p{...}");
    REQUIRE(parse<grammar::head_literal>("#count{}") == "#count{}");
    REQUIRE(parse<grammar::head_literal>("{}") == "{}");
    REQUIRE(parse<grammar::head_literal>("not a") == "not a");
    // atom_like relation aggregate
    REQUIRE(parse<grammar::head_literal>("a<{}") == "a<{}");
    REQUIRE(parse<grammar::head_literal>("a<#count{}") == "a<#count{}");
    // atom_like relation term ...
    REQUIRE(parse<grammar::head_literal>("a<b<c") == "a<b<c");
    REQUIRE(parse<grammar::head_literal>("a<a:a") == "a<a:a");
    REQUIRE(parse<grammar::head_literal>("a<a:a;a") == "a<a:a;a");
    REQUIRE(parse<grammar::head_literal>("a<a,a") == "a<a;a");
    // atom_like aggregate
    REQUIRE(parse<grammar::head_literal>("a{}") == "a<={}");
    REQUIRE(parse<grammar::head_literal>("a#count{}") == "a<=#count{}");
    // term aggregate
    REQUIRE(parse<grammar::head_literal>("a+1{}") == "(a+1)<={}");
    REQUIRE(parse<grammar::head_literal>("a+1#count{}") == "(a+1)<=#count{}");
    // term relation aggregate
    REQUIRE(parse<grammar::head_literal>("a+1<{}") == "(a+1)<{}");
    REQUIRE(parse<grammar::head_literal>("a+1<#count{}") == "(a+1)<#count{}");
    // term relation term ...
    REQUIRE(parse<grammar::head_literal>("a+1<b<c") == "(a+1)<b<c");
    REQUIRE(parse<grammar::head_literal>("a+1<a:a") == "(a+1)<a:a");
    REQUIRE(parse<grammar::head_literal>("a+1<a:a;a") == "(a+1)<a:a;a");
    REQUIRE(parse<grammar::head_literal>("a+1<a,a") == "(a+1)<a;a");
    REQUIRE(parse<grammar::head_literal>("a+1<>a,a") == "<failed>");
    // atom ...
    REQUIRE(parse<grammar::head_literal>("-a") == "(-a)");
    REQUIRE(parse<grammar::head_literal>("-a(X)") == "(-a(X))");
    REQUIRE(parse<grammar::head_literal>("a:a") == "a:a");
    REQUIRE(parse<grammar::head_literal>("a:a;a") == "a:a;a");
    REQUIRE(parse<grammar::head_literal>("a,b") == "a;b");
    REQUIRE(parse<grammar::head_literal>("a;b") == "a;b");
    REQUIRE(parse<grammar::head_literal>("a|b") == "a;b");
    // aggregates with guards
    REQUIRE(parse<grammar::head_literal>("a<{}<b") == "a<{}<b");
    REQUIRE(parse<grammar::head_literal>("a{}b") == "a<={}<=b");
    // aggregate elements
    REQUIRE(parse<grammar::head_literal>("#sum{:a;1:a;1,2:a:b,c}") == "#sum{:a;1:a;1,2:a:b,c}");
    REQUIRE(parse<grammar::head_literal>("{1<2;1<2:a;a:b;a:b,c}") == "{1<2;1<2:a;a:b;a:b,c}");
}

} // namespace test

