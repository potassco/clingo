#include <gringo/core/symbol.hh>

#include <catch2/catch_test_macros.hpp>

#include <sstream>

// NOLINTBEGIN(readability-magic-numbers)

namespace Gringo {

// Note: to enable ADL for catch
using SL = std::initializer_list<SymbolRef>;
auto operator==(SymbolSpan a, SL b) -> bool { return std::equal(a.begin(), a.end(), b.begin(), b.end()); }

namespace Test {

namespace {

template <class F> void with_store(F fun) {
    fun(*make_symbol_store(false, false));
    fun(*make_symbol_store(true, false));
    fun(*make_symbol_store(false, true));
    fun(*make_symbol_store(true, true));
}

auto to_str(SymbolRef sym) -> std::string {
    std::ostringstream oss;
    oss << sym;
    return oss.str();
}

} // namespace

TEST_CASE("symbol_number") {
    auto store = make_symbol_store(false, false);
    auto n1 = store->num(Number(1));
    auto n2 = store->num(Number(2));
    auto n3 = store->num(Number(-1));

    REQUIRE(n1.type() == SymbolType::number);
    REQUIRE(n2.type() == SymbolType::number);
    REQUIRE(n3.type() == SymbolType::number);

    REQUIRE(to_str(n1) == "1");
    REQUIRE(to_str(n2) == "2");
    REQUIRE(to_str(n3) == "-1");

    REQUIRE(*n1.num() == 1);
    REQUIRE(*n2.num() == 2);
    REQUIRE(*n3.num() == -1);

    REQUIRE(n1 == n1);
    REQUIRE(!(n1 == n2));

    REQUIRE(!n1.has_sign());
    REQUIRE(n3.has_sign());
}

TEST_CASE("symbol_constant") {
    auto n1 = SymbolStore::inf();
    auto n2 = SymbolStore::sup();

    REQUIRE(to_str(n1) == "#inf");
    REQUIRE(to_str(n2) == "#sup");

    REQUIRE(n1.type() == SymbolType::inf);
    REQUIRE(n2.type() == SymbolType::sup);
    REQUIRE(n1 == n1);
    REQUIRE(!(n1 == n2));
}

TEST_CASE("string") {
    with_store([](SymbolStore &store) {
        auto sx1 = store.string("x");
        auto sx2 = store.string("x");
        auto sy = store.string("y");

        REQUIRE(sx1 == sx1);
        REQUIRE(sx1 == sx2);
        REQUIRE(!(sx1 == sy));
    });
}

TEST_CASE("symbol_string") {
    with_store([](SymbolStore &store) {
        auto sx = store.string("x");
        auto sy = store.string("y");
        auto sym_sx = SymbolStore::str(sx);
        auto sym_sy = SymbolStore::str(sy);

        REQUIRE(to_str(sym_sx) == "\"x\"");
        REQUIRE(to_str(sym_sy) == "\"y\"");

        REQUIRE(sym_sx.type() == SymbolType::string);
        REQUIRE(sym_sx.str() == sx);
        REQUIRE(!(sym_sx.str() == sy));
        REQUIRE(sym_sx == sym_sx);
        REQUIRE(!(sym_sx == sym_sy));
    });
}

TEST_CASE("symbol_tuple") {
    with_store([](SymbolStore &store) {
        auto n0 = store.num(Number(0));
        auto n1 = store.num(Number(1));
        auto n2 = store.num(Number(2));
        auto n3 = store.num(Number(3));

        auto t1 = store.tup(SL{});
        auto t2 = store.tup(SL{n0});
        auto t3 = store.tup(SL{n0, n1});
        auto t4 = store.tup(SL{n0, n1, n2});
        auto t5 = store.tup(SL{n0, n1, n3});
        auto t6 = store.tup(SL{n0, n1, n3});

        REQUIRE(t1.type() == SymbolType::tuple);
        REQUIRE(t2.type() == SymbolType::tuple);
        REQUIRE(t3.type() == SymbolType::tuple);
        REQUIRE(t4.type() == SymbolType::tuple);
        REQUIRE(t5.type() == SymbolType::tuple);
        REQUIRE(t6.type() == SymbolType::tuple);

        REQUIRE(to_str(t1) == "()");
        REQUIRE(to_str(t2) == "(0,)");
        REQUIRE(to_str(t3) == "(0,1)");
        REQUIRE(to_str(t4) == "(0,1,2)");
        REQUIRE(to_str(t5) == "(0,1,3)");
        REQUIRE(to_str(t6) == "(0,1,3)");

        REQUIRE(t1.args().empty());
        REQUIRE(t2.args() == SL{n0});
        REQUIRE(t3.args() == SL{n0, n1});
        REQUIRE(t4.args() == SL{n0, n1, n2});
        REQUIRE(t5.args() == SL{n0, n1, n3});
        REQUIRE(t6.args() == SL{n0, n1, n3});

        REQUIRE(t4 != t5);
        REQUIRE(t4 != t6);
        REQUIRE(t5 == t6);

        size_t n = 1 << 20;
        std::vector<SymbolRef> args;
        args.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            args.emplace_back(store.num(Number(static_cast<int32_t>(i))));
        }
        auto t = store.tup(SymbolSpan{args.data(), args.size()});
        REQUIRE(t.type() == SymbolType::tuple);
        REQUIRE(t.args().size() == n);
    });
}

TEST_CASE("symbol_function") {
    with_store([](SymbolStore &store) {
        auto s1 = store.string("f");
        auto s2 = store.string("g");

        auto n0 = store.num(Number(0));
        auto n1 = store.num(Number(1));
        auto n2 = store.num(Number(2));
        auto n3 = store.num(Number(3));

        auto f1 = store.fun(s1, SL{}, false);
        auto g1 = store.fun(s1, SL{}, true);
        auto f2 = store.fun(s1, SL{n0}, false);
        auto f3 = store.fun(s1, SL{n0, n1}, false);
        auto f4 = store.fun(s1, SL{n0, n1, n2}, false);
        auto f5 = store.fun(s1, SL{n0, n1, n3}, false);
        auto f6 = store.fun(s1, SL{n0, n1, n3}, false);
        auto f7 = store.fun(s2, SL{n0, n1, n3}, false);
        auto g7 = store.fun(s2, SL{n0, n1, n3}, true);

        REQUIRE(f1.type() == SymbolType::function);
        REQUIRE(g1.type() == SymbolType::function);
        REQUIRE(f2.type() == SymbolType::function);
        REQUIRE(f3.type() == SymbolType::function);
        REQUIRE(f4.type() == SymbolType::function);
        REQUIRE(f5.type() == SymbolType::function);
        REQUIRE(f6.type() == SymbolType::function);
        REQUIRE(f7.type() == SymbolType::function);
        REQUIRE(g7.type() == SymbolType::function);

        REQUIRE(to_str(f1) == "f");
        REQUIRE(to_str(g1) == "-f");
        REQUIRE(to_str(f2) == "f(0)");
        REQUIRE(to_str(f3) == "f(0,1)");
        REQUIRE(to_str(f4) == "f(0,1,2)");
        REQUIRE(to_str(f5) == "f(0,1,3)");
        REQUIRE(to_str(f6) == "f(0,1,3)");
        REQUIRE(to_str(f7) == "g(0,1,3)");
        REQUIRE(to_str(g7) == "-g(0,1,3)");

        REQUIRE(!f1.has_sign());
        REQUIRE(g1.has_sign());
        REQUIRE(f1 != g1);
        REQUIRE(!f7.has_sign());
        REQUIRE(g7.has_sign());
        REQUIRE(f7 != g7);

        REQUIRE(f1.name() == s1);
        REQUIRE(f2.name() == s1);
        REQUIRE(f3.name() == s1);
        REQUIRE(f4.name() == s1);
        REQUIRE(f5.name() == s1);
        REQUIRE(f6.name() == s1);
        REQUIRE(f7.name() == s2);
        REQUIRE(f6.name() != f7.name());

        REQUIRE(f1.args().empty());
        REQUIRE(f2.args() == SL{n0});
        REQUIRE(f3.args() == SL{n0, n1});
        REQUIRE(f4.args() == SL{n0, n1, n2});
        REQUIRE(f5.args() == SL{n0, n1, n3});
        REQUIRE(f6.args() == SL{n0, n1, n3});
        REQUIRE(f7.args() == SL{n0, n1, n3});

        REQUIRE(f4 != f5);
        REQUIRE(f4 != f6);
        REQUIRE(f5 == f6);
        REQUIRE(f6 != f7);

        size_t n = 1 << 20;
        std::vector<SymbolRef> args;
        args.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            args.emplace_back(store.num(Number(static_cast<int32_t>(i))));
        }
        auto t = store.fun(s1, SymbolSpan{args.data(), args.size()}, false);
        REQUIRE(t.type() == SymbolType::function);
        REQUIRE(t.name() == s1);
        REQUIRE(t.args().size() == n);
    });
}

// NOLINTEND(readability-magic-numbers)

} // namespace Test

} // namespace Gringo
