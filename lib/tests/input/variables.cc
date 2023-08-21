#include <input/algo/visit_variables.hh>

#include "input/test.hh"

namespace Gringo::Input::Test {

namespace {

template <class T> auto variables_str(T const &value) -> std::string {
    auto vars = select_variables(value);
    std::vector<std::string> sorted;
    sorted.reserve(vars.size());
    for (auto &var : vars) {
        sorted.emplace_back(var.view());
    }
    std::sort(sorted.begin(), sorted.end());
    return to_str(sorted);
}

template <class T> auto variables_str(std::optional<T> const &value) -> std::string {
    if (value.has_value()) {
        return variables_str(value.value());
    }
    return "<failed>";
}

} // namespace

TEST_CASE("variables_term") {
    REQUIRE(variables_str(parse_term("f(X;Y)")) == "[X, Y]");
    REQUIRE(variables_str(parse_term("f(X,Z;X,Y,Z;X,Y)")) == "[X, Y, Z]");
}

} // namespace Gringo::Input::Test
