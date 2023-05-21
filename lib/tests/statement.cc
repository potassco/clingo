#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

TEST_CASE("statement") {
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
    REQUIRE(parse_statement("#show (-a/2).") == "#show -a/2.");
    REQUIRE(parse_statement("#show (-a()/2).") == "#show -a/2.");
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
    REQUIRE(parse_statement("#external -a(X): b(X).") == "#external (-a(X)): b(X).");
    REQUIRE(parse_statement("#external a(X): b(X). [X]") == "#external a(X): b(X). [X]");

    // include
    REQUIRE(parse_statement("#include \"abc\".") == "#include \"abc\".");
    REQUIRE(parse_statement("#include <abc>.") == "#include <abc>.");
}

} // namespace test
