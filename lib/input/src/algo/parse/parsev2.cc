#include <gringo/input/algo/parsev2.hh>

#include "lexer_state.hh"

#include <iostream>

namespace Gringo::Input {

enum class Condition : uint8_t {
    normal,
};

enum class TokenType : uint8_t {
    amp,
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
    Impl(std::istream &in) : state_{in} {}

    auto parse_term() -> bool {
        consume_();
        return parse_term_() && branch_(TokenType::end);
    }

  private:
    enum class Prod : uint8_t { term, fun, add, sub, mul, exp, uminus, bneg, div, mod, band, bor, bxor, interval, tup };

    auto lex_(Condition cond) -> TokenType;

    auto expected(auto... expected) -> bool {
        std::cerr << "<input>:" << state_.token_line() << ":" << state_.token_column() << ": expected one of ";
        ((std::cerr << " " << expected), ...);
        std::cerr << " but got " << token_;
        return false;
    }

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

    static auto right_assoc(Prod prod) -> bool { return prod == Prod::exp; }

    void consume_() { token_ = lex_(Condition::normal); }

    auto branch_(TokenType token) -> bool {
        if (token_ == token) {
            consume_();
            return true;
        }
        return false;
    }

    auto branch_binop() -> std::optional<Prod> {
        if (branch_(TokenType::dstar)) {
            return Prod::exp;
        }
        if (branch_(TokenType::slash)) {
            return Prod::div;
        }
        if (branch_(TokenType::bslash)) {
            return Prod::mod;
        }
        if (branch_(TokenType::star)) {
            return Prod::mul;
        }
        if (branch_(TokenType::minus)) {
            return Prod::sub;
        }
        if (branch_(TokenType::plus)) {
            return Prod::add;
        }
        if (branch_(TokenType::amp)) {
            return Prod::band;
        }
        if (branch_(TokenType::bar)) {
            return Prod::bor;
        }
        if (branch_(TokenType::caret)) {
            return Prod::bxor;
        }
        if (branch_(TokenType::ddot)) {
            return Prod::interval;
        }
        return std::nullopt;
    };

    auto branch_unop() -> std::optional<Prod> {
        if (branch_(TokenType::minus)) {
            return Prod::uminus;
        }
        if (branch_(TokenType::tilde)) {
            return Prod::bneg;
        }
        return std::nullopt;
    };

    //! Continue parsing an expression after a term.
    void cont_expression() {
        stack_.pop_back();
        if (auto prod = branch_binop(); prod) {
            stack_.emplace_back(*prod);
            stack_.emplace_back(Prod::term);
        }
    }

    //! Continue parsing a tuple after tokens '(' or ';'.
    auto init_tuple() -> bool {
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
                    cont_expression();
                    return true;
                }
                if (branch_(TokenType::sem)) {
                    continue;
                }
                return expected(TokenType::rpar, TokenType::sem);
            }
            if (branch_(TokenType::rpar)) {
                cont_expression();
                return true;
            }
            stack_.back() = Prod::tup;
            stack_.push_back(Prod::term);
            return true;
        }
    }

    //! Continue parsing a tuple after a term argument.
    auto cont_tuple() -> bool {
        if (branch_(TokenType::rpar)) {
            cont_expression();
            return true;
        }
        if (branch_(TokenType::sem)) {
            return init_tuple();
        }
        if (branch_(TokenType::comma)) {
            if (branch_(TokenType::rpar)) {
                cont_expression();
                return true;
            }
            if (branch_(TokenType::sem)) {
                return init_tuple();
            }
            stack_.push_back(Prod::term);
            return true;
        }
        return expected(TokenType::rpar, TokenType::sem, TokenType::comma);
    }

    //! Continue parsing a function arguments after tokens '(' or ';'.
    void init_fun() {
        while (branch_(TokenType::sem)) {
        }
        if (branch_(TokenType::rpar)) {
            cont_expression();
        } else {
            stack_.back() = Prod::fun;
            stack_.push_back(Prod::term);
        }
    }

    //! Continue parsing function arguments after a term argument.
    auto cont_fun() -> bool {
        // Fun -> . ')'
        if (branch_(TokenType::rpar)) {
            cont_expression();
            return true;
        }
        // Fun -> . ',' Term Fun
        if (branch_(TokenType::comma)) {
            stack_.push_back(Prod::term);
            return true;
        }
        // Fun -> . ';'+ ( ')' | Term Fun )
        if (branch_(TokenType::sem)) {
            init_fun();
            return true;
        }
        return expected(TokenType::rpar, TokenType::comma, TokenType::sem);
    }

    //! Parse a term.
    //!
    //! Uses a hand written bottom up parser with a stack to avoid stack
    //! overflows.
    auto parse_term_() -> bool {
        // TODO:
        // - error reporting via logger (almost there)
        // - term building using a separate stack
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
                    auto pre = stack_.back();
                    if (auto cur = branch_binop(); cur) {
                        auto pp = priority(pre);
                        auto pc = priority(*cur);
                        if (pp < pc || (pp == pc && right_assoc(*cur))) {
                            // Term -> Term pre (Term . cur Term)
                            stack_.push_back(*cur);
                        } else {
                            // Term -> (Term pre Term) . cur Term
                            stack_.back() = *cur;
                        }
                        stack_.push_back(Prod::term);
                    } else {
                        // Term -> (Term pre Term) .
                        stack_.pop_back();
                    }
                    continue;
                }
                case Prod::uminus:
                case Prod::bneg: {
                    auto pre = stack_.back();
                    if (auto cur = branch_unop(); cur) {
                        if (priority(pre) < priority(*cur)) {
                            // Term -> pre (Term . cur Term)
                            stack_.push_back(*cur);
                        } else {
                            // Term -> (pre Term) . cur Term
                            stack_.back() = *cur;
                        }
                        stack_.push_back(Prod::term);

                    } else {
                        // Term -> (pre Term) .
                        stack_.pop_back();
                    }
                    continue;
                }
                case Prod::term: {
                    // Term -> . '-' Term
                    if (auto unop = branch_unop(); unop) {
                        stack_.back() = *unop;
                        stack_.push_back(Prod::term);
                        continue;
                    }
                    // Term -> . num
                    if (branch_(TokenType::num)) {
                        cont_expression();
                        continue;
                    }
                    // Term -> . str
                    if (branch_(TokenType::str)) {
                        cont_expression();
                        continue;
                    }
                    // Term -> . '_'
                    if (branch_(TokenType::anon)) {
                        cont_expression();
                        continue;
                    }
                    // Term -> . var
                    if (branch_(TokenType::var)) {
                        cont_expression();
                        continue;
                    }
                    // Term -> . id '(' ...
                    bool ext = branch_(TokenType::amp);
                    if (branch_(TokenType::id)) {
                        if (branch_(TokenType::lpar)) {
                            init_fun();
                        } else {
                            cont_expression();
                        }
                        continue;
                    }
                    if (ext) {
                        return expected(TokenType::id);
                    }
                    // Term -> . '(' ...
                    if (branch_(TokenType::lpar)) {
                        if (init_tuple()) {
                            continue;
                        }
                        return false;
                    }
                    return expected(TokenType::minus, TokenType::anon, TokenType::lpar, TokenType::num, TokenType::str,
                                    TokenType::var, TokenType::id);
                }
                case Prod::tup: {
                    if (cont_tuple()) {
                        continue;
                    }
                    return false;
                }
                case Prod::fun: {
                    if (cont_fun()) {
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
    TokenType token_ = TokenType::error;
};

// NOLINTEND(performance-avoid-endl)

Parser::Parser(std::istream &in) : impl_{std::make_unique<Impl>(in)} {}

Parser::Parser(Parser &&other) noexcept = default;

auto Parser::operator=(Parser &&other) noexcept -> Parser & = default;

Parser::~Parser() noexcept = default;

auto Parser::parse_term() -> bool { return impl_->parse_term(); }

#include "algo/parse/lexer_impl.hh"

} // namespace Gringo::Input
