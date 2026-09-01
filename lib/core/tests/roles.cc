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

TEST_CASE("literal_double_negation") {
    auto l = Literal::from_rep(3);
    REQUIRE((~(~l)) == l);
    auto n = Literal::from_rep(-3);
    REQUIRE((~(~n)) == n);
}

TEST_CASE("atom_literal_roundtrip") {
    auto a = Atom::from_rep(7);
    REQUIRE(a.to_lit(false).sign() == false);
    REQUIRE(a.to_lit(true).sign() == true);
    REQUIRE(a.to_lit(false).atom() == a);
    REQUIRE(a.to_lit(true).atom() == a);
    // Check the rep values directly, not just sign()/atom(), so that a
    // regression that gets the sign of the underlying rep backwards (while
    // still reporting correct sign()/atom() views) would be caught.
    REQUIRE(Literal::to_rep(a.to_lit(false)) == 7);
    REQUIRE(Literal::to_rep(a.to_lit(true)) == -7);
}

TEST_CASE("atom_ordering") {
    auto a3 = Atom::from_rep(3);
    auto a5 = Atom::from_rep(5);
    REQUIRE(a3 < a5);
    REQUIRE(a5 > a3);
    REQUIRE(a3 <= a3);
    REQUIRE(a3 <= a5);
    REQUIRE(a5 >= a5);
    REQUIRE_FALSE(a5 < a3);
}

TEST_CASE("literal_ordering") {
    auto l3 = Literal::from_rep(3);
    auto l5 = Literal::from_rep(5);
    REQUIRE(l3 < l5);
    REQUIRE(l5 > l3);
    REQUIRE(l3 <= l3);
    REQUIRE(l3 <= l5);
    REQUIRE(l5 >= l5);
    REQUIRE_FALSE(l5 < l3);
    // negative literals order below positive ones, consistent with the
    // underlying rep (this is the ordering that would be used e.g. as a map
    // key), not just by atom.
    auto neg = Literal::from_rep(-5);
    REQUIRE(neg < l3);
}

TEST_CASE("atom_hash_consistency") {
    auto a1 = Atom::from_rep(11);
    auto a2 = Atom::from_rep(11);
    REQUIRE(a1 == a2);
    REQUIRE(a1.hash() == a2.hash());
}

TEST_CASE("literal_hash_consistency") {
    auto l1 = Literal::from_rep(-11);
    auto l2 = Literal::from_rep(-11);
    REQUIRE(l1 == l2);
    REQUIRE(l1.hash() == l2.hash());
}

TEST_CASE("atom_literal_valid_domain") {
    // Atoms/literals are only meaningful for positive values in
    // [1, prg_lit_max]; INT32_MIN cannot be exercised (it is UB per the
    // type's documented contract), but round-tripping across a
    // representative sample of the valid positive domain should hold,
    // including the boundary values 1 and prg_lit_max.
    for (prg_lit_t v : {prg_lit_t{1}, prg_lit_t{2}, prg_lit_t{42}, prg_lit_max}) {
        auto a = Atom::from_rep(static_cast<prg_atom_t>(v));
        REQUIRE(Atom::to_rep(a) == static_cast<prg_atom_t>(v));

        auto pos = Literal::from_rep(v);
        REQUIRE(pos.sign() == false);
        REQUIRE(pos.atom() == a);
        REQUIRE(Literal::to_rep(pos) == v);

        auto neg = Literal::from_rep(-v);
        REQUIRE(neg.sign() == true);
        REQUIRE(neg.atom() == a);
        REQUIRE(Literal::to_rep(neg) == -v);
    }
}

} // namespace CppClingo::Test
