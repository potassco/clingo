#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

namespace Clingo::Test {

auto ctx(std::string_view name, SymbolSpan params) -> SymbolVector {
    REQUIRE(params.size() == 1);
    REQUIRE(params[0].type() == SymbolType::number);
    if (name == "f") {
        return SymbolVector{Number(params[0].number() + 1)};
    }
    if (name == "g") {
        return SymbolVector{Number(params[0].number() - 1)};
    }
    return SymbolVector{};
}

TEST_CASE("cxx-control") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("{a(@f(1)); b(@g(1))}.");
    ctl.ground(std::nullopt, ctx);
    auto hnd = ctl.solve();
    auto mdls = std::vector<std::string>{};
    for (auto mdl : hnd) {
        auto oss = std::ostringstream{};
        auto syms = mdl.symbols();
        std::ranges::sort(syms);
        for (auto const &sym : syms) {
            oss << sym << " ";
        }
        mdls.emplace_back(oss.str());
        if (!mdls.back().empty()) {
            mdls.back().pop_back();
        }
    }
    std::ranges::sort(mdls);
    REQUIRE(mdls == std::vector<std::string>{"", "a(2)", "a(2) b(0)", "b(0)"});
    REQUIRE(hnd.get().satisfiable());
}

} // namespace Clingo::Test
