#include <clingo/output/backend.hh>

#include <clingo/util/ordered_set.hh>
#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>
#include <clingo/util/unordered_map.hh>

namespace Clingo::Output {

namespace {

//! Output handling conditions.
//!
//! Each literal is mapped to a program literal and appended to a vector of
//! literals that can be retrieved via function literals.
//!
//! Only supports simple literals excluding aggregates, theory atoms and conditions.
class OutputCond : public OutputLit {
  public:
    OutputCond(Backend &backend, size_t &uids) : backend_{&backend}, uids_{&uids} {}

    //! Get the literals of the body.
    //!
    //! Literals are added via the OutputLit interface.
    //!
    //! @return the literals
    [[nodiscard]] auto literals() const -> std::vector<int32_t> const & { return body_; }

    auto backend() -> Backend & { return *backend_; };

    auto append(Sign sign, size_t uid) {
        switch (sign) {
            case Sign::none: {
                body_.emplace_back(static_cast<int32_t>(uid));
                return;
            }
            case Sign::once: {
                body_.emplace_back(-static_cast<int32_t>(uid));
                return;
            }
            case Sign::twice: {
                // Note: better way to handle?
                auto bd = std::array{-static_cast<int32_t>(uid)};
                auto hd = std::array{static_cast<uint32_t>(++*uids_)};
                backend_->rule(hd, bd, false);
                body_.emplace_back(-static_cast<int32_t>(hd[0]));
                return;
            }
        }
        Util::unreachable();
    }

    auto append(size_t uid) { body_.emplace_back(static_cast<int32_t>(uid)); }

    auto uid() -> size_t { return ++*uids_; }

    void start() { body_.clear(); }

  private:
    void do_lit(Sign sign, [[maybe_unused]] Symbol sym, size_t uid) override { append(sign, uid); }

    void do_boolean(bool value) override {
        if (!value) {
            // Note: implemented for completeness; should not happen.
            body_.emplace_back(1);
            body_.emplace_back(-1);
        }
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

    Backend *backend_;
    size_t *uids_;
    std::vector<int32_t> body_;
};

//! Output handling rule bodies.
//!
//! Each literal is mapped to a program literal and appended to a vector of
//! literals that can be retrieved via function literals.
//!
//! Supports the full range of clingo's body literals.
class OutputBody : public OutputCond {
  public:
    OutputBody(Backend &backend, size_t &uids) : OutputCond(backend, uids) {}

  private:
    auto do_cond_lit(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = this->uid();
        }
        append(*uid);
        return *uid;
    }

    auto do_bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = this->uid();
        }
        append(sign, *uid);
        return *uid;
    }

    auto do_bd_theory(Sign sign, std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = this->uid();
        }
        append(sign, *uid);
        return *uid;
    }
};

class OutputBackend : public OutputStm, OutputTheory {
  public:
    OutputBackend(Backend &backend) : backend_{&backend} {};

  private:
    void do_fact(Symbol sym, size_t uid) override {
        auto hd = std::array{static_cast<uint32_t>(uid)};
        auto bd = std::array{static_cast<int32_t>(uid)};
        backend_->rule(hd, std::span<int32_t>{}, false);
        backend_->show(sym, bd);
    }

    [[nodiscard]] auto do_body() -> OutputLit & override {
        body_.start();
        return body_;
    }

    void do_rule(std::optional<std::tuple<Symbol, size_t, bool>> head) override {
        atoms_.clear();
        bool choice = false;
        if (head) {
            choice = get<2>(*head);
            atoms_.emplace_back(get<1>(*head));
        }
        backend_->rule(atoms_, body_.literals(), choice);
    }

    void do_show_term(Symbol term) override { backend_->show(term, body_.literals()); }

    void do_external(Symbol atom, ExternalType type) override {
        static_cast<void>(atom);
        static_cast<void>(type);
        // *out_ << "#external " << atom << ". [" << type << "]\n";
        // out_->endl();
        throw std::logic_error{"implement me: external"};
    }

    void do_project(Symbol atom) override {
        static_cast<void>(atom);
        // *out_ << "#project " << atom << ".\n";
        // out_->endl();
        throw std::logic_error{"implement me: project"};
    }

    auto do_aggr_rule(std::optional<size_t> uid) -> size_t override {
        static_cast<void>(uid);
        // return body_.delay_head(uid, " :- ");
        throw std::logic_error{"implement me: aggr_rule"};
    }

    auto do_theory_rule(std::optional<size_t> uid) -> size_t override {
        static_cast<void>(uid);
        // return body_.delay_head(uid, " :- ");
        throw std::logic_error{"implement me: theory_rule"};
    }

    auto do_disjunctive_rule(std::optional<size_t> uid) -> size_t override {
        static_cast<void>(uid);
        // return body_.delay_head(uid, " :- ");
        throw std::logic_error{"implement me: disjunctive_rule"};
    }

    void do_weak_constraint(Number const &weight, std::optional<Symbol> prio, SymbolSpan terms) override {
        static_cast<void>(weight);
        static_cast<void>(prio);
        static_cast<void>(terms);
        // auto p_tup = [&](auto &out) {
        //     out << ". [" << weight;
        //     if (prio) {
        //         out << "@" << *prio;
        //     }
        //     for (auto const &term : terms) {
        //         out << "," << term;
        //     }
        //     out << "].\n";
        // };
        // if (!body_.delayed()) {
        //     *out_ << " :~ ";
        //     *out_ << body_.end();
        //     p_tup(*out_);
        //     out_->endl();
        // } else {
        //     p_tup(body_.buf());
        //     body_.delay();
        //     body_.buf() << " :~ ";
        //     body_.prepend();
        // }
        throw std::logic_error{"implement me: weak_constraint"};
    }

    void do_heuristic(Symbol atom, Number const &weight, Number const *prio, HeuristicType type) override {
        static_cast<void>(atom);
        static_cast<void>(weight);
        static_cast<void>(prio);
        static_cast<void>(type);
        // auto p_tup = [&](auto &out) {
        //     out << ". [" << weight;
        //     if (prio) {
        //         out << "@" << *prio;
        //     }
        //     out << "," << type;
        //     out << "]\n";
        // };
        // if (!body_.delayed()) {
        //     *out_ << "#heuristic " << atom;
        //     if (!body_.empty()) {
        //         *out_ << ": ";
        //     }
        //     *out_ << body_.end();
        //     p_tup(*out_);
        //     out_->endl();
        // } else {
        //     p_tup(body_.buf());
        //     body_.delay();
        //     body_.buf() << "#heuristic " << atom << ": ";
        //     body_.prepend();
        // }
        throw std::logic_error{"implement me: heuristic"};
    }

    void do_edge(Symbol src, Symbol dst) override {
        static_cast<void>(src);
        static_cast<void>(dst);
        // if (!body_.delayed()) {
        //     *out_ << "#edge (" << src << "," << dst << ")";
        //     if (!body_.empty()) {
        //         *out_ << ": ";
        //     }
        //     *out_ << body_.end() << ".\n";
        //     out_->endl();
        // } else {
        //     body_.buf() << ".\n";
        //     body_.delay();
        //     *out_ << "#edge (" << src << "," << dst << "): ";
        //     body_.prepend();
        // }
        throw std::logic_error{"implement me: edge"};
    }

    auto do_cond() -> OutputLit & override {
        cond_.start();
        return cond_;
    }

    auto do_cond_id() -> size_t override {
        auto lits = cond_.literals();
        // TODO: implement special handling for one-elementary conditions here!!!
        // TODO: maintain a dictionary of conditions
        auto hd = std::array{static_cast<uint32_t>(uid())};
        backend_->rule(hd, lits, false);
        return hd[0];
    }

    auto do_uid() -> size_t override { return ++uids_; }

    void do_cond_lit(size_t uid, CondLits elems) override {
        static_cast<void>(uid);
        static_cast<void>(elems);
        // if (elems.empty()) {
        //     body_.define(uid, "#true");
        // } else {
        //     tmp_.reset();
        //     tmp_ << Util::p_range(elems, "; ", [this](auto &buf, auto const &elem) {
        //         if (elem.first) {
        //             buf << *strs_.nth(*elem.first);
        //         } else {
        //             buf << "#false";
        //         }
        //         buf << ": " << *strs_.nth(elem.second);
        //     });
        //     body_.define(uid, tmp_.str());
        // }
        throw std::logic_error{"implement me: cond_lit"};
    }

    void aggr(size_t uid, AggregateFunction fun, auto elems, Guards guards, auto prt) {
        static_cast<void>(uid);
        static_cast<void>(fun);
        static_cast<void>(elems);
        static_cast<void>(guards);
        static_cast<void>(prt);
        // tmp_.reset();
        // auto it = guards.begin();
        // if (guards.size() > 1) {
        //     tmp_ << it->second << " " << flip(it->first) << " ";
        //     ++it;
        // }
        // tmp_ << fun << " { " << Util::p_range(elems, "; ", [prt](auto &buf, auto const &elem) { prt(buf, elem); })
        //      << (elems.empty() ? "}" : " }");
        // for (auto ie = guards.end(); it != ie; ++it) {
        //     tmp_ << " " << it->first << " " << it->second;
        // }
        // body_.define(uid, tmp_.str());
        throw std::logic_error{"implement me: aggr"};
    }

    void do_bd_aggr(size_t uid, AggregateFunction fun, BdElems elems, Guards guards) override {
        static_cast<void>(uid);
        static_cast<void>(fun);
        static_cast<void>(elems);
        static_cast<void>(guards);
        // aggr(uid, fun, elems, guards, [this](auto &buf, auto const &elem) {
        //     if (elem.second.empty()) {
        //         buf << Util::p_range(elem.first);
        //         if (elem.first.empty()) {
        //             buf << ": ";
        //         }
        //     } else {
        //         buf << Util::p_range(elem.second, "; ", [this, &elem](auto &buf, auto const &cond) {
        //             buf << Util::p_range(elem.first) << ": " << *strs_.nth(cond);
        //         });
        //     }
        // });
        throw std::logic_error{"implement me: bd_aggr"};
    }

    void do_hd_aggr(size_t uid, AggregateFunction fun, HdElems elems, Guards guards) override {
        static_cast<void>(uid);
        static_cast<void>(fun);
        static_cast<void>(elems);
        static_cast<void>(guards);
        // aggr(uid, fun, elems, guards, [this](auto &buf, auto const &elem) {
        //     buf << Util::p_range(elem.second, "; ", [this, &elem](auto &buf, auto const &hc) {
        //         buf << Util::p_range(elem.first) << ": ";
        //         if (hc.first.type() == SymbolType::sup) {
        //             buf << "#true";
        //         } else {
        //             buf << hc.first;
        //         }
        //         buf << *strs_.nth(hc.second);
        //     });
        // });
        throw std::logic_error{"implement me: hd_aggr"};
    }

    void do_disjunction(size_t uid, DisjunctionElems elems) override {
        static_cast<void>(uid);
        static_cast<void>(elems);
        // tmp_.reset();
        // if (elems.empty()) {
        //     tmp_ << "#false";
        // } else {
        //     tmp_ << Util::p_range(elems, "; ", [this](auto &buf, DisjunctionElem const &elem) {
        //         if (elem.second.empty()) {
        //             if (elem.first.type() == SymbolType::sup) {
        //                 buf << "#true";
        //             } else {
        //                 buf << elem.first;
        //             }
        //         } else {
        //             buf << Util::p_range(elem.second, "; ", [this, &elem](auto &buf, auto const &cond) {
        //                 buf << elem.first << ": " << *strs_.nth(cond);
        //             });
        //         }
        //     });
        // }
        // body_.define(uid, tmp_.str());
        throw std::logic_error{"implement me: disjunction"};
    }

    auto do_theory() -> OutputTheory & override {
        // return *this;
        throw std::logic_error{"implement me: theory"};
    }

    void do_flush() override {}

    void do_end_step() override {}

    void do_mark([[maybe_unused]] SymbolCollector &gc) override {}

    auto do_str(String val) -> size_t override {
        static_cast<void>(val);
        // return str_id_(val.view());
        throw std::logic_error{"implement me: theory str"};
    }

    auto do_num(Number const &val) -> size_t override {
        static_cast<void>(val);
        // if (val < 0) {
        //     tmp_.reset() << "(" << val << ")";
        // } else {
        //     tmp_.reset() << val;
        // }
        // return str_id_(tmp_.view());
        throw std::logic_error{"implement me: theory num"};
    }

    auto do_fun(String name, std::span<size_t const> args) -> size_t override {
        static_cast<void>(name);
        static_cast<void>(args);
        // auto is_theory_op = [](std::string_view str) {
        //     if (str.empty()) {
        //         return false;
        //     }
        //     switch (str.front()) {
        //         case '/':
        //         case '!':
        //         case '<':
        //         case '=':
        //         case '>':
        //         case '+':
        //         case '-':
        //         case '*':
        //         case '\\':
        //         case '?':
        //         case '&':
        //         case '@':
        //         case '|':
        //         case ':':
        //         case ';':
        //         case '~':
        //         case '^':
        //         case '.': {
        //             return true;
        //         }
        //         default: {
        //             return false;
        //         }
        //     }
        // };
        // if (args.size() == 1 && is_theory_op(name.view())) {
        //     tmp_.reset() << "(" << name << *strs_.nth(args.back()) << ")";
        // } else if (args.size() == 2 && is_theory_op(name.view())) {
        //     tmp_.reset() << "(" << *strs_.nth(args.front()) << name << *strs_.nth(args.back()) << ")";
        // } else {
        //     tmp_.reset() << name;
        //     if (!args.empty()) {
        //         tmp_ << "(" << Util::p_range(args, [&](auto &out, auto idx) { out << *strs_.nth(idx); }) << ")";
        //     }
        // }
        // return str_id_(tmp_.view());
        throw std::logic_error{"implement me: theory fun"};
    }

    auto do_tup(TheoryTermTupleType type, std::span<size_t const> args) -> size_t override {
        static_cast<void>(type);
        static_cast<void>(args);
        // auto [od, cd] = [&]() -> std::pair<char const *, char const *> {
        //     switch (type) {
        //         case TheoryTermTupleType::list: {
        //             return {"[", "]"};
        //         }
        //         case TheoryTermTupleType::set: {
        //             return {"{", "}"};
        //         }
        //         case TheoryTermTupleType::tuple: {
        //             return {"(", args.size() == 1 ? ",)" : ")"};
        //         }
        //     }
        //     Util::unreachable();
        // }();
        // tmp_.reset() << od << Util::p_range(args, [&](auto &out, auto idx) { out << *strs_.nth(idx); }) << cd;
        // return str_id_(tmp_.view());
        throw std::logic_error{"implement me: theory tup"};
    }

    auto do_elem(IndexSpan tuple, size_t cond) -> size_t override {
        static_cast<void>(tuple);
        static_cast<void>(cond);
        // tmp_.reset() << Util::p_range(tuple, [this](auto &out, auto idx) { out << *strs_.nth(idx); });
        // auto const &sc = *strs_.nth(cond);
        // if (tuple.empty() || !sc.empty()) {
        //     tmp_ << ": ";
        // }
        // tmp_ << *strs_.nth(cond);
        // return str_id_(tmp_.view());
        throw std::logic_error{"implement me: theory elem"};
    }

    void do_atm(size_t atom_uid, Symbol name, IndexSpan elems, OptGuard guard) override {
        static_cast<void>(atom_uid);
        static_cast<void>(name);
        static_cast<void>(elems);
        static_cast<void>(guard);
        // tmp_.reset() << "&" << name;
        // if (!elems.empty()) {
        //     tmp_ << " { " << Util::p_range(elems, "; ", [this](auto &out, auto idx) { out << *strs_.nth(idx); })
        //          << " }";
        // }
        // if (guard) {
        //     tmp_ << " " << *strs_.nth(guard->first) << " " << *strs_.nth(guard->second);
        // }
        // body_.define(atom_uid, tmp_.str());
        throw std::logic_error{"implement me: theory atom"};
    }

    size_t uids_ = 0;
    Backend *backend_;
    OutputBody body_{*backend_, uids_};
    OutputCond cond_{*backend_, uids_};
    std::vector<uint32_t> atoms_;
    // Util::OutputBuffer tmp_;
    // OutputCond cond_;
    // Util::ordered_set<std::string> strs_;
    // bool explicit_show_ = true;
};

} // namespace

auto make_backend_output(Backend &backend) -> UOutputStm { return std::make_unique<OutputBackend>(backend); }

} // namespace Clingo::Output
