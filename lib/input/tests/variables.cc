#include "test.hh"

#include <clingo/input/rewrite/visit_variables.hh>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

namespace CppClingo::Input::Test {

namespace {

template <class T> auto variables_str(T const &value) -> std::string {
    auto vars = select_variables(value);
    std::vector<std::string> sorted;
    sorted.reserve(vars.size());
    for (auto &var : vars) {
        sorted.emplace_back(var.view());
    }
    std::ranges::sort(sorted);
    return to_str(sorted);
}

template <class T> auto variables_str(std::optional<T> const &value) -> std::string {
    if (value.has_value()) {
        return variables_str(value.value());
    }
    return "<failed>";
}

auto variables_term(std::string const &str) -> std::string {
    ParseHelper ph;
    return variables_str(ph.term(str));
}

} // namespace

TEST_CASE("variables_term") {
    REQUIRE(variables_term("f(X;Y)") == "[X, Y]");
    REQUIRE(variables_term("f(X,Z;X,Y,Z;X,Y)") == "[X, Y, Z]");
    REQUIRE(variables_term("f\"{X}: {Y}\"") == "[X, Y]");
}

} // namespace CppClingo::Input::Test
