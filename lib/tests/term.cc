#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("terms") {
    REQUIRE(parse_term("42") == "42");
    REQUIRE(parse_term("f") == "f");
    REQUIRE(parse_term("f(  )+5") == "(f+5)");
    REQUIRE(parse_term("f(1)") == "f(1)");
    REQUIRE(parse_term("f ( 1 , 2 ; 4 )") == "f(1,2;4)");
    REQUIRE(parse_term("1 + f") == "(1+f)");
    REQUIRE(parse_term("@f(1,2)") == "@f(1,2)");
    REQUIRE(parse_term("|42|") == "|42|");
    REQUIRE(parse_term("||42||") == "||42||");
    REQUIRE(parse_term("f(_,X)") == "f(_,X)");
    REQUIRE(parse_term("(a)") == "a");
    REQUIRE(parse_term("(a;a,b;a,b,c)") == "(a;a,b;a,b,c)");
    REQUIRE(parse_term("(a, ; a,b,;a,b,c, )") == "(a,;a,b;a,b,c)");
    REQUIRE(parse_term("(a)") == "a");
    REQUIRE(parse_term("(a,)") == "(a,)");
    REQUIRE(parse_term("()") == "()");
    REQUIRE(parse_term("(,)") == "()");
    REQUIRE(parse_term("(;)") == "(;)");
    REQUIRE(parse_term("(,;,)") == "(;)");
    REQUIRE(parse_term("f(;)") == "f(;)");
}

} // namespace test
