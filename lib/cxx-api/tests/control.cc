#include <clingo/control.hh>

#include <catch2/catch_test_macros.hpp>

// NOLINTBEGIN(readability-magic-numbers)

namespace Clingo::Test {

auto ctx(char const *name, SymbolSpan params) -> SymbolVector {
    REQUIRE(params.size() == 1);
    REQUIRE(params[0].type() == SymbolType::number);
    if (std::strcmp(name, "f") == 0) {
        return SymbolVector{Number(params[0].number() + 1)};
    }
    if (std::strcmp(name, "g") == 0) {
        return SymbolVector{Number(params[0].number() - 1)};
    }
    return SymbolVector{};
}

TEST_CASE("control") {
    auto lib = Library{};
    auto ctl = Control{lib, {"0"}};
    ctl.parse_string("{a(@f(1)); b(@g(1))}.");
    ctl.ground(std::nullopt, ctx);
    // TODO: solve
}

} // namespace Clingo::Test

// NOLINTEND(readability-magic-numbers)
