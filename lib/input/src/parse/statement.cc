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

auto parse_opt_body(ParserState &state, TokenType init) -> std::optional<std::vector<BdLit>> {
    if (state.token() == init) {
        state.consume();
        return parse_body(state);
    }
    if (state.token() == TokenType::dot) {
        return std::vector<BdLit>{};
    }
    return state.expected<std::nullopt>(init, TokenType::dot);
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

//! Check if the term captures a signature.
auto check_term_sig(Term const &term) -> std::optional<std::tuple<bool, String, int>> {
    auto get_name = [](Term const &x) -> std::optional<String> {
        if (auto const *y = std::get_if<TermSymbol>(&x)) {
            if (auto sym = y->value(); sym.type() == SymbolType::function && sym.args().empty()) {
                return sym.name();
            }
        }
        return std::nullopt;
    };
    auto get_arity = [](Term const &x) -> std::optional<int> {
        if (auto const *y = std::get_if<TermSymbol>(&x)) {
            if (y->value().type() == SymbolType::number) {
                return y->value().num().as_int();
            }
        }
        return std::nullopt;
    };
    auto get_bin = [](Term const &x) -> TermBinary const * {
        if (auto const *y = std::get_if<TermBinary>(&x)) {
            if (y->op() == BinaryOperator::div) {
                return y;
            }
        }
        return nullptr;
    };
    auto get_sign = [](Term const &x) -> Term const * {
        if (auto const *y = std::get_if<TermUnary>(&x)) {
            if (y->op() == UnaryOperator::negate) {
                return &y->rhs().get();
            }
        }
        return nullptr;
    };
    if (auto const *bin = get_bin(term)) {
        auto const *sub = get_sign(*bin->lhs());
        if (auto name = get_name(sub != nullptr ? *sub : *bin->lhs())) {
            if (auto arity = get_arity(*bin->rhs())) {
                return std::make_tuple(sub != nullptr, *name, *arity);
            }
        }
    }
    return std::nullopt;
}

//! Parse a show statement.
auto parse_show(ParserState &state) -> std::optional<Stm> {
    assert(state.token() == TokenType::show);
    auto loc = state.loc();
    state.consume();
    if (auto term = parse_term(state)) {
        if (state.token() == TokenType::dot) {
            loc += state.cursor_pos();
            state.consume();
            if (auto res = check_term_sig(*term)) {
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

//! Parse an optimize statement.
auto parse_optimize(ParserState &state) -> std::optional<Stm> {
    bool min = state.token() == TokenType::minimize;
    auto loc = state.loc();
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
        return StmOptimize{std::move(loc), min ? OptimizeType::minimize : OptimizeType::maximize, *std::move(elems)};
    }
    return std::nullopt;
}

//! Parse a weak constraint.
auto parse_weak(ParserState &state) -> std::optional<Stm> {
    auto loc = state.loc();
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
                    return StmWeakConstraint{std::move(loc), *std::move(body),
                                             OptimizeTuple{*std::move(weight), *std::move(prio), *std::move(terms)}};
                }
            }
        }
    }
    return std::nullopt;
}

//! Parse an integrity constraint.
auto parse_constraint(ParserState &state) -> std::optional<Stm> {
    auto loc = state.loc();
    // integrity constraints
    state.consume();
    if (auto body = parse_body(state)) {
        loc += state.cursor_pos();
        // consume dot
        state.consume();
        return StmRule{std::move(loc), HdLit{std::in_place_type<HdLitSimple>, LitBool{state.loc(), Sign::none, false}},
                       *std::move(body)};
    }
    return std::nullopt;
}

//! Parse a rule.
auto parse_rule(ParserState &state) -> std::optional<Stm> {
    if (auto hd = parse_head_literal(state)) {
        auto loc = location(*hd);
        if (auto body = parse_opt_body(state, TokenType::if_)) {
            loc += state.cursor_pos();
            // consume dot
            state.consume();
            return StmRule{std::move(loc), *std::move(hd), *std::move(body)};
        }
    }
    return std::nullopt;
}

//! Parse a defined statement.
auto parse_defined(ParserState &state) -> std::optional<Stm> {
    auto loc = state.loc();
    assert(state.token() == TokenType::defined);
    state.consume();
    bool sign = state.branch(TokenType::minus);
    if (state.token() != TokenType::id) {
        return state.expected<std::nullopt>(TokenType::id);
    }
    auto name = state.str();
    state.consume();
    if (!state.branch(TokenType::slash)) {
        return state.expected<std::nullopt>(TokenType::slash);
    }
    if (state.token() != TokenType::num) {
        return state.expected<std::nullopt>("<int>");
    }
    auto arity = state.num().as_int();
    if (!arity) {
        return state.expected<std::nullopt>("<int>");
    }
    state.consume();
    loc += state.cursor_pos();
    if (!state.branch(TokenType::dot)) {
        return state.expected<std::nullopt>(TokenType::dot);
    }
    return StmDefined{std::move(loc), sign, name, *arity};
}

//! Parse a project statement.
auto parse_project(ParserState &state) -> std::optional<Stm> {
    assert(state.token() == TokenType::project);
    auto loc = state.loc();
    // consume #project
    state.consume();
    auto loc_sign = state.loc();
    // optinal minus
    bool sign = state.branch(TokenType::minus);
    auto loc_atom = state.loc();
    if (state.token() != TokenType::id) {
        return state.expected<std::nullopt>(TokenType::id);
    }
    auto name = state.str();
    loc_sign += state.cursor_pos();
    loc_atom += loc_sign;
    // consume name
    state.consume();
    // signature
    if (state.branch(TokenType::slash)) {
        if (state.token() != TokenType::num) {
            return state.expected<std::nullopt>("<int>");
        }
        auto arity = state.num().as_int();
        if (!arity) {
            return state.expected<std::nullopt>("<int>");
        }
        // consume num
        state.consume();
        loc += state.cursor_pos();
        // consume dot
        if (!state.branch(TokenType::dot)) {
            return state.expected<std::nullopt>(TokenType::dot);
        }
        return StmProjectSig{loc, sign, name, *arity};
    }
    if (!state.expect(TokenType::slash, TokenType::dot, TokenType::lpar)) {
        return std::nullopt;
    }
    // no arguments
    auto atom = std::optional<Term>{};
    if (state.token() == TokenType::lpar) {
        auto args = parse_args(state);
        if (!args) {
            return std::nullopt;
        }
        loc_sign += state.cursor_pos();
        loc_atom += loc_sign;
        state.consume();
        atom.emplace(std::in_place_type<TermFunction>, std::move(loc_atom), name, *std::move(args), false);
        if (sign) {
            atom = Term{std::in_place_type<TermUnary>, std::move(loc_sign), UnaryOperator::negate, *std::move(atom)};
        }
    } else {
        atom.emplace(std::in_place_type<TermSymbol>, loc_sign, state.store().fun_ref(name, {}, sign));
    }
    auto body = parse_opt_body(state, TokenType::colon);
    if (!body) {
        return std::nullopt;
    }
    loc += state.cursor_pos();
    // consume dot
    state.consume();
    return StmProject{loc, *std::move(atom), *std::move(body)};
}

auto parse_single_edge(ParserState &state) -> std::optional<Edge> {
    if (auto u = parse_term(state)) {
        if (state.branch(TokenType::comma)) {
            if (auto v = parse_term(state)) {
                return Edge{*std::move(u), *std::move(v)};
            }
        }
    }
    return std::nullopt;
}

//! Parse an edge statement.
auto parse_edge(ParserState &state) -> std::optional<Stm> {
    assert(state.token() == TokenType::edge);
    auto loc = state.loc();
    state.consume();
    if (auto edges = state.delimited(TokenType::lpar, parse_single_edge, TokenType::sem, TokenType::rpar)) {
        loc += state.cursor_pos();
        state.consume();
        if (auto body = parse_opt_body(state, TokenType::colon)) {
            loc += state.cursor_pos();
            // consume dot
            state.consume();
            return StmEdge{std::move(loc), *std::move(edges), *std::move(body)};
        }
    }
    return std::nullopt;
}

//! Parse a signed atom.
auto parse_atom(ParserState &state) -> std::optional<Term> {
    auto loc_sign = state.loc();
    // optional minus
    bool sign = state.branch(TokenType::minus);
    auto loc_atom = state.loc();
    if (state.expect(TokenType::id)) {
        auto name = state.str();
        loc_sign += state.cursor_pos();
        loc_atom += loc_sign;
        // consume name
        state.consume();
        // arguments
        if (state.token() != TokenType::lpar) {
            return std::make_optional<Term>(std::in_place_type<TermSymbol>, loc_sign,
                                            state.store().fun_ref(name, {}, sign));
        }

        if (auto args = parse_args(state)) {
            loc_sign += state.cursor_pos();
            loc_atom += loc_sign;
            // consume closing paren
            state.consume();
            auto atom = std::optional<Term>{};
            atom.emplace(std::in_place_type<TermFunction>, std::move(loc_atom), name, *std::move(args), false);
            if (sign) {
                atom =
                    Term{std::in_place_type<TermUnary>, std::move(loc_sign), UnaryOperator::negate, *std::move(atom)};
            }
            return atom;
        }
    }
    return std::nullopt;
}

//! Parse a heuristic statement.
auto parse_heuristic(ParserState &state) -> std::optional<Stm> {
    assert(state.token() == TokenType::heuristic);
    auto loc = state.loc();
    // consume #heuristic
    state.consume();
    if (auto atom = parse_atom(state)) {
        if (auto body = parse_opt_body(state, TokenType::colon)) {
            // consume dot
            state.consume();
            // tuple
            if (state.expect(TokenType::lbrack)) {
                state.consume();
                if (auto weight = parse_term(state)) {
                    if (auto prio = parse_prio(state)) {
                        if (state.expect(TokenType::comma)) {
                            state.consume();
                            if (auto type = parse_term(state)) {
                                if (state.expect(TokenType::rbrack)) {
                                    loc += location(*type);
                                    state.consume();
                                    return StmHeuristic{std::move(loc),     *std::move(atom), *std::move(body),
                                                        *std::move(weight), *std::move(prio), *std::move(type)};
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return std::nullopt;
}

//! Parse an external statement.
auto parse_external(ParserState &state) -> std::optional<Stm> {
    assert(state.token() == TokenType::external);
    auto loc = state.loc();
    // consume #heuristic
    state.consume();
    if (auto atom = parse_atom(state)) {
        if (auto body = parse_opt_body(state, TokenType::colon)) {
            // consume dot
            state.consume();
            // tuple
            if (state.branch(TokenType::lbrack)) {
                if (auto type = parse_term(state)) {
                    if (state.expect(TokenType::rbrack)) {
                        loc += location(*type);
                        state.consume();
                        // NOLINTNEXTLINE(bugprone-optional-value-conversion)
                        return StmExternal{std::move(loc), *std::move(atom), *std::move(body), *std::move(type)};
                    }
                }
            } else {
                return StmExternal{std::move(loc), *std::move(atom), *std::move(body)};
            }
        }
    }
    return std::nullopt;
}

//! Parse a script statement.
auto parse_script(ParserState &state) -> std::optional<Stm> {
    assert(state.token() == TokenType::script);
    auto loc = state.loc();
    // consume #script
    state.consume();
    if (state.expect(TokenType::lpar)) {
        // consume opening parenthesis
        state.consume();
        if (state.expect(TokenType::id)) {
            auto type = state.str();
            // consume id
            state.consume();
            if (state.expect(TokenType::rpar)) {
                // consume closing parenthesis
                state.consume(Condition::script);
                if (state.expect(TokenType::script_content)) {
                    auto value = state.str();
                    // consume content
                    state.consume();
                    if (state.expect(TokenType::script_end)) {
                        state.consume();
                        if (state.expect(TokenType::dot)) {
                            loc += state.cursor_pos();
                            // consume dot
                            state.consume();
                            return StmScript{std::move(loc), type, value};
                        }
                    }
                }
            }
        }
    }
    return std::nullopt;
}

//! Parse a include statement.
auto parse_include(ParserState &state) -> std::optional<Stm> {
    assert(state.token() == TokenType::include);
    auto loc = state.loc();
    state.consume(Condition::include);
    if (state.expect(TokenType::str, TokenType::str_include)) {
        auto sys = state.token() == TokenType::str;
        auto value = state.str();
        state.consume();
        if (state.expect(TokenType::dot)) {
            loc += state.cursor_pos();
            state.consume();
            return StmInclude{std::move(loc), sys ? IncludeType::system : IncludeType::inbuild, value};
        }
    }
    return std::nullopt;
}

//! Parse a const statement.
auto parse_const(ParserState &state) -> std::optional<Stm> {
    static_cast<void>(state);
    throw std::runtime_error("implement me: const");
}

//! Parse a program statement.
auto parse_program(ParserState &state) -> std::optional<Stm> {
    static_cast<void>(state);
    throw std::runtime_error("implement me: program");
}

//! Parse a theory statement.
auto parse_theory(ParserState &state) -> std::optional<Stm> {
    static_cast<void>(state);
    throw std::runtime_error("implement me: theory");
}

} // namespace

auto parse_args(ParserState &state) -> std::optional<PoolArray> {
    assert(state.token() == TokenType::lpar);
    // consume the opening parenthesis
    state.consume();
    // parse arguments
    auto args = std::vector<ArgumentTuple>{};
    std::vector<Argument> tup;
    while (true) {
        if (state.token() == TokenType::rpar) {
            args.emplace_back(std::move(tup));
            return args;
        }
        if (state.token() == TokenType::sem) {
            state.consume();
            args.emplace_back(std::move(tup));
            tup.clear();
            continue;
        }
        if (state.token() != TokenType::star && !check_term(state.token())) {
            return state.expected<std::nullopt>(TokenType::star, "<term>");
        }
        if (state.token() == TokenType::star) {
            tup.emplace_back(Projection{state.loc()});
            state.consume();
        } else if (auto term = parse_term(state); term) {
            tup.emplace_back(*std::move(term));
        } else {
            return std::nullopt;
        }
        if (state.token() == TokenType::comma) {
            state.consume();
            if (state.token() == TokenType::rpar || state.token() == TokenType::sem) {
                return state.expected<std::nullopt>("<term>");
            }
        } else {
            if (state.token() != TokenType::rpar && state.token() != TokenType::sem) {
                return state.expected<std::nullopt>(TokenType::rpar, TokenType::sem);
            }
        }
    }
}

auto parse_statement(ParserState &state) -> std::optional<Stm> {
    switch (state.token()) {
        case TokenType::if_: {
            return parse_constraint(state);
        }
        case TokenType::wif: {
            return parse_weak(state);
        }
        case TokenType::minimize:
        case TokenType::maximize: {
            return parse_optimize(state);
        }
        case TokenType::show: {
            return parse_show(state);
        }
        case TokenType::const_: {
            return parse_const(state);
        }
        case TokenType::defined: {
            return parse_defined(state);
        }
        case TokenType::project: {
            return parse_project(state);
        }
        case TokenType::edge: {
            return parse_edge(state);
        }
        case TokenType::external: {
            return parse_external(state);
        }
        case TokenType::heuristic: {
            return parse_heuristic(state);
        }
        case TokenType::script: {
            return parse_script(state);
        }
        case TokenType::include: {
            return parse_include(state);
        }
        case TokenType::program: {
            return parse_program(state);
        }
        case TokenType::theory: {
            return parse_theory(state);
        }
        default: {
            // TODO: better error message possible by considering the lookahead tokens
            return parse_rule(state);
        }
    }
}

} // namespace Gringo::Input::Parse
