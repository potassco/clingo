#include <gringo/input/algo/check_syntax.hh>
#include <gringo/input/algo/parsev2.hh>

#include <gringo/input/term.hh>

#include <gringo/util/string.hh>

#include "lexer_state.hh"

namespace Gringo::Input {

namespace {

//! The list of lexer conditions for stateful lexing.
enum class Condition : uint8_t {
    normal,
};

//! The available tokens produced by the lexer.
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
    sup,
    inf,
    tilde,
    var,
};

//! Output token in human readable form.
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
        case TokenType::sup: {
            return out << "'#sup'";
        }
        case TokenType::inf: {
            return out << "'#inf'";
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
enum class Prod : uint8_t {
    term,
    fun,
    add,
    sub,
    mul,
    exp,
    uminus,
    bneg,
    div,
    mod,
    band,
    bor,
    bxor,
    interval,
    tup,
    abs
};

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
            return UnaryOperator::invert;
        }
        default: {
            assert(prod == Prod::uminus);
            return UnaryOperator::negate;
        }
    }
}

//! Capture a position in a file.
using Pos = std::pair<size_t, size_t>;

//! Capture a partial absolute term.
struct Abs {
    Abs(size_t line, size_t column) : line{line}, column{column} {}
    std::vector<Term> args;
    size_t line;
    size_t column;
};

//! Capture a partial function term.
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

//! Capture a partial tuple term.
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

#include "algo/parse/lexer_impl_h.hh"

} // namespace

//! The parser implementation.
class Parser::Impl {
  public:
    //! Contstructor.
    Impl(Logger &log, SymbolStore &store, std::istream &in, String file)
        : state_{in, YYMAXFILL}, log_{&log}, store_{&store}, file_{file} {}

    Impl(Logger &log, SymbolStore &store, std::string_view in, String file)
        : state_{in, YYMAXFILL}, log_{&log}, store_{&store}, file_{file} {}

    [[nodiscard]] auto store() const -> SymbolStore & { return *store_; }

    [[nodiscard]] auto log() const -> Logger & { return *log_; }

    //! Parse a term.
    [[nodiscard]] auto parse_term() -> std::optional<Term> {
        consume_();
        if (parse_term_() && branch_(TokenType::end)) {
            assert(stack_.empty() && values_.size() == 1);
            return pop_<Term>();
        }
        return std::nullopt;
    }

  private:
    //! Compute the next token.
    auto lex_(Condition cond) -> TokenType;

    //! Compute the location of the current token.
    auto loc_() -> Location {
        return Location{Position{*file_, state_.token_line(), state_.token_column()},
                        Position{*file_, state_.cursor_line(), state_.cursor_column()}};
    }

    //! Pop the last element on the value stack.
    template <class T> auto pop_() -> T {
        auto res = std::move(top_<T>());
        values_.pop_back();
        return res;
    }

    //! Get a reference to the last element on the value stack.
    template <class T> auto top_() -> T & {
        assert(!values_.empty() && std::holds_alternative<T>(values_.back()));
        return std::get<T>(values_.back());
    }

    //! Get a reference to the i-th last element on the value stack.
    template <class T> auto top_(size_t i) -> T & {
        assert(i < values_.size());
        auto j = values_.size() - 1 - i;
        assert(std::holds_alternative<T>(values_[j]));
        return std::get<T>(values_[j]);
    }

    //! Push an element on the value stack.
    template <class T, class... U> auto push_(U &&...args) {
        values_.emplace_back(std::in_place_type<T>, std::forward<U>(args)...);
    }

    //! Replace the last element on the value stack.
    template <class T, class... U> auto replace_(U &&...args) {
        assert(!values_.empty());
        values_.back().emplace<T>(std::forward<U>(args)...);
    }

    //! Report an error message indicating that one of the given tokens was expected.
    auto expected_(auto... expected) -> bool {
        if (log_->check(MessageCode::error)) {
            auto rep = Report{*log_, MessageCode::error, loc_()};
            rep.out() << "expected one of ";
            ((rep.out() << " " << expected), ...);
            rep.out() << " but got " << token_;
        }
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

    //! Continue parsing an abs term after a '|' token.
    void cont_abs_() {
        assert(token_ == TokenType::bar);
        values_.emplace_back(std::in_place_type<Abs>, state_.token_line(), state_.token_column());
        consume_();
        stack_.back() = Prod::abs;
        stack_.emplace_back(Prod::term);
    }

    //! Continue parsing arguments of an abs term.
    auto cont_abs_args_() -> bool {
        auto &abs = top_<Abs>(1);
        abs.args.emplace_back(pop_<Term>());
        if (token_ == TokenType::bar) {
            auto loc = Location{Position{*file_, abs.line, abs.column},
                                Position{*file_, state_.cursor_line(), state_.cursor_column()}};
            auto args = std::move(abs.args);
            replace_<Term>(std::in_place_type<TermAbs>, std::move(loc), std::move(args));
            consume_();
            cont_expr_();
            return true;
        }
        if (token_ == TokenType::sem) {
            consume_();
            stack_.emplace_back(Prod::term);
            return true;
        }
        return expected_(TokenType::bar, TokenType::sem);
    }

    //! Continue parsing a tuple after a '(' token.
    auto cont_tup_() -> bool {
        assert(token_ == TokenType::lpar);
        push_<Tup>(state_.token_line(), state_.token_column());
        return cont_tup_args_(false);
    }

    //! Continue parsing tuple arguments.
    //!
    //! If arg is true, continue parsing a tuple after a term argument.
    //! Otherwise, continue parsing a tuple after tokens '(' or ';'.
    auto cont_tup_args_(bool arg) -> bool {
        auto &tup = top_<Tup>(arg ? 1 : 0);
        if (arg) {
            tup.tup.emplace_back(pop_<Term>());
        } else {
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
                    return expected_(TokenType::rpar, TokenType::sem, TokenType::comma);
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
                return expected_(TokenType::rpar, TokenType::sem);
            }
            // leading star that must be part of a tuple
            if (token_ == TokenType::star) {
                tup.tup.emplace_back(Projection{loc_()});
                consume_();
                if (token_ == TokenType::comma) {
                    arg = true;
                    continue;
                }
                return expected_(TokenType::comma);
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
        auto tup = pop_<Tup>();
        tup.finish_pool();
        if (auto *term = std::get_if<Term>(tup.args.size() == 1 ? &tup.args.front() : nullptr); term != nullptr) {
            push_<Term>(std::move(*term));
        } else {
            push_<Term>(std::in_place_type<TermTuple>,
                        Location{Position{*file_, tup.line, tup.column},
                                 Position{*file_, state_.cursor_line(), state_.cursor_column()}},
                        TupleElementArray{std::move(tup.args)});
        }
        consume_();
        cont_expr_();
    }

    //! Continue parsing a function assuming an at or id token was read.
    auto cont_fun_() -> bool {
        auto line = state_.token_line();
        auto column = state_.token_column();
        bool ext = token_ == TokenType::at;
        if (ext) {
            consume_();
            if (token_ != TokenType::id) {
                return expected_(TokenType::id);
            }
        }
        auto name = store_->string_ref(state_.view());
        consume_();
        if (token_ == TokenType::lpar) {
            push_<Fun>(line, column, name, ext);
            if (!cont_fun_args_(false)) {
                return false;
            }
        } else {
            auto loc = Location{Position{*file_, line, column},
                                Position{*file_, state_.cursor_line(), state_.cursor_column()}};
            if (ext) {
                push_<Term>(std::in_place_type<TermFunction>, std::move(loc), name,
                            Util::make_immutable_array<ArgumentTuple>(ArgumentTuple{{}}), true);
            } else {
                push_<Term>(std::in_place_type<TermSymbol>, std::move(loc), store_->fun_ref(name, {}, false));
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
        auto &fun = top_<Fun>(arg ? 1 : 0);
        if (arg) {
            fun.tup.emplace_back(pop_<Term>());
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
                    return expected_(TokenType::rpar, TokenType::comma, TokenType::sem);
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
        auto fun = pop_<Fun>();
        fun.finish_pool();
        push_<Term>(std::in_place_type<TermFunction>,
                    Location{Position{*file_, fun.line, fun.column},
                             Position{*file_, state_.cursor_line(), state_.cursor_column()}},
                    fun.name, PoolArray{std::move(fun.args)}, fun.external);
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
        push_<Term>(std::in_place_type<TermSymbol>, loc_(), store_->num_ref(Number{buf_.c_str(), base}));
        consume_();
        cont_expr_();
    }

    //! Continue parsing a string term assuming a str token is on the stack.
    void cont_str_() {
        assert(token_ == TokenType::str);
        auto view = state_.view();
        buf_.clear();
        buf_.reserve(view.size() - 2);
        Util::unquote(view.substr(1, view.size() - 2), std::back_inserter(buf_));
        auto str = store_->string_ref(std::string_view{buf_.begin(), buf_.end()});
        push_<Term>(std::in_place_type<TermSymbol>, loc_(), SymbolStore::str_ref(str));
        consume_();
        cont_expr_();
    }

    //! Continue parsing a variable term assuming a var or anon token is on the stack.
    void cont_var_(bool anonymous) {
        assert(token_ == (anonymous ? TokenType::anon : TokenType::var));
        auto str = store_->string_ref(state_.view());
        push_<Term>(std::in_place_type<TermVariable>, loc_(), str, anonymous);
        consume_();
        cont_expr_();
    }

    //! Parse a term.
    //!
    //! Uses a hand written bottom up parser with a stack to avoid stack
    //! overflows.
    auto parse_term_() -> bool {
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
                    auto rhs = pop_<Term>();
                    auto lhs = pop_<Term>();
                    auto loc = location(lhs) + location(rhs);
                    push_<Term>(std::in_place_type<TermBinary>, loc, std::move(lhs), map_binop(stack_.back()),
                                std::move(rhs));
                    cont_expr_();
                    continue;
                }
                case Prod::uminus:
                case Prod::bneg: {
                    auto rhs = pop_<Term>();
                    auto [line, column] = pop_<Pos>();
                    auto loc = Position{*file_, line, column} + location(rhs);
                    push_<Term>(std::in_place_type<TermUnary>, loc, map_unop(stack_.back()), std::move(rhs));
                    cont_expr_();
                    continue;
                }
                case Prod::term: {
                    switch (token_) {
                        case TokenType::minus:
                        case TokenType::tilde: {
                            auto unop = map_unop(token_);
                            push_<Pos>(state_.token_line(), state_.token_column());
                            consume_();
                            stack_.back() = unop;
                            stack_.push_back(Prod::term);
                            continue;
                        }
                        case TokenType::sup: {
                            push_<Term>(std::in_place_type<TermSymbol>, loc_(), SymbolStore::sup());
                            consume_();
                            cont_expr_();
                            continue;
                        }
                        case TokenType::inf: {
                            push_<Term>(std::in_place_type<TermSymbol>, loc_(), SymbolStore::inf());
                            consume_();
                            cont_expr_();
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
                            // Note: could also report that a term is expected
                            return expected_(TokenType::bar, TokenType::tilde, TokenType::minus, TokenType::anon,
                                             TokenType::lpar, TokenType::sup, TokenType::inf, TokenType::num,
                                             TokenType::str, TokenType::var, TokenType::id);
                        }
                    }
                }
                case Prod::abs: {
                    if (!cont_abs_args_()) {
                        return false;
                    }
                    continue;
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

    using Value = std::variant<Pos, Term, Abs, Fun, Tup>;

    LexerState state_;
    Logger *log_;
    SymbolStore *store_;
    SharedString file_;
    std::vector<Prod> stack_;
    std::vector<Value> values_;
    std::string buf_;
    TokenType token_ = TokenType::error;
};

Parser::Parser(Logger &log, SymbolStore &store, std::istream &in, String file)
    : impl_{std::make_unique<Impl>(log, store, in, file)} {}

Parser::Parser(Logger &log, SymbolStore &store, std::string_view in, String file)
    : impl_{std::make_unique<Impl>(log, store, in, file)} {}

Parser::Parser(Parser &&other) noexcept = default;

auto Parser::operator=(Parser &&other) noexcept -> Parser & = default;

Parser::~Parser() noexcept = default;

auto Parser::parse_term() -> std::optional<Term> {
    auto lock = GCLock{impl_->store()};
    if (auto res = impl_->parse_term(); res && check_term(impl_->log(), *res)) {
        return res;
    }
    return std::nullopt;
}

#include "algo/parse/lexer_impl.hh"

} // namespace Gringo::Input
