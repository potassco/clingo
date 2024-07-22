#include <gringo/input/algo/parsev2.hh>

#include <gringo/input/term.hh>

#include "lexer_state.hh"

// TODO: remove
#include <gringo/input/algo/print.hh>
#include <iostream>

namespace Gringo::Input {

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

static auto operator<<(std::ostream &out, TokenType token) -> std::ostream & {
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
            for (auto &term : terms_) {
                std::cerr << "term: " << term << std::endl;
            }
            assert(terms_.size() == 1);
            auto ret = std::move(terms_.back());
            terms_.pop_back();
            return ret;
        }
        return std::nullopt;
    }

  private:
    //! The available productions.
    enum class Prod : uint8_t { term, fun, add, sub, mul, exp, uminus, bneg, div, mod, band, bor, bxor, interval, tup };

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

    //! Check if the given production is an arithmetic operation or interval.
    static auto is_op(Prod prod) -> bool {
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
    static auto priority(Prod prod) -> int {
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
    static auto left_assoc_(Prod prod) -> bool { return prod != Prod::exp; }

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

    //! Check if the current token is a binary operation.
    auto check_binop_() -> std::optional<Prod> {
        switch (token_) {
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
    };

    //! Check if the current token is a unary operation.
    auto check_unop_() -> std::optional<Prod> {
        switch (token_) {
            case TokenType::minus: {
                return Prod::uminus;
            }
            case TokenType::tilde: {
                return Prod::bneg;
            }
            default: {
                return std::nullopt;
            }
        }
    };

    //! Continue parsing an expression if followed by a binary operation.
    //!
    //! Depending on the priority of the previous operator on the stack, this
    //! function either shifts the next binary operation or does nothing which
    //! results in a reduction in the next iteration.
    void cont_expression() {
        stack_.pop_back();
        if (auto cur = check_binop_(); cur) {
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

    //! Continue parsing a tuple after tokens '(' or ';'.
    auto init_tuple_args_() -> bool {
        // First handle prefixes consisting of empty tuple arguments with
        // optional trailing commas. Then either finish the tuple with a
        // closing parenthesis or by continuing to parse the next term
        // arguments.
        while (true) {
            if (branch_(TokenType::sem)) {
                continue;
            }
            if (branch_(TokenType::comma)) {
                if (branch_(TokenType::rpar)) {
                    // TODO: dummy
                    terms_.emplace_back(TermSymbol{loc_(), SymbolStore::inf()});
                    cont_expression();
                    return true;
                }
                if (branch_(TokenType::sem)) {
                    continue;
                }
                return expected(TokenType::rpar, TokenType::sem);
            }
            if (branch_(TokenType::rpar)) {
                // TODO: dummy
                terms_.emplace_back(TermSymbol{loc_(), SymbolStore::inf()});
                cont_expression();
                return true;
            }
            stack_.back() = Prod::tup;
            stack_.push_back(Prod::term);
            return true;
        }
    }

    //! Continue parsing a tuple after a term argument.
    auto cont_tup_args_() -> bool {
        // TODO: add to tuple args
        terms_.pop_back();
        if (branch_(TokenType::rpar)) {
            // TODO: dummy
            terms_.emplace_back(TermSymbol{loc_(), SymbolStore::inf()});
            cont_expression();
            return true;
        }
        if (branch_(TokenType::sem)) {
            return init_tuple_args_();
        }
        if (branch_(TokenType::comma)) {
            if (branch_(TokenType::rpar)) {
                // TODO: dummy
                terms_.emplace_back(TermSymbol{loc_(), SymbolStore::inf()});
                cont_expression();
                return true;
            }
            if (branch_(TokenType::sem)) {
                return init_tuple_args_();
            }
            stack_.push_back(Prod::term);
            return true;
        }
        return expected(TokenType::rpar, TokenType::sem, TokenType::comma);
    }

    //! Finish an argument tuple adding it the vector of all arguments.
    void finish_fun_tup_() {
        auto &fun = funs_.back();
        std::get<0>(fun).emplace_back(std::move(std::get<1>(fun)));
        std::get<1>(fun).clear();
    }

    //! Finish a function adding it as a term.
    void finish_fun_() {
        finish_fun_tup_();
        auto file = store_->string_ref("<input>");
        auto &[args, tup, line, column, name, ext] = funs_.back();
        terms_.emplace_back(TermFunction{
            Location{Position{file, line, column}, Position{file, state_.cursor_line(), state_.cursor_column()}}, name,
            PoolArray{std::move(args)}, ext});
        funs_.pop_back();
        consume_();
        cont_expression();
    }

    //! Continue parsing a function arguments after tokens '(' or ';'.
    void init_fun_args_() {
        while (branch_(TokenType::sem)) {
            finish_fun_tup_();
        }
        if (token_ == TokenType::rpar) {
            finish_fun_();
        } else {
            stack_.back() = Prod::fun;
            stack_.push_back(Prod::term);
        }
    }

    //! Continue parsing function arguments after a term argument.
    auto cont_fun_args_() -> bool {
        assert(!funs_.empty() && !terms_.empty());
        std::get<1>(funs_.back()).emplace_back(std::move(terms_.back()));
        terms_.pop_back();
        // Fun -> . ')'
        if (token_ == TokenType::rpar) {
            finish_fun_();
            return true;
        }
        // Fun -> . ',' Term Fun
        if (branch_(TokenType::comma)) {
            stack_.push_back(Prod::term);
            return true;
        }
        // Fun -> . ';'+ ( ')' | Term Fun )
        if (branch_(TokenType::sem)) {
            finish_fun_tup_();
            init_fun_args_();
            return true;
        }
        return expected(TokenType::rpar, TokenType::comma, TokenType::sem);
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
        if (branch_(TokenType::lpar)) {
            funs_.emplace_back(std::vector<ArgumentTuple>{}, std::vector<Argument>{}, line, column, name, ext);
            init_fun_args_();
        } else {
            terms_.emplace_back(TermSymbol{
                Location{Position{file, line, column}, Position{file, state_.cursor_line(), state_.cursor_column()}},
                store_->fun_ref(name, {}, false)});
            cont_expression();
        }
        return true;
    }

    //! Continue parsing a number term assuming a num token is on the stack.
    void cont_num_() {
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
        cont_expression();
    }

    //! Continue parsing a string term assuming a str token is on the stack.
    void cont_str_() {
        auto view = state_.view();
        buf_.clear();
        buf_.reserve(view.size() - 2);
        unquote(view.substr(1, view.size() - 2), std::back_inserter(buf_));
        auto str = store_->string_ref(std::string_view{buf_.begin(), buf_.end()});
        terms_.emplace_back(TermSymbol{loc_(), SymbolStore::str_ref(str)});
        consume_();
        cont_expression();
    }

    //! Continue parsing a variable term assuming a var or anon token is on the stack.
    void cont_var_(bool anonymous) {
        auto str = store_->string_ref(state_.view());
        terms_.emplace_back(TermVariable{loc_(), str, anonymous});
        consume_();
        cont_expression();
    }

    //! Parse a term.
    //!
    //! Uses a hand written bottom up parser with a stack to avoid stack
    //! overflows.
    auto parse_term_() -> bool {
        // TODO:
        // - error reporting via logger (almost there)
        // - term building using a separate stack (partial)
        // - the abs term is missing
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
                case Prod::interval:
                case Prod::uminus:
                case Prod::bneg: {
                    // TODO: construct term here
                    if (stack_.back() != Prod::uminus && stack_.back() != Prod::bneg) {
                        // for now just drop everything on the right
                        terms_.pop_back();
                    }
                    cont_expression();
                    continue;
                }
                case Prod::term: {
                    // Term -> . '-' Term
                    if (auto unop = check_unop_(); unop) {
                        // TODO: remember location
                        consume_();
                        stack_.back() = *unop;
                        stack_.push_back(Prod::term);
                        continue;
                    }
                    // Term -> . num
                    if (token_ == TokenType::num) {
                        cont_num_();
                        continue;
                    }
                    // Term -> . str
                    if (token_ == TokenType::str) {
                        cont_str_();
                        continue;
                    }
                    // Term -> . '_'
                    if (token_ == TokenType::anon) {
                        cont_var_(true);
                        continue;
                    }
                    // Term -> . var
                    if (token_ == TokenType::var) {
                        cont_var_(false);
                        continue;
                    }
                    // Term -> . id '(' ...
                    if (token_ == TokenType::id || token_ == TokenType::at) {
                        if (!cont_fun_()) {
                            return false;
                        }
                        continue;
                    }
                    // Term -> . '(' ...
                    if (branch_(TokenType::lpar)) {
                        // TODO
                        if (init_tuple_args_()) {
                            continue;
                        }
                        return false;
                    }
                    return expected(TokenType::minus, TokenType::anon, TokenType::lpar, TokenType::num, TokenType::str,
                                    TokenType::var, TokenType::id);
                }
                case Prod::tup: {
                    if (cont_tup_args_()) {
                        continue;
                    }
                    return false;
                }
                case Prod::fun: {
                    if (cont_fun_args_()) {
                        continue;
                    }
                    return false;
                }
            }
        }
        return true;
    }

    LexerState state_;
    std::vector<Prod> stack_;
    std::vector<Term> terms_;
    std::vector<std::tuple<std::vector<ArgumentTuple>, std::vector<Argument>, size_t, size_t, String, bool>> funs_;
    std::string buf_;
    SymbolStore *store_;
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
