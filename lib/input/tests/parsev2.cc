#include <gringo/input/algo/parsev2.hh>

#include "test.hh"

namespace Gringo::Input::Test {

TEST_CASE("lex_test") {
    std::istringstream iss(R"(f((), (a), (@a,), (,), (,;), (;;a,;,;;), "a", _, X * 2 + 1, -1+2*3, g(;f,x;;g;)))");
    auto parser = Parser{iss};
    REQUIRE(to_str(parser.parse_term()) == R"(f((),a,(@a,),(),(;),(;;a,;;;),"a",_,X*2+1,-1+2*3,g(;f,x;;g;)))");
}

TEST_CASE("lex_test2") {
    std::istringstream iss("||a;b|;c|");
    auto parser = Parser{iss};
    REQUIRE(to_str(parser.parse_term()) == "||a;b|;c|");
}

TEST_CASE("lex_test3") {
    std::istringstream iss(R"(f(x,))");
    auto parser = Parser{iss};
    REQUIRE(parser.parse_term());
}

} // namespace Gringo::Input::Test
