#include <catch2/catch_test_macros.hpp>

#include "parser.hh"

namespace test {

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
    REQUIRE(to_str(parse_statement("#show (-a/2).")) == "#show ((-a)/2): .");
    REQUIRE(to_str(parse_statement("#show (-a()/2).")) == "#show ((-a)/2): .");
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
    REQUIRE(to_str(parse_statement("#external -a(X): b(X).")) == "#external (-a(X)): b(X).");
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

TEST_CASE("unpool_statement") {
    // rule
    REQUIRE(unpool_str(parse_statement("a(1;2) :- b(3;4), b(5,6)."), " ") == "[a(1) :- b(3); b(5,6)."
                                                                             " a(1) :- b(4); b(5,6)."
                                                                             " a(2) :- b(3); b(5,6)."
                                                                             " a(2) :- b(4); b(5,6).]");
    REQUIRE(unpool_str(parse_statement("a(1;2) :- b(3;4), b(5;6)."), " ") == "[a(1) :- b(3); b(5)."
                                                                             " a(1) :- b(4); b(5)."
                                                                             " a(1) :- b(3); b(6)."
                                                                             " a(1) :- b(4); b(6)."
                                                                             " a(2) :- b(3); b(5)."
                                                                             " a(2) :- b(4); b(5)."
                                                                             " a(2) :- b(3); b(6)."
                                                                             " a(2) :- b(4); b(6).]");
    // minimize
    REQUIRE(unpool_str(parse_statement("#minimize { (1;2)@(2;3),(4;5): p(1;2) }.")) ==
            "[#minimize { "
            "1@2,4: p(1); 1@2,4: p(2); 1@2,5: p(1); 1@2,5: p(2); "
            "1@3,4: p(1); 1@3,4: p(2); 1@3,5: p(1); 1@3,5: p(2); "
            "2@2,4: p(1); 2@2,4: p(2); 2@2,5: p(1); 2@2,5: p(2); "
            "2@3,4: p(1); 2@3,4: p(2); 2@3,5: p(1); 2@3,5: p(2) "
            "}.]");
    // weak constraint
    REQUIRE(unpool_str(parse_statement(":~ p(1;2). [(1;2)@(2;3),(4;5)]"), " ") ==
            "[ :~ p(1). [1@2,4]  :~ p(2). [1@2,4]  :~ p(1). [1@2,5]  :~ p(2). [1@2,5]"
            "  :~ p(1). [1@3,4]  :~ p(2). [1@3,4]  :~ p(1). [1@3,5]  :~ p(2). [1@3,5]"
            "  :~ p(1). [2@2,4]  :~ p(2). [2@2,4]  :~ p(1). [2@2,5]  :~ p(2). [2@2,5]"
            "  :~ p(1). [2@3,4]  :~ p(2). [2@3,4]  :~ p(1). [2@3,5]  :~ p(2). [2@3,5]]");
    // show
    REQUIRE(unpool_str(parse_statement("#show (1;2): p(1;2)."), " ") == "[#show 1: p(1). #show 1: p(2)."
                                                                        " #show 2: p(1). #show 2: p(2).]");
    // project
    REQUIRE(unpool_str(parse_statement("#project q(1;2): p(1;2)."), " ") ==
            "[#project q(1): p(1). #project q(1): p(2)."
            " #project q(2): p(1). #project q(2): p(2).]");
    // edge
    REQUIRE(unpool_str(parse_statement("#edge ((a;b),(c;d)) : p(1;2)."), " ") ==
            "[#edge (a,c): p(1). #edge (a,c): p(2)."
            " #edge (a,d): p(1). #edge (a,d): p(2)."
            " #edge (b,c): p(1). #edge (b,c): p(2)."
            " #edge (b,d): p(1). #edge (b,d): p(2).]");
    REQUIRE(unpool_str(parse_statement("#heuristic p(1;2). [(1;2)@(3;4),(5;6)]"), " ") ==
            "[#heuristic p(1). [1@3,5] #heuristic p(1). [1@3,6] #heuristic p(1). [1@4,5] #heuristic p(1). [1@4,6]"
            " #heuristic p(1). [2@3,5] #heuristic p(1). [2@3,6] #heuristic p(1). [2@4,5] #heuristic p(1). [2@4,6]"
            " #heuristic p(2). [1@3,5] #heuristic p(2). [1@3,6] #heuristic p(2). [1@4,5] #heuristic p(2). [1@4,6]"
            " #heuristic p(2). [2@3,5] #heuristic p(2). [2@3,6] #heuristic p(2). [2@4,5] #heuristic p(2). [2@4,6]]");
    REQUIRE(unpool_str(parse_statement("#const x=(1)."), "") == "[#const x=1. [default]]");

    // local <-> global
    REQUIRE(unpool_str(parse_statement(":- p(X): q(X;Y)."), " ") == "[ :- p(X): q(X); p(X): q(Y).]");
    REQUIRE_THROWS(unpool_str(parse_statement(":- p(X;Y): q(X)."), " "));
    REQUIRE_THROWS(unpool_str(parse_statement(":- p(X;Y): q(X;Y)."), " "));
    REQUIRE_THROWS(unpool_str(parse_statement(":- p(X): q(Y); r(X;Y)."), " "));
}

TEST_CASE("project_statement") { REQUIRE(project_str(parse_statement(":- p(X,Y), q(X).")) == " :- p(X,*); q(X)."); }

} // namespace test
