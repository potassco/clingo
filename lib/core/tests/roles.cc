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

} // namespace CppClingo::Test
