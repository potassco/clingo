#include "test.hh"

#include <clingo/input/rewrite/evaluate.hh>

#include <cmath>

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN(readability-magic-numbers)

namespace CppClingo::Input::Test {

namespace {

using SL = std::initializer_list<Symbol>;

class ConstHelper : public ParseHelper {
  public:
    auto const_def(std::string_view str) -> StmConst {
        reset();
        auto stm = statement(str);
        REQUIRE(stm.has_value());
        REQUIRE(std::holds_alternative<StmConst>(opt_value(stm)));
        return std::move(std::get<StmConst>(opt_value(stm)));
    }
};

}; // namespace

TEST_CASE("evaluate_unary") {
    auto store = make_symbol_store(false, false);
    auto n1 = store->num_ref(Number(-10));
    auto n2 = store->num_ref(Number(std::numeric_limits<int32_t>::min()));

    auto r1 = evaluate(*store, UnaryOperator::negate, n1);
    REQUIRE(opt_value(r1).num() == ~(-10));
    auto r2 = evaluate(*store, UnaryOperator::minus, n1);
    REQUIRE(opt_value(r2).num() == 10);
    auto r3 = evaluate(*store, UnaryOperator::minus, n2);
    REQUIRE(opt_value(r3).num() == Number("2147483648"));
}

TEST_CASE("evaluate_binary") {
    auto store = make_symbol_store(false, false);
    int i1 = 3;
    int i2 = 1 << 16;
    int i3 = 0;
    int i4 = 42;
    auto n1 = store->num_ref(Number(i1));
    auto n2 = store->num_ref(Number(i2));
    auto n3 = store->num_ref(Number(i3));
    auto n4 = store->num_ref(Number(i4));

    // plus
    auto res = evaluate(*store, n1, BinaryOperator::plus, n2);
    REQUIRE(opt_value(res).num() == i1 + i2);
    // minus
    res = evaluate(*store, n1, BinaryOperator::minus, n2);
    REQUIRE(opt_value(res).num() == i1 - i2);
    // times
    res = evaluate(*store, n1, BinaryOperator::times, n2);
    REQUIRE(opt_value(res).num() == i1 * i2);
    res = evaluate(*store, n2, BinaryOperator::times, n2);
    REQUIRE(opt_value(res).num() == Number("4294967296"));
    // div
    res = evaluate(*store, n1, BinaryOperator::div, n2);
    REQUIRE(opt_value(res).num() == i1 / i2);
    res = evaluate(*store, n2, BinaryOperator::div, n3);
    REQUIRE(!res.has_value());
    // mod
    res = evaluate(*store, n1, BinaryOperator::mod, n2);
    REQUIRE(opt_value(res).num() == i1 % i2);
    res = evaluate(*store, n2, BinaryOperator::mod, n3);
    REQUIRE(!res.has_value());
    // pow
    res = evaluate(*store, n4, BinaryOperator::pow, n1);
    REQUIRE(opt_value(res).num() == static_cast<int32_t>(std::pow(i4, i1)));
    res = evaluate(*store, n2, BinaryOperator::pow, n1);
    REQUIRE(opt_value(res).num() == Number("281474976710656"));
    // xor_
    res = evaluate(*store, n1, BinaryOperator::xor_, n2);
    REQUIRE(opt_value(res).num() == (i1 ^ i2));
    // or_
    res = evaluate(*store, n1, BinaryOperator::or_, n2);
    REQUIRE(opt_value(res).num() == (i1 | i2));
    // and_
    res = evaluate(*store, n1, BinaryOperator::and_, n2);
    REQUIRE(opt_value(res).num() == (i1 & i2));
}

TEST_CASE("evaluate_const") {
    ConstHelper ch;
    auto stms = std::vector<StmConst>{};

    SECTION("cycle") {
        stms.emplace_back(ch.const_def("#const a = b."));
        stms.emplace_back(ch.const_def("#const b = a."));
        ConstMap map;
        REQUIRE_THROWS(evaluate_const(ch, ch, stms, map));
    }

    SECTION("redefinition") {
        stms.emplace_back(ch.const_def("#const a = x."));
        stms.emplace_back(ch.const_def("#const a = y."));
        ConstMap map;
        REQUIRE_THROWS(evaluate_const(ch, ch, stms, map));
    }

    SECTION("depend") {
        auto fg = ch.store().fun_ref(ch.store().string_ref("g"), SL{ch.store().num_ref(Number(-18))}, true);
        auto ff = ch.store().fun_ref(ch.store().string_ref("f"), SL{ch.store().num_ref(Number(6)), fg}, false);
        stms.emplace_back(ch.const_def("#const a = 1+2."));
        stms.emplace_back(ch.const_def("#const b = 2*a."));
        stms.emplace_back(ch.const_def("#const c = f(b,-g(-a*b))."));
        ConstMap map;
        evaluate_const(ch, ch, stms, map);
        REQUIRE(map.size() == 3);
        REQUIRE(map.contains(ch.store().string_ref("a")));
        REQUIRE(map.contains(ch.store().string_ref("b")));
        REQUIRE(map.contains(ch.store().string_ref("c")));
        REQUIRE(map.at(ch.store().string_ref("a")).second == ch.store().num_ref(Number(3)));
        REQUIRE(map.at(ch.store().string_ref("b")).second == ch.store().num_ref(Number(6)));
        REQUIRE(map.at(ch.store().string_ref("c")).second == ff);
    }
}

} // namespace CppClingo::Input::Test

// NOLINTEND(readability-magic-numbers)
