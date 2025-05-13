#include <clingo/input/literal.hh>

#include <clingo/input/rewrite/analyze.hh>

#include "parser_state.hh"

namespace CppClingo::Input::Parse {

auto check_relation(TokenType token) -> std::optional<Relation> {
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

auto cont_literal(ParserState &state, Position pos, Sign sign, Term term, Relation rel) -> std::optional<Lit> {
    auto guards = std::vector<Guard>{};
    if (auto rhs = parse_term(state)) {
        guards.emplace_back(rel, *std::move(rhs));
    } else {
        return std::nullopt;
    }
    while (auto rel = check_relation(state.token())) {
        state.consume();
        if (auto rhs = parse_term(state); rhs) {
            guards.emplace_back(*rel, *std::move(rhs));
        } else {
            return std::nullopt;
        }
    }
    return Lit{std::in_place_type<LitComparison>, Location{std::move(pos), state.cursor_pos()}, sign, std::move(term),
               std::move(guards)};
}

auto cont_literal(ParserState &state, Position pos, Sign sign, Term term) -> std::optional<Lit> {
    if (auto rel = check_relation(state.token())) {
        state.consume();
        return cont_literal(state, std::move(pos), sign, std::move(term), *rel);
    }
    if (is_atom(term)) {
        return Lit{std::in_place_type<LitSymbolic>, Location{std::move(pos), state.cursor_pos()}, sign,
                   std::move(term)};
    }
    return state.expected<std::nullopt>("<relation>");
}

auto cont_literal(ParserState &state, Position pos, Sign sign) -> std::optional<Lit> {
    if (bool val = state.token() == TokenType::true_; val || state.token() == TokenType::false_) {
        auto ret = Lit{std::in_place_type<LitBool>, Location{std::move(pos), state.cursor_pos()}, sign, val};
        state.consume();
        return ret;
    }

    if (!check_term(state.token())) {
        return state.expected<std::nullopt>(TokenType::true_, TokenType::false_, "<term>");
    }
    if (auto term = parse_term(state); term) {
        return cont_literal(state, std::move(pos), sign, *std::move(term));
    }

    return std::nullopt;
}

auto parse_sign(ParserState &state) -> Sign {
    auto sign = Sign::none;
    if (state.branch(TokenType::not_)) {
        sign = Sign::once;
    }
    if (state.branch(TokenType::not_)) {
        sign = Sign::twice;
    }
    return sign;
}

auto parse_literal(ParserState &state) -> std::optional<Lit> {
    auto pos = state.token_pos();
    auto sign = parse_sign(state);

    return cont_literal(state, std::move(pos), sign);
}

} // namespace CppClingo::Input::Parse
