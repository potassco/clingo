#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("theory atoms") {
    // empty guards/elements
    REQUIRE(parse_head_literal("&p(x,y;z){}<=a") == "&p(x,y;z) { } <= a");
    REQUIRE(parse_head_literal("&p(x,y;z)<=a") == "&p(x,y;z) { } <= a");
    REQUIRE(parse_head_literal("&p(x,y;z){}") == "&p(x,y;z)");
    REQUIRE(parse_head_literal("&p(x,y;z)") == "&p(x,y;z)");
    // empty tuples/conditions
    REQUIRE(parse_head_literal("&p{1:a}") == "&p { 1: a }");
    REQUIRE(parse_head_literal("&p{1}") == "&p { 1 }");
    REQUIRE(parse_head_literal("&p{1:}") == "&p { 1 }");
    REQUIRE(parse_head_literal("&p{:a}") == "&p { : a }");
    REQUIRE(parse_head_literal("&p{:}") == "&p { :  }");
    // term types
    REQUIRE(parse_head_literal("&p{1}") == "&p { 1 }");
    REQUIRE(parse_head_literal("&p{a}") == "&p { a }");
    REQUIRE(parse_head_literal("&p{\"a\"}") == "&p { \"a\" }");
    REQUIRE(parse_head_literal("&p{#sup}") == "&p { #sup }");
    REQUIRE(parse_head_literal("&p{_X}") == "&p { _X }");
    REQUIRE(parse_head_literal("&p{_}") == "&p { _ }");
    // tuple
    REQUIRE(parse_head_literal("&p{()}") == "&p { () }");
    REQUIRE(parse_head_literal("&p{(a)}") == "&p { a }");
    REQUIRE(parse_head_literal("&p{(a,b)}") == "&p { (a,b) }");
    REQUIRE(parse_head_literal("&p{(a,b,)}") == "&p { (a,b) }");
    REQUIRE(parse_head_literal("&p{(a,)}") == "&p { (a,) }");
    // set
    REQUIRE(parse_head_literal("&p{{}}") == "&p { {} }");
    REQUIRE(parse_head_literal("&p{{a}}") == "&p { {a} }");
    REQUIRE(parse_head_literal("&p{{a,b}}") == "&p { {a,b} }");
    // list
    REQUIRE(parse_head_literal("&p{[]}") == "&p { [] }");
    REQUIRE(parse_head_literal("&p{[a]}") == "&p { [a] }");
    REQUIRE(parse_head_literal("&p{[a,b]}") == "&p { [a,b] }");
    // unparsed
    REQUIRE(parse_head_literal("&p{+- *a -* + c}") == "&p { (+- * a -* + c) }");
}

} // namespace test
