#include <clingo/util/string.hh>

#include "parser_state.hh"

namespace Clingo::Input::Parse {

namespace {

class token_error : public std::exception {
  public:
    token_error(AspifToken expected) : expected_{expected} {}
    [[nodiscard]] auto what() const noexcept -> char const * override { return "unexpected aspif token"; }
    [[nodiscard]] auto expected() const -> AspifToken { return expected_; }

  private:
    AspifToken expected_;
};

class value_error : public std::exception {
  public:
    [[nodiscard]] auto what() const noexcept -> char const * override { return "unexpected aspif value"; }
};

class AspifParser {
  public:
    AspifParser(ParserState &state) : state_{&state} {}

    //! Parses a program in aspif format assuming that the lexer currently sits
    //! on the the "asp" token.
    void parse() {
        try {
            preamble_();
        } catch (token_error const &e) {
            // TODO: report the preamble parsing failed
            recover_();
        }
        while (true) {
            try {
                auto type = expect_unsigned_();
                if (type == 0) {
                    try {
                        expect_(AspifToken::newline);
                        expect_(AspifToken::end);

                    } catch (token_error const &e) {
                        // TODO: report that there are unexpected characters
                        // after the terminating directive
                        static_cast<void>(e);
                    }

                } else {
                    statement_(type);
                }
            } catch (token_error const &e) {
                // TODO: report the statement parsing failed
                recover_();
            } catch ([[maybe_unused]] value_error const &e) {
                // NOTE: value errors are reported where they occur
                recover_();
            }
        }
    }

  private:
    void statement_(unsigned type) {
        switch (type) {
            case 1: {
                static_cast<void>(this);
                throw std::logic_error{"implement me!!!"};
            }
            default: {
                throw std::logic_error{"handle me gracefully"};
            }
        }
    }
    void preamble_() {
        auto major = expect_unsigned_();
        expect_(AspifToken::space);
        auto minor = expect_unsigned_();
        expect_(AspifToken::space);
        auto revision = expect_unsigned_();
        expect_(AspifToken::space);
        expect_(AspifToken::newline);
        // TODO: parse tags
        // TODO: report preamble
        static_cast<void>(major);
        static_cast<void>(minor);
        static_cast<void>(revision);
    }

    void recover_() {
        auto gobble = state_->lex_str();
        if (gobble == AspifToken::str) {
            state_->lex_aspif();
        } else {
            if (gobble == AspifToken::end) {
                // TODO: report
                throw std::runtime_error("unexpected end of file");
            }
            // TODO: report
            throw std::runtime_error("unexpected token");
        }
    }

    auto expect_signed_() -> int {
        auto token = state_->lex_aspif();
        if (token == AspifToken::num_pos || token == AspifToken::num_neg) {
            auto str = state_->view();
            int res = 0;
            std::from_chars(str.begin(), str.end(), res);
            return res;
        }
        has_error_ = true;
        throw token_error{AspifToken::num_neg};
    }

    auto expect_unsigned_() -> unsigned {
        expect_(AspifToken::num_pos);
        auto str = state_->view();
        unsigned res = 0;
        std::from_chars(str.begin(), str.end(), res);
        return res;
    }

    void expect_(AspifToken token) {
        if (state_->lex_aspif() != token) {
            has_error_ = true;
            throw token_error{token};
        }
    }

    ParserState *state_;
    bool has_error_ = false;
};

} // namespace

auto parse_aspif(ParserState &state) {
    // TODO: this most likely has to be called manually befor scan statement.
    // We simply set the parse mode to program; if the parser encounters an
    // aspif preamble it switches to aspif parsing mode and, otherwise, goes
    // into normal mode after which scan statement can be called as usual.
    //
    // TODO: To get aspif parsing into clingo. The ParseHelper::process method
    // should handle aspif parsing.
    //
    // TODO: This function should return some code indicating whether parsing
    // was successfull, failed, or was not even attempted because the file is
    // not in aspif format.
    AspifParser{state}.parse();
}

} // namespace Clingo::Input::Parse
