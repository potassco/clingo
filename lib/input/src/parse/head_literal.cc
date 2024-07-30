#include <gringo/input/literal.hh>

#include <gringo/input/rewrite/analyze.hh>

#include "parser_state.hh"

namespace Gringo::Input::Parse {

namespace {

//! Continue parsing a disjunction.
auto cont_disjunction(ParserState &state, HdLitDisjunctionElement elem) -> std::optional<HdLit> {
    auto elems = std::vector<HdLitDisjunctionElement>{};
    elems.emplace_back(std::move(elem));
    while (state.token() == TokenType::sem || state.token() == TokenType::bar || state.token() == TokenType::comma) {
        state.consume();
        auto lit = parse_literal(state);
        if (!lit) {
            return std::nullopt;
        }
        if (state.token() == TokenType::colon) {
            auto loc = location(*lit).begin() + state.cursor_pos();
            state.consume();
            auto cond = state.separated_until(parse_literal, TokenType::comma, TokenType::sem, TokenType::dot,
                                              TokenType::bar, TokenType::end);
            if (!cond) {
                return std::nullopt;
            }
            if (!cond->empty()) {
                loc += location(cond->back()).end();
            }
            elems.emplace_back(CondLit{std::move(loc), *std::move(lit), *std::move(cond)});
        } else {
            elems.emplace_back(*std::move(lit));
        }
    }
    auto loc = location(elems.front()) + location(elems.back());
    return HdLitDisjunction{std::move(loc), std::move(elems)};
}

//! Continue parsing a disjunction.
auto cont_disjunction(ParserState &state, Lit lit) -> std::optional<HdLit> {
    if (state.token() == TokenType::colon) {
        auto end = state.cursor_pos();
        state.consume();
        if (auto cond = state.separated_until(parse_literal, TokenType::comma, TokenType::sem, TokenType::dot,
                                              TokenType::bar, TokenType::end)) {
            auto loc = location(lit).begin() + (cond->empty() ? end : location(cond->back()).end());
            return cont_disjunction(state, CondLit{std::move(loc), std::move(lit), *std::move(cond)});
        }
        return std::nullopt;
    }
    if (state.token() == TokenType::sem || state.token() == TokenType::bar || state.token() == TokenType::comma) {
        return cont_disjunction(state, HdLitDisjunctionElement{std::move(lit)});
    }

    return HdLitSimple{std::move(lit)};
}

//! Parse a head aggregate element.
auto parse_hd_aggr_elem(ParserState &state) -> std::optional<HdLitAggregateElement> {
    if (auto tuple = state.separated_until(parse_term, TokenType::comma, TokenType::colon)) {
        auto loc = state.loc();
        state.consume();
        if (auto lit = parse_literal(state)) {
            if (state.token() != TokenType::colon) {
                loc += location(*lit);
                return HdLitAggregateElement{std::move(loc), *std::move(tuple), *std::move(lit), {}};
            }
            loc += state.cursor_pos();
            state.consume();
            if (auto cond = state.separated_until(parse_literal, TokenType::comma, TokenType::sem, TokenType::rbrace)) {
                if (!cond->empty()) {
                    loc += location(cond->back());
                }
                return HdLitAggregateElement{std::move(loc), *std::move(tuple), *std::move(lit), *std::move(cond)};
            }
        }
    }
    return std::nullopt;
}

//! Parse an optional rhs guard.
//!
//! @todo generic
auto parse_rguard(ParserState &state) -> std::optional<RGuard> {
    auto rguard = RGuard{};
    if (auto rel = check_relation(state.token())) {
        state.consume();
        auto term = parse_term(state);
        if (!term) {
            return std::nullopt;
        }
        rguard.emplace(*rel, *std::move(term));
    } else if (check_term(state.token())) {
        auto term = parse_term(state);
        if (!term) {
            return std::nullopt;
        }
        rguard.emplace(Relation::less_equal, *std::move(term));
    }
    return rguard;
}

//! Continue parsing a head aggregate.
auto cont_hd_aggregate(ParserState &state, Position pos, LGuard lguard, AggregateFunction fun) -> std::optional<HdLit> {
    if (auto elems = state.delimited(TokenType::lbrace, parse_hd_aggr_elem, TokenType::sem, TokenType::rbrace)) {
        auto end = state.cursor_pos();
        state.consume();
        if (auto rguard = parse_rguard(state)) {
            if (*rguard) {
                end = location((*rguard)->second).end();
            }
            return HdLitAggregate{std::move(pos) + std::move(end), std::move(lguard), fun, *std::move(elems),
                                  *std::move(rguard)};
        }
    }
    return std::nullopt;
}

//! Parse a head aggregate element.
//!
//! @todo: generic
auto parse_set_aggr_elem(ParserState &state) -> std::optional<SetAggregateElement> {
    if (auto lit = parse_literal(state)) {
        if (state.token() != TokenType::colon) {
            auto loc = location(*lit);
            return SetAggregateElement{std::move(loc), *std::move(lit), {}};
        }
        auto loc = location(*lit) + state.cursor_pos();
        state.consume();
        if (auto cond = state.separated_until(parse_literal, TokenType::comma, TokenType::sem, TokenType::rbrace)) {
            if (!cond->empty()) {
                loc += location(cond->back());
            }
            return SetAggregateElement{std::move(loc), *std::move(lit), *std::move(cond)};
        }
    }
    return std::nullopt;
}

//! Continue parsing a head aggregate.
//!
//! @todo: almost generic
auto cont_hd_set_aggregate(ParserState &state, Position pos, LGuard lguard) -> std::optional<HdLit> {
    if (auto elems = state.delimited(TokenType::lbrace, parse_set_aggr_elem, TokenType::sem, TokenType::rbrace)) {
        auto end = state.cursor_pos();
        state.consume();
        if (auto rguard = parse_rguard(state)) {
            if (*rguard) {
                end = location((*rguard)->second).end();
            }
            return HdLitSetAggregate{std::move(pos) + std::move(end), std::move(lguard), *std::move(elems),
                                     *std::move(rguard)};
        }
    }
    return std::nullopt;
}

//! @todo: generic
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

} // namespace

auto parse_head_literal(ParserState &state) -> std::optional<HdLit> {
    auto pos = state.token_pos();
    auto sign = parse_sign(state);

    if (sign != Sign::none) {
        if (auto term = parse_term(state)) {
            if (auto lit = cont_literal(state, std::move(pos), sign, *std::move(term))) {
                return cont_disjunction(state, *std::move(lit));
            }
        }
        return std::nullopt;
    }

    // handle atoms or guards of aggregates
    if (check_term(state.token())) {
        auto term = parse_term(state);
        if (!term) {
            return std::nullopt;
        }
        // handle aggregates
        if (auto fun = check_aggregate(state.token())) {
            state.consume();
            return cont_hd_aggregate(state, pos, LGuard{std::in_place, *std::move(term), Relation::less_equal}, *fun);
        }
        // handle set aggregates
        if (state.token() == TokenType::lbrace) {
            return cont_hd_set_aggregate(state, pos, LGuard{std::in_place, *std::move(term), Relation::less_equal});
        }
        if (auto rel = check_relation(state.token())) {
            state.consume();
            // handle aggregates
            if (auto fun = check_aggregate(state.token())) {
                state.consume();
                return cont_hd_aggregate(state, pos, LGuard{std::in_place, *std::move(term), *rel}, *fun);
            }
            // handle set aggregates
            if (state.token() == TokenType::lbrace) {
                return cont_hd_set_aggregate(state, pos, LGuard{std::in_place, *std::move(term), *rel});
            }
            // handle comparison literals
            if (auto lit = cont_literal(state, std::move(pos), Sign::none, *std::move(term), *rel)) {
                return cont_disjunction(state, *std::move(lit));
            }
            return std::nullopt;
        }
        // handle symbolic literals/conjunctions
        if (auto lit = cont_literal(state, std::move(pos), Sign::none, *std::move(term))) {
            return cont_disjunction(state, *std::move(lit));
        }
        return std::nullopt;
    }

    // handle Boolean literals
    if (state.token() == TokenType::true_ || state.token() == TokenType::false_) {
        if (auto lit = cont_literal(state, std::move(pos), Sign::none)) {
            return cont_disjunction(state, *std::move(lit));
        }
        return std::nullopt;
    }

    // handle theory atoms
    if (state.token() == TokenType::amp) {
        // NOTE: cont_theory_atom might be a better name
        if (auto atom = parse_theory_atom(state); atom) {
            auto [name, elems, rguard, end] = *std::move(atom);
            return HdLitTheoryAtom{pos + end, name, std::move(elems), std::move(rguard)};
        }
        return std::nullopt;
    }

    // handle aggregates
    if (auto fun = check_aggregate(state.token())) {
        state.consume();
        return cont_hd_aggregate(state, pos, std::nullopt, *fun);
    }

    // handle set aggregate
    if (state.token() == TokenType::lbrace) {
        return cont_hd_set_aggregate(state, pos, std::nullopt);
    }

    return state.expected<std::nullopt>(TokenType::amp, "<aggregate-function>", "<term>");
}

} // namespace Gringo::Input::Parse
