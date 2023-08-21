#include "input/test.hh"

namespace Gringo::Input::Test {

TEST_CASE("parse_term") {
    REQUIRE(to_str(parse_term("42")) == "42");
    REQUIRE(to_str(parse_term("f")) == "f");
    REQUIRE(to_str(parse_term("f(  )+5")) == "f+5");
    REQUIRE(to_str(parse_term("f(1)")) == "f(1)");
    REQUIRE(to_str(parse_term("f ( 1 , 2 ; 4 )")) == "f(1,2;4)");
    REQUIRE(to_str(parse_term("1 + f")) == "1+f");
    REQUIRE(to_str(parse_term("@f(1,2)")) == "@f(1,2)");
    REQUIRE(to_str(parse_term("|42|")) == "|42|");
    REQUIRE(to_str(parse_term("||42||")) == "||42||");
    REQUIRE(to_str(parse_term("f(_,X)")) == "f(_,X)");
    REQUIRE(to_str(parse_term("(a)")) == "a");
    REQUIRE(to_str(parse_term("(a;a,b;a,b,c)")) == "(a;a,b;a,b,c)");
    REQUIRE(to_str(parse_term("(a, ; a,b,;a,b,c, )")) == "(a,;a,b;a,b,c)");
    REQUIRE(to_str(parse_term("(a)")) == "a");
    REQUIRE(to_str(parse_term("(a,)")) == "(a,)");
    REQUIRE(to_str(parse_term("()")) == "()");
    REQUIRE(to_str(parse_term("(;)")) == "(;)");
    REQUIRE(to_str(parse_term("(a,;a)")) == "(a,;a)");
    REQUIRE(to_str(parse_term("f(;)")) == "f(;)");
    REQUIRE(to_str(parse_term("f(\"x\")")) == "f(\"x\")");
    REQUIRE(to_str(parse_term("a+b+c")) == "a+b+c");
    REQUIRE(to_str(parse_term("a*b+c")) == "a*b+c");
    REQUIRE(to_str(parse_term("a+b*c")) == "a+b*c");
    REQUIRE(to_str(parse_term("a**b**c")) == "a**b**c");
    REQUIRE(to_str(parse_term("a+(-b)")) == "a+(-b)");
    REQUIRE(to_str(parse_term("-a+b")) == "-a+b");
    REQUIRE(to_str(parse_term("-a**b")) == "-a**b");
    REQUIRE(to_str(parse_term("(-a)**b")) == "(-a)**b");
}

TEST_CASE("parse_literal") {
    REQUIRE(to_str(parse_literal("#true")) == "#true");
    REQUIRE(to_str(parse_literal("#false")) == "#false");
    REQUIRE(to_str(parse_literal("1 < 2")) == "1<2");
    REQUIRE(to_str(parse_literal("-f+1 < 2")) == "-f+1<2");
    REQUIRE(to_str(parse_literal("p(X)")) == "p(X)");
    REQUIRE(to_str(parse_literal("-p(X)")) == "-p(X)");
    REQUIRE(to_str(parse_literal("not p")) == "not p");
    REQUIRE(to_str(parse_literal("not not p")) == "not not p");
    REQUIRE(to_str(parse_literal("5")) == "<failed>");
}

TEST_CASE("parse_theory") {
    // empty guards/elements
    REQUIRE(to_str(parse_head_literal("&p(x,y;z){}<=a")) == "&p(x,y;z) { } <= a");
    REQUIRE(to_str(parse_head_literal("&p(x,y;z)<=a")) == "&p(x,y;z) { } <= a");
    REQUIRE(to_str(parse_head_literal("&p(x,y;z){}")) == "&p(x,y;z)");
    REQUIRE(to_str(parse_head_literal("&p(x,y;z)")) == "&p(x,y;z)");
    // empty tuples/conditions
    REQUIRE(to_str(parse_head_literal("&p{1:a}")) == "&p { 1: a }");
    REQUIRE(to_str(parse_head_literal("&p{1}")) == "&p { 1 }");
    REQUIRE(to_str(parse_head_literal("&p{1:}")) == "&p { 1 }");
    REQUIRE(to_str(parse_head_literal("&p{:a}")) == "&p { : a }");
    REQUIRE(to_str(parse_head_literal("&p{:}")) == "&p { :  }");
    // term types
    REQUIRE(to_str(parse_head_literal("&p{1}")) == "&p { 1 }");
    REQUIRE(to_str(parse_head_literal("&p{a}")) == "&p { a }");
    REQUIRE(to_str(parse_head_literal("&p{\"a\"}")) == "&p { \"a\" }");
    REQUIRE(to_str(parse_head_literal("&p{#sup}")) == "&p { #sup }");
    REQUIRE(to_str(parse_head_literal("&p{_X}")) == "&p { _X }");
    REQUIRE(to_str(parse_head_literal("&p{_}")) == "&p { _ }");
    // tuple
    REQUIRE(to_str(parse_head_literal("&p{()}")) == "&p { () }");
    REQUIRE(to_str(parse_head_literal("&p{(a)}")) == "&p { a }");
    REQUIRE(to_str(parse_head_literal("&p{(a,b)}")) == "&p { (a,b) }");
    REQUIRE(to_str(parse_head_literal("&p{(a,b,)}")) == "&p { (a,b) }");
    REQUIRE(to_str(parse_head_literal("&p{(a,)}")) == "&p { (a,) }");
    // set
    REQUIRE(to_str(parse_head_literal("&p{{}}")) == "&p { {} }");
    REQUIRE(to_str(parse_head_literal("&p{{a}}")) == "&p { {a} }");
    REQUIRE(to_str(parse_head_literal("&p{{a,b}}")) == "&p { {a,b} }");
    // list
    REQUIRE(to_str(parse_head_literal("&p{[]}")) == "&p { [] }");
    REQUIRE(to_str(parse_head_literal("&p{[a]}")) == "&p { [a] }");
    REQUIRE(to_str(parse_head_literal("&p{[a,b]}")) == "&p { [a,b] }");
    // unparsed
    REQUIRE(to_str(parse_head_literal("&p{+- *a -* + c}")) == "&p { (+- * a -* + c) }");
}

TEST_CASE("parse_head_literal") {
    // theory_atom | aggregate | set_aggregate | not disjunction
    REQUIRE(to_str(parse_head_literal("&x{}")) == "&x");
    REQUIRE(to_str(parse_head_literal("#count{}")) == "#count { }");
    REQUIRE(to_str(parse_head_literal("{}")) == "{ }");
    REQUIRE(to_str(parse_head_literal("not a")) == "not a");
    // atom_like relation aggregate
    REQUIRE(to_str(parse_head_literal("a<{}")) == "a < { }");
    REQUIRE(to_str(parse_head_literal("a<#count{}")) == "a < #count { }");
    // atom_like relation term ...
    REQUIRE(to_str(parse_head_literal("a<b<c")) == "a<b<c");
    REQUIRE(to_str(parse_head_literal("a<a:a")) == "a<a: a");
    REQUIRE(to_str(parse_head_literal("a<a:a;a")) == "a<a: a; a");
    REQUIRE(to_str(parse_head_literal("a<a,a")) == "a<a; a");
    // atom_like aggregate
    REQUIRE(to_str(parse_head_literal("a{}")) == "a <= { }");
    REQUIRE(to_str(parse_head_literal("a#count{}")) == "a <= #count { }");
    // term aggregate
    REQUIRE(to_str(parse_head_literal("a+1 { }")) == "a+1 <= { }");
    REQUIRE(to_str(parse_head_literal("a+1#count{}")) == "a+1 <= #count { }");
    // term relation aggregate
    REQUIRE(to_str(parse_head_literal("a+1<{}")) == "a+1 < { }");
    REQUIRE(to_str(parse_head_literal("a+1<#count{}")) == "a+1 < #count { }");
    // term relation term ...
    REQUIRE(to_str(parse_head_literal("a+1<b<c")) == "a+1<b<c");
    REQUIRE(to_str(parse_head_literal("a+1<a:a")) == "a+1<a: a");
    REQUIRE(to_str(parse_head_literal("a+1<a:a;a")) == "a+1<a: a; a");
    REQUIRE(to_str(parse_head_literal("a+1<a,a")) == "a+1<a; a");
    REQUIRE(to_str(parse_head_literal("a+1<>a,a")) == "<failed>");
    // atom ...
    REQUIRE(to_str(parse_head_literal("-a")) == "-a");
    REQUIRE(to_str(parse_head_literal("-a(X)")) == "-a(X)");
    REQUIRE(to_str(parse_head_literal("a:a")) == "a: a");
    REQUIRE(to_str(parse_head_literal("a:a;a")) == "a: a; a");
    REQUIRE(to_str(parse_head_literal("a,b")) == "a; b");
    REQUIRE(to_str(parse_head_literal("a;b")) == "a; b");
    REQUIRE(to_str(parse_head_literal("a|b")) == "a; b");
    REQUIRE(to_str(parse_head_literal("#true|#false|not #true")) == "#true; #false; not #true");
    // aggregates with guards
    REQUIRE(to_str(parse_head_literal("a<{}<b")) == "a < { } < b");
    REQUIRE(to_str(parse_head_literal("a{}b")) == "a <= { } <= b");
    // aggregate elements
    REQUIRE(to_str(parse_head_literal("#sum{: a; 1: a; 1,2: a: b, c}")) == "#sum { : a; 1: a; 1,2: a: b, c }");
    REQUIRE(to_str(parse_head_literal("{1<2;1<2:a;a:b;a:b,c}")) == "{ 1<2; 1<2: a; a: b; a: b, c }");
    // theory atoms
    REQUIRE(to_str(parse_head_literal("&p(X){43+-Y:a} <== 7")) == "&p(X) { (43 +- Y): a } <== 7");
    // conjunction literal
    REQUIRE(to_str(parse_head_literal("#or { : q(X); p(X): q(X); p(X), q(X): r(X) }")) ==
            "#or { : q(X); p(X): q(X); p(X), q(X): r(X) }");
    REQUIRE(to_str(parse_head_literal("#or { }")).empty());
    REQUIRE(to_str(parse_head_literal("#or { }")).empty());
    REQUIRE(to_str(parse_head_literal("#or { : }")) == "#or { : }");
}

TEST_CASE("parse_body_literal") {
    // negation
    REQUIRE(to_str(parse_body_literal("a")) == "a");
    REQUIRE(to_str(parse_body_literal("not a")) == "not a");
    REQUIRE(to_str(parse_body_literal("not not a")) == "not not a");
    REQUIRE(to_str(parse_body_literal("#true")) == "#true");
    REQUIRE(to_str(parse_body_literal("#false")) == "#false");
    // theory_atom | aggregate | set_aggregate
    REQUIRE(to_str(parse_body_literal("&x{}")) == "&x");
    REQUIRE(to_str(parse_body_literal("not &x{}")) == "not &x");
    REQUIRE(to_str(parse_body_literal("#count{}")) == "#count { }");
    REQUIRE(to_str(parse_body_literal("{}")) == "{ }");
    // atom_like relation aggregate
    REQUIRE(to_str(parse_body_literal("a<{}")) == "a < { }");
    REQUIRE(to_str(parse_body_literal("a<#count{}")) == "a < #count { }");
    // atom_like relation term ...
    REQUIRE(to_str(parse_body_literal("a<b<c")) == "a<b<c");
    REQUIRE(to_str(parse_body_literal("a<a:a")) == "a<a: a");
    // atom_like aggregate
    REQUIRE(to_str(parse_body_literal("a{}")) == "a <= { }");
    REQUIRE(to_str(parse_body_literal("a#count{}")) == "a <= #count { }");
    // term aggregate
    REQUIRE(to_str(parse_body_literal("a+1{}")) == "a+1 <= { }");
    REQUIRE(to_str(parse_body_literal("a+1#count{}")) == "a+1 <= #count { }");
    // term relation aggregate
    REQUIRE(to_str(parse_body_literal("a+1<{}")) == "a+1 < { }");
    REQUIRE(to_str(parse_body_literal("a+1<#count{}")) == "a+1 < #count { }");
    // term relation term ...
    REQUIRE(to_str(parse_body_literal("a+1<b<c")) == "a+1<b<c");
    REQUIRE(to_str(parse_body_literal("a+1<a:a")) == "a+1<a: a");
    // atom ...
    REQUIRE(to_str(parse_body_literal("-a")) == "-a");
    REQUIRE(to_str(parse_body_literal("-a(X)")) == "-a(X)");
    REQUIRE(to_str(parse_body_literal("a:b,c")) == "a: b, c");
    REQUIRE(to_str(parse_body_literal("#true:a")) == "#true: a");
    REQUIRE(to_str(parse_body_literal("not #true:a")) == "not #true: a");
    // aggregates with guards
    REQUIRE(to_str(parse_body_literal("a<{}<b")) == "a < { } < b");
    REQUIRE(to_str(parse_body_literal("a{}b")) == "a <= { } <= b");
    // aggregate elements
    REQUIRE(to_str(parse_body_literal("#sum{:a;1:a;1,2:a,b,c}")) == "#sum { : a; 1: a; 1,2: a, b, c }");
    REQUIRE(to_str(parse_body_literal("{1<2;1<2:a;a:b;a:b,c}")) == "{ 1<2; 1<2: a; a: b; a: b, c }");
    // conjunction literal
    REQUIRE(to_str(parse_body_literal("#and { : q(X); p(X): q(X); p(X), q(X): r(X) }")) ==
            "#and { : q(X); p(X): q(X); p(X), q(X): r(X) }");
    REQUIRE(to_str(parse_body_literal("#and { }")) == "#and { }");
    REQUIRE(to_str(parse_body_literal("#and { }")) == "#and { }");
    REQUIRE(to_str(parse_body_literal("#and { : }")) == "#and { : }");
}

TEST_CASE("parse_statement") {
    // rule
    REQUIRE(to_str(parse_statement(":-.")) == " :- .");
    REQUIRE(to_str(parse_statement("a.")) == "a.");
    REQUIRE(to_str(parse_statement("a:-.")) == "a.");
    REQUIRE(to_str(parse_statement("a:-b.")) == "a :- b.");
    REQUIRE(to_str(parse_statement("a:-b,c.")) == "a :- b; c.");
    REQUIRE(to_str(parse_statement("a:-b;c.")) == "a :- b; c.");
    REQUIRE(to_str(parse_statement("a:-a:b,c;d.")) == "a :- a: b, c; d.");

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
    REQUIRE(to_str(parse_statement("#theory x {}.")) == "#theory x { }.");
    REQUIRE(to_str(parse_statement(theory)) == theory);

    // optimize
    REQUIRE(to_str(parse_statement("#minimize {}.")) == "#minimize { }.");
    REQUIRE(to_str(parse_statement("#maximize {}.")) == "#maximize { }.");
    REQUIRE(to_str(parse_statement("#minimize {1}.")) == "#minimize { 1 }.");
    REQUIRE(to_str(parse_statement("#minimize {1@2}.")) == "#minimize { 1@2 }.");
    REQUIRE(to_str(parse_statement("#minimize {1@2,3,4}.")) == "#minimize { 1@2,3,4 }.");
    REQUIRE(to_str(parse_statement("#minimize {1@2,3,4:a}.")) == "#minimize { 1@2,3,4: a }.");
    REQUIRE(to_str(parse_statement("#minimize {1@2;3@4}.")) == "#minimize { 1@2; 3@4 }.");
    REQUIRE(to_str(parse_statement(":~ . [1]")) == " :~ . [1]");
    REQUIRE(to_str(parse_statement(":~ . [1@2]")) == " :~ . [1@2]");
    REQUIRE(to_str(parse_statement(":~ a. [1]")) == " :~ a. [1]");
    REQUIRE(to_str(parse_statement(":~ a; b. [1]")) == " :~ a; b. [1]");

    // show
    REQUIRE(to_str(parse_statement("#show a/2.")) == "#show a/2.");
    REQUIRE(to_str(parse_statement("#show -a/2.")) == "#show -a/2.");
    REQUIRE(to_str(parse_statement("#show (-a/2).")) == "#show (-a/2): .");
    REQUIRE(to_str(parse_statement("#show (-a()/2).")) == "#show (-a/2): .");
    REQUIRE(to_str(parse_statement("#show p(X).")) == "#show p(X): .");
    REQUIRE(to_str(parse_statement("#show p(X): .")) == "#show p(X): .");
    REQUIRE(to_str(parse_statement("#show p(X): a.")) == "#show p(X): a.");

    // project
    REQUIRE(to_str(parse_statement("#project a/2.")) == "#project a/2.");
    REQUIRE(to_str(parse_statement("#project -a/2.")) == "#project -a/2.");
    REQUIRE(to_str(parse_statement("#project p(X).")) == "#project p(X).");
    REQUIRE(to_str(parse_statement("#project p(X): .")) == "#project p(X).");
    REQUIRE(to_str(parse_statement("#project p(X): a.")) == "#project p(X): a.");

    // defined
    REQUIRE(to_str(parse_statement("#defined a/2.")) == "#defined a/2.");
    REQUIRE(to_str(parse_statement("#defined -a/2.")) == "#defined -a/2.");

    // edge
    REQUIRE(to_str(parse_statement("#edge (a,b).")) == "#edge (a,b).");
    REQUIRE(to_str(parse_statement("#edge (a,b):.")) == "#edge (a,b).");
    REQUIRE(to_str(parse_statement("#edge (a,b): c.")) == "#edge (a,b): c.");
    REQUIRE(to_str(parse_statement("#edge (a,b;c,d): e.")) == "#edge (a,b;c,d): e.");
    REQUIRE(to_str(parse_statement("#edge (a,b;c,d): e; f.")) == "#edge (a,b;c,d): e; f.");

    // heuristic
    REQUIRE(to_str(parse_statement("#heuristic a. [level@1,true]")) == "#heuristic a. [level@1,true]");
    REQUIRE(to_str(parse_statement("#heuristic a. [level,true]")) == "#heuristic a. [level,true]");
    REQUIRE(to_str(parse_statement("#heuristic a:. [level,true]")) == "#heuristic a. [level,true]");
    REQUIRE(to_str(parse_statement("#heuristic -a. [level,true]")) == "#heuristic -a. [level,true]");
    REQUIRE(to_str(parse_statement("#heuristic a:a; b. [level,true]")) == "#heuristic a: a; b. [level,true]");

    // script
    REQUIRE(to_str(parse_statement("#script   ( python  )     code   #end.")) == "#script (python)     code   #end.");
    REQUIRE(to_str(parse_statement("#script (python)\ncode\n#end.")) == "#script (python)\ncode\n#end.");
    REQUIRE(to_str(parse_statement("#script (python) всем привет #end.")) == "#script (python) всем привет #end.");

    // external
    REQUIRE(to_str(parse_statement("#external a(X): b(X).")) == "#external a(X): b(X).");
    REQUIRE(to_str(parse_statement("#external -a(X): b(X).")) == "#external -a(X): b(X).");
    REQUIRE(to_str(parse_statement("#external a(X): b(X). [X]")) == "#external a(X): b(X). [X]");

    // include
    REQUIRE(to_str(parse_statement("#include \"abc\".")) == "#include \"abc\".");
    REQUIRE(to_str(parse_statement("#include <abc>.")) == "#include <abc>.");

    // program
    REQUIRE(to_str(parse_statement("#program base.")) == "#program base.");
    REQUIRE(to_str(parse_statement("#program base().")) == "#program base.");
    REQUIRE(to_str(parse_statement("#program step(t).")) == "#program step(t).");
    REQUIRE(to_str(parse_statement("#program step(k,t).")) == "#program step(k,t).");

    // const
    REQUIRE(to_str(parse_statement("#const x=42.")) == "#const x=42. [default]");
    REQUIRE(to_str(parse_statement("#const x=42. [default]")) == "#const x=42. [default]");
    REQUIRE(to_str(parse_statement("#const x=42. [override]")) == "#const x=42. [override]");
}

TEST_CASE("parse_program") {
    std::istringstream in{"%p\na.%a\nb%b\n.%c\nc%d\n"};
    auto store = make_symbol_store(true, true);
    auto scanner = scan_stream(*store, in);
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
