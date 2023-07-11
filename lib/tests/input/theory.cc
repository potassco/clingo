#include <catch2/catch_test_macros.hpp>

#include "input/parser.hh"

namespace Gringo::Input::Test {

TEST_CASE("theory atoms") {
    // empty guards/elements
    REQUIRE(to_str(parse_head_literal("&p(x,y;z){}<=a")) == "&p(x,y;z) { } <= a");
    REQUIRE(to_str(parse_head_literal("&p(x,y;z)<=a")) == "&p(x,y;z) { } <= a");
    REQUIRE(to_str(parse_head_literal("&p(x,y;z){}")) == "&p(x,y;z)");
    REQUIRE(to_str(parse_head_literal("&p(x,y;z)")) == "&p(x,y;z)");
    // empty tuples/conditions
    REQUIRE(to_str(parse_head_literal("&p{1:a}")) == "&p { 1: a }");
    REQUIRE(to_str(parse_head_literal("&p{1}")) == "&p { 1 }");
    REQUIRE(to_str(parse_head_literal("&p{1:}")) == "&p { 1 }");
    REQUIRE(to_str(parse_head_literal("&p{:a}")) == "&p { : a }");
    REQUIRE(to_str(parse_head_literal("&p{:}")) == "&p { :  }");
    // term types
    REQUIRE(to_str(parse_head_literal("&p{1}")) == "&p { 1 }");
    REQUIRE(to_str(parse_head_literal("&p{a}")) == "&p { a }");
    REQUIRE(to_str(parse_head_literal("&p{\"a\"}")) == "&p { \"a\" }");
    REQUIRE(to_str(parse_head_literal("&p{#sup}")) == "&p { #sup }");
    REQUIRE(to_str(parse_head_literal("&p{_X}")) == "&p { _X }");
    REQUIRE(to_str(parse_head_literal("&p{_}")) == "&p { _ }");
    // tuple
    REQUIRE(to_str(parse_head_literal("&p{()}")) == "&p { () }");
    REQUIRE(to_str(parse_head_literal("&p{(a)}")) == "&p { a }");
    REQUIRE(to_str(parse_head_literal("&p{(a,b)}")) == "&p { (a,b) }");
    REQUIRE(to_str(parse_head_literal("&p{(a,b,)}")) == "&p { (a,b) }");
    REQUIRE(to_str(parse_head_literal("&p{(a,)}")) == "&p { (a,) }");
    // set
    REQUIRE(to_str(parse_head_literal("&p{{}}")) == "&p { {} }");
    REQUIRE(to_str(parse_head_literal("&p{{a}}")) == "&p { {a} }");
    REQUIRE(to_str(parse_head_literal("&p{{a,b}}")) == "&p { {a,b} }");
    // list
    REQUIRE(to_str(parse_head_literal("&p{[]}")) == "&p { [] }");
    REQUIRE(to_str(parse_head_literal("&p{[a]}")) == "&p { [a] }");
    REQUIRE(to_str(parse_head_literal("&p{[a,b]}")) == "&p { [a,b] }");
    // unparsed
    REQUIRE(to_str(parse_head_literal("&p{+- *a -* + c}")) == "&p { (+- * a -* + c) }");
}

} // namespace Gringo::Input::Test
