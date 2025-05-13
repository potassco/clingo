#include <clingo/input/literal.hh>

#include <clingo/input/rewrite/analyze.hh>

#include "parser_state.hh"

namespace CppClingo::Input::Parse {

namespace {

//! Continue parsing a disjunction.
auto cont_disjunction(ParserState &state, HdLitDisjunctionElement elem) -> std::optional<HdLit> {
    // the disjunction consists of at least one element
    auto elems = std::vector<HdLitDisjunctionElement>{};
    elems.emplace_back(std::move(elem));
    // disjunction elements can also be separated by comma
    // if the first element is a plain literal without condition
    while (state.token() == TokenType::sem || state.token() == TokenType::bar || state.token() == TokenType::comma) {
        // consume the separator
        state.consume();
        // parse the literal
        auto lit = parse_literal(state);
        if (!lit) {
            return std::nullopt;
        }
        if (state.token() == TokenType::colon) {
            // parse the optional condition
            auto loc = location(*lit).begin() + state.cursor_pos();
            state.consume();
            auto cond = state.separated_until(parse_literal, TokenType::comma, TokenType::sem, TokenType::dot,
                                              TokenType::bar, TokenType::end, TokenType::if_);
            if (!cond) {
                return std::nullopt;
            }
            if (!cond->empty()) {
                loc += location(cond->back()).end();
            }
            elems.emplace_back(CondLit{std::move(loc), *std::move(lit), *std::move(cond)});
        } else {
            // append a plain literal
            elems.emplace_back(*std::move(lit));
        }
    }
    // assemble the disjunction from the elements
    auto loc = location(elems.front()) + location(elems.back());
    return HdLitDisjunction{std::move(loc), std::move(elems)};
}

//! Continue parsing a disjunction.
auto cont_disjunction(ParserState &state, Lit lit) -> std::optional<HdLit> {
    if (state.token() == TokenType::colon) {
        // parse the optional condition
        auto end = state.cursor_pos();
        state.consume();
        if (auto cond = state.separated_until(parse_literal, TokenType::comma, TokenType::sem, TokenType::dot,
                                              TokenType::bar, TokenType::end, TokenType::if_)) {
            auto loc = location(lit).begin() + (cond->empty() ? end : location(cond->back()).end());
            // continue parsing with the just parsed disjunction element
            return cont_disjunction(state, CondLit{std::move(loc), std::move(lit), *std::move(cond)});
        }
        return std::nullopt;
    }
    // only continue parsing if there is a condition
    if (state.token() == TokenType::sem || state.token() == TokenType::bar || state.token() == TokenType::comma) {
        return cont_disjunction(state, HdLitDisjunctionElement{std::move(lit)});
    }
    // return a plain literal if there is no condition
    return HdLitSimple{std::move(lit)};
}

//! Parse a head aggregate element.
auto parse_hd_aggr_elem(ParserState &state) -> std::optional<HdLitAggregateElement> {
    // parse the tuple
    if (auto tuple = state.separated_until(parse_term, TokenType::comma, TokenType::colon)) {
        auto loc = state.loc();
        // consume the colon
        state.consume();
        // parse the literal
        if (auto lit = parse_literal(state)) {
            // handle the empty condition
            if (state.token() != TokenType::colon) {
                loc += location(*lit);
                return HdLitAggregateElement{std::move(loc), *std::move(tuple), *std::move(lit), {}};
            }
            loc += state.cursor_pos();
            // consume the colon
            state.consume();
            // parse the condition
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

//! Continue parsing a head aggregate.
auto cont_hd_aggregate(ParserState &state, Position pos, LGuard lguard, AggregateFunction fun) -> std::optional<HdLit> {
    // parse the elements
    if (auto elems = state.delimited(TokenType::lbrace, parse_hd_aggr_elem, TokenType::sem, TokenType::rbrace)) {
        auto end = state.cursor_pos();
        // consume the closing brace
        state.consume();
        // parse the optional guard
        if (auto rguard = parse_rguard(state)) {
            if (*rguard) {
                end = location((*rguard)->second).end();
            }
            // assemble the aggregate
            return HdLitAggregate{std::move(pos) + std::move(end), std::move(lguard), fun, *std::move(elems),
                                  *std::move(rguard)};
        }
    }
    return std::nullopt;
}

//! Continue parsing a head aggregate.
auto cont_hd_set_aggregate(ParserState &state, Position pos, LGuard lguard) -> std::optional<HdLit> {
    // parse the elements
    if (auto elems = state.delimited(TokenType::lbrace, parse_set_aggr_elem, TokenType::sem, TokenType::rbrace)) {
        auto loc = std::move(pos) + state.cursor_pos();
        // consume the closing brace
        state.consume();
        // parse the optional guard
        if (auto rguard = parse_rguard(state)) {
            if (*rguard) {
                loc += location((*rguard)->second);
            }
            return HdLitSetAggregate{std::move(loc), std::move(lguard), *std::move(elems), *std::move(rguard)};
        }
    }
    return std::nullopt;
}

} // namespace

auto parse_head_literal(ParserState &state) -> std::optional<HdLit> {
    auto pos = state.token_pos();
    auto sign = parse_sign(state);

    // only literals in disjunction can have signs
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

    return state.expected<std::nullopt>(TokenType::amp, TokenType::lbrace, TokenType::true_, TokenType::false_,
                                        "<aggregate-function>", "<term>");
}

} // namespace CppClingo::Input::Parse
