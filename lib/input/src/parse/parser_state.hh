#pragma once

#include <algorithm>
#include <clingo/input/statement.hh>

#include <clingo/core/backend.hh>
#include <clingo/core/logger.hh>

#include <clingo/util/macro.hh>
#include <clingo/util/string.hh>

#include "lexer_state.hh"

#include <deque>

namespace CppClingo::Input::Parse {

//! The available tokens produced by the lexer.
enum class TokenType : uint8_t {
    amp,
    anon,
    at,
    bar,
    begin,
    bslash,
    caret,
    colon,
    comma,
    const_,
    count,
    ddot,
    defined,
    dot,
    dstar,
    edge,
    end,
    eq,
    error,
    error_bc,
    external,
    false_,
    ge,
    gt,
    heuristic,
    id,
    if_,
    include,
    inf,
    lbrace,
    lbrack,
    le,
    lpar,
    lt,
    max,
    maximize,
    min,
    minimize,
    minus,
    ne,
    not_,
    num,
    parts,
    plus,
    program,
    project,
    qmark,
    rbrace,
    rbrack,
    rpar,
    script,
    script_content,
    script_end,
    sem,
    show,
    slash,
    star,
    str,
    str_include,
    sum,
    sump,
    sup,
    theory,
    theory_op,
    tilde,
    true_,
    var,
    wif,
    aspif,
};

enum class AspifToken : uint8_t {
    space,
    newline,
    end,
    error,
    str,
    num_pos,
    num_neg,
    incremental,
    symbols,
};

#include "parse/lexer_impl_h.hh"

//! The list of lexer conditions for stateful lexing.
enum class Condition : uint8_t {
    program = yycprogram,
    normal = yycnormal,
    theory = yyctheory,
    script = yycscript,
    include = yycinclude,
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
        case TokenType::const_: {
            return out << "#const";
        }
        case TokenType::count: {
            return out << "#count";
        }
        case TokenType::defined: {
            return out << "#defined";
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
        case TokenType::edge: {
            return out << "#edge";
        }
        case TokenType::begin: {
            return out << "<bof>";
        }
        case TokenType::end: {
            return out << "<eof>";
        }
        case TokenType::error: {
            return out << "<error>";
        }
        case TokenType::error_bc: {
            return out << "<unclosed-block-comment>";
        }
        case TokenType::external: {
            return out << "#external";
        }
        case TokenType::false_: {
            return out << "#false";
        }
        case TokenType::heuristic: {
            return out << "#heuristic";
        }
        case TokenType::id: {
            return out << "<identifier>";
        }
        case TokenType::include: {
            return out << "#include";
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
        case TokenType::maximize: {
            return out << "#maximize";
        }
        case TokenType::minimize: {
            return out << "#minimize";
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
        case TokenType::parts: {
            return out << "#parts";
        }
        case TokenType::plus: {
            return out << "'+'";
        }
        case TokenType::program: {
            return out << "#program";
        }
        case TokenType::project: {
            return out << "#project";
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
        case TokenType::if_: {
            return out << "':-'";
        }
        case TokenType::script: {
            return out << "#script";
        }
        case TokenType::script_content: {
            return out << "<content>";
        }
        case TokenType::script_end: {
            return out << "#end";
        }
        case TokenType::sem: {
            return out << "';'";
        }
        case TokenType::show: {
            return out << "#show";
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
        case TokenType::str_include: {
            return out << "<include-string>";
        }
        case TokenType::sum: {
            return out << "#sum";
        }
        case TokenType::sump: {
            return out << "#sum+";
        }
        case TokenType::sup: {
            return out << "#sup";
        }
        case TokenType::true_: {
            return out << "#true";
        }
        case TokenType::inf: {
            return out << "#inf";
        }
        case TokenType::theory: {
            return out << "#theory";
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
        case TokenType::max: {
            return out << "#max";
        }
        case TokenType::min: {
            return out << "#min";
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
        case TokenType::wif: {
            return out << "':-'";
        }
        case TokenType::aspif: {
            return out << "asp";
        }
    }
    return out;
}

//! The available productions.
enum class Prod : uint8_t {
    ty_term,
    ty_fun,
    ty_seq,
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

struct TyTerm {
    TyTerm(size_t line, size_t column) : line{line}, column{column} {}
    size_t line;
    size_t column;
    std::vector<UnparsedElement> elems;
};

struct TyFun {
    TyFun(size_t line, size_t column, String name, StringVec ops)
        : line{line}, column{column}, ops{std::move(ops)}, name{name} {}
    size_t line;
    size_t column;
    StringVec ops;
    String name;
    std::vector<TheoryTerm> args;
};

struct TySeq {
    TySeq(size_t line, size_t column, StringVec ops, TheoryTermTupleType type)
        : line{line}, column{column}, ops{std::move(ops)}, type{type}, tuple{type != TheoryTermTupleType::tuple} {}
    size_t line;
    size_t column;
    StringVec ops;
    std::vector<TheoryTerm> args;
    TheoryTermTupleType type;
    bool tuple;
};

//! Capture a partial function term.
struct SymFun {
    SymFun(String name) : name{name} {}
    std::vector<Symbol> args;
    String name;
};

//! Capture a partial tuple term.
struct SymTup {
    SymTup() = default;

    std::vector<Symbol> tup;
    bool term = true;
};

//! The parser implementation.
class ParserState {
  public:
    //! Contstructor.
    //!
    //! The optional backends should be given for aspif parsing.
    ParserState(Logger &log, SymbolStore &store, ProgramBackend *prg_backend = nullptr,
                TheoryBackend *thy_backend = nullptr)
        : log_{&log}, store_{&store}, prg_backend_{prg_backend}, thy_backend_{thy_backend} {}

    //! Initialize the parser state with the given string.
    void init(std::string_view in, String file) {
        mark_ = 0;
        stms_.clear();
        token_ = TokenType::begin;
        file_ = SharedString{file};
        cond_ = yycnormal;
        state_.init(in, YYMAXFILL);
        prg_backend_ = nullptr;
        thy_backend_ = nullptr;
    }

    //! Initialize the parser state with the given stream.
    void init(std::istream &in, String file) {
        mark_ = 0;
        stms_.clear();
        token_ = TokenType::begin;
        file_ = SharedString{file};
        cond_ = prg_backend_ != nullptr && thy_backend_ != nullptr ? yycprogram : yycnormal;
        state_.init(in, YYMAXFILL);
    }

    //! Get the associated symbol store.
    [[nodiscard]] auto store() const -> SymbolStore & { return *store_; }

    //! Get the associated logger.
    [[nodiscard]] auto log() const -> Logger & { return *log_; }

    //! Get the string representation of the current token.
    auto view() -> std::string_view { return state_.view(); }

    //! Get the string representation of a token.
    //!
    //! The function removes quotes from str tokens.
    auto str() -> String {
        if (token() == TokenType::str || token() == TokenType::str_include) {
            auto view = this->view();
            auto &buf = this->buf(view.size() - 2);
            Util::unquote(view.substr(1, view.size() - 2), std::back_inserter(buf));
            return store().string_ref(std::string_view{buf.begin(), buf.end()});
        }
        return store().string_ref(view());
    }

    //! Get the numeric representation of a num token.
    auto num() -> Number {
        assert(token() == TokenType::num);
        auto view = this->view();
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
        auto &buf = this->buf(view.size());
        std::ranges::copy_if(view, std::back_inserter(buf), [](char c) { return c != '\''; });
        return Number{buf.c_str(), base};
    }

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

    //! Mark number of current statements.
    void mark_stms() { mark_ = stms_.size(); }

    //! Check if there are stored statements.
    [[nodiscard]] auto has_stms() const -> bool { return mark_ > 0; }

    //! Pop a stored statement.
    auto pop_stm() -> std::optional<Stm> {
        if (mark_ > 0) {
            --mark_;
            auto res = std::move(stms_.front());
            stms_.pop_front();
            return res;
        }
        return std::nullopt;
    }

    //! Add a comment.
    void push_comment(bool bc = false) {
        auto x = view();
        stms_.emplace_back(StmComment{loc(), bc ? CommentType::block : CommentType::line, store().string_ref(x)});
        state_.start();
    }

    //! Add a statement.
    void push_stm(Stm stm) {
        // Note: insertion is avoidable (but should also not be harmful here)
        stms_.insert(std::next(stms_.begin(), static_cast<ssize_t>(mark_)), std::move(stm));
        ++mark_;
    }

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
    template <class T, class... U> void push_value(U &&...args) {
        values_.emplace_back(std::in_place_type<T>, std::forward<U>(args)...);
    }

    //! Replace the last element on the value stack.
    template <class T, class... U> void replace_value(U &&...args) {
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

    //! Check if the current token is among the given ones.
    auto expect(auto... expected) {
        CLINGO_IGNORE_PAR_EQ_B
        if (((token_ == expected) || ...)) {
            return true;
        }
        CLINGO_IGNORE_PAR_EQ_E
        return this->expected(expected...);
    }

    //! Compute the next token discarding the last one.
    void consume() { token_ = lex_(); }

    //! Compute the next aspif token (excluding strings).
    //!
    //! Requires the parser to be in aspif mode.
    auto lex_aspif() -> AspifToken;

    //! Try to lex a newline-terminated string.
    //!
    //! Requires the parser to be in aspif mode.
    auto lex_str() -> AspifToken;

    //! Try to lex a string with the give size.
    //!
    //! Requires the parser to be in aspif mode.
    auto lex_str(size_t n) -> AspifToken;

    //! Get the associated program backend.
    auto prg_backend() -> ProgramBackend * { return prg_backend_; }

    //! Get the associated theory backend.
    auto thy_backend() -> TheoryBackend * { return thy_backend_; }

    //! Compute the next token discarding the last one.
    void consume(Condition cond) {
        auto old = cond_;
        condition(cond);
        consume();
        cond_ = old;
    }

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

    //! Set the lexer condition.
    void condition(Condition cond) { cond_ = static_cast<int>(cond); }

    //! Parse as long as the current token matches one of the given ones.
    template <class P, class... C>
    auto while_token(P parse, C... cond)
        -> std::optional<std::vector<typename std::invoke_result_t<P, ParserState &>::value_type>> {
        auto elems = std::vector<typename std::invoke_result_t<P, ParserState &>::value_type>{};
        while (((token() == cond) || ...)) {
            if (auto elem = std::invoke(parse, *this); elem) {
                elems.emplace_back(*std::move(elem));
            } else {
                return std::nullopt;
            }
        }
        return elems;
    }

    //! Repeatedly consume token and parse until a stop token is reached.
    //!
    //! The stop token is not consumed. Allows for parsing empty lists.
    template <class P, class... S>
    auto repeat_until(TokenType sep, P parse, S... stop)
        -> std::optional<std::vector<typename std::invoke_result_t<P, ParserState &>::value_type>> {
        auto elems = std::vector<typename std::invoke_result_t<P, ParserState &>::value_type>{};
        if (((token() != stop) && ...)) {
            while (token() == sep) {
                consume();
                if (auto elem = std::invoke(parse, *this); elem) {
                    elems.emplace_back(*std::move(elem));
                } else {
                    return std::nullopt;
                }
                if (((token() == stop) || ...)) {
                    break;
                }
            }
        }
        return elems;
    }

    //! Parse a separated list until a stop token is reached.
    //!
    //! The stop token is not consumed. Allows for parsing empty lists.
    template <class P, class... S>
    auto separated_until(P parse, TokenType sep, S... stop)
        -> std::optional<std::vector<typename std::invoke_result_t<P, ParserState &>::value_type>> {
        auto elems = std::vector<typename std::invoke_result_t<P, ParserState &>::value_type>{};
        if (((token() != stop) && ...)) {
            while (true) {
                if (auto elem = std::invoke(parse, *this); elem) {
                    elems.emplace_back(*std::move(elem));
                } else {
                    return std::nullopt;
                }
                if (((token() == stop) || ...)) {
                    break;
                }
                if (token() != sep) {
                    return expected<std::nullopt>(sep, stop...);
                }
                consume();
            }
        }
        return elems;
    }

    //! Parse a delimited list.
    //!
    //! Note that this does not consume the closing delimiter providing a
    //! convenient way to obtain the end of the list.
    template <class P>
    auto delimited(TokenType open, P parse, TokenType sep, TokenType close)
        -> std::optional<std::vector<typename std::invoke_result_t<P, ParserState &>::value_type>> {
        if (token() != open) {
            return expected<std::nullopt>(open);
        }
        consume();
        return separated_until(std::move(parse), sep, close);
    }

  private:
    using Value = std::variant<Pos, Term, Abs, Fun, Tup, TyTerm, TyFun, TySeq, Symbol, SymFun, SymTup>;

    //! Compute the next token.
    auto lex_() -> TokenType;

    LexerState state_;
    Logger *log_;
    SymbolStore *store_;
    ProgramBackend *prg_backend_ = nullptr;
    TheoryBackend *thy_backend_ = nullptr;
    SharedString file_;
    std::vector<Prod> stack_;
    std::vector<Value> values_;
    std::deque<Stm> stms_;
    std::string buf_;
    size_t mark_ = 0;
    int cond_ = yycnormal;
    int bc_cond_ = 0;
    int bc_ = 0;
    TokenType token_ = TokenType::begin;
};

//! RAII helper to set and restore a condition.
struct set_condition {
    set_condition(ParserState &state, Condition init, Condition restore) : state{&state}, restore{restore} {
        state.condition(init);
    }
    ~set_condition() { state->condition(restore); }
    ParserState *state;
    Condition restore;
};

//! Parse a sign.
auto parse_sign(ParserState &state) -> Sign;

//! Check if a term can start with the given token.
auto check_term(TokenType token) -> bool;

//! Parse a term.
//!
//! Uses a hand written bottom up parser with a stack to avoid stack
//! overflows.
auto parse_term(ParserState &state) -> std::optional<Term>;

//! Parse a symbol.
auto parse_symbol(ParserState &state) -> std::optional<SharedSymbol>;

//! Parse the program params to ground.
auto parse_program_parts(ParserState &state, TokenType end) -> std::optional<ProgramParamVec>;

//! Parse a constant definition.
auto parse_const_def(ParserState &state) -> std::optional<std::pair<SharedString, SharedSymbol>>;

//! Parse a theory term.
//!
//! Uses a hand written bottom up parser with a stack to avoid stack overflows.
//! Furthermore, the function assumes that the lexer is in theory mode.
auto parse_theory_term(ParserState &state) -> std::optional<TheoryTerm>;

//! Parse the name, elements, and guard of a theory atom.
//!
//! Assumes that the current token is '&'.
auto parse_theory_atom(ParserState &state)
    -> std::optional<std::tuple<Term, TheoryElementArray, std::optional<TheoryRGuard>, Position>>;

//! Check if the token is a relation.
auto check_relation(TokenType token) -> std::optional<Relation>;

//! Parse atom arguments enclode in parentheses.
//!
//! Does not consume the closing parenthesis
auto parse_args(ParserState &state) -> std::optional<PoolArray>;

//! Continue parsing a comparison literal.
auto cont_literal(ParserState &state, Position pos, Sign sign, Term term, Relation rel) -> std::optional<Lit>;

//! Continue parsing a symbolic or comparison literal.
auto cont_literal(ParserState &state, Position pos, Sign sign, Term term) -> std::optional<Lit>;

//! Continue parsing a literal.
auto cont_literal(ParserState &state, Position pos, Sign sign) -> std::optional<Lit>;

//! Parse a literal.
auto parse_literal(ParserState &state) -> std::optional<Lit>;

//! Parse a set aggregate element.
auto parse_set_aggr_elem(ParserState &state) -> std::optional<SetAggregateElement>;

//! Parse an optional rhs guard.
auto parse_rguard(ParserState &state) -> std::optional<RGuard>;

//! Check if the token represents an aggregate function.
auto check_aggregate(TokenType token) -> std::optional<AggregateFunction>;

//! Parse a body literal.
auto parse_body_literal(ParserState &state) -> std::optional<BdLit>;

//! Parse a head literal.
auto parse_head_literal(ParserState &state) -> std::optional<HdLit>;

//! Parse a statement.
auto parse_statement(ParserState &state) -> std::optional<Stm>;

//! Parse aspif.
auto parse_aspif(ParserState &state) -> bool;

//! Scan next statement.
auto scan_statement(ParserState &state) -> std::pair<std::optional<Stm>, bool>;

} // namespace CppClingo::Input::Parse
