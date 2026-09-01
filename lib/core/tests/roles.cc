#include <clingo/core/backend.hh>

#include <catch2/catch_test_macros.hpp>

namespace CppClingo::Test {

TEST_CASE("atom_basic") {
    auto a = Atom::from_rep(5);
    REQUIRE(a.index() == 5U);
    REQUIRE(Atom::to_rep(a) == 5U);
    REQUIRE(a == Atom::from_rep(5));
    REQUIRE(a != Atom::from_rep(6));
    STATIC_REQUIRE(std::is_trivially_copyable_v<Atom>);
    STATIC_REQUIRE(sizeof(Atom) == sizeof(prg_atom_t));
}

TEST_CASE("literal_negation") {
    auto l = Literal::from_rep(3);
    REQUIRE((~l) == Literal::from_rep(-3));
    REQUIRE(l.sign() == false);
    REQUIRE((~l).sign() == true);
    REQUIRE(l.atom() == Atom::from_rep(3));
    REQUIRE((~l).atom() == Atom::from_rep(3));
    STATIC_REQUIRE(sizeof(Literal) == sizeof(prg_lit_t));
}
TEST_CASE("atom_literal_roundtrip") {
    auto a = Atom::from_rep(7);
    REQUIRE(a.to_lit(false).sign() == false);
    REQUIRE(a.to_lit(true).sign() == true);
    REQUIRE(a.to_lit(false).atom() == a);
    REQUIRE(a.to_lit(true).atom() == a);
}

} // namespace CppClingo::Test
