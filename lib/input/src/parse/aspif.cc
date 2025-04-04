#include <clingo/core/backend.hh>
#include <clingo/util/string.hh>

#include "parser_state.hh"

namespace Clingo::Input::Parse {

namespace {

auto operator<<(std::ostream &out, AspifToken token) -> std::ostream & {
    switch (token) {
        case AspifToken::str: {
            return out << "<string>";
        }
        case AspifToken::end: {
            return out << "<end>";
        }
        case AspifToken::error: {
            return out << "<error>";
        }
        case AspifToken::incremental: {
            return out << "<incremental>";
        }
        case AspifToken::newline: {
            return out << "<newline>";
        }
        case AspifToken::space: {
            return out << "<space>";
        }
        case AspifToken::num_neg: {
            return out << "<non-positive number>";
        }
        case AspifToken::num_pos: {
            return out << "<non-negative number>";
        }
    }
    return out;
}

class aspif_error : public std::exception {
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
        } catch ([[maybe_unused]] aspif_error const &e) {
            recover_();
        }
        while (true) {
            try {
                auto type = expect_unsigned_();
                if (type == 0) {
                    expect_(AspifToken::newline);
                    expect_(AspifToken::end);
                } else {
                    statement_(type);
                }
            } catch ([[maybe_unused]] aspif_error const &e) {
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

    auto expect_atoms_() -> PrgLitVec {
        auto m = expect_unsigned_();
        auto body = PrgLitVec{};
        body.reserve(m);
        for (unsigned i = 0; i < m; ++i) {
            body.emplace_back(expect_unsigned_());
        }
        return body;
    }

    auto expect_lits_() -> PrgLitVec {
        auto m = expect_unsigned_();
        auto body = PrgLitVec{};
        body.reserve(m);
        for (unsigned i = 0; i < m; ++i) {
            body.emplace_back(expect_signed_());
        }
        return body;
    }

    auto expect_wlits_() -> WeightedPrgLitVec {
        auto m = expect_unsigned_();
        auto body = WeightedPrgLitVec{};
        body.reserve(m);
        for (unsigned i = 0; i < m; ++i) {
            auto lit = expect_signed_();
            body.emplace_back(lit, expect_signed_());
        }
        return body;
    }

    void rule_() {
        expect_(AspifToken::space);
        auto rule_type = static_cast<RuleType>(expect_unsigned_());
        if (rule_type != RuleType::choice && rule_type != RuleType::disjunctive) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc())
                << "unexpected rule type `" << static_cast<unsigned>(rule_type) << "`";
            throw aspif_error{};
        }
        expect_(AspifToken::space);
        auto head = expect_atoms_();
        auto body_type = expect_unsigned_();
        switch (static_cast<BodyType>(body_type)) {
            case BodyType::normal: {
                backend_->rule(head, expect_lits_(), rule_type == RuleType::choice);
                break;
            }
            case BodyType::weight: {
                auto l = expect_signed_();
                backend_->bd_aggr(head, expect_wlits_(), l, rule_type == RuleType::choice);
                break;
            }
            default: {
                GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected body type `" << body_type << "`";
                throw aspif_error{};
            }
        }
    }

    void statement_(unsigned type) {
        switch (static_cast<StatementType>(type)) {
            case StatementType::rule: {
                rule_();
                break;
            }
            default: {
                GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected statement type `" << type << "`";
                throw aspif_error{};
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
        // TODO: check which error to throw or value to return to best indicate failure
        auto gobble = state_->lex_str();
        if (gobble == AspifToken::str) {
            // NOTE: only newlines can follow
            state_->lex_aspif();
        } else {
            if (gobble == AspifToken::end) {
                GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected end of file";
            }
            // NOTE: only a null byte is possible
            throw std::runtime_error("parsing failed");
        }
    }

    auto expect_signed_() -> int {
        expect_(AspifToken::num_pos, AspifToken::num_neg);
        auto str = state_->view();
        int res = 0;
        std::from_chars(str.begin(), str.end(), res);
        return res;
    }

    auto expect_unsigned_() -> unsigned {
        expect_(AspifToken::num_pos);
        auto str = state_->view();
        unsigned res = 0;
        std::from_chars(str.begin(), str.end(), res);
        return res;
    }

    template <class... T> auto expect_(T... tokens) -> AspifToken {
        auto token = state_->lex_aspif();
        if (((token != tokens) && ...)) {
            has_error_ = true;
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected token " << token;
            throw aspif_error{};
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
