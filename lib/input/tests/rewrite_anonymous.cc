#include "test.hh"

#include <clingo/input/rewrite/rewrite_anonymous.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Input::Test {

namespace {

template <class T> auto rewrite_anonymous_str(std::optional<T> value) -> std::string {
    if (value) {
        return to_str(rewrite_anonymous(*make_symbol_store(true, true), value.value()).value_or(value.value()));
    }
    return "<failed>";
}

auto rewrite_statement(std::string const &str) -> std::string {
    ParseHelper ph;
    return rewrite_anonymous_str(ph.statement(str));
}

} // namespace

TEST_CASE("rewrite_anonymous_head") {
    // disjunctions
    REQUIRE(rewrite_statement("p(X,_): q(X,_,*).") == "p(X,__A_0): q(X,__A_1,*).");
    REQUIRE(rewrite_statement("not p(X,_,*): not r(X,_,*).") == "not p(X,__A_0,*): not r(X,__A_1,*).");
    REQUIRE(rewrite_statement("f(X,_)<g(X,_).") == "f(X,__A_0)<g(X,__A_1).");
    REQUIRE(rewrite_statement("#false:#true.") == "#false: #true.");
    // set aggregates
    REQUIRE(rewrite_statement("{ p(X,_) : q(X,_) } != f(X,_).") == "{ p(X,__A_0): q(X,__A_1) } != f(X,__A_2).");
    REQUIRE(rewrite_statement("{ not p(X,_,*) : not q(X,_) } != f(X,_).") ==
            "{ not p(X,__A_0,*): not q(X,__A_1) } != f(X,__A_2).");
    // aggregates
    REQUIRE(rewrite_statement("#count { f(X,_): p(X,_) : q(X,_) } != f(X,_).") ==
            "#count { f(X,__A_0): p(X,__A_1): q(X,__A_2) } != f(X,__A_3).");
    REQUIRE(rewrite_statement("#count { f(X,_): not p(X,_) : not q(X,_) } != f(X,_).") ==
            "#count { f(X,__A_0): not p(X,__A_1): not q(X,__A_2) } != f(X,__A_3).");
    // theory
    REQUIRE(rewrite_statement("&p(X,_) { f(X,_): p(X,_,*), not q(X,_) } != f(X,_).") ==
            "&p(X,__A_0) { f(X,__A_1): p(X,__A_2,*), not q(X,__A_3) } != "
            "f(X,__A_4).");
}

TEST_CASE("rewrite_anonymous_body") {
    // conjunctions
    REQUIRE(rewrite_statement(":- p(X,_,*): q(X,_,*).") == " :- p(X,__A_0,*): q(X,__A_1,*).");
    REQUIRE(rewrite_statement(":- not p(X,_,*): not r(X,_,*).") == " :- not p(X,__A_0,*): not r(X,__A_1,*).");
    REQUIRE(rewrite_statement(":- f(X,_)<g(X,_).") == " :- f(X,__A_0)<g(X,__A_1).");
    REQUIRE(rewrite_statement(":- #false:#true.") == " :- #false: #true.");
    // set aggregates
    REQUIRE(rewrite_statement(":- { p(X,_,*) : q(X,_) } != f(X,_).") ==
            " :- { p(X,__A_0,*): q(X,__A_1) } != f(X,__A_2).");
    REQUIRE(rewrite_statement(":- { not p(X,_,*) : not q(X,_) } != f(X,_).") ==
            " :- { not p(X,__A_0,*): not q(X,__A_1) } != f(X,__A_2).");
    REQUIRE(rewrite_statement(":- not { not p(X,_,*) : not q(X,_) } != f(X,_).") ==
            " :- not { not p(X,__A_0,*): not q(X,__A_1) } != f(X,__A_2).");
    // aggregates
    REQUIRE(rewrite_statement(":- #count { f(X,_) : q(X,_) } != f(X,_).") ==
            " :- #count { f(X,__A_0): q(X,__A_1) } != f(X,__A_2).");
    REQUIRE(rewrite_statement(":- #count { f(X,_) : not q(X,_) } != f(X,_).") ==
            " :- #count { f(X,__A_0): not q(X,__A_1) } != f(X,__A_2).");
    REQUIRE(rewrite_statement(":- not #count { f(X,_) : not q(X,_) } != f(X,_).") ==
            " :- not #count { f(X,__A_0): not q(X,__A_1) } != f(X,__A_2).");
    // theory
    REQUIRE(rewrite_statement(" :- &p(X,_) { f(X,_): p(X,_,*), not q(X,_) } != f(X,_).") ==
            " :- &p(X,__A_0) { f(X,__A_1): p(X,__A_2,*), not q(X,__A_3) } != "
            "f(X,__A_4).");
    REQUIRE(rewrite_statement(" :- not &p(X,_) { f(X,_): p(X,_,*), not q(X,_) } != f(X,_).") ==
            " :- not &p(X,__A_0) { f(X,__A_1): p(X,__A_2,*), not q(X,__A_3) } != "
            "f(X,__A_4).");
}

TEST_CASE("rewrite_anonymous_statement") {
    REQUIRE(rewrite_statement("#theory t {}.") == "#theory t { }.");
    REQUIRE(rewrite_statement("#minimize { Y@Z: not p(X), not p(_) }.") ==
            "#minimize { Y@Z: not p(X), not p(__A_0) }.");
    REQUIRE(rewrite_statement(":~ not p(X), not p(_). [Y]") == " :~ not p(X); not p(__A_0). [Y]");
    REQUIRE(rewrite_statement("#show p(X,Y): not p(X); not q(_).") == "#show p(X,Y): not p(X); not q(__A_0).");
    REQUIRE(rewrite_statement("#show p/2.") == "#show p/2. [true]");
    REQUIRE(rewrite_statement("#project p(X): not q(Y), not q(_).") == "#project p(X): not q(Y); not q(__A_0).");
    REQUIRE(rewrite_statement("#project p/2.") == "#project p/2.");
    REQUIRE(rewrite_statement("#defined p/2.") == "#defined p/2.");
    REQUIRE(rewrite_statement("#external p(X): not q(Y), not q(_). [Z]") ==
            "#external p(X): not q(Y); not q(__A_0). [Z]");
    REQUIRE(rewrite_statement("#edge (X,X): not q(Y), not q(_).") == "#edge (X,X): not q(Y); not q(__A_0).");
    REQUIRE(rewrite_statement("#heuristic p(X): not q(Y), not q(_). [U,V]") ==
            "#heuristic p(X): not q(Y); not q(__A_0). [U,V]");
    REQUIRE(rewrite_statement("#script (python) #end.") == "#script (python) #end.");
    REQUIRE(rewrite_statement("#program p(k,t).") == "#program p(k,t).");
    REQUIRE(rewrite_statement("#const x=42.") == "#const x=42. [default]");
}

} // namespace CppClingo::Input::Test
