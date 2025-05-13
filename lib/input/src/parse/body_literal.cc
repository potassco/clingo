#include <clingo/input/literal.hh>

#include <clingo/input/rewrite/analyze.hh>

#include "parser_state.hh"

namespace CppClingo::Input::Parse {

namespace {

//! Continue parsing a conditional literal.
auto cont_conjunction(ParserState &state, Lit lit) -> std::optional<BdLit> {
    if (state.token() == TokenType::colon) {
        // parse condition
        auto loc = location(lit) + state.cursor_pos();
        state.consume();
        if (auto cond = state.separated_until(parse_literal, TokenType::comma, TokenType::sem, TokenType::dot,
                                              TokenType::end)) {
            if (!cond->empty()) {
                loc += location(cond->back());
            }
            return BdLitConjunction{CondLit{std::move(loc), std::move(lit), *std::move(cond)}};
        }
        return std::nullopt;
    }
    return BdLitSimple{std::move(lit)};
}

//! Parse a body aggregate element.
auto parse_bd_aggr_elem(ParserState &state) -> std::optional<BdLitAggregateElement> {
    // if the tuple is empty it must be followed by a colon
    if (state.token() == TokenType::sem || state.token() == TokenType::rbrace) {
        return state.expected<std::nullopt>(TokenType::colon, "<term>");
    }
    // parse the tuple
    if (auto tuple =
            state.separated_until(parse_term, TokenType::comma, TokenType::colon, TokenType::sem, TokenType::rbrace)) {
        auto loc = tuple->empty() ? state.loc() : location(tuple->front());
        // a tuple without condition
        if (state.token() != TokenType::colon) {
            if (!tuple->empty()) {
                loc += location(tuple->back());
            }
            return BdLitAggregateElement{std::move(loc), *std::move(tuple), {}};
        }
        loc += state.cursor_pos();
        // consume the colon
        state.consume();
        // parse condition
        if (auto cond = state.separated_until(parse_literal, TokenType::comma, TokenType::sem, TokenType::rbrace)) {
            if (!cond->empty()) {
                loc += location(cond->back()).end();
            }
            return BdLitAggregateElement{std::move(loc), *std::move(tuple), *std::move(cond)};
        }
    }
    return std::nullopt;
}

//! Continue parsing a body aggregate.
auto cont_bd_aggregate(ParserState &state, Position pos, Sign sign, LGuard lguard, AggregateFunction fun)
    -> std::optional<BdLit> {
    // parse elements
    if (auto elems = state.delimited(TokenType::lbrace, parse_bd_aggr_elem, TokenType::sem, TokenType::rbrace)) {
        auto loc = std::move(pos) + state.cursor_pos();
        // consume rbrace
        state.consume();
        // parse optional right guard
        if (auto rguard = parse_rguard(state)) {
            if (*rguard) {
                loc += location((*rguard)->second);
            }
            return BdLitAggregate{std::move(loc), sign, std::move(lguard), fun, *std::move(elems), *std::move(rguard)};
        }
    }
    return std::nullopt;
}

//! Continue parsing a body aggregate.
auto cont_bd_set_aggregate(ParserState &state, Position pos, Sign sign, LGuard lguard) -> std::optional<BdLit> {
    // parse elements
    if (auto elems = state.delimited(TokenType::lbrace, parse_set_aggr_elem, TokenType::sem, TokenType::rbrace)) {
        auto end = state.cursor_pos();
        // consume closing brace
        state.consume();
        // parse optional guard
        if (auto rguard = parse_rguard(state)) {
            if (*rguard) {
                end = location((*rguard)->second).end();
            }
            return BdLitSetAggregate{std::move(pos) + std::move(end), sign, std::move(lguard), *std::move(elems),
                                     *std::move(rguard)};
        }
    }
    return std::nullopt;
}

} // namespace

auto check_aggregate(TokenType token) -> std::optional<AggregateFunction> {
    switch (token) {
        case TokenType::sum: {
            return AggregateFunction::sum;
        }
        case TokenType::sump: {
            return AggregateFunction::sump;
        }
        case TokenType::count: {
            return AggregateFunction::count;
        }
        case TokenType::min: {
            return AggregateFunction::min;
        }
        case TokenType::max: {
            return AggregateFunction::max;
        }
        default: {
            return std::nullopt;
        }
    }
}

auto parse_set_aggr_elem(ParserState &state) -> std::optional<SetAggregateElement> {
    // parse literal
    if (auto lit = parse_literal(state)) {
        // empty condition
        if (state.token() != TokenType::colon) {
            auto loc = location(*lit);
            return SetAggregateElement{std::move(loc), *std::move(lit), {}};
        }
        auto loc = location(*lit) + state.cursor_pos();
        // consume colon
        state.consume();
        // parse condition
        if (auto cond = state.separated_until(parse_literal, TokenType::comma, TokenType::sem, TokenType::rbrace)) {
            if (!cond->empty()) {
                loc += location(cond->back());
            }
            return SetAggregateElement{std::move(loc), *std::move(lit), *std::move(cond)};
        }
    }
    return std::nullopt;
}

auto parse_rguard(ParserState &state) -> std::optional<RGuard> {
    auto rguard = RGuard{};
    if (auto rel = check_relation(state.token())) {
        // guard with relation
        state.consume();
        auto term = parse_term(state);
        if (!term) {
            return std::nullopt;
        }
        rguard.emplace(*rel, *std::move(term));
    } else if (check_term(state.token())) {
        // guard with implicit less equal relation
        auto term = parse_term(state);
        if (!term) {
            return std::nullopt;
        }
        rguard.emplace(Relation::less_equal, *std::move(term));
    }
    return rguard;
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
        // handle aggregates
        if (auto fun = check_aggregate(state.token())) {
            state.consume();
            return cont_bd_aggregate(state, pos, sign, LGuard{std::in_place, *std::move(term), Relation::less_equal},
                                     *fun);
        }
        // handle set aggregates
        if (state.token() == TokenType::lbrace) {
            return cont_bd_set_aggregate(state, pos, sign,
                                         LGuard{std::in_place, *std::move(term), Relation::less_equal});
        }
        if (auto rel = check_relation(state.token())) {
            state.consume();
            // handle aggregates
            if (auto fun = check_aggregate(state.token())) {
                state.consume();
                return cont_bd_aggregate(state, pos, sign, LGuard{std::in_place, *std::move(term), *rel}, *fun);
            }
            // handle set aggregates
            if (state.token() == TokenType::lbrace) {
                return cont_bd_set_aggregate(state, pos, sign, LGuard{std::in_place, *std::move(term), *rel});
            }
            // handle comparison literals
            if (auto lit = cont_literal(state, std::move(pos), sign, *std::move(term), *rel)) {
                return cont_conjunction(state, *std::move(lit));
            }
            return std::nullopt;
        }
        // handle symbolic literals/conjunctions
        if (auto lit = cont_literal(state, std::move(pos), sign, *std::move(term))) {
            return cont_conjunction(state, *std::move(lit));
        }
        return std::nullopt;
    }

    // handle Boolean literals
    if (state.token() == TokenType::true_ || state.token() == TokenType::false_) {
        if (auto lit = cont_literal(state, std::move(pos), sign)) {
            return cont_conjunction(state, *std::move(lit));
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
        return cont_bd_aggregate(state, pos, sign, std::nullopt, *fun);
    }

    // handle set aggregate
    if (state.token() == TokenType::lbrace) {
        return cont_bd_set_aggregate(state, pos, sign, std::nullopt);
    }

    return state.expected<std::nullopt>(TokenType::amp, TokenType::lbrace, TokenType::true_, TokenType::false_,
                                        "<aggregate-function>", "<term>");
}

} // namespace CppClingo::Input::Parse
