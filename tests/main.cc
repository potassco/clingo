#include <catch2/catch_test_macros.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/action/scan.hpp>

#include <parser/term.hh>
#include <parser/literal.hh>
#include <parser/head_literal.hh>
#include <parser/body_literal.hh>
#include <parser/statement.hh>

namespace {

namespace test {

namespace dsl = lexy::dsl;

template <class P, char t = '\0'> struct parse_root : grammar::control {
    static constexpr auto terminator() { return t; }
    static constexpr auto eof() {
        if constexpr (t == '\0') {
            return dsl::eof;
        } else {
            return dsl::lit_c<t> + dsl::eof;
        }
    }
    static constexpr auto rule = dsl::p<P> + eof();
    static constexpr auto value = lexy::forward<typename decltype(P::value)::return_type>;
};

template <class P> struct match_root : grammar::control {
    static constexpr auto rule = dsl::p<P> + dsl::eof;
};

using term = parse_root<grammar::term>;
using literal = parse_root<grammar::literal>;
using head_literal = parse_root<grammar::head_literal, '.'>;
using body_literal = parse_root<grammar::body_literal, '.'>;
using statement = parse_root<grammar::statement>;

} // namespace test

template <typename Control> auto parse(std::string str) -> std::string {
    if (Control::terminator() != '\0') {
        str.push_back('.');
    }
    std::istringstream in;
    in.str(std::move(str));
    auto input = grammar::input{in};
    auto stm = lexy::parse<Control>(input, report_error);
    return stm.has_value() ? stm.value()->to_string() : "<failed>";
}

template <typename Control> auto match(std::string str) {
    std::istringstream in;
    in.str(std::move(str));
    auto input = grammar::input{in};
    auto res = lexy::validate<Control>(input, report_error);
    return res.is_success();
}

} // namespace

TEST_CASE("terms") {
    REQUIRE(parse<test::term>("42") == "42");
    REQUIRE(parse<test::term>("f") == "f");
    REQUIRE(parse<test::term>("f(  )+5") == "(f+5)");
    REQUIRE(parse<test::term>("f(1)") == "f(1)");
    REQUIRE(parse<test::term>("f ( 1 , 2 ; 4 )") == "f(1,2;4)");
    REQUIRE(parse<test::term>("1 + f") == "(1+f)");
    REQUIRE(parse<test::term>("@f(1,2)") == "@f(1,2)");
    REQUIRE(parse<test::term>("|42|") == "|42|");
    REQUIRE(parse<test::term>("||42||") == "||42||");
    REQUIRE(parse<test::term>("f(_,X)") == "f(_,X)");
    REQUIRE(parse<test::term>("(a)") == "a");
    REQUIRE(parse<test::term>("(a;a,b;a,b,c)") == "(a;a,b;a,b,c)");
    REQUIRE(parse<test::term>("(a, ; a,b,;a,b,c, )") == "(a,;a,b;a,b,c)");
}

TEST_CASE("literals") {
    REQUIRE(parse<test::literal>("#true") == "#true");
    REQUIRE(parse<test::literal>("#false") == "#false");
    REQUIRE(parse<test::literal>("1 < 2") == "1<2");
    REQUIRE(parse<test::literal>("-f+1 < 2") == "((-f)+1)<2");
    REQUIRE(parse<test::literal>("p(X)") == "p(X)");
    // TODO: get rid of parenthesis
    REQUIRE(parse<test::literal>("-p(X)") == "(-p(X))");
    REQUIRE(parse<test::literal>("not p") == "not p");
    REQUIRE(parse<test::literal>("not not p") == "not not p");
    REQUIRE(parse<test::literal>("5") == "<failed>");
}

TEST_CASE("head literals") {
    // theory_atom | aggregate | set_aggregate | not disjunction
    REQUIRE(parse<test::head_literal>("&x{}") == "&p{...}");
    REQUIRE(parse<test::head_literal>("#count{}") == "#count{}");
    REQUIRE(parse<test::head_literal>("{}") == "{}");
    REQUIRE(parse<test::head_literal>("not a") == "not a");
    // atom_like relation aggregate
    REQUIRE(parse<test::head_literal>("a<{}") == "a<{}");
    REQUIRE(parse<test::head_literal>("a<#count{}") == "a<#count{}");
    // atom_like relation term ...
    REQUIRE(parse<test::head_literal>("a<b<c") == "a<b<c");
    REQUIRE(parse<test::head_literal>("a<a:a") == "a<a:a");
    REQUIRE(parse<test::head_literal>("a<a:a;a") == "a<a:a;a");
    REQUIRE(parse<test::head_literal>("a<a,a") == "a<a;a");
    // atom_like aggregate
    REQUIRE(parse<test::head_literal>("a{}") == "a<={}");
    REQUIRE(parse<test::head_literal>("a#count{}") == "a<=#count{}");
    // term aggregate
    REQUIRE(parse<test::head_literal>("a+1{}") == "(a+1)<={}");
    REQUIRE(parse<test::head_literal>("a+1#count{}") == "(a+1)<=#count{}");
    // term relation aggregate
    REQUIRE(parse<test::head_literal>("a+1<{}") == "(a+1)<{}");
    REQUIRE(parse<test::head_literal>("a+1<#count{}") == "(a+1)<#count{}");
    // term relation term ...
    REQUIRE(parse<test::head_literal>("a+1<b<c") == "(a+1)<b<c");
    REQUIRE(parse<test::head_literal>("a+1<a:a") == "(a+1)<a:a");
    REQUIRE(parse<test::head_literal>("a+1<a:a;a") == "(a+1)<a:a;a");
    REQUIRE(parse<test::head_literal>("a+1<a,a") == "(a+1)<a;a");
    REQUIRE(parse<test::head_literal>("a+1<>a,a") == "<failed>");
    // atom ...
    REQUIRE(parse<test::head_literal>("-a") == "(-a)");
    REQUIRE(parse<test::head_literal>("-a(X)") == "(-a(X))");
    REQUIRE(parse<test::head_literal>("a:a") == "a:a");
    REQUIRE(parse<test::head_literal>("a:a;a") == "a:a;a");
    REQUIRE(parse<test::head_literal>("a,b") == "a;b");
    REQUIRE(parse<test::head_literal>("a;b") == "a;b");
    REQUIRE(parse<test::head_literal>("a|b") == "a;b");
    // aggregates with guards
    REQUIRE(parse<test::head_literal>("a<{}<b") == "a<{}<b");
    REQUIRE(parse<test::head_literal>("a{}b") == "a<={}<=b");
    // aggregate elements
    REQUIRE(parse<test::head_literal>("#sum{:a;1:a;1,2:a:b,c}") == "#sum{:a;1:a;1,2:a:b,c}");
    REQUIRE(parse<test::head_literal>("{1<2;1<2:a;a:b;a:b,c}") == "{1<2;1<2:a;a:b;a:b,c}");
}

TEST_CASE("body literals") {
    // negation
    REQUIRE(parse<test::body_literal>("a") == "a");
    REQUIRE(parse<test::body_literal>("not a") == "not a");
    REQUIRE(parse<test::body_literal>("not not a") == "not not a");
    // theory_atom | aggregate | set_aggregate
    REQUIRE(parse<test::body_literal>("&x{}") == "&p{...}");
    REQUIRE(parse<test::body_literal>("#count{}") == "#count{}");
    REQUIRE(parse<test::body_literal>("{}") == "{}");
    // atom_like relation aggregate
    REQUIRE(parse<test::body_literal>("a<{}") == "a<{}");
    REQUIRE(parse<test::body_literal>("a<#count{}") == "a<#count{}");
    // atom_like relation term ...
    REQUIRE(parse<test::body_literal>("a<b<c") == "a<b<c");
    REQUIRE(parse<test::body_literal>("a<a:a") == "a<a:a");
    // atom_like aggregate
    REQUIRE(parse<test::body_literal>("a{}") == "a<={}");
    REQUIRE(parse<test::body_literal>("a#count{}") == "a<=#count{}");
    // term aggregate
    REQUIRE(parse<test::body_literal>("a+1{}") == "(a+1)<={}");
    REQUIRE(parse<test::body_literal>("a+1#count{}") == "(a+1)<=#count{}");
    // term relation aggregate
    REQUIRE(parse<test::body_literal>("a+1<{}") == "(a+1)<{}");
    REQUIRE(parse<test::body_literal>("a+1<#count{}") == "(a+1)<#count{}");
    // term relation term ...
    REQUIRE(parse<test::body_literal>("a+1<b<c") == "(a+1)<b<c");
    REQUIRE(parse<test::body_literal>("a+1<a:a") == "(a+1)<a:a");
    // atom ...
    REQUIRE(parse<test::body_literal>("-a") == "(-a)");
    REQUIRE(parse<test::body_literal>("-a(X)") == "(-a(X))");
    REQUIRE(parse<test::body_literal>("a:b,c") == "a:b,c");
    // aggregates with guards
    REQUIRE(parse<test::body_literal>("a<{}<b") == "a<{}<b");
    REQUIRE(parse<test::body_literal>("a{}b") == "a<={}<=b");
    // aggregate elements
    REQUIRE(parse<test::body_literal>("#sum{:a;1:a;1,2:a,b,c}") == "#sum{:a;1:a;1,2:a,b,c}");
    REQUIRE(parse<test::body_literal>("{1<2;1<2:a;a:b;a:b,c}") == "{1<2;1<2:a;a:b;a:b,c}");
}

TEST_CASE("statement") {
    // TODO
    // 1. ensure `:` is never followed by `-`
    // 2. print with spaces
    REQUIRE(parse<test::statement>(":-.") == ":-.");
    REQUIRE(parse<test::statement>("a.") == "a.");
    REQUIRE(parse<test::statement>("a:-.") == "a.");
    REQUIRE(parse<test::statement>("a:-b.") == "a:-b.");
    REQUIRE(parse<test::statement>("a:-b,c.") == "a:-b;c.");
    REQUIRE(parse<test::statement>("a:-b;c.") == "a:-b;c.");
    REQUIRE(parse<test::statement>("a:-a:b,c;d.") == "a:-a:b,c;d.");
    REQUIRE(parse<test::statement>(":-.") == ":-.");
}

TEST_CASE("program") {
    std::istringstream in;
    in.str("a.b.c");
    auto input = grammar::input{in};
    auto scanner = lexy::scan<grammar::control>(input, report_error);
    auto stm = scanner.parse<grammar::statement>();
    REQUIRE(stm.has_value());
    REQUIRE(stm.value()->to_string() == "a.");
    input.discard_before(scanner.position());
    stm = scanner.parse<grammar::statement>();
    REQUIRE(stm.has_value());
    REQUIRE(stm.value()->to_string() == "b.");
    input.discard_before(scanner.position());
    stm = scanner.parse<grammar::statement>();
    REQUIRE(!stm.has_value());
}
