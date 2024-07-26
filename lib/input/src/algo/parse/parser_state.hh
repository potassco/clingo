#pragma once

#include <gringo/input/statement.hh>

#include <gringo/core/logger.hh>

#include "lexer_state.hh"

namespace Gringo::Input::Parse {

//! The list of lexer conditions for stateful lexing.
enum class Condition : uint8_t {
    normal,
};

//! The available tokens produced by the lexer.
enum class TokenType : uint8_t {
    amp,
    anon,
    at,
    bar,
    bslash,
    caret,
    colon,
    comma,
    ddot,
    dot,
    dstar,
    end,
    eq,
    error,
    false_,
    ge,
    gt,
    id,
    inf,
    lbrace,
    lbrack,
    le,
    lpar,
    lt,
    minus,
    ne,
    not_,
    num,
    plus,
    qmark,
    rbrace,
    rbrack,
    rpar,
    sem,
    slash,
    star,
    str,
    sup,
    theory_op,
    tilde,
    true_,
    var,
};

//! Output token in human readable form.
inline auto operator<<(std::ostream &out, TokenType token) -> std::ostream & {
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
        case TokenType::false_: {
            return out << "'#false'";
        }
        case TokenType::id: {
            return out << "<identifier>";
        }
        case TokenType::lpar: {
            return out << "'('";
        }
        case TokenType::lbrack: {
            return out << "'['";
        }
        case TokenType::lbrace: {
            return out << "'{'";
        }
        case TokenType::minus: {
            return out << "'-'";
        }
        case TokenType::not_: {
            return out << "not";
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
        case TokenType::rbrack: {
            return out << "']'";
        }
        case TokenType::rbrace: {
            return out << "'}'";
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
        case TokenType::true_: {
            return out << "'#true'";
        }
        case TokenType::inf: {
            return out << "'#inf'";
        }
        case TokenType::theory_op: {
            return out << "<theory-operator>";
        }
        case TokenType::tilde: {
            return out << "'~'";
        }
        case TokenType::var: {
            return out << "<variable>";
        }
        case TokenType::lt: {
            return out << "<";
        }
        case TokenType::le: {
            return out << "<=";
        }
        case TokenType::gt: {
            return out << ">";
        }
        case TokenType::ge: {
            return out << ">=";
        }
        case TokenType::eq: {
            return out << "=";
        }
        case TokenType::ne: {
            return out << "!=";
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

//! The parser implementation.
class ParserState {
  public:
    //! Contstructor.
    ParserState(Logger &log, SymbolStore &store, std::istream &in, String file)
        : state_{in, YYMAXFILL}, log_{&log}, store_{&store}, file_{file} {}

    ParserState(Logger &log, SymbolStore &store, std::string_view in, String file)
        : state_{in, YYMAXFILL}, log_{&log}, store_{&store}, file_{file} {}

    [[nodiscard]] auto store() const -> SymbolStore & { return *store_; }

    [[nodiscard]] auto log() const -> Logger & { return *log_; }

    //! Get the string representation of the current token.
    auto view() -> std::string_view { return state_.view(); }

    //! Get a temporary buffer.
    auto buf(size_t n) -> std::string & {
        buf_.reserve(n);
        buf_.clear();
        return buf_;
    }

    //! Get a string identifying the current input.
    auto file() -> String { return *file_; }

    //! Get the starting line of the current token.
    auto token_line() -> size_t { return state_.token_line(); }

    //! Get the starting column of the current token.
    auto token_column() -> size_t { return state_.token_column(); }

    //! Get the ending line of the current token.
    auto cursor_line() -> size_t { return state_.cursor_line(); }

    //! Get the ending column of the current token.
    auto cursor_column() -> size_t { return state_.cursor_column(); }

    auto token_pos() -> Position { return Position{*file_, state_.token_line(), state_.token_column()}; }

    auto cursor_pos() -> Position { return Position{*file_, state_.cursor_line(), state_.cursor_column()}; }

    //! Compute the location of the current token.
    auto loc() -> Location { return Location{token_pos(), cursor_pos()}; }

    //! Get the current token.
    auto token() -> TokenType { return token_; }

    //! Initialize the state to parse a production.
    void init(Prod prod) {
        stack_.clear();
        values_.clear();
        push(prod);
    }

    //! Check if the production stack is empty.
    auto empty() -> bool { return stack_.empty(); }

    //! Pop the last production.
    auto pop() -> Prod {
        assert(!stack_.empty());
        auto ret = stack_.back();
        stack_.pop_back();
        return ret;
    }

    //! Get the last production.
    auto top() -> Prod { return stack_.back(); }

    //! Replace the last production.
    void replace(Prod prod) {
        assert(!stack_.empty());
        stack_.back() = prod;
    }

    //! Push the given production on the production stack.
    void push(Prod prod) { stack_.emplace_back(prod); }

    //! Check if the value stack is empty.
    auto empty_value() -> bool { return values_.empty(); }

    //! Pop the last element on the value stack.
    template <class T> auto pop_value() -> T {
        auto res = std::move(top_value<T>());
        values_.pop_back();
        return res;
    }

    //! Get a reference to the last element on the value stack.
    template <class T> auto top_value() -> T & {
        assert(!values_.empty() && std::holds_alternative<T>(values_.back()));
        return std::get<T>(values_.back());
    }

    //! Get a reference to the i-th last element on the value stack.
    template <class T> auto top_value(size_t i) -> T & {
        assert(i < values_.size());
        auto j = values_.size() - 1 - i;
        assert(std::holds_alternative<T>(values_[j]));
        return std::get<T>(values_[j]);
    }

    //! Push an element on the value stack.
    template <class T, class... U> auto push_value(U &&...args) {
        values_.emplace_back(std::in_place_type<T>, std::forward<U>(args)...);
    }

    //! Replace the last element on the value stack.
    template <class T, class... U> auto replace_value(U &&...args) {
        assert(!values_.empty());
        values_.back().emplace<T>(std::forward<U>(args)...);
    }

    //! Report an error message indicating that one of the given tokens was expected.
    template <auto ret = false> auto expected(auto... expected) {
        if (log_->check(MessageCode::error)) {
            auto rep = Report{*log_, MessageCode::error, loc()};
            rep.out() << "expected one of";
            ((rep.out() << " " << expected), ...);
            rep.out() << " but got " << token_;
        }
        return ret;
    }

    //! Compute the next token discarding the last one.
    void consume() { token_ = lex_(Condition::normal); }

    //! Check if the given token matches the current one.
    //!
    //! In case of a match, it consumes the token.
    auto branch(TokenType token) -> bool {
        if (token_ == token) {
            consume();
            return true;
        }
        return false;
    }

  private:
    using Value = std::variant<Pos, Term, Abs, Fun, Tup>;

    //! Compute the next token.
    auto lex_(Condition cond) -> TokenType;

    LexerState state_;
    Logger *log_;
    SymbolStore *store_;
    SharedString file_;
    std::vector<Prod> stack_;
    std::vector<Value> values_;
    std::string buf_;
    TokenType token_ = TokenType::error;
};

//! Check if a term can start with the given token.
auto check_term(TokenType token) -> bool;

//! Parse a term.
//!
//! Uses a hand written bottom up parser with a stack to avoid stack
//! overflows.
auto parse_term(ParserState &state) -> std::optional<Term>;

//! Parse a literal.
auto parse_literal(ParserState &state) -> std::optional<Lit>;

} // namespace Gringo::Input::Parse
