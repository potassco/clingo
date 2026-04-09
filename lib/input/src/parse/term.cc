#include <clingo/input/rewrite/evaluate.hh>

#include "parser_state.hh"

namespace CppClingo::Input::Parse {

namespace {

//! Check if the given production is an arithmetic operation or interval.
auto is_op(Prod prod) -> bool {
    switch (prod) {
        case Prod::exp:
        case Prod::div:
        case Prod::mod:
        case Prod::mul:
        case Prod::sub:
        case Prod::add:
        case Prod::band:
        case Prod::bor:
        case Prod::bxor:
        case Prod::interval:
        case Prod::uminus:
        case Prod::bneg: {
            return true;
        }
        default: {
            return false;
        }
    }
};

//! Get the priority of an arithmetic operation or interval.
auto priority(Prod prod) -> int {
    // NOLINTBEGIN(readability-magic-numbers)
    switch (prod) {
        case Prod::exp: {
            return 7;
        }
        case Prod::bneg:
        case Prod::uminus: {
            return 6;
        }
        case Prod::div:
        case Prod::mod:
        case Prod::mul: {
            return 5;
        }
        case Prod::sub:
        case Prod::add: {
            return 4;
        }
        case Prod::band: {
            return 3;
        }
        case Prod::bor: {
            return 2;
        }
        case Prod::bxor: {
            return 1;
        }
        case Prod::interval: {
            return 0;
        }
        default: {
            assert(prod == Prod::interval);
            return 0;
        }
    }
    // NOLINTEND(readability-magic-numbers)
};

//! Check if the given binary operation is left associative.
auto left_assoc_(Prod prod) -> bool {
    return prod != Prod::exp;
}

//! Map the given token to a production of a binary operation if possible.
auto map_binop(TokenType token) -> std::optional<Prod> {
    switch (token) {
        case TokenType::dstar: {
            return Prod::exp;
        }
        case TokenType::slash: {
            return Prod::div;
        }
        case TokenType::bslash: {
            return Prod::mod;
        }
        case TokenType::star: {
            return Prod::mul;
        }
        case TokenType::minus: {
            return Prod::sub;
        }
        case TokenType::plus: {
            return Prod::add;
        }
        case TokenType::amp: {
            return Prod::band;
        }
        case TokenType::qmark: {
            return Prod::bor;
        }
        case TokenType::caret: {
            return Prod::bxor;
        }
        case TokenType::ddot: {
            return Prod::interval;
        }
        default: {
            return std::nullopt;
        }
    }
}

//! Map the given token to a production of a unary operation if possible.
auto map_unop(TokenType token) -> Prod {
    switch (token) {
        case TokenType::minus: {
            return Prod::uminus;
        }
        default: {
            assert(token == TokenType::tilde);
            return Prod::bneg;
        }
    }
}

auto map_binop(Prod prod) -> BinaryOperator {
    switch (prod) {
        case Prod::exp: {
            return BinaryOperator::pow;
        }
        case Prod::div: {
            return BinaryOperator::div;
        }
        case Prod::mod: {
            return BinaryOperator::mod;
        }
        case Prod::mul: {
            return BinaryOperator::times;
        }
        case Prod::sub: {
            return BinaryOperator::minus;
        }
        case Prod::add: {
            return BinaryOperator::plus;
        }
        case Prod::band: {
            return BinaryOperator::and_;
        }
        case Prod::bor: {
            return BinaryOperator::or_;
        }
        case Prod::bxor: {
            return BinaryOperator::xor_;
        }
        default: {
            assert(prod == Prod::interval);
            return BinaryOperator::dots;
        }
    }
}

auto map_unop(Prod prod) -> UnaryOperator {
    switch (prod) {
        case Prod::bneg: {
            return UnaryOperator::negate;
        }
        default: {
            assert(prod == Prod::uminus);
            return UnaryOperator::minus;
        }
    }
}

//! Continue parsing an expression if followed by a binary operation.
//!
//! Depending on the priority of the previous operator on the stack, this
//! function either shifts the next binary operation or does nothing which
//! results in a reduction in the next iteration.
void cont_expr(ParserState &state) {
    state.pop();
    if (auto cur = map_binop(state.token()); cur) {
        // reduce
        if (!state.empty() && is_op(state.top())) {
            auto pre = state.top();
            auto pp = priority(pre);
            auto pc = priority(*cur);
            if (pc < pp || (pc == pp && left_assoc_(*cur))) {
                return;
            }
        }
        // shift
        state.consume();
        state.push(*cur);
        state.push(Prod::term);
    }
}

//! Continue parsing a format string after an opening `f"` or some field has been added.
auto cont_fstr(ParserState &state) -> bool {
    auto &str = state.top_value<FStr>();
    if (state.token() == TokenType::fstring_cont) {
        if (auto view = state.view().substr(0, state.view().size() - 1); !view.empty()) {
            auto loc =
                Location{state.cursor_pos(), Position(state.file(), state.token_line(), state.token_column() - 1)};
            auto &buf = state.buf(view.size());
            Util::unquote(view, std::back_inserter(buf), true);
            str.fields.emplace_back(std::in_place_type<FormatFieldLiteral>, loc, state.store().string(buf));
        }
        state.consume();
        state.push(Prod::fstr_field);
        state.push(Prod::term);
    } else if (state.token() == TokenType::fstring_close) {
        if (auto view = state.view().substr(0, state.view().size() - 1); !view.empty()) {
            auto loc =
                Location{state.cursor_pos(), Position(state.file(), state.token_line(), state.token_column() - 1)};
            auto &buf = state.buf(view.size());
            Util::unquote(view, std::back_inserter(buf), true);
            str.fields.emplace_back(std::in_place_type<FormatFieldLiteral>, loc, state.store().string(buf));
        }
        auto fields = std::move(str.fields);
        auto start = Position{state.file(), str.line, str.column};
        state.replace_value<Term>(std::in_place_type<TermFormatString>, start + state.cursor_pos(), std::move(fields));
        state.pop();
        state.consume();
        cont_expr(state);
    } else {
        return state.expected(TokenType::fstring_cont, TokenType::fstring_spec, TokenType::fstring_close);
    }
    return true;
}

//! Continue parsing a format string after a term has been parsed.
auto cont_fstr_field(ParserState &state) -> bool {
    state.pop();
    auto term = state.pop_value<Term>();
    auto &str = state.top_value<FStr>();
    // The start is either after the f" of the f-string or directly after the last field.
    auto start =
        !str.fields.empty() ? location(str.fields.back()).end() : Position{state.file(), str.line, str.column + 2};
    if (state.token() == TokenType::rbrace) {
        str.fields.emplace_back(
            FormatFieldExpression{std::move(start) + state.cursor_pos(), std::move(term), FormatSpec{}});
        state.consume(Condition::fstring);
    } else {
        // NOTE: We unconsume the last token here to lex it differently.
        state.unconsume();
        state.consume(Condition::fstring_spec);
        if (state.token() != TokenType::fstring_spec) {
            return state.expected(TokenType::fstring_spec, TokenType::rbrace);
        }
        auto spec = FormatSpec::build(state.store(), state.view());
        if (spec) {
            state.consume();
            if (state.token() != TokenType::rbrace) {
                return state.expected(TokenType::rbrace);
            }
            str.fields.emplace_back(
                FormatFieldExpression{std::move(start) + state.cursor_pos(), std::move(term), *std::move(spec)});
            state.consume(Condition::fstring);
        } else {
            return state.expected(TokenType::fstring_spec);
        }
    }
    return true;
}

//! Continue parsing an abs term after a '|' token.
void cont_abs(ParserState &state) {
    assert(state.token() == TokenType::bar);
    state.push_value<Abs>(state.token_line(), state.token_column());
    state.consume();
    state.replace(Prod::abs);
    state.push(Prod::term);
}

//! Continue parsing arguments of an abs term.
auto cont_abs_args(ParserState &state) -> bool {
    auto &abs = state.top_value<Abs>(1);
    abs.args.emplace_back(state.pop_value<Term>());
    if (state.token() == TokenType::bar) {
        auto loc = Location{Position{state.file(), abs.line, abs.column},
                            Position{state.file(), state.cursor_line(), state.cursor_column()}};
        auto args = std::move(abs.args);
        state.replace_value<Term>(std::in_place_type<TermAbs>, std::move(loc), std::move(args));
        state.consume();
        cont_expr(state);
        return true;
    }
    if (state.token() == TokenType::sem) {
        state.consume();
        state.push(Prod::term);
        return true;
    }
    return state.expected(TokenType::bar, TokenType::sem);
}

//! Finish a tuple after reading a ')' token.
void finish_tup(ParserState &state) {
    assert(state.token() == TokenType::rpar);
    auto tup = state.pop_value<Tup>();
    tup.finish_pool();
    state.push_value<Term>(std::in_place_type<TermTuple>,
                           Location{Position{state.file(), tup.line, tup.column},
                                    Position{state.file(), state.cursor_line(), state.cursor_column()}},
                           TupleElementArray{std::move(tup.args)});
    state.consume();
    cont_expr(state);
}

//! Continue parsing tuple arguments.
//!
//! If arg is true, continue parsing a tuple after a term argument.
//! Otherwise, continue parsing a tuple after tokens '(' or ';'.
auto cont_tup_args(ParserState &state, bool arg) -> bool {
    auto &tup = state.top_value<Tup>(arg ? 1 : 0);
    if (arg) {
        tup.tup.emplace_back(state.pop_value<Term>());
    } else {
        assert(state.token() == TokenType::lpar || state.token() == TokenType::sem);
        state.consume();
    }
    while (true) {
        if (arg) {
            // closing parenthesis after term or projection
            if (state.token() == TokenType::rpar) {
                finish_tup(state);
                return true;
            }
            // comma after term or projection
            if (state.branch(TokenType::comma)) {
                tup.term = false;
                if (state.token() == TokenType::star) {
                    tup.tup.emplace_back(Projection{state.loc()});
                    state.consume();
                    continue;
                }
                if (state.token() == TokenType::rpar) {
                    finish_tup(state);
                    return true;
                }
                if (state.token() != TokenType::sem) {
                    state.push(Prod::term);
                    return true;
                }
                // semicolon after term or projection falls through
            } else if (state.token() != TokenType::sem) {
                return state.expected(TokenType::rpar, TokenType::sem, TokenType::comma);
            }
        }

        // finish pools
        while (state.branch(TokenType::sem)) {
            tup.finish_pool();
        }
        // closing parenthesis after empty tuple
        if (state.token() == TokenType::rpar) {
            finish_tup(state);
            return true;
        }
        // coma indicating empty tuple
        if (state.branch(TokenType::comma)) {
            tup.term = false;
            if (state.token() == TokenType::rpar) {
                finish_tup(state);
                return true;
            }
            if (state.branch(TokenType::sem)) {
                tup.finish_pool();
                arg = false;
                continue;
            }
            return state.expected(TokenType::rpar, TokenType::sem);
        }
        // leading star that must be part of a tuple
        if (state.token() == TokenType::star) {
            tup.tup.emplace_back(Projection{state.loc()});
            state.consume();
            if (state.token() == TokenType::comma) {
                arg = true;
                continue;
            }
            return state.expected(TokenType::comma);
        }
        // parse a term
        state.replace(Prod::tup);
        state.push(Prod::term);
        return true;
    }
}

//! Continue parsing a tuple after a '(' token.
auto cont_tup(ParserState &state) -> bool {
    assert(state.token() == TokenType::lpar);
    state.push_value<Tup>(state.token_line(), state.token_column());
    return cont_tup_args(state, false);
}

//! Finish a function after reading a ')' token.
void finish_fun(ParserState &state) {
    assert(state.token() == TokenType::rpar);
    auto fun = state.pop_value<Fun>();
    fun.finish_pool();
    state.push_value<Term>(std::in_place_type<TermFunction>,
                           Location{Position{state.file(), fun.line, fun.column},
                                    Position{state.file(), state.cursor_line(), state.cursor_column()}},
                           fun.name, PoolArray{std::move(fun.args)}, fun.external);
    state.consume();
    cont_expr(state);
}

//! Continue parsing function arguments.
//!
//! If arg is false, then continue after tokens '(' or ';'.
//! If arg is true, then continue after an argument.
auto cont_fun_args(ParserState &state, bool arg) -> bool {
    auto &fun = state.top_value<Fun>(arg ? 1 : 0);
    if (arg) {
        fun.tup.emplace_back(state.pop_value<Term>());
    } else {
        assert(state.token() == TokenType::lpar || state.token() == TokenType::sem);
        state.consume();
    }
    while (true) {
        if (arg) {
            // finish all argument pools
            if (state.token() == TokenType::rpar) {
                finish_fun(state);
                return true;
            }
            // a comma that must be followed by another term or projection
            if (state.branch(TokenType::comma)) {
                if (state.token() == TokenType::star) {
                    fun.tup.emplace_back(Projection{state.loc()});
                    state.consume();
                    continue;
                }
                state.replace(Prod::fun);
                state.push(Prod::term);
                return true;
            }
            // fail if the argument pool is not closed
            // otherwise, finish the pool below
            if (state.token() != TokenType::sem) {
                return state.expected(TokenType::rpar, TokenType::comma, TokenType::sem);
            }
        }
        // finish argument pools
        while (state.branch(TokenType::sem)) {
            fun.finish_pool();
        }
        // finish all argument pools
        if (state.token() == TokenType::rpar) {
            finish_fun(state);
            return true;
        }
        // the pool must begin with a term
        if (state.token() != TokenType::star) {
            state.replace(Prod::fun);
            state.push(Prod::term);
            return true;
        }
        // the pool begins with a projection
        fun.tup.emplace_back(Projection{state.loc()});
        state.consume();
        arg = true;
    }
}

//! Continue parsing a function assuming an at or id token was read.
auto cont_fun(ParserState &state) -> bool {
    auto line = state.token_line();
    auto column = state.token_column();
    bool ext = state.token() == TokenType::at;
    if (ext) {
        state.consume();
        if (state.token() != TokenType::id) {
            return state.expected(TokenType::id);
        }
    }
    auto name = state.store().string_ref(state.view());
    state.consume();
    if (state.token() == TokenType::lpar) {
        state.push_value<Fun>(line, column, name, ext);
        if (!cont_fun_args(state, false)) {
            return false;
        }
    } else {
        auto loc = Location{Position{state.file(), line, column},
                            Position{state.file(), state.cursor_line(), state.cursor_column()}};
        if (ext) {
            state.push_value<Term>(std::in_place_type<TermFunction>, std::move(loc), name,
                                   Util::make_immutable_array<ArgumentTuple>(ArgumentTuple{{}}), true);
        } else {
            state.push_value<Term>(std::in_place_type<TermSymbol>, std::move(loc),
                                   state.store().fun_ref(name, {}, false));
        }
        cont_expr(state);
    }
    return true;
}

//! Continue parsing an expression if followed by a binary operation.
//!
//! See cont_expr for more  details.
void cont_sym_expr(ParserState &state) {
    state.pop();
    if (auto cur = map_binop(state.token()); cur && *cur != Prod::interval) {
        // reduce
        if (!state.empty() && is_op(state.top())) {
            auto pre = state.top();
            auto pp = priority(pre);
            auto pc = priority(*cur);
            if (pc < pp || (pc == pp && left_assoc_(*cur))) {
                return;
            }
        }
        // shift
        state.consume();
        state.push(*cur);
        state.push(Prod::term);
    }
}

//! Finish a function after reading a ')' token.
void finish_sym_fun(ParserState &state) {
    assert(state.token() == TokenType::rpar);
    auto fun = state.pop_value<SymFun>();
    state.push_value<Symbol>(state.store().fun_ref(fun.name, fun.args, false));
    state.consume();
    cont_sym_expr(state);
}

//! Continue parsing function arguments.
//!
//! If arg is false, then continue after token '('.
//! If arg is true, then continue after an argument.
auto cont_sym_fun_args(ParserState &state, bool arg) -> bool {
    auto &fun = state.top_value<SymFun>(arg ? 1 : 0);
    if (arg) {
        fun.args.emplace_back(state.pop_value<Symbol>());
    } else {
        assert(state.token() == TokenType::lpar);
        state.consume();
    }
    // finish
    if (state.token() == TokenType::rpar) {
        finish_sym_fun(state);
        return true;
    }
    if (arg) {
        // a comma that must be followed by another term or projection
        if (state.branch(TokenType::comma)) {
            state.push(Prod::term);
            return true;
        }
        return state.expected(TokenType::rpar, TokenType::comma);
    }
    state.replace(Prod::fun);
    state.push(Prod::term);
    return true;
}

//! Continue parsing a function assuming an id token was read.
auto cont_sym_fun(ParserState &state) -> bool {
    auto name = state.str();
    state.consume();
    if (state.token() == TokenType::lpar) {
        state.push_value<SymFun>(name);
        if (!cont_sym_fun_args(state, false)) {
            return false;
        }
    } else {
        state.push_value<Symbol>(state.store().fun_ref(name, {}, false));
        cont_sym_expr(state);
    }
    return true;
}

//! Finish a tuple after reading a ')' token.
void finish_sym_tup(ParserState &state) {
    assert(state.token() == TokenType::rpar);
    auto tup = state.pop_value<SymTup>();
    if (tup.tup.size() == 1 && tup.term) {
        state.push_value<Symbol>(tup.tup.front());
    } else {
        state.push_value<Symbol>(state.store().tup_ref(tup.tup));
    }
    state.consume();
    cont_sym_expr(state);
}

//! Continue parsing tuple arguments.
//!
//! If arg is true, continue parsing a tuple after a symbol argument.
//! Otherwise, continue parsing a tuple after token '('.
auto cont_sym_tup_args(ParserState &state, bool arg) -> bool {
    auto &tup = state.top_value<SymTup>(arg ? 1 : 0);
    if (arg) {
        tup.tup.emplace_back(state.pop_value<Symbol>());
    } else {
        assert(state.token() == TokenType::lpar);
        state.consume();
    }

    // closing parenthesis after symbol or empty tuple
    if (state.token() == TokenType::rpar) {
        finish_sym_tup(state);
        return true;
    }
    if (arg) {
        // comma after symbol
        if (state.branch(TokenType::comma)) {
            tup.term = false;
            if (state.token() == TokenType::rpar) {
                finish_sym_tup(state);
                return true;
            }
            state.replace(Prod::tup);
            state.push(Prod::term);
            return true;
        }
        return state.expected(TokenType::rpar, TokenType::comma);
    }

    // coma indicating empty tuple
    if (state.branch(TokenType::comma)) {
        tup.term = false;
        if (state.expect(TokenType::rpar)) {
            finish_sym_tup(state);
            return true;
        }
        return false;
    }
    // parse a term
    state.replace(Prod::tup);
    state.push(Prod::term);
    return true;
}

//! Continue parsing a tuple after a '(' token.
auto cont_sym_tup(ParserState &state) -> bool {
    assert(state.token() == TokenType::lpar);
    state.push_value<SymTup>();
    return cont_sym_tup_args(state, false);
}

//! Parse a program param.
auto parse_program_param(ParserState &state) -> std::optional<ProgramParam> {
    if (state.expect(TokenType::id)) {
        auto name = state.str();
        state.consume();
        if (state.token() == TokenType::lpar) {
            if (auto args = state.delimited(TokenType::lpar, parse_symbol, TokenType::comma, TokenType::rpar)) {
                state.consume();
                return ProgramParam{SharedString{name}, *std::move(args)};
            }
        } else {
            return ProgramParam{SharedString{name}, {}};
        }
    }
    return std::nullopt;
}

} // namespace

auto check_term(TokenType token) -> bool {
    switch (token) {
        case TokenType::at:
        case TokenType::id:
        case TokenType::inf:
        case TokenType::sup:
        case TokenType::str:
        case TokenType::num:
        case TokenType::var:
        case TokenType::anon:
        case TokenType::lpar:
        case TokenType::bar:
        case TokenType::tilde:
        case TokenType::minus: {
            return true;
        }
        default: {
            return false;
        }
    }
}

auto parse_term(ParserState &state) -> std::optional<Term> {
    state.init(Prod::term);

    while (!state.empty()) {
        switch (state.top()) {
            case Prod::exp:
            case Prod::div:
            case Prod::mod:
            case Prod::mul:
            case Prod::sub:
            case Prod::add:
            case Prod::band:
            case Prod::bor:
            case Prod::bxor:
            case Prod::interval: {
                auto rhs = state.pop_value<Term>();
                auto lhs = state.pop_value<Term>();
                auto loc = location(lhs) + location(rhs);
                state.push_value<Term>(std::in_place_type<TermBinary>, loc, std::move(lhs), map_binop(state.top()),
                                       std::move(rhs));
                cont_expr(state);
                continue;
            }
            case Prod::uminus:
            case Prod::bneg: {
                auto rhs = state.pop_value<Term>();
                auto [line, column] = state.pop_value<Pos>();
                auto loc = Position{state.file(), line, column} + location(rhs);
                state.push_value<Term>(std::in_place_type<TermUnary>, loc, map_unop(state.top()), std::move(rhs));
                cont_expr(state);
                continue;
            }
            case Prod::fstr: {
                if (!cont_fstr(state)) {
                    return std::nullopt;
                }
                continue;
            }
            case Prod::fstr_field: {
                if (!cont_fstr_field(state)) {
                    return std::nullopt;
                }
                continue;
            }
            case Prod::term: {
                switch (state.token()) {
                    case TokenType::minus:
                    case TokenType::tilde: {
                        auto unop = map_unop(state.token());
                        state.push_value<Pos>(state.token_line(), state.token_column());
                        state.consume();
                        state.replace(unop);
                        state.push(Prod::term);
                        continue;
                    }
                    case TokenType::sup: {
                        state.push_value<Term>(std::in_place_type<TermSymbol>, state.loc(), SymbolStore::sup());
                        state.consume();
                        cont_expr(state);
                        continue;
                    }
                    case TokenType::inf: {
                        state.push_value<Term>(std::in_place_type<TermSymbol>, state.loc(), SymbolStore::inf());
                        state.consume();
                        cont_expr(state);
                        continue;
                    }
                    case TokenType::num: {
                        state.push_value<Term>(std::in_place_type<TermSymbol>, state.loc(),
                                               state.store().num_ref(state.num()));
                        state.consume();
                        cont_expr(state);
                        continue;
                    }
                    case TokenType::str: {
                        state.push_value<Term>(std::in_place_type<TermSymbol>, state.loc(),
                                               SymbolStore::str_ref(state.str()));
                        state.consume();
                        cont_expr(state);
                        continue;
                    }

                    case TokenType::fstring_start: {
                        state.push(Prod::fstr);
                        state.push_value<FStr>(state.token_line(), state.token_column());
                        state.consume(Condition::fstring);
                        if (!cont_fstr(state)) {
                            return std::nullopt;
                        }
                        continue;
                    }
                    case TokenType::anon:
                    case TokenType::var: {
                        state.push_value<Term>(std::in_place_type<TermVariable>, state.loc(), state.str(),
                                               state.token() == TokenType::anon);
                        state.consume();
                        cont_expr(state);
                        continue;
                    }
                    case TokenType::bar: {
                        cont_abs(state);
                        continue;
                    }
                    case TokenType::id:
                    case TokenType::at: {
                        if (!cont_fun(state)) {
                            return std::nullopt;
                        }
                        continue;
                    }
                    case TokenType::lpar: {
                        if (!cont_tup(state)) {
                            return std::nullopt;
                        }
                        continue;
                    }
                    default: {
                        return state.expected<std::nullopt>("<term>");
                    }
                }
            }
            case Prod::abs: {
                if (!cont_abs_args(state)) {
                    return std::nullopt;
                }
                continue;
            }
            case Prod::tup: {
                if (!cont_tup_args(state, true)) {
                    return std::nullopt;
                }
                continue;
            }
            case Prod::fun: {
                if (!cont_fun_args(state, true)) {
                    return std::nullopt;
                }
                continue;
            }
            default: {
                Util::unreachable();
            }
        }
    }
    assert(!state.empty_value());
    auto ret = state.pop_value<Term>();
    assert(state.empty_value());
    return ret;
}

namespace {} // namespace

auto parse_symbol(ParserState &state) -> std::optional<SharedSymbol> {
    state.init(Prod::term);

    while (!state.empty()) {
        switch (state.top()) {
            case Prod::exp:
            case Prod::div:
            case Prod::mod:
            case Prod::mul:
            case Prod::sub:
            case Prod::add:
            case Prod::band:
            case Prod::bor:
            case Prod::bxor: {
                auto rhs = state.pop_value<Symbol>();
                auto lhs = state.pop_value<Symbol>();
                if (auto res = evaluate(state.store(), lhs, map_binop(state.top()), rhs)) {
                    state.push_value<Symbol>(*res);
                    cont_sym_expr(state);
                    continue;
                }
                return std::nullopt;
            }
            case Prod::uminus:
            case Prod::bneg: {
                auto rhs = state.pop_value<Symbol>();
                if (auto res = evaluate(state.store(), map_unop(state.top()), rhs)) {
                    state.push_value<Symbol>(*res);
                    cont_sym_expr(state);
                    continue;
                }
                return std::nullopt;
            }
            case Prod::term: {
                switch (state.token()) {
                    case TokenType::minus:
                    case TokenType::tilde: {
                        auto unop = map_unop(state.token());
                        state.consume();
                        state.replace(unop);
                        state.push(Prod::term);
                        continue;
                    }
                    case TokenType::sup: {
                        state.push_value<Symbol>(SymbolStore::sup());
                        state.consume();
                        cont_sym_expr(state);
                        continue;
                    }
                    case TokenType::inf: {
                        state.push_value<Symbol>(SymbolStore::inf());
                        state.consume();
                        cont_sym_expr(state);
                        continue;
                    }
                    case TokenType::num: {
                        state.push_value<Symbol>(state.store().num_ref(state.num()));
                        state.consume();
                        cont_sym_expr(state);
                        continue;
                    }
                    case TokenType::str: {
                        state.push_value<Symbol>(SymbolStore::str_ref(state.str()));
                        state.consume();
                        cont_sym_expr(state);
                        continue;
                    }
                    case TokenType::bar: {
                        state.replace(Prod::abs);
                        state.consume();
                        state.push(Prod::term);
                        continue;
                    }
                    case TokenType::id: {
                        if (!cont_sym_fun(state)) {
                            return std::nullopt;
                        }
                        continue;
                    }
                    case TokenType::lpar: {
                        if (!cont_sym_tup(state)) {
                            return std::nullopt;
                        }
                        continue;
                    }
                    default: {
                        return state.expected<std::nullopt>("<symbol>");
                    }
                }
            }
            case Prod::abs: {
                if (state.expect(TokenType::bar)) {
                    auto sym = state.pop_value<Symbol>();
                    if (sym.type() == SymbolType::number) {
                        state.push_value<Symbol>(state.store().num_ref(abs(sym.num())));
                        state.consume();
                        cont_sym_expr(state);
                        continue;
                    }
                    CLINGO_REPORT(state.log(), info_operation_undefined) << "operation undefined:\n"
                                                                         << "  |" << sym << "|\n";
                }
                return std::nullopt;
            }
            case Prod::tup: {
                if (!cont_sym_tup_args(state, true)) {
                    return std::nullopt;
                }
                continue;
            }
            case Prod::fun: {
                if (!cont_sym_fun_args(state, true)) {
                    return std::nullopt;
                }
                continue;
            }
            default: {
                Util::unreachable();
            }
        }
    }
    assert(!state.empty_value());
    auto ret = state.pop_value<Symbol>();
    assert(state.empty_value());
    return SharedSymbol{ret};
}

auto parse_const_def(ParserState &state) -> std::optional<std::pair<SharedString, SharedSymbol>> {
    if (state.expect(TokenType::id)) {
        auto name = state.str();
        state.consume();
        if (state.expect(TokenType::eq)) {
            state.consume();
            if (auto value = parse_symbol(state)) {
                return std::make_pair(SharedString{name}, *value);
            }
        }
    }
    return std::nullopt;
}

auto parse_program_parts(ParserState &state, TokenType end) -> std::optional<ProgramParamVec> {
    return state.separated_until(parse_program_param, TokenType::comma, end);
}

} // namespace CppClingo::Input::Parse
