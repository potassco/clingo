#include "lexer_state.hh"

#include <iostream>
#include <sstream>

namespace Gringo::Input {

enum class TokenType : uint8_t {
    end,
    error,
    num,
    str,
    anon,
    var,
    id,
    lpar,
    rpar,
    plus,
    minus,
    star,
    slash,
    bslash,
    qmark,
    caret,
    amp,
    bar,
};

static auto lex(LexerState &state) -> TokenType;

void test() {
    std::istringstream iss(R"(425 _abc _ _ABC "\n1x'_:" #xxx)");
    auto state = LexerState{std::make_unique<std::istringstream>(std::move(iss))};
    // NOLINTBEGIN(performance-avoid-endl)
    for (auto token = lex(state); token != TokenType::end; token = lex(state)) {
        std::cerr << "token: " << static_cast<int>(token) << std::endl;
    }
    // NOLINTEND(performance-avoid-endl)
}

#include "algo/parse/lexer_impl.hh"

} // namespace Gringo::Input
