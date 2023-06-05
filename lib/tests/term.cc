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
    // TODO: make more compact
    auto term = parse_term("42").value();
    auto pool = term->unpool();
    REQUIRE(pool.size() == 1);
    REQUIRE(pool.back() == term);

    term = parse_term("(1;2)").value();
    pool = term->unpool();
    REQUIRE(pool.size() == 2);
    REQUIRE(pool.front()->to_string() == "1");
    REQUIRE(pool.back()->to_string() == "2");

    term = parse_term("((1;2);(3;4))").value();
    pool = term->unpool();
    REQUIRE(pool.size() == 4);
    REQUIRE(pool[0]->to_string() == "1");
    REQUIRE(pool[1]->to_string() == "2");
    REQUIRE(pool[2]->to_string() == "3");
    REQUIRE(pool[3]->to_string() == "4");

    term = parse_term("((1;2),(3;4))").value();
    pool = term->unpool();
    REQUIRE(pool.size() == 4);
    REQUIRE(pool[0]->to_string() == "(1,3)");
    REQUIRE(pool[1]->to_string() == "(2,3)");
    REQUIRE(pool[2]->to_string() == "(1,4)");
    REQUIRE(pool[3]->to_string() == "(2,4)");

    term = parse_term("1+(2;3)").value();
    pool = term->unpool();
    REQUIRE(pool.size() == 2);
    REQUIRE(pool[0]->to_string() == "(1+2)");
    REQUIRE(pool[1]->to_string() == "(1+3)");
}

} // namespace test
