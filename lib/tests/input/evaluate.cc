#include <input/algo/evaluate.hh>

#include "input/test.hh"

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
