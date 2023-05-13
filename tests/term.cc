#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

#include <parser/term.hh>

namespace test {

namespace grammar {

using term = parse_root<::grammar::term>;

} // namespace grammar

TEST_CASE("terms") {
    REQUIRE(parse<grammar::term>("42") == "42");
    REQUIRE(parse<grammar::term>("f") == "f");
    REQUIRE(parse<grammar::term>("f(  )+5") == "(f+5)");
    REQUIRE(parse<grammar::term>("f(1)") == "f(1)");
    REQUIRE(parse<grammar::term>("f ( 1 , 2 ; 4 )") == "f(1,2;4)");
    REQUIRE(parse<grammar::term>("1 + f") == "(1+f)");
    REQUIRE(parse<grammar::term>("@f(1,2)") == "@f(1,2)");
    REQUIRE(parse<grammar::term>("|42|") == "|42|");
    REQUIRE(parse<grammar::term>("||42||") == "||42||");
    REQUIRE(parse<grammar::term>("f(_,X)") == "f(_,X)");
    REQUIRE(parse<grammar::term>("(a)") == "a");
    REQUIRE(parse<grammar::term>("(a;a,b;a,b,c)") == "(a;a,b;a,b,c)");
    REQUIRE(parse<grammar::term>("(a, ; a,b,;a,b,c, )") == "(a,;a,b;a,b,c)");
}

} // namespace test
