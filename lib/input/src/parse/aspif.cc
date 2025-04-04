#include <clingo/core/backend.hh>
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
    AspifParser(ParserState &state, ProgramBackend &backend, TheoryBackend &theory)
        : state_{&state}, backend_{&backend}, theory_{&theory} {
        // TODO: implement theory
        static_cast<void>(theory_);
    }

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
    // NOLINTNEXTLINE(performance-enum-size)
    enum class StatementType : unsigned {
        rule = 1,
    };
    // NOLINTNEXTLINE(performance-enum-size)
    enum class RuleType : unsigned {
        disjunctive = 0,
        choice = 1,
    };
    // NOLINTNEXTLINE(performance-enum-size)
    enum class BodyType : unsigned {
        normal = 0,
        weight = 1,
    };

    void statement_(unsigned type) {
        switch (static_cast<StatementType>(type)) {
            case StatementType::rule: {
                expect_(AspifToken::space);
                auto rule_type = static_cast<RuleType>(expect_unsigned_());
                expect_(AspifToken::space);
                auto m = expect_unsigned_();
                auto head = std::vector<prg_lit_t>{};
                head.reserve(m);
                for (unsigned i = 0; i < m; ++i) {
                    head.emplace_back(expect_unsigned_());
                }
                auto body_type = static_cast<BodyType>(expect_unsigned_());
                switch (body_type) {
                    case BodyType::normal: {
                        auto m = expect_unsigned_();
                        auto body = std::vector<prg_lit_t>{};
                        body.reserve(m);
                        for (unsigned i = 0; i < m; ++i) {
                            body.emplace_back(expect_signed_());
                        }
                        backend_->rule(head, body, rule_type == RuleType::choice);
                        break;
                    }
                    case BodyType::weight: {
                        auto l = expect_signed_(); // TODO: check signed/unsigned
                        auto m = expect_unsigned_();
                        auto body = WeightedPrgLitVec{};
                        body.reserve(m);
                        for (unsigned i = 0; i < m; ++i) {
                            auto lit = expect_signed_();
                            body.emplace_back(lit, expect_signed_());
                        }
                        if (head.size() != 0 || rule_type == RuleType::choice) {
                            throw std::logic_error{"the backend has to be extended to support the full aspif syntax"};
                        }
                        backend_->bd_aggr(head[0], body, l);
                        break;
                    }
                    default: {
                        throw std::logic_error{"handle me gracefully"};
                    }
                }
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
        if (expect_(AspifToken::newline, AspifToken::space) == AspifToken::space) {
            expect_(AspifToken::incremental);
            expect_(AspifToken::newline);
        }
        // TODO: extend backend to report preamble
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

    template <class... T> auto expect_(T... tokens) -> AspifToken {
        auto token = state_->lex_aspif();
        if (((token != tokens) && ...)) {
            has_error_ = true;
            throw token_error{token};
        }
        return token;
    }

    ParserState *state_;
    ProgramBackend *backend_;
    TheoryBackend *theory_;
    bool has_error_ = false;
};

} // namespace

auto parse_aspif(ParserState &state, ProgramBackend &backend, TheoryBackend &theory) {
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
    AspifParser{state, backend, theory}.parse();
}

} // namespace Clingo::Input::Parse
