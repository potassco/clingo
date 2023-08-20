#include <catch2/catch_test_macros.hpp>

#include <symbol_old.hh>

#include <iostream>

// NOLINTBEGIN(readability-magic-numbers)

namespace Gringo {

using S = std::initializer_list<Symbol>;
auto operator==(SymbolSpan a, S b) -> bool { return std::equal(a.begin(), a.end(), b.begin(), b.end()); }

} // namespace Gringo

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

TEST_CASE("symbol_tuple") {
    USymbolStore store = make_symbol_store(false);

    auto n0 = store->number(0);
    auto n1 = store->number(1);
    auto n2 = store->number(2);
    auto n3 = store->number(3);

    auto t1 = store->tuple(S{});
    auto t2 = store->tuple(S{n0});
    auto t3 = store->tuple(S{n0, n1});
    auto t4 = store->tuple(S{n0, n1, n2});
    auto t5 = store->tuple(S{n0, n1, n3});
    auto t6 = store->tuple(S{n0, n1, n3});

    REQUIRE(t1.type() == SymbolType::tuple);
    REQUIRE(t2.type() == SymbolType::tuple);
    REQUIRE(t3.type() == SymbolType::tuple);
    REQUIRE(t4.type() == SymbolType::tuple);
    REQUIRE(t5.type() == SymbolType::tuple);
    REQUIRE(t6.type() == SymbolType::tuple);

    REQUIRE(t1.args().empty());
    REQUIRE(t2.args() == S{n0});
    REQUIRE(t3.args() == S{n0, n1});
    REQUIRE(t4.args() == S{n0, n1, n2});
    REQUIRE(t5.args() == S{n0, n1, n3});
    REQUIRE(t6.args() == S{n0, n1, n3});

    REQUIRE(t4 != t5);
    REQUIRE(t4 != t6);
    REQUIRE(t5 == t6);

    size_t n = 1 >> 20;
    std::vector<Symbol> args;
    args.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        args.emplace_back(store->number(i));
    }
    auto t = store->tuple(SymbolSpan{args.data(), args.size()});
    REQUIRE(t.type() == SymbolType::tuple);
    REQUIRE(t.args().size() == n);
}

// NOLINTEND(readability-magic-numbers)

} // namespace Gringo::Test
