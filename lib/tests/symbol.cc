#include <catch2/catch_test_macros.hpp>

#include <symbol_old.hh>

namespace Gringo::Test {

TEST_CASE("symbol_number") {
    USymbolStore store = make_symbol_store(false);

    auto n1 = store->number(1);
    auto n2 = store->number(2);

    REQUIRE(n1.type() == SymbolType::number);
    REQUIRE(n2.type() == SymbolType::number);
    REQUIRE(n1 == n1);
    REQUIRE(!(n1 == n2));
}

TEST_CASE("symbol_constant") {
    USymbolStore store = make_symbol_store(false);

    auto n1 = store->inf();
    auto n2 = store->sup();

    REQUIRE(n1.type() == SymbolType::inf);
    REQUIRE(n2.type() == SymbolType::sup);
    REQUIRE(n1 == n1);
    REQUIRE(!(n1 == n2));
}

TEST_CASE("string") {
    USymbolStore store = make_symbol_store(false);

    auto sx1 = store->string("x");
    auto sx2 = store->string("x");
    auto sy = store->string("y");

    REQUIRE(sx1 == sx1);
    REQUIRE(sx1 == sx2);
    REQUIRE(!(sx1 == sy));
}

TEST_CASE("symbol_string") {
    USymbolStore store = make_symbol_store(false);

    auto sx = store->string("x");
    auto sy = store->string("y");
    auto sym_sx = store->string(sx);
    auto sym_sy = store->string(sy);

    REQUIRE(sym_sx.type() == SymbolType::string);
    REQUIRE(sym_sx.str() == sx);
    REQUIRE(!(sym_sx.str() == sy));
    REQUIRE(sym_sx == sym_sx);
    REQUIRE(!(sym_sx == sym_sy));
}

} // namespace Gringo::Test
