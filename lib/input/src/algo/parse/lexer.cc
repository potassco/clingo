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

    auto parse_fun() -> bool {
        if (!branch(TokenType::lpar)) {
            return true;
        }
        if (branch(TokenType::comma)) {
            return require(TokenType::rpar);
        }
        while (true) {
            if (branch(TokenType::rpar)) {
                return true;
            }
            if (!parse_atomic_term()) {
                return false;
            }
            if (!branch(TokenType::comma)) {
                return require(TokenType::rpar);
            }
        }
    }

    auto parse_atomic_term() -> bool {
        if (branch(TokenType::num)) {
            return true;
        }
        if (branch(TokenType::str)) {
            return true;
        }
        if (branch(TokenType::anon)) {
            return true;
        }
        if (branch(TokenType::var)) {
            return true;
        }
        if (branch(TokenType::id)) {
            return parse_fun();
        }
        // TODO: tuples
        if (branch(TokenType::lpar)) {
            return parse_term() && require(TokenType::rpar);
        }
        return false;
    }

    auto parse_term() -> bool {
        // TODO
        return parse_atomic_term();
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
        if (parse2() && require(TokenType::end)) {
            std::cerr << "parsing successful" << std::endl;
        } else {
            std::cerr << "parsing failed" << std::endl;
        }
    }

    auto parse2() -> bool {
        stack_.emplace_back(Prod::term);
        while (!stack_.empty()) {
            switch (stack_.back()) {
                case Prod::term: {
                    if (branch(TokenType::num)) {
                        stack_.pop_back();
                        continue;
                    }
                    if (branch(TokenType::str)) {
                        stack_.pop_back();
                        continue;
                    }
                    if (branch(TokenType::anon)) {
                        stack_.pop_back();
                        continue;
                    }
                    if (branch(TokenType::var)) {
                        stack_.pop_back();
                        continue;
                    }
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
                        stack_.pop_back();
                        continue;
                    }
                    return false;
                }
                case Prod::fun: {
                    if (branch(TokenType::rpar)) {
                        stack_.pop_back();
                        continue;
                    }
                    if (branch(TokenType::comma)) {
                        stack_.push_back(Prod::term);
                        continue;
                    }
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
                    return false;
                }
            }
        }
        return true;
    }

  private:
    enum class Prod : uint8_t { term, fun };
    LexerState state_;
    std::vector<Prod> stack_;
    TokenType token_ = TokenType::error;
};

// NOLINTEND(performance-avoid-endl)

void test() {
    std::istringstream iss(R"(f("a", _, X, 1, g(;f,x;;g;)))");
    auto parser = Parser{std::make_unique<std::istringstream>(std::move(iss))};
    parser.parse();
}

#include "algo/parse/lexer_impl.hh"

} // namespace Gringo::Input
