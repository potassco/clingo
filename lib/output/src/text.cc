#include <clingo/output/text.hh>

#include <clingo/util/ordered_set.hh>
#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>
#include <clingo/util/unordered_map.hh>

namespace CppClingo::Output {

namespace {

class OutputBody : public OutputLit {
  public:
    OutputBody(size_t &uids) : uids_{&uids} {}

    void start() {
        buf_.reset();
        if (delayed_.empty() || !delayed_.back().empty()) {
            delayed_.emplace_back();
        }
        has_body_ = false;
    }

    [[nodiscard]] auto end() -> std::string_view {
        assert(delayed_.back().empty());
        return buf_.view();
    }

    [[nodiscard]] auto delayed() -> bool { return !delayed_.back().empty(); }

    void delay() {
        if (!buf_.empty()) {
            assert(!delayed_.empty());
            delayed_.back().emplace_back(buf_.str());
            buf_.reset();
        }
    }

    [[nodiscard]] auto delay_head(std::optional<size_t> uid, char const *sep) -> size_t {
        bool fact = buf_.empty() && delayed_.back().empty();
        buf_ << ".\n";
        delayed_.back().emplace_back(buf_.str());
        buf_.reset();
        if (!uid) {
            uid = ++*uids_;
        }
        // Note: leaves room for optimization...
        if (!fact) {
            delayed_.back().emplace(delayed_.back().begin(), sep);
        }
        delayed_.back().emplace(delayed_.back().begin(), *uid);
        return *uid;
    }

    [[nodiscard]] auto buf() -> Util::OutputBuffer & { return buf_; }

    void prepend() { delayed_.back().insert(delayed_.back().begin(), buf_.str()); }

    [[nodiscard]] auto empty() const -> bool { return !has_body_; }

    void define(size_t index, std::string str) { defined_.emplace(index, std::move(str)); }

    void flush(Util::OutputBuffer &out) {
        for (auto &delayed : delayed_) {
            for (auto &elem : delayed) {
                std::visit(
                    [&out, this]<class T>(T const &elem) {
                        if constexpr (Util::is_among_v<T, std::string>) {
                            out << elem;
                        } else if constexpr (Util::is_among_v<T, size_t>) {
                            auto it = defined_.find(elem);
                            if (it != defined_.end()) {
                                out << it.value();
                            } else {
                                out << "#false";
                            }
                        }
                    },
                    elem);
                out.endl();
            }
        }
        delayed_.clear();
    }

    void end_step() {
        delayed_.clear();
        defined_.clear();
    }

  private:
    void sep() {
        if (has_body_) {
            buf_ << "; ";
        } else {
            has_body_ = true;
        }
    }

    void do_lit(Sign sign, Symbol sym, [[maybe_unused]] size_t uid) override {
        sep();
        buf_ << sign << sym;
    }

    void do_boolean(bool value) override {
        sep();
        buf_ << (value ? "#true" : "#false");
    }

    auto do_cond_lit(std::optional<size_t> uid) -> size_t override { return do_bd_aggr(Sign::none, uid); }

    auto do_bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = ++*uids_;
        }
        sep();
        buf_ << sign;
        delay();
        delayed_.back().emplace_back(*uid);
        return *uid;
    }

    auto do_bd_theory(Sign sign, std::optional<size_t> uid) -> size_t override { return do_bd_aggr(sign, uid); }

    bool has_body_ = false;
    size_t *uids_;
    Util::OutputBuffer buf_;
    Util::unordered_map<size_t, std::string> defined_;
    std::vector<std::vector<std::variant<std::string, size_t>>> delayed_;
};

class OutputCond : public OutputLit {
  public:
    void start() {
        buf_.reset();
        has_lits_ = false;
    }

    [[nodiscard]] auto end() -> std::string_view { return buf_.view(); }

  private:
    void sep() {
        if (has_lits_) {
            buf_ << ", ";
        } else {
            has_lits_ = true;
        }
    }

    void do_lit(Sign sign, Symbol sym, [[maybe_unused]] size_t uid) override {
        sep();
        buf_ << sign << sym;
    }

    void do_boolean(bool value) override {
        sep();
        buf_ << (value ? "#true" : "#false");
    }

    auto do_cond_lit([[maybe_unused]] std::optional<size_t> uid) -> size_t override {
        throw std::runtime_error("unsupported literal");
    }

    auto do_bd_aggr([[maybe_unused]] Sign sign, [[maybe_unused]] std::optional<size_t> uid) -> size_t override {
        throw std::runtime_error("unsupported literal");
    }

    auto do_bd_theory([[maybe_unused]] Sign sign, [[maybe_unused]] std::optional<size_t> uid) -> size_t override {
        throw std::runtime_error("unsupported literal");
    }

    bool has_lits_ = false;
    Util::OutputBuffer buf_;
};

class OutputText : public OutputStm, OutputTheory {
  public:
    OutputText(Util::OutputBuffer &out) : out_{&out} {};

  private:
    template <class T> static void simple_head_(T &out, std::optional<std::tuple<Symbol, size_t, bool>> const &head) {
        if (head) {
            if (get<2>(*head)) {
                out << "{ ";
            }
            out << get<0>(*head);
            if (get<2>(*head)) {
                out << " }";
            }
        }
    }

    void do_project_atom([[maybe_unused]] size_t p_atom, [[maybe_unused]] size_t atom) override {
        // nothing
    }

    void do_fact(Symbol sym, [[maybe_unused]] size_t uid) override {
        *out_ << sym << ".\n";
        out_->endl();
    }

    auto do_body() -> OutputLit & override {
        body_.start();
        return body_;
    }

    void do_rule(std::optional<std::tuple<Symbol, size_t, bool>> head) override {
        if (!body_.delayed()) {
            simple_head_(*out_, head);
            if (!body_.empty() || !head) {
                *out_ << " :- ";
            }
            *out_ << body_.end() << ".\n";
            out_->endl();
        } else {
            body_.buf() << ".\n";
            body_.delay();
            simple_head_(body_.buf(), head);
            body_.buf() << " :- ";
            body_.prepend();
        }
    }

    void do_show_atom(Symbol atom, [[maybe_unused]] size_t uid) override {
        auto sign = atom.has_classical_sign();
        auto name = atom.name();
        auto arity = atom.args().size();
        if (seen_.emplace(sign, name, arity).second) {
            *out_ << "#show " << (sign ? "-" : "") << name << "/" << arity << ".\n";
        }
    }
    void do_show_term(Symbol term) override {
        if (!body_.delayed()) {
            *out_ << "#show " << term;
            if (!body_.empty()) {
                *out_ << ": " << body_.end();
            }
            *out_ << ".\n";
            out_->endl();
        } else {
            body_.buf() << ".\n";
            body_.delay();
            body_.buf() << "#show " << term << ": ";
            body_.prepend();
        }
    }

    void do_external(Symbol atom, [[maybe_unused]] size_t uid, ExternalType type) override {
        *out_ << "#external " << atom << ". [" << type << "]\n";
        out_->endl();
    }

    void do_project(Symbol atom, [[maybe_unused]] size_t uid) override {
        *out_ << "#project " << atom << ".\n";
        out_->endl();
    }

    auto do_aggr_rule(std::optional<size_t> uid) -> size_t override { return body_.delay_head(uid, " :- "); }

    auto do_theory_rule(std::optional<size_t> uid) -> size_t override { return body_.delay_head(uid, " :- "); }

    auto do_disjunctive_rule(std::optional<size_t> uid) -> size_t override { return body_.delay_head(uid, " :- "); }

    void do_weak_constraint(Number const &weight, Number const *prio, SymbolSpan terms) override {
        auto p_tup = [&](auto &out) {
            out << ". [" << weight;
            if (prio != nullptr) {
                out << "@" << *prio;
            }
            for (auto const &term : terms) {
                out << "," << term;
            }
            out << "].\n";
        };
        if (!body_.delayed()) {
            *out_ << " :~ ";
            *out_ << body_.end();
            p_tup(*out_);
            out_->endl();
        } else {
            p_tup(body_.buf());
            body_.delay();
            body_.buf() << " :~ ";
            body_.prepend();
        }
    }

    void do_heuristic(Symbol atom, [[maybe_unused]] size_t uid, Number const &weight, Number const *prio,
                      HeuristicType type) override {
        auto p_tup = [&](auto &out) {
            out << ". [" << weight;
            if (prio) {
                out << "@" << *prio;
            }
            out << "," << type;
            out << "]\n";
        };
        if (!body_.delayed()) {
            *out_ << "#heuristic " << atom;
            if (!body_.empty()) {
                *out_ << ": ";
            }
            *out_ << body_.end();
            p_tup(*out_);
            out_->endl();
        } else {
            p_tup(body_.buf());
            body_.delay();
            body_.buf() << "#heuristic " << atom << ": ";
            body_.prepend();
        }
    }

    void do_edge(Symbol src, Symbol dst) override {
        if (!body_.delayed()) {
            *out_ << "#edge (" << src << "," << dst << ")";
            if (!body_.empty()) {
                *out_ << ": ";
            }
            *out_ << body_.end() << ".\n";
            out_->endl();
        } else {
            body_.buf() << ".\n";
            body_.delay();
            *out_ << "#edge (" << src << "," << dst << "): ";
            body_.prepend();
        }
    }

    auto do_cond() -> OutputLit & override {
        cond_.start();
        return cond_;
    }

    template <class T> auto str_id_(T &&str) -> size_t {
        auto it = strs_.emplace(std::forward<T>(str)).first;
        return static_cast<size_t>(it - strs_.begin());
    }

    auto do_cond_id() -> size_t override { return str_id_(cond_.end()); }

    auto do_uid([[maybe_unused]] bool fact) -> size_t override { return ++uids_; }

    void do_cond_lit(size_t uid, CondLitSpan elems) override {
        if (elems.empty()) {
            body_.define(uid, "#true");
        } else {
            tmp_.reset();
            tmp_ << Util::p_range(elems, "; ", [this](auto &buf, auto const &elem) {
                if (elem.first) {
                    buf << *strs_.nth(*elem.first);
                } else {
                    buf << "#false";
                }
                buf << ": " << *strs_.nth(elem.second);
            });
            body_.define(uid, tmp_.str());
        }
    }

    void aggr(size_t uid, AggregateFunction fun, auto elems, GuardSpan guards, auto prt) {
        tmp_.reset();
        auto it = guards.begin();
        if (guards.size() > 1) {
            tmp_ << it->second << " " << flip(it->first) << " ";
            ++it;
        }
        tmp_ << fun << " { " << Util::p_range(elems, "; ", [prt](auto &buf, auto const &elem) { prt(buf, elem); })
             << (elems.empty() ? "}" : " }");
        for (auto ie = guards.end(); it != ie; ++it) {
            tmp_ << " " << it->first << " " << it->second;
        }
        body_.define(uid, tmp_.str());
    }

    void do_bd_aggr(size_t uid, AggregateFunction fun, BdElemSpan elems, GuardSpan guards) override {
        aggr(uid, fun, elems, guards, [this](auto &buf, auto const &elem) {
            if (elem.second.empty()) {
                buf << Util::p_range(elem.first);
                if (elem.first.empty()) {
                    buf << ": ";
                }
            } else {
                buf << Util::p_range(elem.second, "; ", [this, &elem](auto &buf, auto const &cond) {
                    buf << Util::p_range(elem.first) << ": " << *strs_.nth(cond);
                });
            }
        });
    }

    void do_hd_aggr(size_t uid, AggregateFunction fun, HdElemSpan elems, GuardSpan guards) override {
        aggr(uid, fun, elems, guards, [this](auto &buf, auto const &elem) {
            buf << Util::p_range(elem.second, "; ", [this, &elem](auto &buf, auto const &hc) {
                buf << Util::p_range(elem.first) << ": ";
                if (get<0>(hc).type() == SymbolType::sup) {
                    buf << "#true";
                } else {
                    buf << get<0>(hc);
                }
                if (auto const &str = *strs_.nth(get<2>(hc)); !str.empty()) {
                    buf << ": " << str;
                }
            });
        });
    }

    void do_disjunction(size_t uid, DisjElemSpan elems) override {
        tmp_.reset();
        if (elems.empty()) {
            tmp_ << "#false";
        } else {
            tmp_ << Util::p_range(elems, "; ", [this](auto &buf, DisjElem const &elem) {
                if (get<2>(elem).empty()) {
                    if (get<0>(elem).type() == SymbolType::sup) {
                        buf << "#true";
                    } else {
                        buf << get<0>(elem);
                    }
                } else {
                    buf << Util::p_range(get<2>(elem), "; ", [this, &elem](auto &buf, auto const &cond) {
                        buf << get<0>(elem) << ": " << *strs_.nth(cond);
                    });
                }
            });
        }
        body_.define(uid, tmp_.str());
    }

    auto do_theory() -> OutputTheory & override { return *this; }

    void do_flush() override { body_.flush(*out_); }

    void do_classical_negation([[maybe_unused]] size_t atom_a, [[maybe_unused]] size_t atom_b) override {
        // nothing to do here
    }

    void do_end_ground() override {
        if (std::exchange(explicit_show_, false)) {
            *out_ << "#show.\n";
        }
        out_->flush();
    }

    void do_mark([[maybe_unused]] SymbolCollector &gc) override {}

    auto do_str(String val) -> size_t override { return str_id_(val.view()); }

    auto do_num(Number const &val) -> size_t override {
        if (val < 0) {
            tmp_.reset() << "(" << val << ")";
        } else {
            tmp_.reset() << val;
        }
        return str_id_(tmp_.view());
    }

    auto do_fun(String name, std::span<size_t const> args) -> size_t override {
        auto is_theory_op = [](std::string_view str) {
            if (str.empty()) {
                return false;
            }
            switch (str.front()) {
                case '/':
                case '!':
                case '<':
                case '=':
                case '>':
                case '+':
                case '-':
                case '*':
                case '\\':
                case '?':
                case '&':
                case '@':
                case '|':
                case ':':
                case ';':
                case '~':
                case '^':
                case '.': {
                    return true;
                }
                default: {
                    return false;
                }
            }
        };
        if (args.size() == 1 && is_theory_op(name.view())) {
            tmp_.reset() << "(" << name << *strs_.nth(args.back()) << ")";
        } else if (args.size() == 2 && is_theory_op(name.view())) {
            tmp_.reset() << "(" << *strs_.nth(args.front()) << name << *strs_.nth(args.back()) << ")";
        } else {
            tmp_.reset() << name;
            if (!args.empty()) {
                tmp_ << "(" << Util::p_range(args, [&](auto &out, auto idx) { out << *strs_.nth(idx); }) << ")";
            }
        }
        return str_id_(tmp_.view());
    }

    auto do_tup(TheoryTermTupleType type, std::span<size_t const> args) -> size_t override {
        auto [od, cd] = [&]() -> std::pair<char const *, char const *> {
            switch (type) {
                case TheoryTermTupleType::list: {
                    return {"[", "]"};
                }
                case TheoryTermTupleType::set: {
                    return {"{", "}"};
                }
                case TheoryTermTupleType::tuple: {
                    return {"(", args.size() == 1 ? ",)" : ")"};
                }
            }
            Util::unreachable();
        }();
        tmp_.reset() << od << Util::p_range(args, [&](auto &out, auto idx) { out << *strs_.nth(idx); }) << cd;
        return str_id_(tmp_.view());
    }

    auto do_sym(Symbol sym) -> size_t override {
        tmp_.reset() << sym;
        return str_id_(tmp_.view());
    }

    auto do_elem(IndexSpan tuple, size_t cond) -> size_t override {
        tmp_.reset() << Util::p_range(tuple, [this](auto &out, auto idx) { out << *strs_.nth(idx); });
        auto const &sc = *strs_.nth(cond);
        if (tuple.empty() || !sc.empty()) {
            tmp_ << ": ";
        }
        tmp_ << *strs_.nth(cond);
        return str_id_(tmp_.view());
    }

    void do_atom([[maybe_unused]] OutputTheory::AtomType type, size_t atom_uid, Symbol name, IndexSpan elems,
                 OptGuard guard) override {
        tmp_.reset() << "&" << name;
        if (!elems.empty()) {
            tmp_ << " { " << Util::p_range(elems, "; ", [this](auto &out, auto idx) { out << *strs_.nth(idx); })
                 << " }";
        }
        if (guard) {
            tmp_ << " " << guard->first << " " << *strs_.nth(guard->second);
        }
        body_.define(atom_uid, tmp_.str());
    }

    void do_simplify([[maybe_unused]] std::function<TruthValue(prg_lit_t)> const &pred) override {}

    Util::unordered_set<std::tuple<bool, SharedString, size_t>> seen_;
    Util::OutputBuffer *out_;
    Util::OutputBuffer tmp_;
    OutputBody body_{uids_};
    OutputCond cond_;
    Util::ordered_set<std::string> strs_;
    size_t uids_ = 0;
    bool explicit_show_ = true;
};

} // namespace

auto make_text_output(Util::OutputBuffer &out) -> UOutputStm {
    return std::make_unique<OutputText>(out);
}

} // namespace CppClingo::Output
