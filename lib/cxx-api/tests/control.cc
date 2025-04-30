#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN(readability-magic-numbers)

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
    // TODO: solve
}

} // namespace Clingo::Test

// NOLINTEND(readability-magic-numbers)
