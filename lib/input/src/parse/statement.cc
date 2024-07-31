#include <gringo/input/literal.hh>

#include <gringo/input/rewrite/analyze.hh>

#include <gringo/util/type_traits.hh>

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

//! Parse an optional priority.
auto parse_prio(ParserState &state) -> std::optional<std::optional<Term>> {
    if (state.token() != TokenType::at) {
        return std::make_optional<std::optional<Term>>(std::nullopt);
    }
    state.consume();
    if (auto term = parse_term(state)) {
        return term;
    }
    return std::nullopt;
}

//! Parse a condition.
auto parse_cond(ParserState &state) -> std::optional<LitArray> {
    if (state.token() == TokenType::colon) {
        // consume colon
        state.consume();
        return state.separated_until(parse_literal, TokenType::comma, TokenType::sem, TokenType::rbrace);
    }
    return LitArray{};
}

//! Parse an optimize element.
auto parse_opt_elem(ParserState &state) -> std::optional<OptimizeElement> {
    if (auto weight = parse_term(state)) {
        if (auto prio = parse_prio(state)) {
            if (auto terms = state.repeat_until(TokenType::comma, parse_term, TokenType::colon, TokenType::sem,
                                                TokenType::rbrace)) {
                if (auto cond = parse_cond(state)) {
                    return OptimizeElement{OptimizeTuple{*std::move(weight), *std::move(prio), *std::move(terms)},
                                           *std::move(cond)};
                }
            }
        }
    }
    return std::nullopt;
}

auto check_show_sig(Term const &term) -> std::optional<std::tuple<bool, String, int>> {
    auto get_name = []<class T>(T const &x) -> std::optional<String> {
        if constexpr (Util::matches<T, TermSymbol>) {
            auto sym = x.value();
            if (sym.type() == SymbolType::function && sym.args().empty()) {
                return sym.name();
            }
        }
        if constexpr (Util::matches<T, TermFunction>) {
            if (!x.external() && x.pool().size() == 1 && x.pool().front().elems().empty()) {
                return x.name();
            }
        }
        return std::nullopt;
    };
    auto get_arity = []<class T>(T const &x) -> std::optional<int> {
        if constexpr (Util::matches<T, TermSymbol>) {
            if (x.value().type() == SymbolType::number) {
                return x.value().num().as_int();
            }
        }
        return std::nullopt;
    };
    auto get_bin = [&]<class T>(T const &x) -> TermBinary const * {
        if constexpr (Util::matches<T, TermBinary>) {
            if (x.op() == BinaryOperator::div) {
                return &x;
            }
        }
        return nullptr;
    };
    auto get_sign = [&]<class T>(T const &x) -> Term const * {
        if constexpr (Util::matches<T, TermUnary>) {
            if (x.op() == UnaryOperator::negate) {
                return &x.rhs().get();
            }
        }
        return nullptr;
    };
    if (auto const *bin = std::visit(get_bin, term)) {
        auto const *sub = std::visit(get_sign, *bin->lhs());
        if (auto name = std::visit(get_name, sub != nullptr ? *sub : *bin->lhs())) {
            if (auto arity = std::visit(get_arity, *bin->rhs())) {
                return std::make_tuple(sub != nullptr, *name, *arity);
            }
        }
    }
    return std::nullopt;
}

//! Parse a show statement.
auto parse_show(ParserState &state) -> std::optional<Stm> {
    auto loc = state.loc();
    state.consume();
    if (auto term = parse_term(state)) {
        if (state.token() == TokenType::dot) {
            loc += state.cursor_pos();
            state.consume();
            if (auto res = check_show_sig(*term)) {
                auto [sign, name, arity] = *std::move(res);
                return StmShowSig{std::move(loc), sign, name, arity};
            }
            return StmShow{state.loc(), *std::move(term), {}};
        }
        if (state.token() == TokenType::colon) {
            state.consume();
            if (auto body = parse_body(state)) {
                loc += state.cursor_pos();
                state.consume();
                return StmShow{std::move(loc), *std::move(term), *std::move(body)};
            }
        } else {
            return state.expected<std::nullopt>(TokenType::dot, TokenType::colon);
        }
    }
    return std::nullopt;
}

} // namespace

auto parse_statement(ParserState &state) -> std::optional<Stm> {
    auto loc = state.loc();
    if (state.token() == TokenType::if_) {
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
    } else if (state.token() == TokenType::wif) {
        // integrity constraints
        state.consume();
        if (auto body = parse_body(state)) {
            loc += state.cursor_pos();
            // consume dot
            state.consume();
            if (state.token() != TokenType::lbrack) {
                return state.expected<std::nullopt>(TokenType::lbrack);
            }
            // consume lbrack
            state.consume();
            if (auto weight = parse_term(state)) {
                if (auto prio = parse_prio(state)) {
                    if (auto terms = state.repeat_until(TokenType::comma, parse_term, TokenType::rbrack)) {
                        loc += state.cursor_pos();
                        // consume rbrack
                        state.consume();
                        return StmWeakConstraint{
                            std::move(loc), *std::move(body),
                            OptimizeTuple{*std::move(weight), *std::move(prio), *std::move(terms)}};
                    }
                }
            }
        }
    } else if (auto min = state.token() == TokenType::minimize; min || state.token() == TokenType::maximize) {
        // consume <optimize>
        state.consume();
        if (auto elems = state.delimited(TokenType::lbrace, parse_opt_elem, TokenType::sem, TokenType::rbrace)) {
            // consume rbrace
            state.consume();
            if (state.token() != TokenType::dot) {
                return state.expected<std::nullopt>(TokenType::dot);
            }
            loc += state.cursor_pos();
            // consume dot
            state.consume();
            return StmOptimize{std::move(loc), min ? OptimizeType::minimize : OptimizeType::maximize,
                               *std::move(elems)};
        }
    } else if (state.token() == TokenType::show) {
        return parse_show(state);
    } else if (auto hd = parse_head_literal(state)) {
        // rules with head and body
        if (state.token() == TokenType::if_) {
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
