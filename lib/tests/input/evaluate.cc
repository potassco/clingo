#include <input/algo/evaluate.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

TEST_CASE("evaluate_const") {
    Logger log;
    log.set_level(LogLevel::trace);
    using Gringo::Input::parse_statement;
    auto store = make_symbol_store(true, true);

    auto stms = std::vector<StatementConst>{};
    stms.emplace_back(std::get<StatementConst>(*parse_statement(*store, "#const a = b.")));
    stms.emplace_back(std::get<StatementConst>(*parse_statement(*store, "#const b = a.")));
    evaluate_const(log, *store, stms);

    stms.clear();
    stms.emplace_back(std::get<StatementConst>(*parse_statement(*store, "#const a = x.")));
    stms.emplace_back(std::get<StatementConst>(*parse_statement(*store, "#const a = y.")));
    evaluate_const(log, *store, stms);
}

} // namespace Gringo::Input::Test
