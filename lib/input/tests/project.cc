#include "test.hh"

#include <clingo/input/rewrite/project.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Input::Test {

namespace {

template <class T> auto project_str(std::optional<T> value) -> std::string {
    if (value) {
        return to_str(project(RewriteOptions{ProjectionMode::pure, true}, value.value()).value_or(value.value()));
    }
    return "<failed>";
}

auto project_statement(std::string const &str) -> std::string {
    ParseHelper ph;
    return project_str(ph.statement(str));
}

} // namespace

TEST_CASE("project_statement_body") {
    // simple literals
    REQUIRE(project_statement(":- p(X,Y), q(X).") == " :- p(X,*); q(X).");
    REQUIRE(project_statement(":- p(X,_), q(X).") == " :- p(X,*); q(X).");
    REQUIRE(project_statement(":- p(X,*), q(X).") == " :- p(X,*); q(X).");
    REQUIRE(project_statement("p(X) :- p(X,Y), q(X).") == "p(X) :- p(X,*); q(X).");
    REQUIRE(project_statement("p(X) :- p(X,Y), X > 10.") == "p(X) :- p(X,Y); X>10.");
    // conjunctions
    REQUIRE(project_statement(":- p(X,Y): q(X).") == " :- p(X,*): q(X).");
    REQUIRE(project_statement(":- p(X,_): q(X).") == " :- p(X,*): q(X).");
    REQUIRE(project_statement(":- p(X,*): q(X).") == " :- p(X,*): q(X).");
    REQUIRE(project_statement(":- p(X): q(X,Y).") == " :- p(X): q(X,*).");
    REQUIRE(project_statement(":- p(X): q(X,_).") == " :- p(X): q(X,*).");
    REQUIRE(project_statement("h:- p(X): q(X,Y).") == "h :- p(X): q(X,Y).");
    REQUIRE(project_statement("h:- p(X): q(X,_).") == "h :- p(X): q(X,_).");
    REQUIRE(project_statement("h:- p(X): q(X,*).") == "h :- p(X): q(X,*).");
    REQUIRE(project_statement("h(X) :- #false: q(X,Y).") == "h(X) :- #false: q(X,*).");
    REQUIRE(project_statement("h(X) :- #false: q(X,_).") == "h(X) :- #false: q(X,*).");
    // set aggregates
    REQUIRE(project_statement(":- { p(X) : q(X,Y) } != 5.") == " :- { p(X): q(X,*) } != 5.");
    REQUIRE(project_statement("h :- { p(X) : q(X,Y) } != 5.") == "h :- { p(X): q(X,Y) } != 5.");
    REQUIRE(project_statement("h :- { p(X) : q(X,Y) } > 5.") == "h :- { p(X): q(X,*) } > 5.");
    REQUIRE(project_statement("h :- not { p(X) : q(X,Y) } != 5.") == "h :- not { p(X): q(X,*) } != 5.");
    // aggregates
    REQUIRE(project_statement(":- #count { X: p(X), q(X,Y) } != 5.") == " :- #count { X: p(X), q(X,*) } != 5.");
    REQUIRE(project_statement("h :- #count { X: p(X), q(X,Y) } != 5.") == "h :- #count { X: p(X), q(X,Y) } != 5.");
    REQUIRE(project_statement("h :- #count { X: p(X), q(X,Y) } > 5.") == "h :- #count { X: p(X), q(X,*) } > 5.");
    REQUIRE(project_statement("h :- not #count { X: p(X), q(X,Y) } != 5.") ==
            "h :- not #count { X: p(X), q(X,*) } != 5.");
    REQUIRE(project_statement("h :- 1 < #count { X: p(X), q(X,Y) } < 2.") ==
            "h :- 1 < #count { X: p(X), q(X,*) } < 2.");
    REQUIRE(project_statement("h :- 1 < #sum+ { X: p(X), q(X,Y) } < 2.") == "h :- 1 < #sum+ { X: p(X), q(X,*) } < 2.");
    REQUIRE(project_statement("h :- 1 < #sum { X: p(X), q(X,Y) } < 2.") == "h :- 1 < #sum { X: p(X), q(X,Y) } < 2.");
    // theory
    REQUIRE(project_statement(":- &p { X: p(X), q(X,Y) } != 5.") == " :- &p { X: p(X), q(X,Y) } != 5.");
    REQUIRE(project_statement(":- not &p { X: p(X), q(X,Y) } != 5.") == " :- not &p { X: p(X), q(X,Y) } != 5.");
}

TEST_CASE("project_statement_head") {
    // disjunctions
    REQUIRE(project_statement("p(X,Y): q(X).") == "p(X,Y): q(X).");
    REQUIRE(project_statement("p(X,_): q(X).") == "p(X,_): q(X).");
    REQUIRE(project_statement(" :- p(X,*): q(X).") == " :- p(X,*): q(X).");
    REQUIRE(project_statement("p(X): q(X,Y).") == "p(X): q(X,*).");
    REQUIRE(project_statement("p(X): q(X,_).") == "p(X): q(X,*).");
    REQUIRE(project_statement("p(X): q(X,*).") == "p(X): q(X,*).");
    REQUIRE(project_statement("#false: q(X,Y):- p(X).") == "#false: q(X,*) :- p(X).");
    REQUIRE(project_statement("#false: q(X,_):- p(X).") == "#false: q(X,*) :- p(X).");
    // set aggregates
    REQUIRE(project_statement("{ p(X) : q(X,Y) } != 5.") == "{ p(X): q(X,*) } != 5.");
    REQUIRE(project_statement("{ p(X) : q(X,_) } != 5.") == "{ p(X): q(X,*) } != 5.");
    REQUIRE(project_statement("{ p(X) : q(X,Y) } > 5.") == "{ p(X): q(X,*) } > 5.");
    REQUIRE(project_statement("{ p(X,Y) : q(X) } > 5.") == "{ p(X,Y): q(X) } > 5.");
    REQUIRE(project_statement("{ not p(X,Y) : q(X) } > 5.") == "{ not p(X,Y): q(X) } > 5.");
    REQUIRE(project_statement("{ not p(X,_) : q(X) } > 5.") == "{ not p(X,*): q(X) } > 5.");
    // aggregates
    REQUIRE(project_statement("p(X) = #min { }.") == "p(X) = #min { }.");
    REQUIRE(project_statement("#count { X: p(X) : q(X,Y) } != 5.") == "#count { X: p(X): q(X,*) } != 5.");
    REQUIRE(project_statement("#count { X: p(X) : q(X,_) } != 5.") == "#count { X: p(X): q(X,*) } != 5.");
    REQUIRE(project_statement("#count { X: p(X) : q(X,Y) } > 5.") == "#count { X: p(X): q(X,*) } > 5.");
    REQUIRE(project_statement("#count { X: p(X,Y) : q(X) } > 5.") == "#count { X: p(X,Y): q(X) } > 5.");
    REQUIRE(project_statement("#count { X: p(X,_) : q(X) } > 5.") == "#count { X: p(X,_): q(X) } > 5.");
    REQUIRE(project_statement("#count { X: not p(X,Y) : q(X) } > 5.") == "#count { X: not p(X,Y): q(X) } > 5.");
    REQUIRE(project_statement("#count { X: not p(X,_) : q(X) } > 5.") == "#count { X: not p(X,*): q(X) } > 5.");
    // theory
    REQUIRE(project_statement("&p { X: p(X), q(X,Y) } != 5.") == "&p { X: p(X), q(X,Y) } != 5.");
    REQUIRE(project_statement("&p { X: p(X), not q(X,_) } != 5.") == "&p { X: p(X), not q(X,*) } != 5.");
}

TEST_CASE("project_statement") {
    REQUIRE(project_statement("#theory t {}.") == "#theory t { }.");
    REQUIRE(project_statement("#minimize { X,Y: p(X), p(Z) }.") == "#minimize { X,Y: p(X), p(*) }.");
    REQUIRE(project_statement(":~ p(X), p(Z). [X,Y]") == " :~ p(X); p(*). [X,Y]");
    REQUIRE(project_statement("#show p(X,Y): q(X,Z).") == "#show p(X,Y): q(X,*).");
    REQUIRE(project_statement("#show p/2. [true]") == "#show p/2. [true]");
    REQUIRE(project_statement("#show p/2. [false]") == "#show p/2. [false]");
    REQUIRE(project_statement("#project p(X,Y): q(X,Z).") == "#project p(X,Y): q(X,*).");
    REQUIRE(project_statement("#project p/2.") == "#project p/2.");
    REQUIRE(project_statement("#defined p/2.") == "#defined p/2.");
    REQUIRE(project_statement("#external p(X,Y): q(X,Z). [U]") == "#external p(X,Y): q(X,*). [U]");
    REQUIRE(project_statement("#edge (X,Y): q(X,Z).") == "#edge (X,Y): q(X,*).");
    REQUIRE(project_statement("#heuristic p(X,Y): q(X,Z). [X,U]") == "#heuristic p(X,Y): q(X,*). [X,U]");
    REQUIRE(project_statement("#script (python) #end.") == "#script (python) #end.");
    REQUIRE(project_statement("#program p(k,t).") == "#program p(k,t).");
    REQUIRE(project_statement("#const x=42.") == "#const x=42. [default]");
}

} // namespace CppClingo::Input::Test
