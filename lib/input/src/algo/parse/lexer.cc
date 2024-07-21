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
    var,
};

static auto lex(LexerState &state, Condition cond) -> TokenType;

// NOLINTBEGIN(performance-avoid-endl)

class Parser {
  public:
    Parser(std::unique_ptr<std::istream> in) : state_{std::move(in)} {}

    auto parse_term() -> bool {
        // TODO:
        // - tuples/terms in parenthesis
        // - remaining arithmetic operations
        //   (written compactly)
        // - external functions
        // - error reporting
        // - term building using a separate stack
        stack_.emplace_back(Prod::term);
        auto cont_expression = [this] {
            stack_.pop_back();
            if (branch(TokenType::plus)) {
                stack_.emplace_back(Prod::add);
                stack_.emplace_back(Prod::term);
            }
            if (branch(TokenType::star)) {
                stack_.emplace_back(Prod::mul);
                stack_.emplace_back(Prod::term);
            }
        };

        while (!stack_.empty()) {
            switch (stack_.back()) {
                case Prod::add: {
                    // Term -> Term '+' (Term . '*' Term)
                    if (branch(TokenType::star)) {
                        // shift
                        stack_.push_back(Prod::mul);
                        stack_.push_back(Prod::term);
                        continue;
                    }
                    // Term -> (Term '+' Term) . '+' Term
                    if (branch(TokenType::plus)) {
                        // reduce + shift
                        stack_.back() = Prod::add;
                        stack_.push_back(Prod::term);
                        continue;
                    }
                    // Term -> (Term '+' Term) .
                    stack_.pop_back();
                    continue;
                }
                case Prod::mul: {
                    // Term -> (Term '*' Term) . '*' Term
                    if (branch(TokenType::star)) {
                        // reduce + shift
                        stack_.back() = Prod::mul;
                        stack_.push_back(Prod::term);
                        continue;
                    }
                    // Term -> (Term '*' Term) . '+' Term
                    if (branch(TokenType::plus)) {
                        // reduce + shift
                        stack_.back() = Prod::mul;
                        stack_.push_back(Prod::term);
                        continue;
                    }
                    // Term -> (Term '*' Term) .
                    stack_.pop_back();
                    continue;
                }
                case Prod::uminus: {
                    // Term -> ('-' Term) . '*' Term
                    if (branch(TokenType::star)) {
                        // reduce + shift
                        stack_.back() = Prod::mul;
                        stack_.push_back(Prod::term);
                        continue;
                    }
                    // Term -> ('-' Term) . '+' Term
                    if (branch(TokenType::plus)) {
                        // reduce + shift
                        stack_.back() = Prod::add;
                        stack_.push_back(Prod::term);
                        continue;
                    }
                    // Term -> ('-' Term) .
                    stack_.pop_back();
                    continue;
                }
                case Prod::term: {
                    // Term -> . '-' Term
                    if (branch(TokenType::minus)) {
                        stack_.back() = Prod::uminus;
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

    auto branch(TokenType token) -> bool {
        if (token_ == token) {
            consume();
            return true;
        }
        return false;
    }

    auto require(TokenType token) -> bool {
        auto ret = token_ == token;
        consume();
        return ret;
    }

    void consume() { token_ = lex(state_, Condition::normal); }

    void parse() {
        // stack:
        // function: name args
        //
        consume();
        if (parse_term() && require(TokenType::end)) {
            std::cerr << "parsing successful" << std::endl;
        } else {
            std::cerr << "parsing failed" << std::endl;
        }
    }

  private:
    enum class Prod : uint8_t { term, fun, add, mul, uminus };
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
