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
    AspifParser(ParserState &state) : state_{&state} {}

    //! Parses a program in aspif format assuming that the lexer currently sits
    //! on the the "asp" token.
    bool parse() {
        bool res = true;
        try {
            preamble_();
        } catch ([[maybe_unused]] aspif_error const &e) {
            res = false;
            recover_();
        }
        while (true) {
            try {
                auto type = expect_unsigned_();
                if (type == 0) {
                    expect_(AspifToken::newline);
                    expect_(AspifToken::end);
                    state_->prg_backend()->end();
                    return res;
                }
                statement_(statement_type_(type));
            } catch ([[maybe_unused]] aspif_error const &e) {
                res = false;
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
        heuristic = 7,
        edge = 8,
        theory = 9,
        comment = 10,
    };
    enum class RuleType : uint8_t {
        disjunctive = 0,
        choice = 1,
    };
    static constexpr unsigned max_rule_type = 1;
    enum class BodyType : uint8_t {
        normal = 0,
        weight = 1,
    };
    static constexpr unsigned max_body_type = 1;
    static constexpr unsigned max_heuristic_type = 5;
    static constexpr unsigned max_external_type = 4;
    enum class TheoryType : uint8_t {
        number = 0,
        symbol = 1,
        compound = 2,
        element = 4,
        atom = 5,
        atom_with_guard = 6,
    };
    static constexpr unsigned theory_type_reserved = 3;
    static constexpr unsigned max_theory_type = 6;
    static constexpr int max_theory_compound_type = 3;
    static constexpr unsigned max_statement_type = 10;

    template <class... T> auto expect_(T... tokens) -> AspifToken {
        auto token = state_->lex_aspif();
        if (((token != tokens) && ...)) {
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

    auto expect_ids_() -> PrgIdVec {
        auto m = expect_unsigned_();
        auto ids = PrgIdVec{};
        ids.reserve(m);
        for (unsigned i = 0; i < m; ++i) {
            expect_(AspifToken::space);
            ids.emplace_back(expect_unsigned_());
        }
        return ids;
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
        auto token = state_->lex_str();
        if (token == AspifToken::str) {
            state_->lex_aspif(); // gobble newline
        } else {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected token `" << token << "`";
            throw std::runtime_error("parsing failed");
        }
    }

    void preamble_() {
        expect_(AspifToken::space);
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
        state_->prg_backend()->preamble(major, minor, revision, incremental);
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
        auto rule_type = rule_type_();
        expect_(AspifToken::space);
        auto head = expect_atoms_();
        expect_(AspifToken::space);
        auto body_type = body_type_();
        expect_(AspifToken::space);
        switch (body_type) {
            case BodyType::normal: {
                state_->prg_backend()->rule(head, expect_lits_(), rule_type == RuleType::choice);
                break;
            }
            case BodyType::weight: {
                auto l = expect_signed_();
                expect_(AspifToken::space);
                state_->prg_backend()->bd_aggr(head, expect_wlits_(), l, rule_type == RuleType::choice);
                break;
            }
        }
    }

    void minimize_() {
        auto p = expect_signed_();
        expect_(AspifToken::space);
        state_->prg_backend()->minimize(p, expect_wlits_());
    }

    void project_() { state_->prg_backend()->project(expect_atoms_()); }

    void output_() {
        state_symbol_.init(expect_nstr_(), *str_symbol_);
        state_symbol_.consume();
        auto sym = parse_symbol(state_symbol_);
        if (!sym || !state_symbol_.branch(Parse::TokenType::end)) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc())
                << "parsing symbol failed: `" << state_->view() << "`";
            throw aspif_error{};
        }
        expect_(AspifToken::space);
        auto body = expect_lits_();
        if (body.size() == 1 && body.front() > 0) {
            state_->prg_backend()->show_atom(*sym.value(), body.front());
        } else {
            state_->prg_backend()->show(*sym.value(), body);
        }
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
        auto a = expect_unsigned_();
        expect_(AspifToken::space);
        state_->prg_backend()->external(static_cast<prg_lit_t>(a), external_type_());
    }

    void assume_() { state_->prg_backend()->assume(expect_lits_()); }

    auto heuristic_type_() -> HeuristicType {
        auto type = expect_unsigned_();
        if (type > max_heuristic_type) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected heuristic type `" << type << "`";
            throw aspif_error{};
        }
        return static_cast<HeuristicType>(type);
    }

    void heuristic_() {
        auto type = heuristic_type_();
        expect_(AspifToken::space);
        auto atom = expect_unsigned_();
        expect_(AspifToken::space);
        auto weight = expect_signed_();
        expect_(AspifToken::space);
        auto priority = expect_signed_();
        expect_(AspifToken::space);
        state_->prg_backend()->heuristic(static_cast<prg_lit_t>(atom), weight, priority, type, expect_lits_());
    }

    void edge_() {
        auto u = expect_unsigned_();
        expect_(AspifToken::space);
        auto v = expect_unsigned_();
        expect_(AspifToken::space);
        state_->prg_backend()->edge(u, v, expect_lits_());
    }

    auto theory_type_() -> TheoryType {
        auto type = expect_unsigned_();
        if (type == theory_type_reserved || type > max_theory_type) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected theory type `" << type << "`";
            throw aspif_error{};
        }
        return static_cast<TheoryType>(type);
    }

    void theory_number_() {
        auto id = expect_unsigned_();
        expect_(AspifToken::space);
        auto num = expect_signed_();
        state_->thy_backend()->num(id, num);
    }
    void theory_symbol_() {
        auto id = expect_unsigned_();
        expect_(AspifToken::space);
        auto num = expect_nstr_();
        state_->thy_backend()->str(id, num);
    }

    auto theory_compound_type_(int type) -> TheoryTermTupleType {
        if (type >= 0 || type <= -max_theory_compound_type) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected compound type `" << type << "`";
        }
        return static_cast<TheoryTermTupleType>(-type);
    }

    void theory_compound_() {
        auto id = expect_unsigned_();
        expect_(AspifToken::space);
        auto type = expect_signed_();
        expect_(AspifToken::space);
        auto terms = expect_ids_();
        if (type >= 0) {
            state_->thy_backend()->fun(id, type, terms);
        } else {
            state_->thy_backend()->tup(id, theory_compound_type_(type), terms);
        }
    }
    void theory_element_() {
        auto id = expect_unsigned_();
        expect_(AspifToken::space);
        auto tuple = expect_ids_();
        expect_(AspifToken::space);
        auto cond = expect_lits_();
        state_->thy_backend()->elem(id, tuple, cond);
    }

    void theory_atom_(bool parse_guard) {
        auto atom = expect_signed_();
        expect_(AspifToken::space);
        auto name = expect_unsigned_();
        expect_(AspifToken::space);
        auto elems = expect_ids_();
        auto guard = std::optional<std::pair<prg_id_t, prg_id_t>>{};
        if (parse_guard) {
            expect_(AspifToken::space);
            auto op = expect_unsigned_();
            expect_(AspifToken::space);
            auto term = expect_unsigned_();
            guard.emplace(op, term);
        }
        state_->thy_backend()->atom(atom, name, elems, guard);
    }

    void theory_() {
        auto type = theory_type_();
        expect_(AspifToken::space);
        switch (type) {
            case TheoryType::number: {
                theory_number_();
                break;
            }
            case TheoryType::symbol: {
                theory_symbol_();
                break;
            }
            case TheoryType::compound: {
                theory_compound_();
                break;
            }
            case TheoryType::element: {
                theory_element_();
                break;
            }
            case TheoryType::atom: {
                theory_atom_(false);
                break;
            }
            case TheoryType::atom_with_guard: {
                theory_atom_(true);
                break;
            }
        }
    }

    void comment_() { expect_str_(); }

    auto statement_type_(unsigned st) -> StatementType {
        if (st > max_statement_type) {
            GRINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected statment type `" << st << "`";
            throw aspif_error{};
        }
        return static_cast<StatementType>(st);
    }

    void statement_(StatementType type) {
        expect_(AspifToken::space);
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
            case StatementType::heuristic: {
                heuristic_();
                break;
            }
            case StatementType::edge: {
                edge_();
                break;
            }
            case StatementType::theory: {
                theory_();
                break;
            }
            case StatementType::comment: {
                comment_();
                break;
            }
        }
        expect_(AspifToken::newline);
    }

    ParserState *state_;
    ParserState state_symbol_{state_->log(), state_->store()};
    SharedString str_symbol_{*state_->store().string("symbol")};
};

} // namespace

auto parse_aspif(ParserState &state) -> bool {
    return AspifParser{state}.parse();
}

} // namespace Clingo::Input::Parse
