#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("parse_term") {
    REQUIRE(to_str(parse_term("42")) == "42");
    REQUIRE(to_str(parse_term("f")) == "f");
    REQUIRE(to_str(parse_term("f(  )+5")) == "(f+5)");
    REQUIRE(to_str(parse_term("f(1)")) == "f(1)");
    REQUIRE(to_str(parse_term("f ( 1 , 2 ; 4 )")) == "f(1,2;4)");
    REQUIRE(to_str(parse_term("1 + f")) == "(1+f)");
    REQUIRE(to_str(parse_term("@f(1,2)")) == "@f(1,2)");
    REQUIRE(to_str(parse_term("|42|")) == "|42|");
    REQUIRE(to_str(parse_term("||42||")) == "||42||");
    REQUIRE(to_str(parse_term("f(_,X)")) == "f(_,X)");
    REQUIRE(to_str(parse_term("(a)")) == "a");
    REQUIRE(to_str(parse_term("(a;a,b;a,b,c)")) == "(a;a,b;a,b,c)");
    REQUIRE(to_str(parse_term("(a, ; a,b,;a,b,c, )")) == "(a,;a,b;a,b,c)");
    REQUIRE(to_str(parse_term("(a)")) == "a");
    REQUIRE(to_str(parse_term("(a,)")) == "(a,)");
    REQUIRE(to_str(parse_term("()")) == "()");
    REQUIRE(to_str(parse_term("(;)")) == "(;)");
    REQUIRE(to_str(parse_term("(a,;a)")) == "(a,;a)");
    REQUIRE(to_str(parse_term("f(;)")) == "f(;)");
    REQUIRE(to_str(parse_term("f(\"x\")")) == "f(\"x\")");
}

TEST_CASE("unpool_term") {
    REQUIRE(unpool_str(parse_term("x")) == "[x]");
    REQUIRE(unpool_str(parse_term("42")) == "[42]");
    REQUIRE(unpool_str(parse_term("(1;2)")) == "[1, 2]");
    REQUIRE(unpool_str(parse_term("f(1;2)")) == "[f(1), f(2)]");
    REQUIRE(unpool_str(parse_term("-(1;2)")) == "[(-1), (-2)]");
    REQUIRE(unpool_str(parse_term("((1;2);(3;4))")) == "[1, 2, 3, 4]");
    REQUIRE(unpool_str(parse_term("((1;2),(3;4))")) == "[(1,3), (2,3), (1,4), (2,4)]");
    REQUIRE(unpool_str(parse_term("(1;2)+(3;4)")) == "[(1+3), (1+4), (2+3), (2+4)]");
    REQUIRE(unpool_str(parse_term("|(1;2);3|")) == "[|1|, |2|, |3|]");
    REQUIRE(unpool_str(parse_term("|1;2;3|")) == "[|1|, |2|, |3|]");
}

TEST_CASE("variables_term") {
    REQUIRE(variables_str(parse_term("f(X;Y)"), VariableSelectMode::add) == "[X, Y]");
    REQUIRE(variables_str(parse_term("f(X;Y)"), VariableSelectMode::del) == "[]");
    REQUIRE(variables_str(parse_term("f(X,Z;X,Y,Z;X,Y)"), VariableSelectMode::add) == "[X, Y, Z]");
}

} // namespace test
