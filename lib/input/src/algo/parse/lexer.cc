#include "lexer_state.hh"

#include <iostream>
#include <sstream>

namespace Gringo {

static auto lex(LexerState &state) -> int;

void test() {
    std::istringstream iss(R"(425 _abc _ _ABC "\n1x'_:" #xxx)");
    auto state = LexerState{std::make_unique<std::istringstream>(std::move(iss))};
    // NOLINTBEGIN(performance-avoid-endl)
    for (auto token = lex(state); token != 0; token = lex(state)) {
        std::cerr << "token: " << token << std::endl;
    }
    // NOLINTEND(performance-avoid-endl)
}

#include "algo/parse/lexer.hh"

} // namespace Gringo
