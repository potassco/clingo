#include <gringo/input/literal.hh>

#include <gringo/input/rewrite/analyze.hh>

#include "parser_state.hh"

namespace Gringo::Input::Parse {

namespace {}

auto check_aggregate(TokenType token) -> bool {
    switch (token) {
        case TokenType::sum:
        case TokenType::sump:
        case TokenType::count:
        case TokenType::min:
        case TokenType::max: {
            return true;
        }
        default: {
            return false;
        }
    }
}

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
        auto term = parse_term(state);
        if (!term) {
            return std::nullopt;
        }
        if (auto rel = check_relation(state.token())) {
            state.consume();
            // handle aggregate
            if (auto fun = check_aggregate(state.token())) {
                state.consume();
                throw std::runtime_error("implement me!!!");
            }
            // handle set aggregate
            if (state.token() == TokenType::lbrace) {
                state.consume();
                throw std::runtime_error("implement me!!!");
            }
            // handle comparision literal
            // TODO: copy from literal
            // TODO: handle possible condition
            throw std::runtime_error("implement me!!!");
        }
        // check that the term is an atom
        // TODO: copy from literal
        // TODO: handle possible condition
        throw std::runtime_error("implement me!!!");
    }
    // handle theory atoms
    if (state.token() == TokenType::amp) {
        // NOTE: cont_theory_atom might be a better name
        if (auto atom = parse_theory_atom(state); atom) {
            throw std::runtime_error("implement me!!!");
        }
    }

    // handle aggregates
    if (auto fun = check_aggregate(state.token())) {
        state.consume();
        throw std::runtime_error("implement me!!!");
    }
    // handle set aggregate
    if (state.token() == TokenType::lbrace) {
        state.consume();
        throw std::runtime_error("implement me!!!");
    }

    return state.expected<std::nullopt>(TokenType::amp, "<aggregate-function>", "<term>");
}

} // namespace Gringo::Input::Parse
