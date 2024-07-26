#include <gringo/util/string.hh>

#include "parser_state.hh"

namespace Gringo::Input::Parse {

namespace {

void cont_expr(ParserState &state) {
    if (state.token() != TokenType::theory_op) {
        state.pop();
    }
}

auto finish_term(ParserState &state) -> TheoryTerm {
    auto val = state.pop_value<TyTerm>();
    return TheoryTerm{std::in_place_type<TheoryTermUnparsed>,
                      Location{Position{state.file(), val.line, val.column}, state.cursor_pos()}, std::move(val.elems)};
}

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
        val.elems.emplace_back(ops, TheoryTermSymbol{Location(pos, state.cursor_pos()), state.store().tup_ref({})});
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
                throw std::logic_error("implement me: seq args");
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
                        auto &val = state.top_value<TyTerm>();
                        val.elems.emplace_back(ops, TheoryTermSymbol{state.loc(), state.store().num_ref(state.num())});
                        state.consume();
                        cont_expr(state);
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
                        auto &val = state.top_value<TyTerm>();
                        val.elems.emplace_back(ops, TheoryTermSymbol{state.loc(), SymbolStore::str_ref(state.str())});
                        state.consume();
                        cont_expr(state);
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

} // namespace Gringo::Input::Parse
