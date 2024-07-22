#include <gringo/input/algo/parsev2.hh>

#include <catch2/catch_test_macros.hpp>

namespace Gringo::Input::Test {

TEST_CASE("lex_test") {
    std::istringstream iss(R"(f((), (a), (@a,), (,), (,;), (;;a,;,;;), "a", _, X * 2 + 1, -1+2*3, g(;f,x;;g;)))");
    auto parser = Parser{iss};
    REQUIRE(parser.parse_term());
}

TEST_CASE("lex_test2") {
    std::istringstream iss(R"(f(x,))");
    auto parser = Parser{iss};
    REQUIRE(parser.parse_term());
}

} // namespace Gringo::Input::Test
