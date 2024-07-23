#include <gringo/input/algo/parsev2.hh>

#include <gringo/input/term.hh>

#include "lexer_state.hh"

// TODO: remove
#include <gringo/input/algo/print.hh>
#include <iostream>

namespace Gringo::Input {

namespace {

void quote(std::string_view in, auto out) {
    for (auto c : in) {
        switch (c) {
            case '\n': {
                *out++ = '\\';
                *out++ = 'n';
                break;
            }
            case '\\': {
                *out++ = '\\';
                *out++ = '\\';
                break;
            }
            case '"': {
                *out++ = '\\';
                *out++ = '"';
                break;
            }
            default: {
                *out++ = c;
                break;
            }
        }
    }
}
void unquote(std::string_view in, auto out) {
    bool slash = false;
    for (auto c : in) {
        if (slash) {
            switch (c) {
                case 'n': {
                    *out++ = '\n';
                    break;
                }
                case '\\': {
                    *out++ = '\\';
                    break;
                }
                case '"': {
                    *out++ = '"';
                    break;
                }
                default: {
                    assert(false);
                    break;
                }
            }
            slash = false;
        } else if (c == '\\') {
            slash = true;
        } else {
            *out++ = c;
        }
    }
}

enum class Condition : uint8_t {
    normal,
};

enum class TokenType : uint8_t {
    amp,
    at,
    anon,
    bar,
    bslash,
    caret,
    colon,
    comma,
    dot,
    ddot,
    dstar,
    end,
    error,
    id,
    lpar,
    minus,
    num,
    plus,
    qmark,
    rpar,
    sem,
    slash,
    star,
    str,
    tilde,
    var,
};

auto operator<<(std::ostream &out, TokenType token) -> std::ostream & {
    switch (token) {
        case TokenType::amp: {
            return out << "'&'";
        }
        case TokenType::at: {
            return out << "'@'";
        }
        case TokenType::anon: {
            return out << "'_'";
        }
        case TokenType::bar: {
            return out << "'|'";
        }
        case TokenType::bslash: {
            return out << "'\\'";
        }
        case TokenType::caret: {
            return out << "'^'";
        }
        case TokenType::colon: {
            return out << "':'";
        }
        case TokenType::comma: {
            return out << "','";
        }
        case TokenType::dot: {
            return out << "'.'";
        }
        case TokenType::ddot: {
            return out << "'..'";
        }
        case TokenType::dstar: {
            return out << "'**'";
        }
        case TokenType::end: {
            return out << "<eof>";
        }
        case TokenType::error: {
            return out << "<error>";
        }
        case TokenType::id: {
            return out << "<identifier>";
        }
        case TokenType::lpar: {
            return out << "'('";
        }
        case TokenType::minus: {
            return out << "'-'";
        }
        case TokenType::num: {
            return out << "<number>";
        }
        case TokenType::plus: {
            return out << "'+'";
        }
        case TokenType::qmark: {
            return out << "'?'";
        }
        case TokenType::rpar: {
            return out << "')'";
        }
        case TokenType::sem: {
            return out << "';'";
        }
        case TokenType::slash: {
            return out << "'/'";
        }
        case TokenType::star: {
            return out << "'*'";
        }
        case TokenType::str: {
            return out << "<string>";
        }
        case TokenType::tilde: {
            return out << "'~'";
        }
        case TokenType::var: {
            return out << "<var>";
        }
    }
    return out;
}

//! The available productions.
enum class Prod : uint8_t { term, fun, add, sub, mul, exp, uminus, bneg, div, mod, band, bor, bxor, interval, tup };

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
auto left_assoc_(Prod prod) -> bool { return prod != Prod::exp; }

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
        case TokenType::bar: {
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
            return UnaryOperator::invert;
        }
        default: {
            assert(prod == Prod::uminus);
            return UnaryOperator::negate;
        }
    }
}

struct Fun {
    Fun(size_t line, size_t column, String name, bool external)
        : line{line}, column{column}, name{name}, external{external} {}
    void finish_pool() {
        args.emplace_back(std::move(tup));
        tup.clear();
    }
    std::vector<ArgumentTuple> args;
    std::vector<Argument> tup;
    size_t line;
    size_t column;
    String name;
    bool external;
};

struct Tup {
    Tup(size_t line, size_t column) : line{line}, column{column} {}

    void finish_pool() {
        if (term && tup.size() == 1) {
            args.emplace_back(std::move(std::get<Term>(tup.front())));
        } else {
            args.emplace_back(ArgumentTuple{std::move(tup)});
        }
        term = true;
        tup.clear();
    }

    std::vector<TupleElement> args;
    std::vector<Argument> tup;
    size_t line;
    size_t column;
    bool term = true;
};

} // namespace

// NOLINTBEGIN(performance-avoid-endl)

class Parser::Impl {
  public:
    //! @todo: pass logger
    //! @todo: pass symbol store
    Impl(std::istream &in) : state_{in}, store_{&default_symbol_store()} {}

    //! Parse a term.
    auto parse_term() -> std::optional<Term> {
        consume_();
        if (parse_term_() && branch_(TokenType::end)) {
            assert(terms_.size() == 1);
            auto ret = std::move(terms_.back());
            terms_.pop_back();
            return ret;
        }
        return std::nullopt;
    }

  private:
    //! Compute the next token.
    auto lex_(Condition cond) -> TokenType;

    //! Compute the location of the current token.
    auto loc_() -> Location {
        auto file = store_->string_ref("<input>");
        return Location{Position{file, state_.token_line(), state_.token_column()},
                        Position{file, state_.cursor_line(), state_.cursor_column()}};
    }

    //! Report an error message indicating that one of the given tokens was expected.
    auto expected(auto... expected) -> bool {
        std::cerr << "<input>:" << state_.token_line() << ":" << state_.token_column() << ": expected one of ";
        ((std::cerr << " " << expected), ...);
        std::cerr << " but got " << token_;
        return false;
    }

    //! Compute the next token discarding the last one.
    void consume_() { token_ = lex_(Condition::normal); }

    //! Check if the given token matches the current one.
    //!
    //! In case of a match, it consumes the token.
    auto branch_(TokenType token) -> bool {
        if (token_ == token) {
            consume_();
            return true;
        }
        return false;
    }

    //! Continue parsing an expression if followed by a binary operation.
    //!
    //! Depending on the priority of the previous operator on the stack, this
    //! function either shifts the next binary operation or does nothing which
    //! results in a reduction in the next iteration.
    void cont_expr_() {
        stack_.pop_back();
        if (auto cur = map_binop(token_); cur) {
            // reduce
            if (!stack_.empty() && is_op(stack_.back())) {
                auto pre = stack_.back();
                auto pp = priority(pre);
                auto pc = priority(*cur);
                if (pc < pp || (pc == pp && left_assoc_(*cur))) {
                    return;
                }
            }
            // shift
            consume_();
            stack_.emplace_back(*cur);
            stack_.emplace_back(Prod::term);
        }
    }

    //! Continue parsing a tuple after a '(' token.
    auto cont_tup_() -> bool {
        assert(token_ == TokenType::lpar);
        tups_.emplace_back(state_.token_line(), state_.token_column());
        return cont_tup_args_(false);
    }

    //! Continue parsing tuple arguments.
    //!
    //! If arg is true, continue parsing a tuple after a term argument.
    //! Otherwise, continue parsing a tuple after tokens '(' or ';'.
    auto cont_tup_args_(bool arg) -> bool {
        assert(!tups_.empty());
        auto &tup = tups_.back();
        if (arg) {
            assert(!terms_.empty());
            tup.tup.emplace_back(std::move(terms_.back()));
            terms_.pop_back();
        } else {
            // First handle prefixes consisting of empty tuple arguments with
            // optional trailing commas. Then either finish the tuple with a
            // closing parenthesis or by continuing to parse the next term
            // arguments.
            assert(token_ == TokenType::lpar || token_ == TokenType::sem);
            consume_();
        }
        while (true) {
            if (arg) {
                // closing parenthesis after term or projection
                if (token_ == TokenType::rpar) {
                    finish_tup_();
                    return true;
                }
                // comma after term or projection
                if (branch_(TokenType::comma)) {
                    tup.term = false;
                    if (token_ == TokenType::star) {
                        tup.tup.emplace_back(Projection{loc_()});
                        consume_();
                        continue;
                    }
                    if (token_ == TokenType::rpar) {
                        finish_tup_();
                        return true;
                    }
                    if (token_ != TokenType::sem) {
                        stack_.push_back(Prod::term);
                        return true;
                    }
                    // semicolon after term or projection falls through
                } else if (token_ != TokenType::sem) {
                    return expected(TokenType::rpar, TokenType::sem, TokenType::comma);
                }
            }

            // finish pools
            while (branch_(TokenType::sem)) {
                tup.finish_pool();
            }
            // closing parenthesis after empty tuple
            if (token_ == TokenType::rpar) {
                finish_tup_();
                return true;
            }
            // coma indicating empty tuple
            if (branch_(TokenType::comma)) {
                tup.term = false;
                if (token_ == TokenType::rpar) {
                    finish_tup_();
                    return true;
                }
                if (branch_(TokenType::sem)) {
                    tup.finish_pool();
                    arg = false;
                    continue;
                }
                return expected(TokenType::rpar, TokenType::sem);
            }
            // leading star that must be part of a tuple
            if (token_ == TokenType::star) {
                tup.tup.emplace_back(Projection{loc_()});
                consume_();
                if (token_ == TokenType::comma) {
                    arg = true;
                    continue;
                }
                return expected(TokenType::comma);
            }
            // parse a term
            stack_.back() = Prod::tup;
            stack_.push_back(Prod::term);
            return true;
        }
    }

    //! Finish a tuple after reading a ')' token.
    void finish_tup_() {
        assert(token_ == TokenType::rpar);
        auto file = store_->string_ref("<input>");
        auto &tup = tups_.back();
        tup.finish_pool();
        if (auto *term = std::get_if<Term>(tup.args.size() == 1 ? &tup.args.front() : nullptr); term != nullptr) {
            terms_.emplace_back(std::move(*term));
        } else {
            terms_.emplace_back(TermTuple{Location{Position{file, tup.line, tup.column},
                                                   Position{file, state_.cursor_line(), state_.cursor_column()}},
                                          TupleElementArray{std::move(tup.args)}});
        }
        tups_.pop_back();
        consume_();
        cont_expr_();
    }

    //! Continue parsing a function assuming an at or id token was read.
    auto cont_fun_() -> bool {
        auto file = store_->string_ref("<input>");
        auto line = state_.token_line();
        auto column = state_.token_column();
        bool ext = token_ == TokenType::at;
        if (ext) {
            consume_();
            if (token_ != TokenType::id) {
                return expected(TokenType::id);
            }
        }
        auto name = store_->string_ref(state_.view());
        consume_();
        if (token_ == TokenType::lpar) {
            funs_.emplace_back(line, column, name, ext);
            if (!cont_fun_args_(false)) {
                return false;
            }
        } else {
            auto loc =
                Location{Position{file, line, column}, Position{file, state_.cursor_line(), state_.cursor_column()}};
            if (ext) {
                terms_.emplace_back(TermFunction{std::move(loc), name,
                                                 Util::make_immutable_array<ArgumentTuple>(ArgumentTuple{{}}), true});
            } else {
                terms_.emplace_back(TermSymbol{std::move(loc), store_->fun_ref(name, {}, false)});
            }
            cont_expr_();
        }
        return true;
    }

    //! Continue parsing function arguments.
    //!
    //! If arg is false, then continue after tokens '(' or ';'.
    //! If arg is true, then continue after an argument.
    auto cont_fun_args_(bool arg) -> bool {
        auto &fun = funs_.back();
        if (arg) {
            assert(!terms_.empty());
            fun.tup.emplace_back(std::move(terms_.back()));
            terms_.pop_back();
        } else {
            assert(token_ == TokenType::lpar || token_ == TokenType::sem);
            consume_();
        }
        while (true) {
            if (arg) {
                // finish all argument pools
                if (token_ == TokenType::rpar) {
                    finish_fun_();
                    return true;
                }
                // a comma that must be followed by another term or projection
                if (branch_(TokenType::comma)) {
                    if (token_ == TokenType::star) {
                        fun.tup.emplace_back(Projection{loc_()});
                        consume_();
                        continue;
                    }
                    stack_.push_back(Prod::term);
                    return true;
                }
                // fail if the argument pool is not closed
                // otherwise, finish the pool below
                if (token_ != TokenType::sem) {
                    return expected(TokenType::rpar, TokenType::comma, TokenType::sem);
                }
            }
            // finish argument pools
            while (branch_(TokenType::sem)) {
                fun.finish_pool();
            }
            // finish all argument pools
            if (token_ == TokenType::rpar) {
                finish_fun_();
                return true;
            }
            // the pool must begin with a term
            if (token_ != TokenType::star) {
                stack_.back() = Prod::fun;
                stack_.push_back(Prod::term);
                return true;
            }
            // the pool begins with a projection
            fun.tup.emplace_back(Projection{loc_()});
            consume_();
            arg = true;
        }
    }

    //! Finish a function after reading a ')' token.
    void finish_fun_() {
        assert(token_ == TokenType::rpar);
        auto file = store_->string_ref("<input>");
        auto &fun = funs_.back();
        fun.finish_pool();
        terms_.emplace_back(TermFunction{Location{Position{file, fun.line, fun.column},
                                                  Position{file, state_.cursor_line(), state_.cursor_column()}},
                                         fun.name, PoolArray{std::move(fun.args)}, fun.external});
        funs_.pop_back();
        consume_();
        cont_expr_();
    }

    //! Continue parsing a number term assuming a num token is on the stack.
    void cont_num_() {
        assert(token_ == TokenType::num);
        auto view = state_.view();
        auto base = Base::dec;
        if (view.starts_with("0b")) {
            base = Base::bin;
        } else if (view.starts_with("0o")) {
            base = Base::oct;
        } else if (view.starts_with("0x")) {
            base = Base::hex;
        }
        if (base != Base::dec) {
            view = view.substr(2);
        }
        buf_.clear();
        buf_.reserve(view.size());
        std::copy_if(view.begin(), view.end(), std::back_inserter(buf_), [](char c) { return c != '\''; });
        terms_.emplace_back(TermSymbol{loc_(), store_->num_ref(Number{buf_.c_str(), base})});
        consume_();
        cont_expr_();
    }

    //! Continue parsing a string term assuming a str token is on the stack.
    void cont_str_() {
        assert(token_ == TokenType::str);
        auto view = state_.view();
        buf_.clear();
        buf_.reserve(view.size() - 2);
        unquote(view.substr(1, view.size() - 2), std::back_inserter(buf_));
        auto str = store_->string_ref(std::string_view{buf_.begin(), buf_.end()});
        terms_.emplace_back(TermSymbol{loc_(), SymbolStore::str_ref(str)});
        consume_();
        cont_expr_();
    }

    //! Continue parsing a variable term assuming a var or anon token is on the stack.
    void cont_var_(bool anonymous) {
        assert(token_ == (anonymous ? TokenType::anon : TokenType::var));
        auto str = store_->string_ref(state_.view());
        terms_.emplace_back(TermVariable{loc_(), str, anonymous});
        consume_();
        cont_expr_();
    }

    //! Continue parsing an abs term after a '|' token.
    void cont_abs_() {
        static_cast<void>(this);
        throw std::runtime_error("implement me!!!");
    }

    //! Parse a term.
    //!
    //! Uses a hand written bottom up parser with a stack to avoid stack
    //! overflows.
    auto parse_term_() -> bool {
        // TODO:
        // - error reporting via logger (almost there)
        // - properly handle filename
        // - abs term is missing
        stack_.emplace_back(Prod::term);

        while (!stack_.empty()) {
            switch (stack_.back()) {
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
                    assert(terms_.size() >= 2);
                    auto rhs = std::move(terms_.back());
                    terms_.pop_back();
                    auto lhs = std::move(terms_.back());
                    terms_.pop_back();
                    auto loc = location(lhs) + location(rhs);
                    terms_.emplace_back(TermBinary{loc, std::move(lhs), map_binop(stack_.back()), std::move(rhs)});
                    cont_expr_();
                    continue;
                }
                case Prod::uminus:
                case Prod::bneg: {
                    assert(!terms_.empty());
                    auto rhs = std::move(terms_.back());
                    terms_.pop_back();
                    auto loc = location(rhs);
                    terms_.emplace_back(TermUnary{loc, map_unop(stack_.back()), std::move(rhs)});
                    cont_expr_();
                    continue;
                }
                case Prod::term: {
                    switch (token_) {
                        case TokenType::minus:
                        case TokenType::tilde: {
                            auto unop = map_unop(token_);
                            // TODO: remember location
                            consume_();
                            stack_.back() = unop;
                            stack_.push_back(Prod::term);
                            continue;
                        }
                        case TokenType::num: {
                            cont_num_();
                            continue;
                        }
                        case TokenType::str: {
                            cont_str_();
                            continue;
                        }
                        case TokenType::anon: {
                            cont_var_(true);
                            continue;
                        }
                        case TokenType::var: {
                            cont_var_(false);
                            continue;
                        }
                        case TokenType::bar: {
                            cont_abs_();
                            continue;
                        }
                        case TokenType::id:
                        case TokenType::at: {
                            if (!cont_fun_()) {
                                return false;
                            }
                            continue;
                        }
                        case TokenType::lpar: {
                            if (!cont_tup_()) {
                                return false;
                            }
                            continue;
                        }
                        default: {
                            return expected(TokenType::tilde, TokenType::minus, TokenType::anon, TokenType::lpar,
                                            TokenType::num, TokenType::str, TokenType::var, TokenType::id);
                        }
                    }
                }
                case Prod::tup: {
                    if (!cont_tup_args_(true)) {
                        return false;
                    }
                    continue;
                }
                case Prod::fun: {
                    if (!cont_fun_args_(true)) {
                        return false;
                    }
                    continue;
                }
            }
        }
        return true;
    }

    LexerState state_;
    SymbolStore *store_;
    std::vector<Prod> stack_;
    std::vector<Term> terms_;
    std::vector<Fun> funs_;
    std::vector<Tup> tups_;
    std::string buf_;
    TokenType token_ = TokenType::error;
};

// NOLINTEND(performance-avoid-endl)

Parser::Parser(std::istream &in) : impl_{std::make_unique<Impl>(in)} {}

Parser::Parser(Parser &&other) noexcept = default;

auto Parser::operator=(Parser &&other) noexcept -> Parser & = default;

Parser::~Parser() noexcept = default;

auto Parser::parse_term() -> std::optional<Term> { return impl_->parse_term(); }

#include "algo/parse/lexer_impl.hh"

} // namespace Gringo::Input
