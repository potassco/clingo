#include <input/algo/evaluate.hh>

#include "input/test.hh"

// NOLINTBEGIN(readability-magic-numbers)

namespace Gringo::Input::Test {

namespace {

auto parse_const(SymbolStore &store, std::string_view str) -> StatementConst {
    using Gringo::Input::parse_statement;
    auto stm = parse_statement(store, str);
    REQUIRE(stm.has_value());
    REQUIRE(std::holds_alternative<StatementConst>(*stm));
    return std::move(std::get<StatementConst>(*stm));
}

}; // namespace

TEST_CASE("evaluate_unary") {
    auto n1 = SymbolStore::num(-10);
    auto n2 = SymbolStore::num(std::numeric_limits<int32_t>::min());

    auto r1 = evaluate(UnaryOperator::invert, n1);
    REQUIRE(r1.has_value());
    REQUIRE(r1->num() == ~(-10));
    auto r2 = evaluate(UnaryOperator::negate, n1);
    REQUIRE(r2.has_value());
    REQUIRE(r2->num() == 10);
    auto r3 = evaluate(UnaryOperator::negate, n2);
    REQUIRE(!r3.has_value());
}

TEST_CASE("evaluate_binary") {
    auto n1 = SymbolStore::num(2);
    auto n2 = SymbolStore::num(1 << 16);
    auto r1 = evaluate(n1, BinaryOperator::plus, n2);
    REQUIRE(r1.has_value());
    REQUIRE(r1->num() == (1 << 16) + 2);
    auto r2 = evaluate(n1, BinaryOperator::times, n2);
    REQUIRE(r2.has_value());
    REQUIRE(r2->num() == (1 << 16) * 2);
    auto r3 = evaluate(n2, BinaryOperator::times, n2);
    // TODO: check the other ops too
    REQUIRE(!r3.has_value());
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
    stms.emplace_back(parse_const(*store, "#const a = b."));
    stms.emplace_back(parse_const(*store, "#const b = a."));
    evaluate_const(log, *store, stms);
    REQUIRE(log.has_error());

    log.reset();
    stms.clear();
    stms.emplace_back(parse_const(*store, "#const a = x."));
    stms.emplace_back(parse_const(*store, "#const a = y."));
    evaluate_const(log, *store, stms);
    REQUIRE(log.has_error());

    log.reset();
    stms.clear();
    stms.emplace_back(parse_const(*store, "#const a = 1+2."));
    stms.emplace_back(parse_const(*store, "#const b = 2*a."));
    auto map = evaluate_const(log, *store, stms);
    REQUIRE(!log.has_error());
    REQUIRE(map.size() == 2);
    REQUIRE(map.contains(store->string("a")));
    REQUIRE(map.contains(store->string("b")));
    REQUIRE(map[store->string("a")] == SymbolStore::num(3));
    REQUIRE(map[store->string("b")] == SymbolStore::num(6));
}

} // namespace Gringo::Input::Test

// NOLINTEND(readability-magic-numbers)
