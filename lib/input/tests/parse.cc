#include "test.hh"

namespace Gringo::Input::Test {

namespace {

auto parse_term(std::string const &str) -> std::string {
    ParseHelper ph;
    return to_str(ph.term(str));
}

auto parse_literal(std::string const &str) -> std::string {
    ParseHelper ph;
    return to_str(ph.literal(str));
}

auto parse_head_literal(std::string const &str) -> std::string {
    ParseHelper ph;
    return to_str(ph.head_literal(str));
}

auto parse_body_literal(std::string const &str) -> std::string {
    ParseHelper ph;
    return to_str(ph.body_literal(str));
}

auto parse_statement(std::string const &str) -> std::string {
    ParseHelper ph;
    return to_str(ph.statement(str));
}

} // namespace

TEST_CASE("parse_term") {
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
}

TEST_CASE("parse_literal") {
    REQUIRE(parse_literal("#true") == "#true");
    REQUIRE(parse_literal("#false") == "#false");
    REQUIRE(parse_literal("1 < 2") == "1<2");
    REQUIRE(parse_literal("-f+1 < 2") == "-f+1<2");
    REQUIRE(parse_literal("p(X)") == "p(X)");
    REQUIRE(parse_literal("-p(X)") == "-p(X)");
    REQUIRE(parse_literal("not p") == "not p");
    REQUIRE(parse_literal("not not p") == "not not p");
    REQUIRE(parse_literal("5") == "<failed>");
}

TEST_CASE("parse_theory") {
    // empty guards/elements
    REQUIRE(parse_head_literal("&p(x,y;z){}<=a") == "&p(x,y;z) { } <= a");
    REQUIRE(parse_head_literal("&p(x,y;z)<=a") == "&p(x,y;z) { } <= a");
    REQUIRE(parse_head_literal("&p(x,y;z){}") == "&p(x,y;z)");
    REQUIRE(parse_head_literal("&p(x,y;z)") == "&p(x,y;z)");
    // empty tuples/conditions
    REQUIRE(parse_head_literal("&p{1:a}") == "&p { 1: a }");
    REQUIRE(parse_head_literal("&p{1}") == "&p { 1 }");
    REQUIRE(parse_head_literal("&p{1:}") == "&p { 1 }");
    REQUIRE(parse_head_literal("&p{:a}") == "&p { : a }");
    REQUIRE(parse_head_literal("&p{:}") == "&p { :  }");
    // term types
    REQUIRE(parse_head_literal("&p{1}") == "&p { 1 }");
    REQUIRE(parse_head_literal("&p{a}") == "&p { a }");
    REQUIRE(parse_head_literal("&p{\"a\"}") == "&p { \"a\" }");
    REQUIRE(parse_head_literal("&p{#sup}") == "&p { #sup }");
    REQUIRE(parse_head_literal("&p{_X}") == "&p { _X }");
    REQUIRE(parse_head_literal("&p{_}") == "&p { _ }");
    // tuple
    REQUIRE(parse_head_literal("&p{()}") == "&p { () }");
    REQUIRE(parse_head_literal("&p{(a)}") == "&p { a }");
    REQUIRE(parse_head_literal("&p{(a,b)}") == "&p { (a,b) }");
    REQUIRE(parse_head_literal("&p{(a,b,)}") == "&p { (a,b) }");
    REQUIRE(parse_head_literal("&p{(a,)}") == "&p { (a,) }");
    // set
    REQUIRE(parse_head_literal("&p{{}}") == "&p { {} }");
    REQUIRE(parse_head_literal("&p{{a}}") == "&p { {a} }");
    REQUIRE(parse_head_literal("&p{{a,b}}") == "&p { {a,b} }");
    // list
    REQUIRE(parse_head_literal("&p{[]}") == "&p { [] }");
    REQUIRE(parse_head_literal("&p{[a]}") == "&p { [a] }");
    REQUIRE(parse_head_literal("&p{[a,b]}") == "&p { [a,b] }");
    // unparsed
    REQUIRE(parse_head_literal("&p{+- *a -* + c}") == "&p { (+- * a -* + c) }");
}

TEST_CASE("parse_head_literal") {
    // theory_atom | aggregate | set_aggregate | not disjunction
    REQUIRE(parse_head_literal("&x{}") == "&x");
    REQUIRE(parse_head_literal("#count{}") == "#count { }");
    REQUIRE(parse_head_literal("{}") == "{ }");
    REQUIRE(parse_head_literal("not a") == "not a");
    // atom_like relation aggregate
    REQUIRE(parse_head_literal("a<{}") == "a < { }");
    REQUIRE(parse_head_literal("a<#count{}") == "a < #count { }");
    // atom_like relation term ...
    REQUIRE(parse_head_literal("a<b<c") == "a<b<c");
    REQUIRE(parse_head_literal("a<a:a") == "a<a: a");
    REQUIRE(parse_head_literal("a<a:a;a") == "a<a: a; a");
    REQUIRE(parse_head_literal("a<a,a") == "a<a; a");
    // atom_like aggregate
    REQUIRE(parse_head_literal("a{}") == "a <= { }");
    REQUIRE(parse_head_literal("a#count{}") == "a <= #count { }");
    // term aggregate
    REQUIRE(parse_head_literal("a+1 { }") == "a+1 <= { }");
    REQUIRE(parse_head_literal("a+1#count{}") == "a+1 <= #count { }");
    // term relation aggregate
    REQUIRE(parse_head_literal("a+1<{}") == "a+1 < { }");
    REQUIRE(parse_head_literal("a+1<#count{}") == "a+1 < #count { }");
    // term relation term ...
    REQUIRE(parse_head_literal("a+1<b<c") == "a+1<b<c");
    REQUIRE(parse_head_literal("a+1<a:a") == "a+1<a: a");
    REQUIRE(parse_head_literal("a+1<a:a;a") == "a+1<a: a; a");
    REQUIRE(parse_head_literal("a+1<a,a") == "a+1<a; a");
    REQUIRE(parse_head_literal("a+1<>a,a") == "<failed>");
    // atom ...
    REQUIRE(parse_head_literal("-a") == "-a");
    REQUIRE(parse_head_literal("-a(X)") == "-a(X)");
    REQUIRE(parse_head_literal("a:a") == "a: a");
    REQUIRE(parse_head_literal("a:a;a") == "a: a; a");
    REQUIRE(parse_head_literal("a,b") == "a; b");
    REQUIRE(parse_head_literal("a;b") == "a; b");
    REQUIRE(parse_head_literal("a|b") == "a; b");
    REQUIRE(parse_head_literal("#true|#false|not #true") == "#true; #false; not #true");
    // aggregates with guards
    REQUIRE(parse_head_literal("a<{}<b") == "a < { } < b");
    REQUIRE(parse_head_literal("a{}b") == "a <= { } <= b");
    // aggregate elements
    REQUIRE(parse_head_literal("#sum{: a; 1: a; 1,2: a: b, c}") == "#sum { : a; 1: a; 1,2: a: b, c }");
    REQUIRE(parse_head_literal("{1<2;1<2:a;a:b;a:b,c}") == "{ 1<2; 1<2: a; a: b; a: b, c }");
    // theory atoms
    REQUIRE(parse_head_literal("&p(X){43+-Y:a} <== 7") == "&p(X) { (43 +- Y): a } <== 7");
}

TEST_CASE("parse_body_literal") {
    // negation
    REQUIRE(parse_body_literal("a") == "a");
    REQUIRE(parse_body_literal("not a") == "not a");
    REQUIRE(parse_body_literal("not not a") == "not not a");
    REQUIRE(parse_body_literal("#true") == "#true");
    REQUIRE(parse_body_literal("#false") == "#false");
    // theory_atom | aggregate | set_aggregate
    REQUIRE(parse_body_literal("&x{}") == "&x");
    REQUIRE(parse_body_literal("not &x{}") == "not &x");
    REQUIRE(parse_body_literal("#count{}") == "#count { }");
    REQUIRE(parse_body_literal("{}") == "{ }");
    // atom_like relation aggregate
    REQUIRE(parse_body_literal("a<{}") == "a < { }");
    REQUIRE(parse_body_literal("a<#count{}") == "a < #count { }");
    // atom_like relation term ...
    REQUIRE(parse_body_literal("a<b<c") == "a<b<c");
    REQUIRE(parse_body_literal("a<a:a") == "a<a: a");
    // atom_like aggregate
    REQUIRE(parse_body_literal("a{}") == "a <= { }");
    REQUIRE(parse_body_literal("a#count{}") == "a <= #count { }");
    // term aggregate
    REQUIRE(parse_body_literal("a+1{}") == "a+1 <= { }");
    REQUIRE(parse_body_literal("a+1#count{}") == "a+1 <= #count { }");
    // term relation aggregate
    REQUIRE(parse_body_literal("a+1<{}") == "a+1 < { }");
    REQUIRE(parse_body_literal("a+1<#count{}") == "a+1 < #count { }");
    // term relation term ...
    REQUIRE(parse_body_literal("a+1<b<c") == "a+1<b<c");
    REQUIRE(parse_body_literal("a+1<a:a") == "a+1<a: a");
    // atom ...
    REQUIRE(parse_body_literal("-a") == "-a");
    REQUIRE(parse_body_literal("-a(X)") == "-a(X)");
    REQUIRE(parse_body_literal("a:b,c") == "a: b, c");
    REQUIRE(parse_body_literal("#true:a") == "#true: a");
    REQUIRE(parse_body_literal("not #true:a") == "not #true: a");
    // aggregates with guards
    REQUIRE(parse_body_literal("a<{}<b") == "a < { } < b");
    REQUIRE(parse_body_literal("a{}b") == "a <= { } <= b");
    // aggregate elements
    REQUIRE(parse_body_literal("#sum{:a;1:a;1,2:a,b,c}") == "#sum { : a; 1: a; 1,2: a, b, c }");
    REQUIRE(parse_body_literal("{1<2;1<2:a;a:b;a:b,c}") == "{ 1<2; 1<2: a; a: b; a: b, c }");
}

TEST_CASE("parse_statement") {
    // rule
    REQUIRE(parse_statement(":-.") == " :- .");
    REQUIRE(parse_statement("a.") == "a.");
    REQUIRE(parse_statement("a:-.") == "a.");
    REQUIRE(parse_statement("a:-b.") == "a :- b.");
    REQUIRE(parse_statement("a:-b,c.") == "a :- b; c.");
    REQUIRE(parse_statement("a:-b;c.") == "a :- b; c.");
    REQUIRE(parse_statement("a:-a:b,c;d.") == "a :- a: b, c; d.");

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
    REQUIRE(parse_statement("#theory x {}.") == "#theory x { }.");
    REQUIRE(parse_statement(theory) == theory);

    // optimize
    REQUIRE(parse_statement("#minimize {}.") == "#minimize { }.");
    REQUIRE(parse_statement("#maximize {}.") == "#maximize { }.");
    REQUIRE(parse_statement("#minimize {1}.") == "#minimize { 1 }.");
    REQUIRE(parse_statement("#minimize {1@2}.") == "#minimize { 1@2 }.");
    REQUIRE(parse_statement("#minimize {1@2,3,4}.") == "#minimize { 1@2,3,4 }.");
    REQUIRE(parse_statement("#minimize {1@2,3,4:a}.") == "#minimize { 1@2,3,4: a }.");
    REQUIRE(parse_statement("#minimize {1@2;3@4}.") == "#minimize { 1@2; 3@4 }.");
    REQUIRE(parse_statement(":~ . [1]") == " :~ . [1]");
    REQUIRE(parse_statement(":~ . [1@2]") == " :~ . [1@2]");
    REQUIRE(parse_statement(":~ a. [1]") == " :~ a. [1]");
    REQUIRE(parse_statement(":~ a; b. [1]") == " :~ a; b. [1]");

    // show
    REQUIRE(parse_statement("#show a/2.") == "#show a/2.");
    REQUIRE(parse_statement("#show -a/2.") == "#show -a/2.");
    REQUIRE(parse_statement("#show (-a/2).") == "#show (-a/2): .");
    REQUIRE(parse_statement("#show (-a()/2).") == "#show (-a/2): .");
    REQUIRE(parse_statement("#show p(X).") == "#show p(X): .");
    REQUIRE(parse_statement("#show p(X): .") == "#show p(X): .");
    REQUIRE(parse_statement("#show p(X): a.") == "#show p(X): a.");

    // project
    REQUIRE(parse_statement("#project a/2.") == "#project a/2.");
    REQUIRE(parse_statement("#project -a/2.") == "#project -a/2.");
    REQUIRE(parse_statement("#project p(X).") == "#project p(X).");
    REQUIRE(parse_statement("#project p(X): .") == "#project p(X).");
    REQUIRE(parse_statement("#project p(X): a.") == "#project p(X): a.");

    // defined
    REQUIRE(parse_statement("#defined a/2.") == "#defined a/2.");
    REQUIRE(parse_statement("#defined -a/2.") == "#defined -a/2.");

    // edge
    REQUIRE(parse_statement("#edge (a,b).") == "#edge (a,b).");
    REQUIRE(parse_statement("#edge (a,b):.") == "#edge (a,b).");
    REQUIRE(parse_statement("#edge (a,b): c.") == "#edge (a,b): c.");
    REQUIRE(parse_statement("#edge (a,b;c,d): e.") == "#edge (a,b;c,d): e.");
    REQUIRE(parse_statement("#edge (a,b;c,d): e; f.") == "#edge (a,b;c,d): e; f.");

    // heuristic
    REQUIRE(parse_statement("#heuristic a. [level@1,true]") == "#heuristic a. [level@1,true]");
    REQUIRE(parse_statement("#heuristic a. [level,true]") == "#heuristic a. [level,true]");
    REQUIRE(parse_statement("#heuristic a:. [level,true]") == "#heuristic a. [level,true]");
    REQUIRE(parse_statement("#heuristic -a. [level,true]") == "#heuristic -a. [level,true]");
    REQUIRE(parse_statement("#heuristic a:a; b. [level,true]") == "#heuristic a: a; b. [level,true]");

    // script
    REQUIRE(parse_statement("#script   ( python  )     code   #end.") == "#script (python)     code   #end.");
    REQUIRE(parse_statement("#script (python)\ncode\n#end.") == "#script (python)\ncode\n#end.");
    REQUIRE(parse_statement("#script (python) всем привет #end.") == "#script (python) всем привет #end.");

    // external
    REQUIRE(parse_statement("#external a(X): b(X).") == "#external a(X): b(X).");
    REQUIRE(parse_statement("#external -a(X): b(X).") == "#external -a(X): b(X).");
    REQUIRE(parse_statement("#external a(X): b(X). [X]") == "#external a(X): b(X). [X]");

    // include
    REQUIRE(parse_statement("#include \"abc\".") == "#include \"abc\".");
    REQUIRE(parse_statement("#include <abc>.") == "#include <abc>.");

    // program
    REQUIRE(parse_statement("#program base.") == "#program base.");
    REQUIRE(parse_statement("#program base().") == "#program base.");
    REQUIRE(parse_statement("#program step(t).") == "#program step(t).");
    REQUIRE(parse_statement("#program step(k,t).") == "#program step(k,t).");

    // const
    REQUIRE(parse_statement("#const x=42.") == "#const x=42. [default]");
    REQUIRE(parse_statement("#const x=42. [default]") == "#const x=42. [default]");
    REQUIRE(parse_statement("#const x=42. [override]") == "#const x=42. [override]");
}

TEST_CASE("parse_program") {
    ParseHelper ph;
    std::istringstream in{"%p\na.%a\nb%b\n.%c\nc%d\n"};
    auto scanner = scan_stream(ph, ph, in);
    REQUIRE(to_str(scanner.scan()) == "%p");
    REQUIRE(to_str(scanner.scan()) == "a.");
    REQUIRE(to_str(scanner.scan()) == "%a");
    REQUIRE(to_str(scanner.scan()) == "%b");
    REQUIRE(to_str(scanner.scan()) == "b.");
    REQUIRE(to_str(scanner.scan()) == "%c");
    REQUIRE(to_str(scanner.scan()) == "%d");
    REQUIRE(to_str(scanner.scan()) == "<failed>");
}

} // namespace Gringo::Input::Test
