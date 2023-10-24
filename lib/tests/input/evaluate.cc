#include <cmath>
#include <iostream>

#include <input/algo/evaluate.hh>

#include "input/test.hh"

// NOLINTBEGIN(readability-magic-numbers)

namespace Gringo::Input::Test {

namespace {

using SL = std::initializer_list<Symbol>;

auto parse_const(SymbolStore &store, std::string_view str) -> StatementConst {
    using Gringo::Input::parse_statement;
    auto stm = parse_statement(store, str);
    REQUIRE(stm.has_value());
    REQUIRE(std::holds_alternative<StatementConst>(*stm));
    return std::move(std::get<StatementConst>(*stm));
}

}; // namespace

TEST_CASE("evaluate_unary") {
    auto store = make_symbol_store(false, false);
    auto n1 = store->num(Number(-10));
    auto n2 = store->num(Number(std::numeric_limits<int32_t>::min()));

    auto r1 = evaluate(*store, UnaryOperator::invert, n1);
    REQUIRE(r1.has_value());
    REQUIRE(*r1->num() == ~(-10));
    auto r2 = evaluate(*store, UnaryOperator::negate, n1);
    REQUIRE(r2.has_value());
    REQUIRE(*r2->num() == 10);
    auto r3 = evaluate(*store, UnaryOperator::negate, n2);
    REQUIRE(*r3->num() == Number("2147483648"));
}

TEST_CASE("evaluate_binary") {
    auto store = make_symbol_store(false, false);
    int i1 = 3;
    int i2 = 1 << 16;
    int i3 = 0;
    int i4 = 42;
    auto n1 = store->num(Number(i1));
    auto n2 = store->num(Number(i2));
    auto n3 = store->num(Number(i3));
    auto n4 = store->num(Number(i4));

    // plus
    auto res = evaluate(*store, n1, BinaryOperator::plus, n2);
    REQUIRE(res.has_value());
    REQUIRE(*res->num() == i1 + i2);
    // minus
    res = evaluate(*store, n1, BinaryOperator::minus, n2);
    REQUIRE(res.has_value());
    REQUIRE(*res->num() == i1 - i2);
    // times
    res = evaluate(*store, n1, BinaryOperator::times, n2);
    REQUIRE(res.has_value());
    REQUIRE(*res->num() == i1 * i2);
    res = evaluate(*store, n2, BinaryOperator::times, n2);
    REQUIRE(*res->num() == Number("4294967296"));
    // div
    res = evaluate(*store, n1, BinaryOperator::div, n2);
    REQUIRE(res.has_value());
    REQUIRE(*res->num() == i1 / i2);
    res = evaluate(*store, n2, BinaryOperator::div, n3);
    REQUIRE(!res.has_value());
    // mod
    res = evaluate(*store, n1, BinaryOperator::mod, n2);
    REQUIRE(res.has_value());
    REQUIRE(*res->num() == i1 % i2);
    res = evaluate(*store, n2, BinaryOperator::mod, n3);
    REQUIRE(!res.has_value());
    // pow
    res = evaluate(*store, n4, BinaryOperator::pow, n1);
    REQUIRE(res.has_value());
    REQUIRE(*res->num() == static_cast<int32_t>(std::pow(i4, i1)));
    res = evaluate(*store, n2, BinaryOperator::pow, n1);
    REQUIRE(*res->num() == Number("281474976710656"));
    // xor_
    res = evaluate(*store, n1, BinaryOperator::xor_, n2);
    REQUIRE(res.has_value());
    REQUIRE(*res->num() == (i1 ^ i2));
    // or_
    res = evaluate(*store, n1, BinaryOperator::or_, n2);
    REQUIRE(res.has_value());
    REQUIRE(*res->num() == (i1 | i2));
    // and_
    res = evaluate(*store, n1, BinaryOperator::and_, n2);
    REQUIRE(res.has_value());
    REQUIRE(*res->num() == (i1 & i2));
}

TEST_CASE("evaluate_const") {
    Logger log{[](MessageCode code, char const *msg) {
        // TODO: maybe put some requirements on messages
        static_cast<void>(code);
        static_cast<void>(msg);
    }};
    log.set_level(LogLevel::trace);
    auto store = make_symbol_store(true, true);
    auto stms = std::vector<StatementConst>{};

    SECTION("cycle") {
        stms.emplace_back(parse_const(*store, "#const a = b."));
        stms.emplace_back(parse_const(*store, "#const b = a."));
        evaluate_const(log, *store, stms);
        REQUIRE(log.has_error());
    }

    SECTION("redefinition") {
        stms.emplace_back(parse_const(*store, "#const a = x."));
        stms.emplace_back(parse_const(*store, "#const a = y."));
        evaluate_const(log, *store, stms);
        REQUIRE(log.has_error());
    }

    SECTION("depend") {
        auto fg = store->fun(store->string("g"), SL{store->num(Number(-18))}, true);
        auto ff = store->fun(store->string("f"), SL{store->num(Number(6)), fg}, false);
        stms.emplace_back(parse_const(*store, "#const a = 1+2."));
        stms.emplace_back(parse_const(*store, "#const b = 2*a."));
        stms.emplace_back(parse_const(*store, "#const c = f(b,-g(-a*b))."));
        auto map = evaluate_const(log, *store, stms);
        REQUIRE(!log.has_error());
        REQUIRE(map.size() == 3);
        REQUIRE(map.contains(store->string("a")));
        REQUIRE(map.contains(store->string("b")));
        REQUIRE(map.contains(store->string("c")));
        REQUIRE(map[store->string("a")] == store->num(Number(3)));
        REQUIRE(map[store->string("b")] == store->num(Number(6)));
        REQUIRE(map[store->string("c")] == ff);
    }
}

} // namespace Gringo::Input::Test

// NOLINTEND(readability-magic-numbers)
