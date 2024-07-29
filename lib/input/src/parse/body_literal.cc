#include <gringo/input/literal.hh>

#include <gringo/input/rewrite/analyze.hh>

#include "parser_state.hh"

namespace Gringo::Input::Parse {

namespace {

auto cont_condlit(ParserState &state, Lit lit) -> std::optional<BdLit> {
    if (state.token() == TokenType::colon) {
        std::vector<Lit> cond;
        do {
            state.consume();
            if (auto lit = parse_literal(state)) {
                cond.emplace_back(*std::move(lit));
            } else {
                return std::nullopt;
            }
        } while (state.token() == TokenType::comma);
        return BdLitConjunction{
            CondLit{location(lit).begin() + location(cond.back()).end(), std::move(lit), std::move(cond)}};
    }
    return BdLitSimple{std::move(lit)};
}

} // namespace

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
    auto sign = parse_sign(state);

    // handle atoms or guards of aggregates
    if (check_term(state.token())) {
        auto term = parse_term(state);
        if (!term) {
            return std::nullopt;
        }
        if (auto rel = check_relation(state.token())) {
            state.consume();
            // handle aggregates
            if (auto fun = check_aggregate(state.token())) {
                state.consume();
                throw std::runtime_error("implement me!!!");
            }
            // handle set aggregates
            if (state.token() == TokenType::lbrace) {
                state.consume();
                throw std::runtime_error("implement me!!!");
            }
            if (auto lit = cont_literal(state, std::move(pos), sign, *std::move(term), *rel)) {
                return cont_condlit(state, *std::move(lit));
            }
            return std::nullopt;
        }
        if (auto lit = cont_literal(state, std::move(pos), sign, *std::move(term))) {
            return cont_condlit(state, *std::move(lit));
        }
        return std::nullopt;
    }

    // handle Boolean literals
    if (state.token() == TokenType::true_ || state.token() == TokenType::false_) {
        if (auto lit = cont_literal(state, std::move(pos), sign)) {
            return cont_condlit(state, *std::move(lit));
        }
        return std::nullopt;
    }

    // handle theory atoms
    if (state.token() == TokenType::amp) {
        // NOTE: cont_theory_atom might be a better name
        if (auto atom = parse_theory_atom(state); atom) {
            auto [name, elems, rguard, end] = *std::move(atom);
            return BdLitTheoryAtom{pos + end, sign, name, std::move(elems), std::move(rguard)};
        }
        return std::nullopt;
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
