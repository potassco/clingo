#include "lexer_state.hh"

#include <iostream>
#include <sstream>

namespace Gringo {

static auto lex(LexerState &state) -> int;

void test() {
    std::istringstream iss("abxa");
    auto state = LexerState{std::make_unique<std::istringstream>(std::move(iss))};
    // NOLINTBEGIN(performance-avoid-endl)
    std::cerr << "token: " << lex(state) << std::endl;
    std::cerr << "token: " << lex(state) << std::endl;
    std::cerr << "token: " << lex(state) << std::endl;
    std::cerr << "token: " << lex(state) << std::endl;
    std::cerr << "token: " << lex(state) << std::endl;
    // NOLINTEND(performance-avoid-endl)
}

#include "algo/parse/lexer.hh"

} // namespace Gringo
