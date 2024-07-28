#include <gringo/input/literal.hh>

#include <gringo/input/rewrite/analyze.hh>

#include "parser_state.hh"

namespace Gringo::Input::Parse {

namespace {

auto is_rel(TokenType token) -> std::optional<Relation> {
    switch (token) {
        case TokenType::lt: {
            return Relation::less;
        }
        case TokenType::le: {
            return Relation::less_equal;
        }
        case TokenType::gt: {
            return Relation::greater;
        }
        case TokenType::ge: {
            return Relation::greater_equal;
        }
        case TokenType::eq: {
            return Relation::equal;
        }
        case TokenType::ne: {
            return Relation::not_equal;
        }
        default: {
            return std::nullopt;
        }
    }
}

} // namespace

auto parse_literal(ParserState &state) -> std::optional<Lit> {
    auto pos = state.token_pos();

    auto sign = Sign::none;
    if (state.branch(TokenType::not_)) {
        sign = Sign::once;
    }
    if (state.branch(TokenType::not_)) {
        sign = Sign::twice;
    }

    if (bool val = state.token() == TokenType::true_; val || state.token() == TokenType::false_) {
        auto ret = Lit{std::in_place_type<LitBool>, Location{std::move(pos), state.cursor_pos()}, sign, val};
        state.consume();
        return ret;
    }

    if (!check_term(state.token())) {
        return state.expected<std::nullopt>(TokenType::true_, TokenType::false_, "<term>");
    }

    if (auto term = parse_term(state); term) {
        auto guards = std::vector<Guard>{};
        while (auto rel = is_rel(state.token())) {
            state.consume();
            if (auto rhs = parse_term(state); rhs) {
                guards.emplace_back(*rel, *std::move(rhs));
            } else {
                return std::nullopt;
            }
        }
        if (!guards.empty()) {
            return Lit{std::in_place_type<LitComparison>, Location{std::move(pos), state.cursor_pos()}, sign,
                       std::move(*term), std::move(guards)};
        }
        if (is_atom(*term)) {
            return Lit{std::in_place_type<LitSymbolic>, Location{std::move(pos), state.cursor_pos()}, sign,
                       std::move(*term)};
        }
        return state.expected<std::nullopt>("<relation>");
    }

    return std::nullopt;
}

} // namespace Gringo::Input::Parse
