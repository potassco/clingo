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
                    backend_->end();
                    return;
                }
                statement_(statement_type_(type));
            } catch ([[maybe_unused]] aspif_error const &e) {
                recover_();
            }
        }
    }

  private:
    enum class StatementType : uint8_t {
        rule = 1,
        minimize = 2,
        project = 3,
        output = 4,
        external = 5,
        assume = 6,
    };
    static constexpr auto max_statement_type = 10;
    enum class RuleType : uint8_t {
        disjunctive = 0,
        choice = 1,
    };
    static constexpr auto max_rule_type = 1;
    enum class BodyType : uint8_t {
        normal = 0,
        weight = 1,
    };
    static constexpr auto max_body_type = 1;
    static constexpr auto max_external_type = 4;

    template <class... T> auto expect_(T... tokens) -> AspifToken {
        auto token = state_->lex_aspif();
        if (((token != tokens) && ...)) {
            has_error_ = true;
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected token " << token;
            throw aspif_error{};
        }
        return token;
    }

    auto expect_str_() -> std::string_view {
        if (auto token = state_->lex_str(); token != AspifToken::str) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected token " << token;
            throw aspif_error{};
        }
        return state_->view();
    }

    auto expect_nstr_() -> std::string_view {
        auto m = expect_unsigned_();
        expect_(AspifToken::space);
        if (auto token = state_->lex_str(m); token != AspifToken::str) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected token " << token;
            throw aspif_error{};
        }
        return state_->view();
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

    auto expect_atoms_() -> PrgLitVec {
        auto m = expect_unsigned_();
        auto body = PrgLitVec{};
        body.reserve(m);
        for (unsigned i = 0; i < m; ++i) {
            expect_(AspifToken::space);
            body.emplace_back(expect_unsigned_());
        }
        return body;
    }

    auto expect_lits_() -> PrgLitVec {
        auto m = expect_unsigned_();
        auto body = PrgLitVec{};
        body.reserve(m);
        for (unsigned i = 0; i < m; ++i) {
            expect_(AspifToken::space);
            body.emplace_back(expect_signed_());
        }
        return body;
    }

    auto expect_wlits_() -> WeightedPrgLitVec {
        auto m = expect_unsigned_();
        auto body = WeightedPrgLitVec{};
        body.reserve(m);
        for (unsigned i = 0; i < m; ++i) {
            expect_(AspifToken::space);
            auto lit = expect_signed_();
            expect_(AspifToken::space);
            body.emplace_back(lit, expect_signed_());
        }
        return body;
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

    void preamble_() {
        auto major = expect_unsigned_();
        expect_(AspifToken::space);
        auto minor = expect_unsigned_();
        expect_(AspifToken::space);
        auto revision = expect_unsigned_();
        bool incremental = false;
        if (expect_(AspifToken::newline, AspifToken::space) == AspifToken::space) {
            incremental = true;
            expect_(AspifToken::incremental);
            expect_(AspifToken::newline);
        }
        backend_->preamble(major, minor, revision, incremental);
    }

    auto rule_type_() -> RuleType {
        auto rt = expect_unsigned_();
        if (rt > max_rule_type) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected rule type `" << rt << "`";
            throw aspif_error{};
        }
        return static_cast<RuleType>(rt);
    }

    auto body_type_() -> BodyType {
        auto bt = expect_unsigned_();
        if (bt > max_body_type) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected body type `" << bt << "`";
            throw aspif_error{};
        }
        return static_cast<BodyType>(bt);
    }

    void rule_() {
        expect_(AspifToken::space);
        auto rule_type = rule_type_();
        expect_(AspifToken::space);
        auto head = expect_atoms_();
        expect_(AspifToken::space);
        auto body_type = body_type_();
        expect_(AspifToken::space);
        switch (body_type) {
            case BodyType::normal: {
                backend_->rule(head, expect_lits_(), rule_type == RuleType::choice);
                break;
            }
            case BodyType::weight: {
                auto l = expect_signed_();
                expect_(AspifToken::space);
                backend_->bd_aggr(head, expect_wlits_(), l, rule_type == RuleType::choice);
                break;
            }
        }
        expect_(AspifToken::newline);
    }

    void minimize_() {
        expect_(AspifToken::space);
        auto p = expect_signed_();
        expect_(AspifToken::space);
        backend_->minimize(p, expect_wlits_());
        expect_(AspifToken::newline);
    }

    void project_() {
        expect_(AspifToken::space);
        backend_->project(expect_atoms_());
        expect_(AspifToken::newline);
    }

    void output_() {
        expect_(AspifToken::space);
        state_term_.init(expect_nstr_(), *str_symbol_);
        auto sym = parse_symbol(state_term_);
        if (!sym) {
            throw aspif_error{};
        }
        expect_(AspifToken::space);
        auto body = expect_lits_();
        if (body.size() == 1 && body.front() > 0) {
            backend_->show_atom(*sym.value(), body.front());
        } else {
            backend_->show(*sym.value(), body);
        }
        expect_(AspifToken::newline);
    }

    auto external_type_() -> Clingo::ExternalType {
        auto v = expect_unsigned_();
        if (v > max_external_type) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected external type `" << v << "`";
            throw aspif_error{};
        }
        return static_cast<ExternalType>(v);
    }

    void external_() {
        expect_(AspifToken::space);
        auto a = expect_unsigned_();
        expect_(AspifToken::space);
        backend_->external(static_cast<prg_lit_t>(a), external_type_());
        expect_(AspifToken::newline);
    }

    void assume_() {
        expect_(AspifToken::space);
        backend_->assume(expect_lits_());
        expect_(AspifToken::newline);
    }

    auto statement_type_(unsigned st) -> StatementType {
        if (st > max_statement_type) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected statment type `" << st << "`";
            throw aspif_error{};
        }
        return static_cast<StatementType>(st);
    }

    void statement_(StatementType type) {
        switch (type) {
            case StatementType::rule: {
                rule_();
                break;
            }
            case StatementType::minimize: {
                minimize_();
                break;
            }
            case StatementType::project: {
                project_();
                break;
            }
            case StatementType::output: {
                output_();
                break;
            }
            case StatementType::external: {
                external_();
                break;
            }
            case StatementType::assume: {
                assume_();
                break;
            }
        }
    }

    ParserState *state_;
    ParserState state_term_{state_->log(), state_->store()};
    SharedString str_symbol_{*state_->store().string("symbol")};
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
