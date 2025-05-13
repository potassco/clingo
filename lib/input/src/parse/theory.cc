#include <clingo/util/string.hh>

#include "parser_state.hh"

namespace CppClingo::Input::Parse {

namespace {

//! Get the closing token for the given tuple type.
auto map_close(TheoryTermTupleType type) {
    switch (type) {
        case TheoryTermTupleType::list: {
            return TokenType::rbrack;
        }
        case TheoryTermTupleType::set: {
            return TokenType::rbrace;
        }
        case TheoryTermTupleType::tuple: {
            return TokenType::rpar;
        }
    }
    Util::unreachable();
}

//! Continue parsing a theory term if followed by an operator.
void cont_expr(ParserState &state) {
    if (state.token() != TokenType::theory_op) {
        state.pop();
    }
}

//! Construct an unparsed theory term.
auto finish_term(ParserState &state) -> TheoryTerm {
    auto val = state.pop_value<TyTerm>();
    return TheoryTerm{std::in_place_type<TheoryTermUnparsed>,
                      Location{Position{state.file(), val.line, val.column}, state.cursor_pos()}, std::move(val.elems)};
}

//! Continue parsing a theory sequence.
auto cont_seq(ParserState &state, StringVec ops, TheoryTermTupleType type, TokenType close) -> bool {
    auto pos = state.token_pos();
    state.consume();
    // handle trailing comma
    if (type == TheoryTermTupleType::tuple && state.token() == TokenType::comma) {
        state.consume();
        if (state.token() != close) {
            return state.expected(close);
        }
    }
    // handle empty tuple
    if (state.token() == close) {
        auto &val = state.top_value<TyTerm>();
        val.elems.emplace_back(ops, TheoryTermTuple{Location(pos, state.cursor_pos()), type, {}});
        state.consume();
        cont_expr(state);
    }
    // handle elements
    else {
        state.push_value<TySeq>(pos.line(), pos.column(), ops, type);
        state.push_value<TyTerm>(state.token_line(), state.token_column());
        state.push(Prod::ty_seq);
        state.push(Prod::ty_term);
    }
    return true;
}

//! Continue parsing the arguments of a theory sequence.
auto cont_seq_args(ParserState &state) -> bool {
    // add current term to arguments
    auto &val = state.top_value<TySeq>(1);
    val.args.emplace_back(finish_term(state));
    // handle next argument
    if (state.token() == TokenType::comma) {
        state.consume();
        // handle trailing comma of tuples
        if (val.type != TheoryTermTupleType::tuple || state.token() != TokenType::rpar) {
            state.push(Prod::ty_term);
            state.push_value<TyTerm>(state.token_line(), state.token_column());
            return true;
        }
        val.tuple = true;
    }
    // finish the tuple
    if (state.token() == map_close(val.type)) {
        auto val = state.pop_value<TySeq>();
        auto &top = state.top_value<TyTerm>();
        if (val.args.size() == 1 && !val.tuple) {
            top.elems.emplace_back(std::move(val.ops), std::move(val.args.front()));
        } else {
            top.elems.emplace_back(
                std::move(val.ops),
                TheoryTermTuple{Location{Position{state.file(), val.line, val.column}, state.cursor_pos()}, val.type,
                                std::move(val.args)});
        }
        state.pop();
        state.consume();
        cont_expr(state);
        return true;
    }
    return state.expected(TokenType::comma, TokenType::rpar);
}

//! Continue parsing a theory function.
void cont_fun(ParserState &state, StringVec ops) {
    auto loc = state.loc();
    auto name = state.str();
    state.consume();
    // handle identifier
    if (state.token() != TokenType::lpar) {
        auto &val = state.top_value<TyTerm>();
        val.elems.emplace_back(ops, TheoryTermSymbol{std::move(loc), state.store().fun_ref(name, {}, false)});
        cont_expr(state);
        return;
    }
    state.consume();
    // handle empty arguments
    if (state.token() == TokenType::rpar) {
        auto &val = state.top_value<TyTerm>();
        val.elems.emplace_back(
            ops, TheoryTermSymbol{Location(loc.begin(), state.cursor_pos()), state.store().fun_ref(name, {}, false)});
        state.consume();
        cont_expr(state);
        return;
    }
    // handle arguments
    state.push_value<TyFun>(loc.begin().line(), loc.begin().column(), name, std::move(ops));
    state.push_value<TyTerm>(state.token_line(), state.token_column());
    state.push(Prod::ty_fun);
    state.push(Prod::ty_term);
}

//! Continue parsing the arguments of a theory function.
auto cont_fun_args(ParserState &state) -> bool {
    // add current term to arguments
    auto &val = state.top_value<TyFun>(1);
    val.args.emplace_back(finish_term(state));
    // handle next argument
    if (state.token() == TokenType::comma) {
        state.consume();
        state.push(Prod::ty_term);
        state.push_value<TyTerm>(state.token_line(), state.token_column());
        return true;
    }
    // finish the function
    if (state.token() == TokenType::rpar) {
        auto val = state.pop_value<TyFun>();
        state.top_value<TyTerm>().elems.emplace_back(
            std::move(val.ops),
            TheoryTermFunction{Location{Position{state.file(), val.line, val.column}, state.cursor_pos()}, val.name,
                               std::move(val.args)});
        state.pop();
        state.consume();
        cont_expr(state);
        return true;
    }
    return state.expected(TokenType::comma, TokenType::rpar);
}

auto parse_theory_element(ParserState &state) -> std::optional<TheoryElement> {
    if (state.token() == TokenType::sem || state.token() == TokenType::rbrace) {
        return state.expected<std::nullopt>(TokenType::colon, "<theory-term>");
    }
    if (auto tuple = state.separated_until(parse_theory_term, TokenType::comma, TokenType::colon, TokenType::sem,
                                           TokenType::rbrace)) {
        auto sc = set_condition{state, Condition::normal, Condition::theory};
        auto loc = tuple->empty() ? state.loc() : location(tuple->front()) + location(tuple->back());
        if (state.token() != TokenType::colon) {
            return TheoryElement{std::move(loc), *std::move(tuple), {}};
        }
        state.consume();
        if (auto cond = state.separated_until(parse_literal, TokenType::comma, TokenType::sem, TokenType::rbrace)) {
            if (!cond->empty()) {
                loc += location(cond->back());
            }
            return TheoryElement{std::move(loc), *std::move(tuple), *std::move(cond)};
        }
    }
    return std::nullopt;
}

void cont_sym(ParserState &state, std::vector<String> ops, Symbol sym) {
    auto &val = state.top_value<TyTerm>();
    val.elems.emplace_back(ops, TheoryTermSymbol{state.loc(), sym});
    state.consume();
    cont_expr(state);
}

} // namespace

auto parse_theory_term(ParserState &state) -> std::optional<TheoryTerm> {
    state.init(Prod::ty_term);
    state.push_value<TyTerm>(state.token_line(), state.token_column());
    while (!state.empty()) {
        switch (state.top()) {
            case Prod::ty_fun: {
                if (!cont_fun_args(state)) {
                    return std::nullopt;
                }
                continue;
            }
            case Prod::ty_seq: {
                if (!cont_seq_args(state)) {
                    return std::nullopt;
                }
                continue;
            }
            case Prod::ty_term: {
                std::vector<String> ops;
                while (state.token() == TokenType::theory_op) {
                    ops.emplace_back(state.str());
                    state.consume();
                }
                switch (state.token()) {
                    case TokenType::lpar: {
                        if (!cont_seq(state, std::move(ops), TheoryTermTupleType::tuple, TokenType::rpar)) {
                            return std::nullopt;
                        }
                        continue;
                    }
                    case TokenType::lbrack: {
                        if (!cont_seq(state, std::move(ops), TheoryTermTupleType::list, TokenType::rbrack)) {
                            return std::nullopt;
                        }
                        continue;
                    }
                    case TokenType::inf: {
                        cont_sym(state, std::move(ops), SymbolStore::inf());
                        continue;
                    }
                    case TokenType::sup: {
                        cont_sym(state, std::move(ops), SymbolStore::sup());
                        continue;
                    }
                    case TokenType::lbrace: {
                        if (!cont_seq(state, std::move(ops), TheoryTermTupleType::set, TokenType::rbrace)) {
                            return std::nullopt;
                        }
                        continue;
                    }
                    case TokenType::id: {
                        cont_fun(state, std::move(ops));
                        continue;
                    }
                    case TokenType::num: {
                        cont_sym(state, std::move(ops), state.store().num_ref(state.num()));
                        continue;
                    }
                    case TokenType::anon:
                    case TokenType::var: {
                        auto &val = state.top_value<TyTerm>();
                        val.elems.emplace_back(
                            ops, TheoryTermVariable{state.loc(), state.str(), state.token() == TokenType::anon});
                        state.consume();
                        cont_expr(state);
                        continue;
                    }
                    case TokenType::str: {
                        cont_sym(state, std::move(ops), SymbolStore::str_ref(state.str()));
                        continue;
                    }
                    default: {
                        return state.expected<std::nullopt>("<theory-term>");
                    }
                }
            }
            default: {
                Util::unreachable();
            }
        }
    }
    auto ret = finish_term(state);
    assert(state.empty_value());
    return ret;
}

auto parse_theory_atom(ParserState &state)
    -> std::optional<std::tuple<Term, TheoryElementArray, std::optional<TheoryRGuard>, Position>> {
    assert(state.token() == TokenType::amp);
    state.consume();

    // parse the name
    auto begin_name = state.token_pos();
    auto begin_sign = begin_name;
    bool sign = false;
    if (state.token() == TokenType::minus) {
        sign = true;
        state.consume();
        begin_name = state.token_pos();
    }
    if (state.token() != TokenType::id) {
        return state.expected<std::nullopt>(TokenType::id);
    }

    auto str = state.str();
    auto end = state.cursor_pos();
    state.consume(Condition::theory);

    // parse arguments
    auto args = PoolArray{};
    if (state.token() == TokenType::lpar) {
        auto oargs = parse_args(state);
        if (!oargs) {
            return std::nullopt;
        }
        end = state.cursor_pos();
        state.consume(Condition::theory);
        args = *std::move(oargs);

    } else {
        args = Util::make_immutable_array<ArgumentTuple>(ArgumentTuple{{}});
    }

    // build name
    auto name = Term{std::in_place_type<TermFunction>, Location{begin_name, end}, str, std::move(args), false};
    if (sign) {
        name = Term{std::in_place_type<TermUnary>, Location(begin_sign, end), UnaryOperator::minus, std::move(name)};
    }

    // parse the theory elements
    auto sc = set_condition{state, Condition::theory, Condition::normal};
    std::vector<TheoryElement> elems;
    if (state.token() == TokenType::lbrace) {
        if (auto res = state.delimited(TokenType::lbrace, parse_theory_element, TokenType::sem, TokenType::rbrace)) {
            elems = *std::move(res);
        }
        end = state.cursor_pos();
        state.consume();
    }

    // parse the optional right guard
    auto rguard = std::optional<TheoryRGuard>{};
    if (state.token() == TokenType::theory_op) {
        auto op = state.str();
        state.consume();
        if (auto term = parse_theory_term(state); term) {
            end = location(*term).end();
            rguard.emplace(op, *std::move(term));
        } else {
            return std::nullopt;
        }
    }

    return std::make_tuple(std::move(name), std::move(elems), std::move(rguard), std::move(end));
}

} // namespace CppClingo::Input::Parse
