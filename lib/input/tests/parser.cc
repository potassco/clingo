#include <gringo/input/parser.hh>

#include "test.hh"

namespace Gringo::Input::Test {

TEST_CASE("parsev2") {
    std::vector<std::pair<MessageCode, std::string>> messages;
    auto store = make_symbol_store(true, false);
    auto log = Logger{[&](MessageCode code, std::string str) { messages.emplace_back(code, std::move(str)); }};
    auto parser = Parser{log, *store};

    SECTION("term") {
        auto parse = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            parser.init(str, store->string_ref("<input>"));
            return to_str(parser.parse_term());
        };
        REQUIRE(parse("||a;b|;c|") == "||a;b|;c|");
        REQUIRE(parse("42") == "42");
        REQUIRE(parse("1'000'000'000'000'123") == "1000000000000123");
        REQUIRE(parse("f") == "f");
        REQUIRE(parse("f(  )+5") == "f+5");
        REQUIRE(parse("f(1)") == "f(1)");
        REQUIRE(parse("f ( 1 , 2 ; 4 )") == "f(1,2;4)");
        REQUIRE(parse("1 + f") == "1+f");
        REQUIRE(parse("@f(1,2)") == "@f(1,2)");
        REQUIRE(parse("|42|") == "|42|");
        REQUIRE(parse("||42||") == "||42||");
        REQUIRE(parse("f(_,X)") == "f(_,X)");
        REQUIRE(parse("(a)") == "a");
        REQUIRE(parse("(a;a,b;a,b,c)") == "(a;a,b;a,b,c)");
        REQUIRE(parse("(a, ; a,b,;a,b,c, )") == "(a,;a,b;a,b,c)");
        REQUIRE(parse("(a)") == "a");
        REQUIRE(parse("(a,)") == "(a,)");
        REQUIRE(parse("()") == "()");
        REQUIRE(parse("(;)") == "(;)");
        REQUIRE(parse("(a,;a)") == "(a,;a)");
        REQUIRE(parse("f(;)") == "f(;)");
        REQUIRE(parse("f(\"x\")") == "f(\"x\")");
        REQUIRE(parse("a+b+c") == "a+b+c");
        REQUIRE(parse("a*b+c") == "a*b+c");
        REQUIRE(parse("a+b*c") == "a+b*c");
        REQUIRE(parse("a**b**c") == "a**b**c");
        REQUIRE(parse("a+(-b)") == "a+(-b)");
        REQUIRE(parse("-a+b") == "-a+b");
        REQUIRE(parse("-a**b") == "-a**b");
        REQUIRE(parse("(-a)**b") == "(-a)**b");
        REQUIRE(parse("-f(1+2)") == "-f(1+2)");
        REQUIRE(parse("-f(1+X)") == "-f(1+X)");

        REQUIRE(parse(R"(f((), (a), (@a,), (,), (,;), (;;a,;,;;), "a", _, X * 2 + 1, -1+2*3, g(;f,x;;g;)))") ==
                R"(f((),a,(@a,),(),(;),(;;a,;;;),"a",_,X*2+1,-1+2*3,g(;f,x;;g;)))");

        REQUIRE(parse("f(x,)") == "<failed>");
        REQUIRE(messages.size() == 1);
        REQUIRE(messages.back().first == MessageCode::error);
        REQUIRE(messages.back().second == "<input>:1:5-6: error: expected one of <term> but got ')'");
        REQUIRE(parse("(*)") == "<failed>");
        REQUIRE(messages.size() == 1);
        REQUIRE(messages.back().first == MessageCode::error);
        REQUIRE(messages.back().second == "<input>:1:3-4: error: expected one of ',' but got ')'");
    }
    SECTION("theory_term") {
        auto parse = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            parser.init(str, store->string_ref("<input>"));
            return to_str(parser.parse_theory_term());
        };
        REQUIRE(parse("1") == "1");
        REQUIRE(parse("X") == "X");
        REQUIRE(parse("_") == "_");
        REQUIRE(parse(R"("x")") == R"("x")");
        REQUIRE(parse("not 1") == "(not 1)");
        REQUIRE(parse("1+1") == "(1 + 1)");
        REQUIRE(parse("- + 1 + * 1 - 3") == "(- + 1 + * 1 - 3)");
        REQUIRE(parse("+- *a -* + c") == "(+- * a -* + c)");
        REQUIRE(parse("f()") == "f");
        REQUIRE(parse("f(1)") == "f(1)");
        REQUIRE(parse("f(1,2)") == "f(1,2)");
        REQUIRE(parse("f(1,2,3)") == "f(1,2,3)");

        REQUIRE(parse("()") == "()");
        REQUIRE(parse("(,)") == "()");
        REQUIRE(parse("(1)") == "1");
        REQUIRE(parse("(1,)") == "(1,)");
        REQUIRE(parse("(1,2)") == "(1,2)");
        REQUIRE(parse("(1,2,)") == "(1,2)");
        REQUIRE(parse("(1,2,3)") == "(1,2,3)");
        REQUIRE(parse("(1,2,3,)") == "(1,2,3)");

        REQUIRE(parse("f((),(1),(1,),[],[1],[1,2],{},{1},{1,2})") == "f((),1,(1,),[],[1],[1,2],{},{1},{1,2})");
    }
    SECTION("literal") {
        auto parse = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            parser.init(str, store->string_ref("<input>"));
            return to_str(parser.parse_literal());
        };
        REQUIRE(parse("#true") == "#true");
        REQUIRE(parse("#false") == "#false");
        REQUIRE(parse("1 < 2") == "1<2");
        REQUIRE(parse("1 < 2<=3") == "1<2<=3");
        REQUIRE(parse("-f+1 < 2") == "-f+1<2");
        REQUIRE(parse("p(X)") == "p(X)");
        REQUIRE(parse("-p(X)") == "-p(X)");
        REQUIRE(parse("not p") == "not p");
        REQUIRE(parse("not not p") == "not not p");
        REQUIRE(parse("5") == "<failed>");

        REQUIRE(parse("p(1;2,*;*;*,*)") == "p(1;2,*;*;*,*)");
        REQUIRE(parse("p(*)") == "p(*)");
        REQUIRE(parse("p((*,))") == "p((*,))");
        REQUIRE(parse("p((1;2,*;*,;*,*))") == "p((1;2,*;*,;*,*))");
    }
    SECTION("body_literal") {
        auto parse = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            parser.init(str, store->string_ref("<input>"));
            return to_str(parser.parse_body_literal());
        };
        // negation
        REQUIRE(parse("a") == "a");
        REQUIRE(parse("not a") == "not a");
        REQUIRE(parse("not not a") == "not not a");
        REQUIRE(parse("#true") == "#true");
        REQUIRE(parse("#false") == "#false");
        // theory_atom | aggregate | set_aggregate
        REQUIRE(parse("&x{}") == "&x");
        REQUIRE(parse("not &x{}") == "not &x");
        REQUIRE(parse("#count{}") == "#count { }");
        REQUIRE(parse("{}") == "{ }");
        // atom_like relation aggregate
        REQUIRE(parse("a<{}") == "a < { }");
        REQUIRE(parse("a<#count{}") == "a < #count { }");
        // atom_like relation term ...
        REQUIRE(parse("a<b<c") == "a<b<c");
        REQUIRE(parse("a<a:a") == "a<a: a");
        // atom_like aggregate
        REQUIRE(parse("a{}") == "a <= { }");
        REQUIRE(parse("a#count{}") == "a <= #count { }");
        // term aggregate
        REQUIRE(parse("a+1{}") == "a+1 <= { }");
        REQUIRE(parse("a+1#count{}") == "a+1 <= #count { }");
        // term relation aggregate
        REQUIRE(parse("a+1<{}") == "a+1 < { }");
        REQUIRE(parse("a+1<#count{}") == "a+1 < #count { }");
        // term relation term ...
        REQUIRE(parse("a+1<b<c") == "a+1<b<c");
        REQUIRE(parse("a+1<a:a") == "a+1<a: a");
        // atom ...
        REQUIRE(parse("-a") == "-a");
        REQUIRE(parse("-a(X)") == "-a(X)");
        REQUIRE(parse("a:b,c") == "a: b, c");
        REQUIRE(parse("#true:a") == "#true: a");
        REQUIRE(parse("not #true:a") == "not #true: a");
        // aggregates with guards
        REQUIRE(parse("a<{}<b") == "a < { } < b");
        REQUIRE(parse("a{}b") == "a <= { } <= b");
        // aggregate elements
        REQUIRE(parse("#sum{:a;1:a;1,2:a,b,c}") == "#sum { : a; 1: a; 1,2: a, b, c }");
        REQUIRE(parse("{1<2;1<2:a;a:b;a:b,c}") == "{ 1<2; 1<2: a; a: b; a: b, c }");
    }
}

} // namespace Gringo::Input::Test
