#include <clingo/output/backend.hh>

#include <clingo/util/ordered_set.hh>
#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>
#include <clingo/util/unordered_map.hh>

namespace Clingo::Output {

namespace {

//! Output handling rule bodies.
//!
//! Each literal is mapped to a program literal and appended to a vector of
//! literals that can be retrieved via function literals.
class OutputBody : public OutputLit {
  public:
    OutputBody(size_t &uids) : uids_{&uids} {}

    //! Get the literals of the body.
    //!
    //! Literals are added via the OutputLit interface.
    //!
    //! @return the literals
    [[nodiscard]] auto literals() const -> std::vector<size_t> const & { return body_; }

    void start() { body_.clear(); }

  private:
    void do_lit(Sign sign, Symbol sym, size_t uid) override {
        static_cast<void>(sign);
        static_cast<void>(sym);
        body_.emplace_back(uid);
    }

    void do_boolean(bool value) override {
        if (!value) {
            // TODO: need to introduce false literal
            body_.emplace_back(-1);
        }
    }

    auto do_cond_lit(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = ++*uids_;
        }
        body_.emplace_back(*uid);
        return *uid;
    }

    auto do_bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t override {
        static_cast<void>(sign);
        if (!uid) {
            uid = ++*uids_;
        }
        body_.emplace_back(*uid);
        return *uid;
    }

    auto do_bd_theory(Sign sign, std::optional<size_t> uid) -> size_t override {
        static_cast<void>(sign);
        if (!uid) {
            uid = ++*uids_;
        }
        body_.emplace_back(*uid);
        return *uid;
    }

    size_t *uids_;
    std::vector<size_t> body_;
};

/*
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

    void do_lit(Sign sign, Symbol sym) override {
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

*/

class OutputBackend : public OutputStm, OutputTheory {
  public:
    OutputBackend(Backend &backend) : backend_{&backend} {};

  private:
    // template <class T> static void simple_head_(T &out, std::optional<std::pair<Symbol, bool>> const &head) {
    //     if (head) {
    //         if (head->second) {
    //             out << "{ ";
    //         }
    //         out << head->first;
    //         if (head->second) {
    //             out << " }";
    //         }
    //     }
    // }

    void do_fact(Symbol sym) override {
        static_cast<void>(sym);
        static_cast<void>(backend_);
        // *out_ << sym << ".\n";
        // out_->endl();
        throw std::logic_error{"implement me"};
    }

    [[nodiscard]] auto do_body() -> OutputLit & override {
        body_.start();
        return body_;
    }

    void do_rule(std::optional<std::pair<Symbol, bool>> head) override {
        static_cast<void>(head);
        // backend_->rule(head, body_.literals());
        //  TODO: simply pass rule to backend
        throw std::logic_error{"implement me"};
    }

    void do_show_term(Symbol term) override {
        static_cast<void>(term);
        // if (!body_.delayed()) {
        //     *out_ << "#show " << term;
        //     if (!body_.empty()) {
        //         *out_ << ": " << body_.end();
        //     }
        //     *out_ << ".\n";
        //     out_->endl();
        // } else {
        //     body_.buf() << ".\n";
        //     body_.delay();
        //     body_.buf() << "#show " << term << ": ";
        //     body_.prepend();
        // }
        throw std::logic_error{"implement me"};
    }

    void do_external(Symbol atom, ExternalType type) override {
        static_cast<void>(atom);
        static_cast<void>(type);
        // *out_ << "#external " << atom << ". [" << type << "]\n";
        // out_->endl();
        throw std::logic_error{"implement me"};
    }

    void do_project(Symbol atom) override {
        static_cast<void>(atom);
        // *out_ << "#project " << atom << ".\n";
        // out_->endl();
        throw std::logic_error{"implement me"};
    }

    auto do_aggr_rule(std::optional<size_t> uid) -> size_t override {
        static_cast<void>(uid);
        // return body_.delay_head(uid, " :- ");
        throw std::logic_error{"implement me"};
    }

    auto do_theory_rule(std::optional<size_t> uid) -> size_t override {
        static_cast<void>(uid);
        // return body_.delay_head(uid, " :- ");
        throw std::logic_error{"implement me"};
    }

    auto do_disjunctive_rule(std::optional<size_t> uid) -> size_t override {
        static_cast<void>(uid);
        // return body_.delay_head(uid, " :- ");
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
    }

    auto do_cond() -> OutputLit & override {
        // cond_.start();
        // return cond_;
        throw std::logic_error{"implement me"};
    }

    auto do_cond_id() -> size_t override {
        // return str_id_(cond_.end());
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
    }

    auto do_theory() -> OutputTheory & override {
        // return *this;
        throw std::logic_error{"implement me"};
    }

    void do_flush() override {
        // body_.flush(*out_);
        throw std::logic_error{"implement me"};
    }

    void do_end_step() override {
        // if (std::exchange(explicit_show_, false)) {
        //     *out_ << "#show.\n";
        // }
        // out_->flush();
        throw std::logic_error{"implement me"};
    }

    void do_mark([[maybe_unused]] SymbolCollector &gc) override {}

    auto do_str(String val) -> size_t override {
        static_cast<void>(val);
        // return str_id_(val.view());
        throw std::logic_error{"implement me"};
    }

    auto do_num(Number const &val) -> size_t override {
        static_cast<void>(val);
        // if (val < 0) {
        //     tmp_.reset() << "(" << val << ")";
        // } else {
        //     tmp_.reset() << val;
        // }
        // return str_id_(tmp_.view());
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
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
        throw std::logic_error{"implement me"};
    }

    size_t uids_ = 0;
    OutputBody body_{uids_};
    Backend *backend_;
    // Util::OutputBuffer tmp_;
    // OutputCond cond_;
    // Util::ordered_set<std::string> strs_;
    // bool explicit_show_ = true;
};

} // namespace

auto make_backend_output(Backend &backend) -> UOutputStm { return std::make_unique<OutputBackend>(backend); }

} // namespace Clingo::Output
