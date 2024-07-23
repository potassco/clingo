#include <gringo/input/algo/parsev2.hh>

#include "test.hh"

namespace Gringo::Input::Test {

namespace {

auto parse_term(char const *str) -> std::string {
    // TODO: gather log messages
    auto store = make_symbol_store(true, false);
    Logger log;
    std::istringstream iss(str);
    auto parser = Parser{log, *store, iss, store->string_ref("<input>")};
    return to_str(parser.parse_term());
}

} // namespace

TEST_CASE("parsev2") {
    // TODO: restructure + adapt existing tests for term parsing
    REQUIRE(parse_term(R"(f((), (a), (@a,), (,), (,;), (;;a,;,;;), "a", _, X * 2 + 1, -1+2*3, g(;f,x;;g;)))") ==
            R"(f((),a,(@a,),(),(;),(;;a,;;;),"a",_,X*2+1,-1+2*3,g(;f,x;;g;)))");
    REQUIRE(parse_term("||a;b|;c|") == "||a;b|;c|");
    REQUIRE(parse_term("f(x,)") == "<failed>");
}

} // namespace Gringo::Input::Test
