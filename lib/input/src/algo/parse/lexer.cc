#include "lexer_state.hh"

#include <iostream>
#include <sstream>

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

static auto lex(LexerState &state, Condition cond) -> TokenType;

// NOLINTBEGIN(performance-avoid-endl)

class Parser {
  public:
    Parser(std::unique_ptr<std::istream> in) : state_{std::move(in)} {}

    void parse() {
        // stack:
        // function: name args
        //
        consume();
        if (parse_term() && branch(TokenType::end)) {
            std::cerr << "parsing successful" << std::endl;
        } else {
            std::cerr << "parsing failed" << std::endl;
        }
    }

  private:
    enum class Prod : uint8_t { term, fun, add, sub, mul, exp, uminus, bneg, div, mod, band, bor, bxor, interval };

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

    static auto right_assoc(Prod prod) { return prod == Prod::exp; }

    void consume() { token_ = lex(state_, Condition::normal); }

    auto branch(TokenType token) -> bool {
        if (token_ == token) {
            consume();
            return true;
        }
        return false;
    }

    auto branch_binop() -> std::optional<Prod> {
        if (branch(TokenType::dstar)) {
            return Prod::exp;
        }
        if (branch(TokenType::slash)) {
            return Prod::div;
        }
        if (branch(TokenType::bslash)) {
            return Prod::mod;
        }
        if (branch(TokenType::star)) {
            return Prod::mul;
        }
        if (branch(TokenType::minus)) {
            return Prod::sub;
        }
        if (branch(TokenType::plus)) {
            return Prod::add;
        }
        if (branch(TokenType::amp)) {
            return Prod::band;
        }
        if (branch(TokenType::bar)) {
            return Prod::bor;
        }
        if (branch(TokenType::caret)) {
            return Prod::bxor;
        }
        if (branch(TokenType::ddot)) {
            return Prod::interval;
        }
        return std::nullopt;
    };

    auto branch_unop() -> std::optional<Prod> {
        if (branch(TokenType::minus)) {
            return Prod::uminus;
        }
        if (branch(TokenType::tilde)) {
            return Prod::bneg;
        }
        return std::nullopt;
    };

    auto parse_term() -> bool {
        // TODO:
        // - tuples/terms in parenthesis
        // - external functions
        // - error reporting
        // - term building using a separate stack
        stack_.emplace_back(Prod::term);

        auto cont_expression = [this] {
            stack_.pop_back();
            if (auto prod = branch_binop(); prod) {
                stack_.emplace_back(*prod);
                stack_.emplace_back(Prod::term);
            }
        };

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
                    if (branch(TokenType::num)) {
                        cont_expression();
                        continue;
                    }
                    // Term -> . str
                    if (branch(TokenType::str)) {
                        cont_expression();
                        continue;
                    }
                    // Term -> . '_'
                    if (branch(TokenType::anon)) {
                        cont_expression();
                        continue;
                    }
                    // Term -> . var
                    if (branch(TokenType::var)) {
                        cont_expression();
                        continue;
                    }
                    // Term -> . id ('(' ';'* (')' | Term Fun))?
                    if (branch(TokenType::id)) {
                        if (branch(TokenType::lpar)) {
                            while (branch(TokenType::sem)) {
                            }
                            if (!branch(TokenType::rpar)) {
                                stack_.back() = Prod::fun;
                                stack_.push_back(Prod::term);
                                continue;
                            }
                        }
                        cont_expression();
                        continue;
                    }
                    // TODO: report that '-', num, str, '_', var, or identifier is expected
                    return false;
                }
                case Prod::fun: {
                    // Fun -> . ')'
                    if (branch(TokenType::rpar)) {
                        cont_expression();
                        continue;
                    }
                    // Fun -> . ',' Term Fun
                    if (branch(TokenType::comma)) {
                        stack_.push_back(Prod::term);
                        continue;
                    }
                    // Fun -> . ';'+ ( ')' | Term Fun )
                    if (branch(TokenType::sem)) {
                        while (branch(TokenType::sem)) {
                        }
                        if (branch(TokenType::rpar)) {
                            stack_.pop_back();
                            continue;
                        }
                        stack_.push_back(Prod::term);
                        continue;
                    }
                    // TODO: report that ')', ',', or ';' is expected.
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

void test() {
    std::istringstream iss(R"(f("a", _, X * 2 + 1, -1+2*3, g(;f,x;;g;)))");
    auto parser = Parser{std::make_unique<std::istringstream>(std::move(iss))};
    parser.parse();
}

#include "algo/parse/lexer_impl.hh"

} // namespace Gringo::Input
