#include <gringo/input/literal.hh>

#include <gringo/input/rewrite/analyze.hh>

#include "parser_state.hh"

namespace Gringo::Input::Parse {

namespace {}

auto parse_body_literal(ParserState &state) -> std::optional<BdLit> {
    auto pos = state.token_pos();

    auto sign = Sign::none;
    if (state.branch(TokenType::not_)) {
        sign = Sign::once;
    }
    if (state.branch(TokenType::not_)) {
        sign = Sign::twice;
    }

    static_cast<void>(sign);
    static_cast<void>(pos);

    // handle atoms or guards of aggregates
    if (check_term(state.token())) {
        throw std::runtime_error("implement me!!!");
    }
    // handle theory atoms
    if (state.token() == TokenType::amp) {
        if (auto atom = parse_theory_atom(state); atom) {
            throw std::runtime_error("implement me!!!");
        }
    }
    // handle aggregates
    // TODO: ...

    return state.expected<std::nullopt>(TokenType::amp, "<aggregate-function>", "<term>");
}

} // namespace Gringo::Input::Parse
