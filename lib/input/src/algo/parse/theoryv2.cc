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

} // namespace

auto parse_theory_term(ParserState &state) -> std::optional<TheoryTerm> {
    state.init(Prod::ty_term);
    state.push_value<TyTerm>(state.token_line(), state.token_column());
    while (!state.empty()) {
        switch (state.top()) {
            case Prod::ty_term: {
                std::vector<String> ops;
                while (state.token() == TokenType::theory_op) {
                    ops.emplace_back(state.str());
                    state.consume();
                }
                switch (state.token()) {
                    case TokenType::lpar: {
                        throw std::logic_error("implement me: tuple");
                    }
                    case TokenType::lbrack: {
                        throw std::logic_error("implement me: list");
                    }
                    case TokenType::lbrace: {
                        throw std::logic_error("implement me: set");
                    }
                    case TokenType::id: {
                        throw std::logic_error("implement me: fun");
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
