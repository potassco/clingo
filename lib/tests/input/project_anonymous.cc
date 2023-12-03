#include <input/algo/project_anonymous.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

namespace {

template <class T> auto project_anonymous_str(std::optional<T> value) -> std::string {
    if (value) {
        return to_str(project_anonymous(value.value()).value_or(value.value()));
    }
    return "<failed>";
}

} // namespace

TEST_CASE("project_anonymous_head") {
    // disjunctions
    // Note: in the head it should be okay to automatically project in conditions
    REQUIRE(project_anonymous_str(parse_statement("p(X,_): q(X,_,*).")) == "p(X,_): q(X,_,*).");
    REQUIRE(project_anonymous_str(parse_statement("not p(X,_,*): not r(X,_,*).")) == "not p(X,*,*): not r(X,*,*).");
    REQUIRE(project_anonymous_str(parse_statement("f(X,_)<g(X,_).")) == "f(X,_)<g(X,_).");
    REQUIRE(project_anonymous_str(parse_statement("#false:#true.")) == "#false: #true.");
    // set aggregates
    REQUIRE(project_anonymous_str(parse_statement("{ p(X,_) : q(X,_) } != f(X,_).")) ==
            "{ p(X,_): q(X,_) } != f(X,_).");
    REQUIRE(project_anonymous_str(parse_statement("{ not p(X,_,*) : not q(X,_,*) } != f(X,_).")) ==
            "{ not p(X,*,*): not q(X,*,*) } != f(X,_).");
    // aggregates
    REQUIRE(project_anonymous_str(parse_statement("#count { f(X,_): p(X,_) : q(X,_,*) } != f(X,_).")) ==
            "#count { f(X,_): p(X,_): q(X,_,*) } != f(X,_).");
    REQUIRE(project_anonymous_str(parse_statement("#count { f(X,_): not p(X,_,*) : not q(X,_,*) } != f(X,_).")) ==
            "#count { f(X,_): not p(X,*,*): not q(X,*,*) } != f(X,_).");
    // theory
    REQUIRE(project_anonymous_str(parse_statement("&p(X,_) { f(X,_): p(X,_,*), not q(X,_,*) } != f(X,_).")) ==
            "&p(X,_) { f(X,_): p(X,_,*), not q(X,*,*) } != f(X,_).");
}

TEST_CASE("project_anonymous_body") {
    // conjunctions
    REQUIRE(project_anonymous_str(parse_statement(":- p(X,_,*): q(X,_,*).")) == " :- p(X,_,*): q(X,_,*).");
    REQUIRE(project_anonymous_str(parse_statement(":- not p(X,_,*): not r(X,_,*).")) ==
            " :- not p(X,*,*): not r(X,*,*).");
    REQUIRE(project_anonymous_str(parse_statement(":- f(X,_)<g(X,_).")) == " :- f(X,_)<g(X,_).");
    REQUIRE(project_anonymous_str(parse_statement(":- #false:#true.")) == " :- #false: #true.");
    // set aggregates
    REQUIRE(project_anonymous_str(parse_statement(":- { p(X,_,*) : q(X,_,*) } != f(X,_).")) ==
            " :- { p(X,_,*): q(X,_,*) } != f(X,_).");
    REQUIRE(project_anonymous_str(parse_statement(":- { not p(X,_,*) : not q(X,_,*) } != f(X,_).")) ==
            " :- { not p(X,*,*): not q(X,*,*) } != f(X,_).");
    REQUIRE(project_anonymous_str(parse_statement(":- not { not p(X,_,*) : not q(X,_,*) } != f(X,_).")) ==
            " :- not { not p(X,*,*): not q(X,*,*) } != f(X,_).");
    // aggregates
    REQUIRE(project_anonymous_str(parse_statement(":- #count { f(X,_) : q(X,_,*) } != f(X,_).")) ==
            " :- #count { f(X,_): q(X,_,*) } != f(X,_).");
    REQUIRE(project_anonymous_str(parse_statement(":- #count { f(X,_) : not q(X,_,*) } != f(X,_).")) ==
            " :- #count { f(X,_): not q(X,*,*) } != f(X,_).");
    REQUIRE(project_anonymous_str(parse_statement(":- not #count { f(X,_) : not q(X,_,*) } != f(X,_).")) ==
            " :- not #count { f(X,_): not q(X,*,*) } != f(X,_).");
    // theory
    REQUIRE(project_anonymous_str(parse_statement(" :- &p(X,_) { f(X,_): p(X,_,*), not q(X,_,*) } != f(X,_).")) ==
            " :- &p(X,_) { f(X,_): p(X,_,*), not q(X,*,*) } != f(X,_).");
    REQUIRE(project_anonymous_str(parse_statement(" :- not &p(X,_) { f(X,_): p(X,_,*), not q(X,_,*) } != f(X,_).")) ==
            " :- not &p(X,_) { f(X,_): p(X,_,*), not q(X,*,*) } != f(X,_).");
}

TEST_CASE("project_anonymous_statement") {
    REQUIRE(project_anonymous_str(parse_statement("#theory t {}.")) == "#theory t { }.");
    REQUIRE(project_anonymous_str(parse_statement("#minimize { Y@Z: not p(X), not p(_) }.")) ==
            "#minimize { Y@Z: not p(X), not p(*) }.");
    REQUIRE(project_anonymous_str(parse_statement(":~ not p(X); not p(_). [Y]")) == " :~ not p(X); not p(*). [Y]");
    REQUIRE(project_anonymous_str(parse_statement("#show p(X,Y): not p(X), not q(_).")) ==
            "#show p(X,Y): not p(X); not q(*).");
    REQUIRE(project_anonymous_str(parse_statement("#show p/2.")) == "#show p/2.");
    REQUIRE(project_anonymous_str(parse_statement("#project p(X): not q(Y), not q(_).")) ==
            "#project p(X): not q(Y); not q(*).");
    REQUIRE(project_anonymous_str(parse_statement("#project p/2.")) == "#project p/2.");
    REQUIRE(project_anonymous_str(parse_statement("#defined p/2.")) == "#defined p/2.");
    REQUIRE(project_anonymous_str(parse_statement("#external p(X): not q(Y), not q(_). [Z]")) ==
            "#external p(X): not q(Y); not q(*). [Z]");
    REQUIRE(project_anonymous_str(parse_statement("#edge (X,X): not q(Y), not q(_).")) ==
            "#edge (X,X): not q(Y); not q(*).");
    REQUIRE(project_anonymous_str(parse_statement("#heuristic p(X): not q(Y), not q(_). [U,V]")) ==
            "#heuristic p(X): not q(Y); not q(*). [U,V]");
    REQUIRE(project_anonymous_str(parse_statement("#script (python) #end.")) == "#script (python) #end.");
    REQUIRE(project_anonymous_str(parse_statement("#program p(k,t).")) == "#program p(k,t).");
    REQUIRE(project_anonymous_str(parse_statement("#const x=42.")) == "#const x=42. [default]");
}

} // namespace Gringo::Input::Test
