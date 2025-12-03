#include <clingo/input/literal.hh>

#include <clingo/input/rewrite/analyze.hh>

#include <clingo/util/type_traits.hh>

#include "parser_state.hh"

namespace CppClingo::Input::Parse {

namespace {

//! Parse a rule body.
//!
//! Does not consume the terminating dot.
auto parse_body(ParserState &state) -> std::optional<std::vector<BdLit>> {
    auto ret = std::vector<BdLit>{};
    if (state.token() == TokenType::dot) {
        state.mark_stms();
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
    }
    if (state.token() != TokenType::dot) {
        return state.expected<std::nullopt>(TokenType::dot);
    }
    return ret;
}

//! Parse an optional rule body as determined by the init token.
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
            if (y->op() == UnaryOperator::minus) {
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
    if (state.branch(TokenType::dot)) {
        return StmShowNothing{std::move(loc)};
    }
    if (auto term = parse_term(state)) {
        if (state.token() == TokenType::dot) {
            state.mark_stms();
            loc += state.cursor_pos();
            state.consume();
            if (auto res = check_term_sig(*term)) {
                auto [sign, name, arity] = *std::move(res);
                auto value = true;
                if (state.branch(TokenType::lbrack)) {
                    if (!state.expect(TokenType::id)) {
                        return std::nullopt;
                    }
                    auto id = state.str();
                    if (id == "false") {
                        value = false;
                    } else if (id != "true") {
                        return state.expected<std::nullopt>("true", "false");
                    }
                    state.consume();
                    if (!state.expect(TokenType::rbrack)) {
                        return std::nullopt;
                    }
                    loc += state.cursor_pos();
                    state.mark_stms();
                    state.consume();
                }
                return StmShowSig{std::move(loc), sign, name, arity, value};
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
        state.mark_stms();
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
                    state.mark_stms();
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
            state.mark_stms();
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
    if (state.expect(TokenType::dot)) {
        // consume dot
        state.mark_stms();
        state.consume();
        return StmDefined{std::move(loc), sign, name, *arity};
    }
    return std::nullopt;
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
        if (state.expect(TokenType::dot)) {
            // consume dot
            state.mark_stms();
            state.consume();
            return StmProjectSig{loc, sign, name, *arity};
        }
        return std::nullopt;
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
            atom = Term{std::in_place_type<TermUnary>, std::move(loc_sign), UnaryOperator::minus, *std::move(atom)};
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
    state.mark_stms();
    state.consume();
    return StmProject{loc, *std::move(atom), *std::move(body)};
}

//! Parse a single edge.
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
            state.mark_stms();
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
                atom = Term{std::in_place_type<TermUnary>, std::move(loc_sign), UnaryOperator::minus, *std::move(atom)};
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
            state.mark_stms();
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
                                    state.mark_stms();
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
            state.mark_stms();
            state.consume();
            // tuple
            if (state.branch(TokenType::lbrack)) {
                if (auto type = parse_term(state)) {
                    if (state.expect(TokenType::rbrack)) {
                        loc += location(*type);
                        state.mark_stms();
                        state.consume();
                        return StmExternal{std::move(loc), *std::move(atom), *std::move(body), std::move(type)};
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
                            state.mark_stms();
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

//! Parse an include statement.
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
            state.mark_stms();
            state.consume();
            return StmInclude{std::move(loc), sys ? IncludeType::system : IncludeType::inbuild, value};
        }
    }
    return std::nullopt;
}

//! Parse an id.
auto parse_id(ParserState &state) -> std::optional<String> {
    if (state.expect(TokenType::id)) {
        auto ret = state.str();
        state.consume();
        return ret;
    }
    return std::nullopt;
}

//! Parse an id vector enclosed in parentheses.
auto parse_args_id(ParserState &state) -> std::optional<std::vector<String>> {
    if (state.token() == TokenType::lpar) {
        if (auto args = state.delimited(TokenType::lpar, parse_id, TokenType::comma, TokenType::rpar)) {
            state.consume();
            return args;
        }
    }
    return std::vector<String>{};
}

//! Parse a program statement.
auto parse_program(ParserState &state) -> std::optional<Stm> {
    assert(state.token() == TokenType::program);
    auto loc = state.loc();
    state.consume();
    if (state.expect(TokenType::id)) {
        auto name = state.str();
        state.consume();
        if (auto args = parse_args_id(state)) {
            if (state.expect(TokenType::dot)) {
                loc += state.cursor_pos();
                state.mark_stms();
                state.consume();
                return StmProgram{std::move(loc), name, *args};
            }
        }
    }
    return std::nullopt;
}

//! Parse the const type.
//!
//! Advances the location to the end of the tuple.
auto parse_option_value_type(ParserState &state, Location &loc) -> std::optional<Precedence> {
    if (state.token() == TokenType::lbrack) {
        state.consume();
        auto type = Precedence::default_;
        if (state.token() == TokenType::id && state.view() == "override") {
            type = Precedence::override_;
        } else if (state.token() != TokenType::id || state.view() != "default") {
            return state.expected<std::nullopt>("default", "override");
        }
        state.consume();
        if (state.expect(TokenType::rbrack)) {
            loc += state.cursor_pos();
            state.mark_stms();
            state.consume();
            return type;
        }
    }
    return Precedence::default_;
}

//! Parse a const statement.
auto parse_const(ParserState &state) -> std::optional<Stm> {
    assert(state.token() == TokenType::const_);
    auto loc = state.loc();
    state.consume();
    if (state.expect(TokenType::id)) {
        auto name = state.str();
        state.consume();
        if (state.expect(TokenType::eq)) {
            state.consume();
            if (auto term = parse_term(state)) {
                if (state.expect(TokenType::dot)) {
                    loc += state.cursor_pos();
                    state.mark_stms();
                    state.consume();
                    if (auto type = parse_option_value_type(state, loc)) {
                        return StmConst{std::move(loc), *type, name, *std::move(term)};
                    }
                }
            }
        }
    }
    return std::nullopt;
}

//! Parse a parts statement.
auto parse_parts(ParserState &state) -> std::optional<Stm> {
    assert(state.token() == TokenType::parts);
    auto loc = state.loc();
    state.consume();
    if (auto parts = parse_program_parts(state, TokenType::dot)) {
        if (state.expect(TokenType::dot)) {
            loc += state.cursor_pos();
            state.mark_stms();
            state.consume();
            if (auto type = parse_option_value_type(state, loc)) {
                return StmParts{loc, *type, *std::move(parts)};
            }
        }
    }
    return std::nullopt;
}

//! The type of a theory operator.
//!
//! Does not consume the last token.
auto parse_op_type(ParserState &state) -> std::optional<TheoryOpType> {
    if (state.token() == TokenType::id) {
        auto view = state.view();
        if (view == "unary") {
            return TheoryOpType::unary;
        }
        if (view == "binary") {
            state.consume();
            if (state.expect(TokenType::comma)) {
                state.consume();
                if (state.token() == TokenType::id) {
                    auto view = state.view();
                    if (view == "left") {
                        return TheoryOpType::binary_left;
                    }
                    if (view == "right") {
                        return TheoryOpType::binary_right;
                    }
                }
                return state.expected<std::nullopt>("'left'", "'right'");
            }
            return std::nullopt;
        }
    }
    return state.expected<std::nullopt>("'unary'", "'binary'");
}

//! Parse an int.
auto parse_int(ParserState &state) -> std::optional<int> {
    if (state.expect(TokenType::num)) {
        if (auto num = state.num().as_int()) {
            state.consume();
            return num;
        }
        return state.expected<std::nullopt>("<int32>");
    }
    return std::nullopt;
}

//! Parse a theory operator definition.
auto parse_op_def(ParserState &state) -> std::optional<TheoryOpDefinition> {
    if (state.expect(TokenType::theory_op)) {
        auto loc = state.loc();
        auto op = state.str();
        state.consume();
        if (state.expect(TokenType::colon)) {
            state.consume();
            if (auto prio = parse_int(state)) {
                if (state.expect(TokenType::comma)) {
                    state.consume();
                    if (auto type = parse_op_type(state)) {
                        loc += state.cursor_pos();
                        state.consume();
                        return TheoryOpDefinition{loc, op, *prio, *type};
                    }
                }
            }
        }
    }
    return std::nullopt;
}

//! The a theory term definition.
auto parse_term_def(ParserState &state) -> std::optional<TheoryTermDefinition> {
    assert(state.token() == TokenType::id);
    auto loc = state.loc();
    auto name = state.str();
    state.consume();
    auto sc = set_condition{state, Condition::theory, Condition::normal};
    if (auto op_defs = state.delimited(TokenType::lbrace, parse_op_def, TokenType::sem, TokenType::rbrace)) {
        loc += state.cursor_pos();
        state.consume();
        return TheoryTermDefinition{std::move(loc), name, *std::move(op_defs)};
    }
    return std::nullopt;
}

//! Parse a single operator.
auto parse_op(ParserState &state) -> std::optional<String> {
    if (state.expect(TokenType::theory_op)) {
        auto op = state.str();
        state.consume();
        return op;
    }
    return std::nullopt;
}

//! The a theory guard definition.
auto parse_guard_def(ParserState &state) -> std::optional<std::optional<TheoryRGuardDefinition>> {
    if (state.token() == TokenType::lbrace) {
        auto pt = [](ParserState &state) {
            auto sc = set_condition{state, Condition::theory, Condition::normal};
            return state.delimited(TokenType::lbrace, parse_op, TokenType::comma, TokenType::rbrace);
        };
        if (auto ops = pt(state)) {
            state.consume();
            if (state.expect(TokenType::comma)) {
                state.consume();
                if (state.expect(TokenType::id)) {
                    auto str = state.str();
                    state.consume();
                    if (state.expect(TokenType::comma)) {
                        state.consume();
                        return std::make_optional<TheoryRGuardDefinition>(*ops, str);
                    }
                }
            }
        }
        return std::nullopt;
    }
    return std::optional<TheoryRGuardDefinition>{};
}

//! The a theory atom definition.
auto parse_atom_def(ParserState &state) -> std::optional<TheoryAtomDefinition> {
    assert(state.token() == TokenType::amp);
    auto loc = state.loc();
    state.consume();
    if (state.expect(TokenType::id)) {
        auto name = state.str();
        state.consume();
        if (state.expect(TokenType::slash)) {
            state.consume();
            if (auto num = parse_int(state)) {
                if (state.expect(TokenType::colon)) {
                    state.consume();
                    if (state.expect(TokenType::id)) {
                        auto term = state.str();
                        state.consume();
                        if (state.expect(TokenType::comma)) {
                            state.consume();
                            if (auto guard = parse_guard_def(state)) {
                                if (state.expect(TokenType::id)) {
                                    auto view = state.view();
                                    auto type = TheoryAtomType::any;
                                    if (view == "head") {
                                        type = TheoryAtomType::head;
                                    } else if (view == "body") {
                                        type = TheoryAtomType::body;
                                    } else if (view == "directive") {
                                        type = TheoryAtomType::directive;
                                    } else if (view != "any") {
                                        return state.expected<std::nullopt>("'head'", "'body'", "'directive'", "'any'");
                                    }
                                    loc += state.cursor_pos();
                                    state.consume();
                                    return TheoryAtomDefinition{loc, name, *num, term, *std::move(guard), type};
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

//! Parse a theory term definition followed by a semicolon or closing brace.
//!
//! The semicolon is consumed but not the brace.
auto parse_term_def_sep(ParserState &state) -> std::optional<TheoryTermDefinition> {
    assert(state.token() == TokenType::id);
    if (auto term_def = parse_term_def(state)) {
        if (state.expect(TokenType::sem, TokenType::rbrace)) {
            if (state.token() == TokenType::sem) {
                state.consume();
            }
            return term_def;
        }
    }
    return std::nullopt;
};

//! Parse a theory statement.
auto parse_theory(ParserState &state) -> std::optional<Stm> {
    assert(state.token() == TokenType::theory);
    auto loc = state.loc();
    state.consume();
    if (state.expect(TokenType::id)) {
        auto name = state.str();
        state.consume();
        if (state.expect(TokenType::lbrace)) {
            state.consume();
            if (auto term_defs = state.while_token(parse_term_def_sep, TokenType::id)) {
                if (state.token() == TokenType::sem) {
                    state.consume();
                }
                if (auto atom_defs = state.separated_until(parse_atom_def, TokenType::sem, TokenType::rbrace)) {
                    state.consume();
                    if (state.expect(TokenType::dot)) {
                        loc += state.cursor_pos();
                        state.mark_stms();
                        state.consume();
                        return StmTheory{std::move(loc), name, *std::move(term_defs), *std::move(atom_defs)};
                    }
                }
            }
        }
    }
    return std::nullopt;
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
        case TokenType::parts: {
            return parse_parts(state);
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
            // However, a statement can start with a lot of different tokens...
            return parse_rule(state);
        }
    }
}

auto recover(ParserState &state) -> bool {
    bool brack = false;
    bool dot = false;
    while (true) {
        switch (state.token()) {
            case TokenType::dot: {
                state.consume();
                dot = true;
                continue;
            }
            case TokenType::lbrack: {
                state.consume();
                brack = true;
                continue;
            }
            case TokenType::error_bc: {
                return false;
            }
            case TokenType::error: {
                state.consume();
                continue;
            }
            case TokenType::end: {
                return false;
            }
            case TokenType::rbrack: {
                state.consume();
                brack = false;
                continue;
            }
            default: {
                if (brack || !dot) {
                    state.consume();
                    continue;
                }
                return true;
            }
        }
    }
}

auto scan_statement(ParserState &state) -> std::pair<std::optional<Stm>, bool> {
    assert(!state.has_stms());
    // parse statement
    if (state.token() == TokenType::end) {
        return {std::nullopt, true};
    }
    if (auto stm = parse_statement(state)) {
        return {std::move(stm), true};
    }
    // error recovery
    while (true) {
        if (!recover(state)) {
            return {std::nullopt, false};
        }
        if (auto stm = parse_statement(state)) {
            return {std::move(stm), false};
        }
    }
}

} // namespace CppClingo::Input::Parse
