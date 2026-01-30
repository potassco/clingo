#include <clingo/core/backend.hh>
#include <clingo/util/string.hh>

#include "parser_state.hh"

namespace CppClingo::Input::Parse {

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
        case AspifToken::symbols: {
            return out << "<symbols>";
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
    //! on the "asp" token.
    auto parse() -> bool {
        bool res = true;
        try {
            preamble_();
        } catch ([[maybe_unused]] aspif_error const &e) {
            res = false;
            recover_();
        }
        for (bool begin_step = false;;) {
            try {
                auto type = expect_unsigned_();
                for (;;) {
                    if (!begin_step) {
                        state_->prg_backend()->begin_step();
                        begin_step = true;
                    }
                    if (type != 0) {
                        break;
                    }
                    state_->prg_backend()->end_ground();
                    state_->prg_backend()->end_step();
                    begin_step = false;
                    expect_(AspifToken::newline);
                    if (expect_(AspifToken::end, AspifToken::num_pos) == AspifToken::end) {
                        return res;
                    }
                    auto str = state_->view();
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    std::from_chars(str.data(), str.data() + str.size(), type);
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
    enum class OutputType : uint8_t {
        atom = 0,
        term = 1,
        term_cnd = 2,
        term_ext = 3,
        term_num = 4,
        term_str = 5,
        term_tup = 6,
        term_fun = 7,
    };

    template <class... T> auto expect_(T... tokens) -> AspifToken {
        auto token = state_->lex_aspif();
        if (((token != tokens) && ...)) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected token " << token;
            throw aspif_error{};
        }
        return token;
    }

    auto expect_str_() -> std::string_view {
        if (auto token = state_->lex_str(); token != AspifToken::str) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected token " << token;
            throw aspif_error{};
        }
        return state_->view();
    }

    auto expect_nstr_() -> std::string_view {
        auto m = expect_unsigned_();
        expect_(AspifToken::space);
        if (auto token = state_->lex_str(m); token != AspifToken::str) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected token " << token;
            throw aspif_error{};
        }
        return state_->view();
    }

    auto expect_signed_() -> int {
        expect_(AspifToken::num_pos, AspifToken::num_neg);
        auto str = state_->view();
        int res = 0;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::from_chars(str.data(), str.data() + str.size(), res);
        return res;
    }

    auto expect_unsigned_() -> unsigned {
        expect_(AspifToken::num_pos);
        auto str = state_->view();
        unsigned res = 0;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::from_chars(str.data(), str.data() + str.size(), res);
        return res;
    }

    auto expect_atom_() -> prg_lit_t {
        auto m = static_cast<prg_lit_t>(expect_unsigned_());
        if (m <= 0) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "invalid program atom";
            throw aspif_error{};
        }
        return m;
    }

    auto expect_atoms_() -> PrgLitVec {
        auto m = expect_unsigned_();
        auto atoms = PrgLitVec{};
        atoms.reserve(m);
        for (unsigned i = 0; i < m; ++i) {
            expect_(AspifToken::space);
            atoms.emplace_back(expect_atom_());
        }
        return atoms;
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

    auto expect_lit_() -> prg_lit_t {
        auto lit = expect_signed_();
        if (lit == 0) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "invalid program literal";
            throw aspif_error{};
        }
        return lit;
    }

    auto expect_lits_() -> PrgLitVec {
        auto m = expect_unsigned_();
        auto body = PrgLitVec{};
        body.reserve(m);
        for (unsigned i = 0; i < m; ++i) {
            expect_(AspifToken::space);
            body.emplace_back(expect_lit_());
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
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected token `" << token << "`";
            throw std::runtime_error("parsing failed");
        }
    }

    void preamble_() {
        expect_(AspifToken::space);
        auto loc = state_->loc();
        auto major = expect_unsigned_();
        expect_(AspifToken::space);
        auto minor = expect_unsigned_();
        expect_(AspifToken::space);
        auto revision = expect_unsigned_();
        if ((major != 1 && major != 2) || minor != 0) {
            CLINGO_REPORT_LOC(state_->log(), error, loc + state_->loc())
                << "unsupported aspif version `" << major << "." << minor << "." << revision << "`";
            throw aspif_error{};
        }
        bool incremental = false;
        while (expect_(AspifToken::newline, AspifToken::space) == AspifToken::space) {
            switch (expect_(AspifToken::incremental, AspifToken::symbols)) {
                case AspifToken::incremental: {
                    incremental = true;
                    break;
                }
                case AspifToken::symbols: {
                    if (major == 1) {
                        CLINGO_REPORT_LOC(state_->log(), error, loc + state_->loc()) << "unsupported tag `symbols`";
                        throw aspif_error{};
                    }
                    symbol_ = true;
                    break;
                }
                default: {
                    Util::unreachable();
                }
            }
        }
        version_ = major;
        state_->prg_backend()->preamble(major, minor, revision, incremental);
    }

    auto rule_type_() -> RuleType {
        auto rt = expect_unsigned_();
        if (rt > max_rule_type) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected rule type `" << rt << "`";
            throw aspif_error{};
        }
        return static_cast<RuleType>(rt);
    }

    auto body_type_() -> BodyType {
        auto bt = expect_unsigned_();
        if (bt > max_body_type) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected body type `" << bt << "`";
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
        auto sym = output_symbol_();
        expect_(AspifToken::space);
        auto body = expect_lits_();
        if (body.size() == 1 && body.front() > 0) {
            state_->prg_backend()->show_atom(*sym, body.front());
        } else if (auto fact = body.empty() ? state_->prg_backend()->fact_lit() : std::nullopt) {
            state_->prg_backend()->show_atom(*sym, *fact);
        } else {
            state_->prg_backend()->show_term(*sym, body);
        }
    }

    auto output_symbol_() -> SharedSymbol {
        state_symbol_.init(expect_nstr_(), *str_symbol_);
        state_symbol_.consume();
        auto sym = parse_symbol(state_symbol_);
        if (!sym || !state_symbol_.branch(Parse::TokenType::end)) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc())
                << "parsing symbol failed: `" << state_->view() << "`";
            throw aspif_error{};
        }
        return *sym;
    }

    auto output_type_(OutputType max_type) -> OutputType {
        auto ot = expect_unsigned_();
        if (ot > static_cast<unsigned>(max_type)) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected output type `" << ot << "`";
            throw aspif_error{};
        }
        return static_cast<OutputType>(ot);
    }
    void output_term_or_atom_() {
        auto ot = output_type_(OutputType::term_cnd);
        expect_(AspifToken::space);
        switch (ot) {
            case OutputType::atom: {
                auto atom = expect_atom_();
                expect_(AspifToken::space);
                auto sym = output_symbol_();
                state_->prg_backend()->show_atom(*sym, atom);
                break;
            }
            case OutputType::term: {
                auto term = expect_unsigned_();
                expect_(AspifToken::space);
                auto sym = output_symbol_();
                state_->prg_backend()->show_term(*sym, term);
                break;
            }
            case OutputType::term_cnd: {
                auto term = expect_unsigned_();
                expect_(AspifToken::space);
                auto body = expect_lits_();
                state_->prg_backend()->show_term(term, body);
                break;
            }
            default: {
                Util::unreachable();
            }
        }
    }
    void output_symbols_() {
        auto type = output_type_(OutputType::term_fun);
        expect_(AspifToken::space);
        auto sym_id = expect_unsigned_();
        auto add = [this, sym_id]<class T>(T &&sym) {
            if (sym_id < symbols_.size()) {
                symbols_[sym_id] = SharedSymbol{std::forward<T>(sym)};
            } else {
                while (symbols_.size() < sym_id) {
                    symbols_.emplace_back();
                }
                assert(symbols_.size() == sym_id);
                symbols_.emplace_back(std::forward<T>(sym));
            }
        };
        auto get = [this](unsigned id) {
            if (id < symbols_.size()) {
                return symbols_.at(id);
            }
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unknown symbol id `" << id << "`";
            throw aspif_error{};
        };
        switch (type) {
            case OutputType::atom: {
                auto sym = get(sym_id);
                expect_(AspifToken::space);
                auto atom = expect_atom_();
                state_->prg_backend()->show_atom(*sym, atom);
                break;
            }
            case OutputType::term: {
                auto sym = get(sym_id);
                expect_(AspifToken::space);
                auto id = expect_unsigned_();
                state_->prg_backend()->show_term(*sym, id);
                break;
            }
            case OutputType::term_cnd: {
                expect_(AspifToken::space);
                auto body = expect_lits_();
                state_->prg_backend()->show_term(sym_id, body);
                break;
            }
            case OutputType::term_ext: {
                expect_(AspifToken::space);
                auto type = expect_unsigned_();
                if (type > 1) {
                    CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "expected inf or sup `" << type << "`";
                    throw aspif_error{};
                }
                add(type == 0 ? SymbolStore::inf() : SymbolStore::sup());
                break;
            }
            case OutputType::term_str: {
                expect_(AspifToken::space);
                auto str = expect_nstr_();
                add(SymbolStore::str(state_->store().string(str)));
                break;
            }
            case OutputType::term_num: {
                expect_(AspifToken::space);
                expect_(AspifToken::num_pos, AspifToken::num_neg);
                add(state_->store().num(Number(state_->view())));
                break;
            }
            case OutputType::term_tup: {
                expect_(AspifToken::space);
                auto ids = expect_ids_();
                buf_.clear();
                for (auto id : ids) {
                    buf_.emplace_back(*get(id));
                }
                add(state_->store().tup(buf_));
                break;
            }
            case OutputType::term_fun: {
                expect_(AspifToken::space);
                auto sign = expect_unsigned_();
                if (sign > 1) {
                    CLINGO_REPORT_LOC(state_->log(), error, state_->loc())
                        << "expected classical sign `" << sign << "`";
                    throw aspif_error{};
                }
                expect_(AspifToken::space);
                auto name = get(expect_unsigned_());
                if (name->type() != SymbolType::string) {
                    CLINGO_REPORT_LOC(state_->log(), error, state_->loc())
                        << "expected string instead of `" << *name << "`";
                    throw aspif_error{};
                }
                expect_(AspifToken::space);
                auto ids = expect_ids_();
                buf_.clear();
                for (auto id : ids) {
                    buf_.emplace_back(*get(id));
                }
                add(state_->store().fun(name->str(), buf_, sign == 1));
                break;
            }
        }
    }

    auto external_type_() -> CppClingo::ExternalType {
        auto v = expect_unsigned_();
        if (v > max_external_type) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected external type `" << v << "`";
            throw aspif_error{};
        }
        return static_cast<ExternalType>(v);
    }

    void external_() {
        auto a = expect_atom_();
        expect_(AspifToken::space);
        state_->prg_backend()->external(a, external_type_());
    }

    void assume_() { state_->prg_backend()->assume(expect_lits_()); }

    auto heuristic_type_() -> HeuristicType {
        auto type = expect_unsigned_();
        if (type > max_heuristic_type) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected heuristic type `" << type << "`";
            throw aspif_error{};
        }
        return static_cast<HeuristicType>(type);
    }

    void heuristic_() {
        auto type = heuristic_type_();
        expect_(AspifToken::space);
        auto atom = expect_atom_();
        expect_(AspifToken::space);
        auto weight = expect_signed_();
        expect_(AspifToken::space);
        auto priority = expect_signed_();
        expect_(AspifToken::space);
        state_->prg_backend()->heuristic(atom, weight, priority, type, expect_lits_());
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
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected theory type `" << type << "`";
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
        if (type >= 0 || type < -max_theory_compound_type) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected compound type `" << type << "`";
            throw aspif_error{};
        }
        return static_cast<TheoryTermTupleType>(-type - 1);
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

    auto statement_type_(unsigned type) -> StatementType {
        if (type == 0 || type > max_statement_type) {
            CLINGO_REPORT_LOC(state_->log(), error, state_->loc()) << "unexpected statment type `" << type << "`";
            throw aspif_error{};
        }
        return static_cast<StatementType>(type);
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
                if (symbol_) {
                    output_symbols_();
                } else if (version_ == 1) {
                    output_();
                } else {
                    output_term_or_atom_();
                }
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
    SharedSymbolVec symbols_;
    SymbolVec buf_;
    uint32_t version_ = 2;
    bool symbol_ = false;
};

} // namespace

auto parse_aspif(ParserState &state) -> bool {
    return AspifParser{state}.parse();
}

} // namespace CppClingo::Input::Parse
