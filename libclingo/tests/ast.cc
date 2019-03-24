// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#include "tests.hh"
#include <iostream>
#include <fstream>
#include <sstream>

namespace Clingo { namespace Test {

namespace {

struct CB {
    CB(std::string &ret)
    : ret(ret) { ret.clear(); }
    void operator()(AST::Statement &&x) {
        std::ostringstream oss;
        oss << x;
        if (first) {
            REQUIRE(oss.str() == "#program base.");
            first = false;
        }
        else {
            if (!ret.empty()) { ret+= "\n"; }
            ret+= oss.str();
        }
    }
    bool first = true;
    std::string &ret;
};

std::string parse(char const *prg) {
    std::string ret;
    parse_program(prg, CB(ret));
    return ret;
}

ModelVec solve(char const *prg, PartSpan parts = {{"base", {}}}) {
    MessageVec messages;
    ModelVec models;
    Control ctl{{"0"}, [&messages](WarningCode code, char const *msg) { messages.emplace_back(code, msg); }, 20};
    ctl.with_builder([prg](ProgramBuilder &b){
        parse_program(prg, [&b](AST::Statement const &stm) { b.add(stm); });
    });
    ctl.ground(parts);
    test_solve(ctl.solve(), models);
    return models;
}

using StringVec = std::vector<std::string>;
StringVec parse_theory(char const *prg, char const *theory) {
    MessageVec messages;
    ModelVec models;
    Control ctl{{"0"}, [&messages](WarningCode code, char const *msg) { messages.emplace_back(code, msg); }, 20};
    ctl.with_builder([prg, theory](ProgramBuilder &b){
        parse_program(theory, [&b](AST::Statement const &stm) { b.add(stm); });
        parse_program(prg, [&b](AST::Statement const &stm) { b.add(stm); });
    });
    ctl.ground({{"base", {}}});
    StringVec ret;
    for (auto x : ctl.theory_atoms()) {
        std::ostringstream oss;
        oss << x;
        ret.emplace_back(oss.str());
    }
    return ret;
}

} // namespace

TEST_CASE("parse-ast", "[clingo]") {
    SECTION("statement") {
        REQUIRE(parse("a.") == "a.");
        REQUIRE(parse("a:-b.") == "a :- b.");
        REQUIRE(parse("#const a=10. [override]") == "#const a = 10. [override]");
        REQUIRE(parse("#const a=10. [default]") == "#const a = 10.");
        REQUIRE(parse("#const a=10.") == "#const a = 10.");
        REQUIRE(parse("#show a/1.") == "#show a/1.");
        REQUIRE(parse("#show $a/1.") == "#show $a/1.");
        REQUIRE(parse("#show a : b.") == "#show a : b.");
        REQUIRE(parse("#show $a : b.") == "#show $a : b.");
        REQUIRE(parse("#show $a : b.") == "#show $a : b.");
        REQUIRE(parse("#minimize{ 1:b }.") == ":~ b. [1@0]");
        REQUIRE(parse("#script (python) 42 #end.") == "#script (python) 42 #end.");
        REQUIRE(parse("#program p(k).") == "#program p(k).");
        REQUIRE(parse("#external p(k).") == "#external p(k). [false]");
        REQUIRE(parse("#external p(k). [true]") == "#external p(k). [true]");
        REQUIRE(parse("#external p(k) : a, b.") == "#external p(k) : a; b. [false]");
        REQUIRE(parse("#edge (u,v) : a, b.") == "#edge (u,v) : a; b.");
        REQUIRE(parse("#heuristic a : b, c. [L@P,level]") == "#heuristic a : b; c. [L@P,level]");
        REQUIRE(parse("#project a : b.") == "#project a : b.");
        REQUIRE(parse("#project a/2.") == "#project a/2.");
        REQUIRE(parse("#theory x {}.") == "#theory x {\n}.");
    }
    SECTION("theory definition") {
        REQUIRE(parse("#theory x { t { ++ : 1, unary } }.") == "#theory x {\n" "  t {\n" "    ++ : 1, unary\n" "  }\n" "}.");
        REQUIRE(parse("#theory x { &a/0 : t, any }.") == "#theory x {\n  &a/0 : t, any\n}.");
        REQUIRE(parse("#theory x { &a/0 : t, {+, -}, u, any }.") == "#theory x {\n  &a/0 : t, { +, - }, u, any\n}.");
    }
    SECTION("body literal") {
        REQUIRE(parse(":-a.") == "#false :- a.");
        REQUIRE(parse(":-not a.") == "#false :- not a.");
        REQUIRE(parse(":-not not a.") == "#false :- not not a.");
        REQUIRE(parse(":-a:b.") == "#false :- a : b.");
        REQUIRE(parse(":-a:b,c;d.") == "#false :- a : b, c; d.");
        REQUIRE(parse(":-1{a:b,c;e}2.") == "#false :- 1 <= { a : b, c; e :  } <= 2.");
        REQUIRE(parse(":-{a:b,c;e}2.") == "#false :- 2 >= { a : b, c; e :  }.");
        REQUIRE(parse(":-1#min{1,2:b,c;1:e}2.") == "#false :- 1 <= #min { 1,2 : b, c; 1 : e } <= 2.");
        REQUIRE(parse(":-&p { 1 : a,b; 2 : c }.") == "#false :- &p { 1 : a,b; 2 : c }.");
        REQUIRE(parse(":-#disjoint {1,2:$x:a,b}.") == "#false :- #disjoint { 1,2 : 1$*$x : a,b }.");
    }
    SECTION("head literal") {
        REQUIRE(parse("a.") == "a.");
        REQUIRE(parse("a:b.") == "a : b.");
        REQUIRE(parse("a:b,c;d.") == "d : ; a : b, c.");
        REQUIRE(parse("1{a:b,c;e}2.") == "1 <= { a : b, c; e :  } <= 2.");
        REQUIRE(parse("{a:b,c;e}2.") == "2 >= { a : b, c; e :  }.");
        REQUIRE(parse("1#min{1,2:h:b,c;1:e}2.") == "1 <= #min { 1,2 : h : b, c; 1 : e :  } <= 2.");
        REQUIRE(parse("&p { 1 : a,b; 2 : c }.") == "&p { 1 : a,b; 2 : c }.");
        REQUIRE(parse("&p { 1 : a,b; 2 : c } ** 33.") == "&p { 1 : a,b; 2 : c } ** 33.");
    }
    SECTION("literal") {
        REQUIRE(parse("#true.") == "#true.");
        REQUIRE(parse("#false.") == "#false.");
        REQUIRE(parse("a.") == "a.");
        REQUIRE(parse("not a.") == "not a.");
        REQUIRE(parse("not not a.") == "not not a.");
        REQUIRE(parse("1 != 3.") == "1!=3.");
        REQUIRE(parse("1 $< 2 $< 3.") == "1$<2$<3.");
        REQUIRE(parse("1 $< 2 $< 3.") == "1$<2$<3.");
    }
    SECTION("terms") {
        REQUIRE(parse("p(a).") == "p(a).");
        REQUIRE(parse("p(X).") == "p(X).");
        REQUIRE(parse("p(-a).") == "p(-a).");
        REQUIRE(parse("p(~a).") == "p(~a).");
        REQUIRE(parse("p(|a|).") == "p(|a|).");
        REQUIRE(parse("p((a+b)).") == "p((a+b)).");
        REQUIRE(parse("p((a-b)).") == "p((a-b)).");
        REQUIRE(parse("p((a*b)).") == "p((a*b)).");
        REQUIRE(parse("p((a/b)).") == "p((a/b)).");
        REQUIRE(parse("p((a\\b)).") == "p((a\\b)).");
        REQUIRE(parse("p((a?b)).") == "p((a?b)).");
        REQUIRE(parse("p((a^b)).") == "p((a^b)).");
        REQUIRE(parse("p(a..b).") == "p((a..b)).");
        REQUIRE(parse("p((),(1,),f(),f(1,2)).") == "p((),(1,),f,f(1,2)).");
        REQUIRE(parse("p(a;b).") == "(p(a);p(b)).");
        REQUIRE(parse("p((a,;b)).") == "p(((a,);b)).");
        REQUIRE(parse("1 $+ 3 $* $x $+ 7 $< 2 $< 3.") == "1$+3$*$x$+7$<2$<3.");
    }
    SECTION("theory terms") {
        REQUIRE(parse("&p{ } !! a.") == "&p {  } !! a.");
        REQUIRE(parse("&p{ } !! X.") == "&p {  } !! X.");
        REQUIRE(parse("&p{ } !! [].") == "&p {  } !! [].");
        REQUIRE(parse("&p{ } !! [1].") == "&p {  } !! [1].");
        REQUIRE(parse("&p{ } !! [1,2].") == "&p {  } !! [1,2].");
        REQUIRE(parse("&p{ } !! ().") == "&p {  } !! ().");
        REQUIRE(parse("&p{ } !! (a).") == "&p {  } !! a.");
        REQUIRE(parse("&p{ } !! (a,).") == "&p {  } !! (a,).");
        REQUIRE(parse("&p{ } !! {}.") == "&p {  } !! {}.");
        REQUIRE(parse("&p{ } !! f().") == "&p {  } !! f.");
        REQUIRE(parse("&p{ } !! f(a,1).") == "&p {  } !! f(a,1).");
        REQUIRE(parse("&p{ } !! 1 + (x + y * z).") == "&p {  } !! (1 + (x + y * z)).");
    }
}

TEST_CASE("add-ast", "[clingo]") {
    SECTION("statement") {
        REQUIRE(solve("a.") == ModelVec({{Id("a")}}));
        REQUIRE(solve("a. c. b :- a, c.") == ModelVec({{Id("a"), Id("b"), Id("c")}}));
        REQUIRE(solve("#const a=10. p(a).") == ModelVec({{Function("p", {Number(10)})}}));
        REQUIRE(solve("a. b. #show a/0.") == ModelVec({{Id("a")}}));
        REQUIRE(solve("$a$=1. $b$=2. #show $a/0.") == ModelVec({{Function("$", {Id("a"), Number(1)})}}));
        REQUIRE(solve("a. #show b : a.") == ModelVec({{Id("a"), Id("b")}}));
        REQUIRE(solve("$a$=1. $b$=2. #show. #show $a.") == ModelVec({{Function("$", {Id("a"), Number(1)})}}));
        REQUIRE(solve("#minimize{ 1:b; 2:a }. {a;b}. :- not a, not b.") == ModelVec({{Id("b")}}));
#ifdef WITH_LUA
        // NOTE: at the moment it is not possible to add additional script code - it is simply ignored
        REQUIRE(solve("#script (lua) function x() return 32; end #end. p(@x()).") == ModelVec({SymbolVector{}}));
#endif
        REQUIRE(solve("#edge (u,v) : a. #edge (v,u) : b. {a;b}.") == ModelVec({{}, {Id("a")}, {Id("b")}}));
        REQUIRE(solve("#theory x {}.") == ModelVec({SymbolVector{}}));
        // these just test parsing...
        REQUIRE(solve("#external a.") == ModelVec({SymbolVector{}}));
        REQUIRE(solve("#heuristic a : b, c. [1@2,level]") == ModelVec({SymbolVector{}}));
        REQUIRE(solve("#project a.") == ModelVec({SymbolVector{}}));
        REQUIRE(solve("#project a/0.") == ModelVec({SymbolVector{}}));
    }
    SECTION("body literal") {
        REQUIRE(solve("{a}. :-a.") == ModelVec({SymbolVector{}}));
        REQUIRE(solve("{a}. :-not a.") == ModelVec({{Id("a")}}));
        REQUIRE(solve("{a}. :-not not a.") == ModelVec({SymbolVector{}}));
        REQUIRE(solve(":-a:b.") == ModelVec({}));
        REQUIRE(solve(":-0{a:b;c}1. {a;b;c}.") == ModelVec({{Id("a"), Id("b"), Id("c")}}));
        REQUIRE(solve(":-0#min{1,2:a,b;2:c}2. {a;b;c}.") == ModelVec({{}, {Id("a")}, {Id("b")}}));
        auto c = [](char const *name, int value) { return Function("$", {Id(name), Number(value)}); };
        REQUIRE(solve("1 $<= $x $<= 2. 1 $<= $y $<= 2. :- #disjoint {1:$x; 2:$y}.") == ModelVec({{c("x", 1), c("y", 1)}, {c("x", 2), c("y", 2)}}));
    }
    SECTION("head literal") {
        REQUIRE(solve("a.") == ModelVec({{Id("a")}}));
        REQUIRE(solve("not a.") == ModelVec({SymbolVector{}}));
        REQUIRE(solve("not not a.") == ModelVec({}));
        REQUIRE(solve("a:b;c.{b}.") == ModelVec({{Id("a"), Id("b")}, {Id("b"), Id("c")}, {Id("c")}}));
        REQUIRE(solve("1{a:b;b}2.") == ModelVec({{Id("a"), Id("b")}, {Id("b")}}));
        REQUIRE(solve("#min{1,2:a;2:c}1.") == ModelVec({{Id("a")}, {Id("a"), Id("c")}}));
    }
    SECTION("literal") {
        REQUIRE(solve("a.") == ModelVec({{Id("a")}}));
        REQUIRE(solve("1=1.") == ModelVec({SymbolVector{}}));
        REQUIRE(solve("1!=1.") == ModelVec({}));
        REQUIRE(solve("#true.") == ModelVec({SymbolVector{}}));
        REQUIRE(solve("#false.") == ModelVec({}));
        REQUIRE(solve("1 $< 2 $< 3.") == ModelVec({SymbolVector{}}));
        REQUIRE(solve("2 $< 3 $< 1.") == ModelVec({}));
    }
    SECTION("terms") {
        auto t = [](char const *s) { return ModelVec({{parse_term(s)}}); };
        auto tt = [](std::initializer_list<char const *> l) {
            SymbolVector ret;
            for (auto &s : l) { ret.emplace_back(parse_term(s)); }
            return ModelVec{ret};
        };
        REQUIRE(solve("p(a).") == t("p(a)"));
        REQUIRE(solve("p(X) :- X=a.") == t("p(a)"));
        REQUIRE(solve("p(-1).") == t("p(-1)"));
        REQUIRE(solve("p(|1|).") == t("p(|1|)"));
        REQUIRE(solve("p((3+2)).") == t("p(5)"));
        REQUIRE(solve("p((3-2)).") == t("p(1)"));
        REQUIRE(solve("p((3*2)).") == t("p(6)"));
        REQUIRE(solve("p((7/2)).") == t("p(3)"));
        REQUIRE(solve("p((7\\2)).") == t("p(1)"));
        REQUIRE(solve("p((7?2)).") == t("p(7)"));
        REQUIRE(solve("p((7^2)).") == t("p(5)"));
        REQUIRE(solve("p(3..3).") == t("p(3)"));
        REQUIRE(solve("p(a;b).") == tt({"p(a)", "p(b)"}));
        REQUIRE(solve("p((),(1,),f(),f(1,2)).") == t("p((),(1,),f,f(1,2))"));
        REQUIRE(solve("p((a,;b)).") == tt({"p(b)", "p((a,))"}));
        REQUIRE(solve("12 $< 1 $+ 3 $* $x $+ 7 $< 17. 0 $<= x $<= 4.") == ModelVec({{Function("$", {Id("x"), Number(2)})}}));
    }
    SECTION("theory") {
        char const *theory = R"(
#theory x {
    t {
        * : 1, binary, left;
        ^ : 2, binary, right;
        - : 3, unary
    };
    &a/0 : t, directive;
    &b/0 : t, {=}, t, any
}.
)";
        REQUIRE(parse("&p{ } !! 1 + (x + y * z).") == "&p {  } !! (1 + (x + y * z)).");
        REQUIRE(parse_theory("&a{}.", theory) == StringVec({"&a{}"}));
        REQUIRE(parse_theory("&a{1,2,3:a,b}. {a;b}.", theory) == StringVec({"&a{1,2,3: a,b}"}));
        REQUIRE(parse_theory("&b{} = a.", theory) == StringVec({"&b{}=a"}));
        REQUIRE(parse_theory("&b{} = X:-X=1.", theory) == StringVec({"&b{}=1"}));
        REQUIRE(parse_theory("&b{} = [].", theory) == StringVec({"&b{}=[]"}));
        REQUIRE(parse_theory("&b{} = [1].", theory) == StringVec({"&b{}=[1]"}));
        REQUIRE(parse_theory("&b{} = [1,2].", theory) == StringVec({"&b{}=[1,2]"}));
        REQUIRE(parse_theory("&b{} = ().", theory) == StringVec({"&b{}=()"}));
        REQUIRE(parse_theory("&b{} = (a).", theory) == StringVec({"&b{}=a"}));
        REQUIRE(parse_theory("&b{} = (a,).", theory) == StringVec({"&b{}=(a,)"}));
        REQUIRE(parse_theory("&b{} = {}.", theory) == StringVec({"&b{}={}"}));
        REQUIRE(parse_theory("&b{} = f().", theory) == StringVec({"&b{}=f()"}));
        REQUIRE(parse_theory("&b{} = f(a,1).", theory) == StringVec({"&b{}=f(a,1)"}));
        REQUIRE(parse_theory("&b{} = a*x.", theory) == StringVec({"&b{}=(a*x)"}));
        REQUIRE(parse_theory("&b{} = -a*x*y^z^u.", theory) == StringVec({"&b{}=(((-a)*x)*(y^(z^u)))"}));
        REQUIRE(parse_theory("&b{} = -(a*x*y^z^u).", theory) == StringVec({"&b{}=(-((a*x)*(y^(z^u))))"}));
    }
}

} } // namespace Test Clingo
