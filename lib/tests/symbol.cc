#include <catch2/catch_test_macros.hpp>

#include <symbol_old.hh>

namespace Gringo::Test {

TEST_CASE("symbols") {
    // TODO: split into groups for different types of symbols
    USymbolStore store = make_symbol_store(false);

    auto n1 = store->number(1);
    auto n2 = store->number(2);
    auto sx1 = store->string("x");
    auto sx2 = store->string("x");
    auto sy = store->string("y");
    auto sym_sx1 = store->string(sx1);
    auto sym_sx2 = store->string(sx2);
    auto sym_sy = store->string(sy);

    REQUIRE(n1 == n1);
    REQUIRE(!(n1 == n2));
    REQUIRE(sx1 == sx1);
    REQUIRE(sx1 == sx2);
    REQUIRE(!(sx1 == sy));
    REQUIRE(sym_sx1.str() == sx1);
    REQUIRE(!(sym_sx1.str() == sy));
    REQUIRE(sym_sx1 == sym_sx1);
    REQUIRE(sym_sx1 == sym_sx2);
    REQUIRE(!(sym_sx1 == sym_sy));
}

} // namespace Gringo::Test
