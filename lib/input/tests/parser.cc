#include <clingo/input/parser.hh>

#include "test.hh"

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Input::Test {

TEST_CASE("parsev2") {
    std::vector<std::pair<MessageCode, std::string>> messages;
    auto store = make_symbol_store(true, false);
    auto log = Logger{[&](MessageCode code, std::string_view str) { messages.emplace_back(code, str); }};
    auto parser = Parser{log, *store};

    SECTION("symbol") {
        auto parse = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            parser.init(str, store->string_ref("<input>"));
            return to_str(parser.parse_symbol());
        };
        REQUIRE(parse("1") == "1");
        REQUIRE(parse("|-1|") == "1");
        REQUIRE(parse("|x|") == "<failed>");
        REQUIRE(parse("a") == "a");
        REQUIRE(parse("a()") == "a");
        REQUIRE(parse("a(1+2)") == "a(3)");
        REQUIRE(parse("a(1+2,3)") == "a(3,3)");
        REQUIRE(parse("a(1+2,3,4)") == "a(3,3,4)");
        REQUIRE(parse("a(1,)") == "<failed>");
        REQUIRE(parse("1-2") == "-1");
        REQUIRE(parse("(,)") == "()");
        REQUIRE(parse("(1,)") == "(1,)");
        REQUIRE(parse("(1,2)") == "(1,2)");
        REQUIRE(parse("(1,2,3)") == "(1,2,3)");
        REQUIRE(parse("(1+2)") == "3");
    }
    SECTION("const_def") {
        auto parse = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            parser.init(str, store->string_ref("<input>"));
            return to_str(parser.parse_const_def());
        };
        REQUIRE(parse("x=5") == "(x, 5)");
    }
    SECTION("program_parts") {
        auto parse = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            parser.init(str, store->string_ref("<input>"));
            return to_str(parser.parse_program_parts());
        };
        REQUIRE(parse("") == "[]");
        REQUIRE(parse("p") == "[(p, [])]");
        REQUIRE(parse("p(1)") == "[(p, [1])]");
        REQUIRE(parse("p(1),p(2),q(3)") == "[(p, [1]), (p, [2]), (q, [3])]");
    }
    SECTION("term") {
        auto parse = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            parser.init(str, store->string_ref("<input>"));
            return to_str(parser.parse_term());
        };
        REQUIRE(parse("(-a+b)") == "-a+b");
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
        REQUIRE(parse("f(\"\\n\")") == "f(\"\\n\")");
        REQUIRE(parse("f(\"\\t\")") == "f(\"\\t\")");
        REQUIRE(parse("f(\"\\r\")") == "f(\"\\r\")");
        REQUIRE(parse("f(\"\\u{041}\")") == "f(\"A\")");
        REQUIRE(parse("f(\"\\u{2665}\")") == "f(\"♥\")");
        REQUIRE(parse("f(\"\\u{1F602}\")") == "f(\"😂\")");
        REQUIRE(parse("f(\"\\u{FF}\")") == "f(\"\xC3\xBF\")");
        REQUIRE(parse("f(\"\\u{1F600}\")") == "f(\"\xF0\x9F\x98\x80\")");
        REQUIRE(parse("f(\"\\u{10FFFF}\")") == "f(\"\xF4\x8F\xBF\xBF\")");
        REQUIRE(parse("f(\"hello\\u{00E9}\\nworld\")") == "f(\"helloé\\nworld\")");
        REQUIRE(parse("f\"hello\\nworld\"") == "f\"hello\\nworld\"");
        REQUIRE(parse("f\"hello\\tworld\"") == "f\"hello\\tworld\"");
        REQUIRE(parse("f\"hello\\rworld\"") == "f\"hello\\rworld\"");
        REQUIRE(parse("f\"{{hello}}\"") == "f\"{{hello}}\"");
        REQUIRE(parse("f\"hello\\u{00E9}\\nworld\"") == "f\"helloé\\nworld\"");
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
        REQUIRE(parse("f\"{X}\"") == "f\"{X}\"");
        REQUIRE(parse("f\"{X[0].name}\"") == "f\"{X[0].name}\"");
        REQUIRE(parse("f\"{X!r}\"") == "f\"{X!r}\"");
        REQUIRE(parse("f\"{X:<}\"") == "f\"{X:<}\"");
        REQUIRE(parse("f\"{X: <}\"") == "f\"{X: <}\"");
        REQUIRE(parse("f\"{X:+}\"") == "f\"{X:+}\"");
        REQUIRE(parse("f\"{X:#}\"") == "f\"{X:#}\"");
        REQUIRE(parse("f\"{X:13}\"") == "f\"{X:13}\"");
        REQUIRE(parse("f\"{X:,}\"") == "f\"{X:,}\"");
        REQUIRE(parse("f\"{X:x}\"") == "f\"{X:x}\"");
        REQUIRE(parse("f\"{X[0]!r: <+#13,x}\"") == "f\"{X[0]!r: <+#13,x}\"");

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
        REQUIRE(parse("p(*,1)") == "p(*,1)");
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

    SECTION("head_literal") {
        auto parse = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            parser.init(str, store->string_ref("<input>"));
            return to_str(parser.parse_head_literal());
        };
        // theory_atom | aggregate | set_aggregate | not disjunction
        REQUIRE(parse("&x{}") == "&x");
        REQUIRE(parse("#count{}") == "#count { }");
        REQUIRE(parse("{}") == "{ }");
        REQUIRE(parse("not a") == "not a");
        // atom_like relation aggregate
        REQUIRE(parse("a<{}") == "a < { }");
        REQUIRE(parse("a<#count{}") == "a < #count { }");
        // atom_like relation term ...
        REQUIRE(parse("a<b<c") == "a<b<c");
        REQUIRE(parse("a<a:a") == "a<a: a");
        REQUIRE(parse("a<a:a;a") == "a<a: a; a");
        REQUIRE(parse("a<a,a") == "a<a; a");
        // atom_like aggregate
        REQUIRE(parse("a{}") == "a <= { }");
        REQUIRE(parse("a#count{}") == "a <= #count { }");
        // term aggregate
        REQUIRE(parse("a+1 { }") == "a+1 <= { }");
        REQUIRE(parse("a+1#count{}") == "a+1 <= #count { }");
        // term relation aggregate
        REQUIRE(parse("a+1<{}") == "a+1 < { }");
        REQUIRE(parse("a+1<#count{}") == "a+1 < #count { }");
        // term relation term ...
        REQUIRE(parse("a+1<b<c") == "a+1<b<c");
        REQUIRE(parse("a+1<a:a") == "a+1<a: a");
        REQUIRE(parse("a+1<a:a;a") == "a+1<a: a; a");
        REQUIRE(parse("a+1<a,a") == "a+1<a; a");
        REQUIRE(parse("a+1<>a,a") == "a+1!=a; a");
        REQUIRE(parse("a+1><a,a") == "<failed>");
        // atom ...
        REQUIRE(parse("-a") == "-a");
        REQUIRE(parse("-a(X)") == "-a(X)");
        REQUIRE(parse("a:a") == "a: a");
        REQUIRE(parse("a:a;a") == "a: a; a");
        REQUIRE(parse("a,b") == "a; b");
        REQUIRE(parse("a;b") == "a; b");
        REQUIRE(parse("a|b") == "a; b");
        REQUIRE(parse("#true|#false|not #true") == "#true; #false; not #true");
        // aggregates with guards
        REQUIRE(parse("a<{}<b") == "a < { } < b");
        REQUIRE(parse("a{}b") == "a <= { } <= b");
        // aggregate elements
        REQUIRE(parse("#sum{: a; 1: a; 1,2: a: b, c}") == "#sum { : a; 1: a; 1,2: a: b, c }");
        REQUIRE(parse("{1<2;1<2:a;a:b;a:b,c}") == "{ 1<2; 1<2: a; a: b; a: b, c }");
        // theory atoms
        REQUIRE(parse("&p(X){43+-Y:a} <== 7") == "&p(X) { (43 +- Y): a } <== 7");
    }

    SECTION("parse_theory") {
        auto parse = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            parser.init(str, store->string_ref("<input>"));
            auto a = to_str(parser.parse_body_literal());
            parser.init(str, store->string_ref("<input>"));
            auto b = to_str(parser.parse_head_literal());
            REQUIRE(a == b);
            return a;
        };
        // empty guards/elements
        REQUIRE(parse("&p(x,y;z){}<=a") == "&p(x,y;z) { } <= a");
        REQUIRE(parse("&p(x,y;z)<=a") == "&p(x,y;z) { } <= a");
        REQUIRE(parse("&p(x,y;z){}") == "&p(x,y;z)");
        REQUIRE(parse("&p(x,y;z)") == "&p(x,y;z)");
        // empty tuples/conditions
        REQUIRE(parse("&p{1:a}") == "&p { 1: a }");
        REQUIRE(parse("&p{1}") == "&p { 1 }");
        REQUIRE(parse("&p{1:}") == "&p { 1 }");
        REQUIRE(parse("&p{:a}") == "&p { : a }");
        REQUIRE(parse("&p{:}") == "&p { :  }");
        // term types
        REQUIRE(parse("&p{1}") == "&p { 1 }");
        REQUIRE(parse("&p{a}") == "&p { a }");
        REQUIRE(parse("&p{\"a\"}") == "&p { \"a\" }");
        REQUIRE(parse("&p{#sup}") == "&p { #sup }");
        REQUIRE(parse("&p{_X}") == "&p { _X }");
        REQUIRE(parse("&p{_}") == "&p { _ }");
        // tuple
        REQUIRE(parse("&p{()}") == "&p { () }");
        REQUIRE(parse("&p{(a)}") == "&p { a }");
        REQUIRE(parse("&p{(a,b)}") == "&p { (a,b) }");
        REQUIRE(parse("&p{(a,b,)}") == "&p { (a,b) }");
        REQUIRE(parse("&p{(a,)}") == "&p { (a,) }");
        // set
        REQUIRE(parse("&p{{}}") == "&p { {} }");
        REQUIRE(parse("&p{{a}}") == "&p { {a} }");
        REQUIRE(parse("&p{{a,b}}") == "&p { {a,b} }");
        // list
        REQUIRE(parse("&p{[]}") == "&p { [] }");
        REQUIRE(parse("&p{[a]}") == "&p { [a] }");
        REQUIRE(parse("&p{[a,b]}") == "&p { [a,b] }");
        // unparsed
        REQUIRE(parse("&p{+- *a -* + c}") == "&p { (+- * a -* + c) }");
    }
    SECTION("statement") {
        auto parse = [&](char const *str) -> std::string {
            log.reset();
            messages.clear();
            parser.init(str, store->string_ref("<input>"));
            return to_str(parser.parse_statement());
        };
        // rule
        REQUIRE(parse("a.") == "a.");
        REQUIRE(parse("a:-.") == "a.");
        REQUIRE(parse("a:-b.") == "a :- b.");
        REQUIRE(parse("a:-b,c.") == "a :- b; c.");
        REQUIRE(parse("a:-b;c.") == "a :- b; c.");
        REQUIRE(parse("a:-a:b,c;d.") == "a :- a: b, c; d.");
        REQUIRE(parse(":-.") == " :- .");

        // optimize
        REQUIRE(parse(":~ . [1]") == " :~ . [1]");
        REQUIRE(parse(":~ . [1@2]") == " :~ . [1@2]");
        REQUIRE(parse(":~ a. [1]") == " :~ a. [1]");
        REQUIRE(parse(":~ a; b. [1]") == " :~ a; b. [1]");
        REQUIRE(parse("#minimize {}.") == "#minimize { }.");
        REQUIRE(parse("#maximize {}.") == "#maximize { }.");
        REQUIRE(parse("#minimize {1}.") == "#minimize { 1 }.");
        REQUIRE(parse("#minimize {1@2}.") == "#minimize { 1@2 }.");
        REQUIRE(parse("#minimize {1@2,3,4}.") == "#minimize { 1@2,3,4 }.");
        REQUIRE(parse("#minimize {1@2,3,4:a}.") == "#minimize { 1@2,3,4: a }.");
        REQUIRE(parse("#minimize {1@2;3@4}.") == "#minimize { 1@2; 3@4 }.");

        // show
        REQUIRE(parse("#show a/2.") == "#show a/2. [true]");
        REQUIRE(parse("#show -a/2.") == "#show -a/2. [true]");
        REQUIRE(parse("#show -a/2. [true]") == "#show -a/2. [true]");
        REQUIRE(parse("#show -a/2. [false]") == "#show -a/2. [false]");
        // Note: the print function could be refined to omit the parenthesis
        REQUIRE(parse("#show a()/2.") == "#show (a/2): .");
        REQUIRE(parse("#show -a()/2.") == "#show (-a/2): .");
        REQUIRE(parse("#show (-a/2).") == "#show -a/2: .");
        REQUIRE(parse("#show (-a()/2).") == "#show -a/2: .");
        REQUIRE(parse("#show p(X).") == "#show p(X): .");
        REQUIRE(parse("#show p(X): .") == "#show p(X): .");
        REQUIRE(parse("#show p(X): a.") == "#show p(X): a.");

        // defined
        REQUIRE(parse("#defined a/2.") == "#defined a/2.");
        REQUIRE(parse("#defined -a/2.") == "#defined -a/2.");

        // project
        REQUIRE(parse("#project a/2.") == "#project a/2.");
        REQUIRE(parse("#project -a/2.") == "#project -a/2.");
        REQUIRE(parse("#project p(X).") == "#project p(X).");
        REQUIRE(parse("#project p(X): .") == "#project p(X).");
        REQUIRE(parse("#project p(X): a.") == "#project p(X): a.");

        // edge
        REQUIRE(parse("#edge (a,b).") == "#edge (a,b).");
        REQUIRE(parse("#edge (a,b):.") == "#edge (a,b).");
        REQUIRE(parse("#edge (a,b): c.") == "#edge (a,b): c.");
        REQUIRE(parse("#edge (a,b;c,d): e.") == "#edge (a,b;c,d): e.");
        REQUIRE(parse("#edge (a,b;c,d): e; f.") == "#edge (a,b;c,d): e; f.");

        // heuristic
        REQUIRE(parse("#heuristic a. [level@1,true]") == "#heuristic a. [level@1,true]");
        REQUIRE(parse("#heuristic a. [level,true]") == "#heuristic a. [level,true]");
        REQUIRE(parse("#heuristic a:. [level,true]") == "#heuristic a. [level,true]");
        REQUIRE(parse("#heuristic -a. [level,true]") == "#heuristic -a. [level,true]");
        REQUIRE(parse("#heuristic a:a; b. [level,true]") == "#heuristic a: a; b. [level,true]");

        // script
        REQUIRE(parse("#script   ( python  )     code   #end.") == "#script (python)     code   #end.");
        REQUIRE(parse("#script (python)\ncode\n#end.") == "#script (python)\ncode\n#end.");
        REQUIRE(parse("#script (python) всем привет #end.") == "#script (python) всем привет #end.");

        // external
        REQUIRE(parse("#external a(X): b(X).") == "#external a(X): b(X).");
        REQUIRE(parse("#external -a(X): b(X).") == "#external -a(X): b(X).");
        REQUIRE(parse("#external a(X): b(X). [X]") == "#external a(X): b(X). [X]");

        // include
        REQUIRE(parse("#include \"abc\".") == "#include \"abc\".");
        REQUIRE(parse("#include <abc>.") == "#include <abc>.");

        // program
        REQUIRE(parse("#program base.") == "#program base.");
        REQUIRE(parse("#program base().") == "#program base.");
        REQUIRE(parse("#program step(t).") == "#program step(t).");
        REQUIRE(parse("#program step(k,t).") == "#program step(k,t).");

        // const
        REQUIRE(parse("#const x=42.") == "#const x=42. [default]");
        REQUIRE(parse("#const x=42. [default]") == "#const x=42. [default]");
        REQUIRE(parse("#const x=42. [override]") == "#const x=42. [override]");

        char const *theory = R"(#theory y {
  a { };
  b { - : 10, unary };
  b {
    - : 10, unary;
    + : 9, binary, right
  };
  &p/0: a, {+,-}, b, head
}.)";

        // theory
        REQUIRE(parse("#theory x {}.") == "#theory x { }.");
        REQUIRE(parse(theory) == theory);

        // extra
        REQUIRE(parse("X=Y+Z=Z: cond.") == "X=Y+Z=Z: cond.");
        REQUIRE(parse("x :- not X=Y+Z=Z: cond.") == "x :- not X=Y+Z=Z: cond.");
        REQUIRE(parse("h :- X=Y, not q(Y), p(X).") == "h :- X=Y; not q(Y); p(X).");
        REQUIRE(parse("h :- X>=1; X<=5, X>=3, X<=7.") == "h :- X>=1; X<=5; X>=3; X<=7.");
        REQUIRE(parse("h :- a, 1<=X<=Y, b.") == "h :- a; 1<=X<=Y; b.");
        REQUIRE(parse("&count { X: X+Y=6, -3*X+Y=2 } >= 1 :- X>=0.") == "&count { X: X+Y=6, -3*X+Y=2 } >= 1 :- X>=0.");
    }

    SECTION("scan_recover") {
        std::istringstream in{"a. . b(. c."};
        parser.init(in, store->string_ref("<input>"));
        REQUIRE(to_str(parser.scan()) == "(a., T)");
        REQUIRE(to_str(parser.scan()) == "(c., F)");
    }

    SECTION("scan_comments") {
        std::istringstream in{"%p\na.%a\nb%b\n.%c\nc%d\n"};
        parser.init(in, store->string_ref("<input>"));
        REQUIRE(to_str(parser.scan()) == "(%p, T)");
        REQUIRE(to_str(parser.scan()) == "(a., T)");
        REQUIRE(to_str(parser.scan()) == "(%a, T)");
        REQUIRE(to_str(parser.scan()) == "(%b, T)");
        REQUIRE(to_str(parser.scan()) == "(b., T)");
        REQUIRE(to_str(parser.scan()) == "(%c, F)");
        REQUIRE(to_str(parser.scan()) == "(%d, T)");
        REQUIRE(to_str(parser.scan()) == "(<failed>, T)");
    }

    SECTION("scan_comments") {
        std::istringstream in{"%xxx"};
        parser.init(in, store->string_ref("<input>"));
        REQUIRE(to_str(parser.scan()) == "(%xxx, T)");
        REQUIRE(to_str(parser.scan()) == "(<failed>, T)");
    }

    SECTION("scan_comments") {
        std::istringstream in{"%"};
        parser.init(in, store->string_ref("<input>"));
        REQUIRE(to_str(parser.scan()) == "(%, T)");
        REQUIRE(to_str(parser.scan()) == "(<failed>, T)");
    }

    SECTION("scan_block_comments") {
        std::istringstream in{"a. %* % *%\n *% b. %* %* *%"};
        parser.init(in, store->string_ref("<input>"));
        REQUIRE(to_str(parser.scan()) == "(a., T)");
        REQUIRE(to_str(parser.scan()) == "(%* % *%\n *%, T)");
        REQUIRE(to_str(parser.scan()) == "(b., T)");
        REQUIRE(to_str(parser.scan()) == "(<failed>, F)");
    }
}

} // namespace CppClingo::Input::Test
