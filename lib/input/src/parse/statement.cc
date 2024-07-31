#include <gringo/input/literal.hh>

#include <gringo/input/rewrite/analyze.hh>

#include "parser_state.hh"

namespace Gringo::Input::Parse {

namespace {

//! Parse a rule body.
//!
//! Does not consume the terminating dot.
auto parse_body(ParserState &state) -> std::optional<std::vector<BdLit>> {
    auto ret = std::vector<BdLit>{};
    if (state.token() == TokenType::dot) {
        return ret;
    }
    if (auto lit = parse_body_literal(state)) {
        ret.emplace_back(*std::move(lit));
    } else {
        return std::nullopt;
    }
    while (state.token() == TokenType::sem || state.token() == TokenType::comma) {
        state.consume();
        if (auto lit = parse_body_literal(state)) {
            ret.emplace_back(*std::move(lit));
        } else {
            return std::nullopt;
        }
        if (state.token() != TokenType::dot) {
            return state.expected<std::nullopt>(TokenType::dot);
        }
    }
    return ret;
}

} // namespace

auto parse_statement(ParserState &state) -> std::optional<Stm> {
    auto loc = state.loc();
    if (state.token() == TokenType::rule) {
        // integrity constraints
        state.consume();
        if (auto body = parse_body(state)) {
            loc += state.cursor_pos();
            // consume dot
            state.consume();
            return StmRule{std::move(loc),
                           HdLit{std::in_place_type<HdLitSimple>, LitBool{state.loc(), Sign::none, false}},
                           *std::move(body)};
        }
    } else if (auto hd = parse_head_literal(state)) {
        // rules with head and body
        if (state.token() == TokenType::rule) {
            // consume :-
            state.consume();
            if (auto body = parse_body(state)) {
                loc += state.cursor_pos();
                // consume dot
                state.consume();
                return StmRule{std::move(loc), *std::move(hd), *std::move(body)};
            }
        }
        // rules with empty body
        else if (state.token() == TokenType::dot) {
            loc += state.cursor_pos();
            // consume the dot
            state.consume();
            return StmRule{std::move(loc), *std::move(hd), {}};
        }
    }
    return std::nullopt;
}

} // namespace Gringo::Input::Parse
