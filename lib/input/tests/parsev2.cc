#include <gringo/input/algo/parsev2.hh>

#include "test.hh"

namespace Gringo::Input::Test {

TEST_CASE("parsev2") {
    std::vector<std::pair<MessageCode, std::string>> messages;
    auto store = make_symbol_store(true, false);
    auto log = Logger{[&](MessageCode code, std::string str) { messages.emplace_back(code, std::move(str)); }};

    SECTION("term") {
        auto parse_term = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            auto iss = std::istringstream{str};
            return to_str(Parser{log, *store, iss, store->string_ref("<input>")}.parse_term());
        };

        REQUIRE(parse_term("||a;b|;c|") == "||a;b|;c|");
        REQUIRE(parse_term("42") == "42");
        REQUIRE(parse_term("f") == "f");
        REQUIRE(parse_term("f(  )+5") == "f+5");
        REQUIRE(parse_term("f(1)") == "f(1)");
        REQUIRE(parse_term("f ( 1 , 2 ; 4 )") == "f(1,2;4)");
        REQUIRE(parse_term("1 + f") == "1+f");
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
        REQUIRE(parse_term("(;)") == "(;)");
        REQUIRE(parse_term("(a,;a)") == "(a,;a)");
        REQUIRE(parse_term("f(;)") == "f(;)");
        REQUIRE(parse_term("f(\"x\")") == "f(\"x\")");
        REQUIRE(parse_term("a+b+c") == "a+b+c");
        REQUIRE(parse_term("a*b+c") == "a*b+c");
        REQUIRE(parse_term("a+b*c") == "a+b*c");
        REQUIRE(parse_term("a**b**c") == "a**b**c");
        REQUIRE(parse_term("a+(-b)") == "a+(-b)");
        REQUIRE(parse_term("-a+b") == "-a+b");
        REQUIRE(parse_term("-a**b") == "-a**b");
        REQUIRE(parse_term("(-a)**b") == "(-a)**b");
        REQUIRE(parse_term("-f(1+2)") == "-f(1+2)");
        REQUIRE(parse_term("-f(1+X)") == "-f(1+X)");
        REQUIRE(parse_term("f(1;2,*;*;*,*)") == "f(1;2,*;*;*,*)");
        REQUIRE(parse_term("f(*)") == "f(*)");
        REQUIRE(parse_term("(*,)") == "(*,)");
        REQUIRE(parse_term("(1;2,*;*,;*,*)") == "(1;2,*;*,;*,*)");
        REQUIRE(parse_term(R"(f((), (a), (@a,), (,), (,;), (;;a,;,;;), "a", _, X * 2 + 1, -1+2*3, g(;f,x;;g;)))") ==
                R"(f((),a,(@a,),(),(;),(;;a,;;;),"a",_,X*2+1,-1+2*3,g(;f,x;;g;)))");

        REQUIRE(parse_term("f(x,)") == "<failed>");
        REQUIRE(messages.size() == 1);
        REQUIRE(messages.back().first == MessageCode::error);
        REQUIRE(messages.back().second == "<input>:1:5-6: error: expected one of  '|' '~' '-' '_' '(' <number> "
                                          "<string> <var> <identifier> but got ')'");
        REQUIRE(parse_term("(*)") == "<failed>");
        REQUIRE(messages.size() == 1);
        REQUIRE(messages.back().first == MessageCode::error);
        REQUIRE(messages.back().second == "<input>:1:3-4: error: expected one of  ',' but got ')'");
    }
}

} // namespace Gringo::Input::Test
