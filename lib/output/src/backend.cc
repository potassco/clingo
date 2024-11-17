#include <clingo/output/backend.hh>

#include <clingo/util/ordered_map.hh>
#include <clingo/util/ordered_set.hh>
#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>
#include <clingo/util/unordered_map.hh>

#include <clingo/util/checked_math.hh>

namespace Clingo::Output {

namespace {

//! The maximum literal.
constexpr auto lit_max = std::numeric_limits<lit_t>::max();
//! The minimum literal.
constexpr auto lit_min = -lit_max;
//! The maximum condition.
constexpr auto cond_max = std::numeric_limits<size_t>::max() >> 1;

//! Convert a uid to an integer literal.
//!
//! @pre 1 <= uid <= lit_max
//!
//! @param uid the uid to convert
//! @return the resulting literal
auto uid_to_lit(size_t uid) -> lit_t {
    assert(1 <= uid && uid <= static_cast<atom_t>(lit_max));
    return static_cast<int32_t>(uid);
}

//! Convert a uid to an atom.
//!
//! @pre 1 <= uid <= lit_max
//!
//! @param uid the uid to convert
//! @return the resulting atom
auto uid_to_atom(size_t uid) -> atom_t {
    assert(1 <= uid && uid <= static_cast<atom_t>(lit_max));
    return static_cast<atom_t>(uid);
}

//! Get the atom corresponding to a literal.
//!
//! @pre lit != 0 && lit >= lit_min
//!
//! @param lit the literal to convert
//! @return the resulting atom
auto lit_to_atom(lit_t lit) -> atom_t {
    assert(lit != 0 && lit >= lit_min);
    return static_cast<atom_t>(lit);
}

//! Available condition types.
//!
//! Conditions are clauses associated with a literal. The equivalence between
//! the literal and the clause is either established with an implication (a
//! rule) or an equivalence.
enum class CondType : uint8_t {
    implication, //!< only forward direction is necessary
    equivalence, //!< forward and backward directions are necessary
};

//! Available types of uinque identifiers.
enum class UIDType : uint8_t {
    lit,    //!< identify a literal
    aggr,   //!< identify a body aggregate
    theory, //!< identify a body theory atom
    cond,   //!< identify a condition
};

//! Helper class to translate formulas (aggregates, conditional literal, etc.)
//! into logic programs as accepted by the backend.
class OutputHelper {
  public:
    //! Construct the output helper.
    //!
    //! The reference to the backend is stored and used by the helper.
    //!
    //! @param backend the underlying backend
    OutputHelper(Backend &backend) : backend_{&backend} {}

    //! Introduce a new uid.
    //!
    //! Literal uids can be used as Tseitin literals.
    //!
    //! @return a fresh uid of the given type
    [[nodiscard]] auto uid(UIDType type) -> size_t {
        switch (type) {
            case UIDType::lit: {
                if (lit_uids_ < lit_max) {
                    return ++lit_uids_;
                }
                throw std::range_error("maximum number of literals exhausted");
            }
            case UIDType::cond: {
                return ++cond_uids_;
            }
            case UIDType::aggr: {
                throw std::logic_error("implement me: uid bd aggr");
            }
            case UIDType::theory: {
                throw std::logic_error("implement me: uid bd theory");
            }
        }
        Util::unreachable();
    }

    //! Get a unique id identifying the given literals.
    //!
    //! The function stores a map from the set of literals to the unique identifiers.
    //!
    //! @param lits the literals
    auto cond(LitVec const &lits) -> size_t {
        auto copy = lits;
        std::ranges::sort(copy);
        copy.erase(std::ranges::unique(copy).begin(), copy.end());
        if (copy.size() == 1) {
            auto res = Util::safe_cast<ssize_t>((int64_t{copy[0]} << 1) | 1);
            return static_cast<size_t>(res);
        }
        auto state = uint64_t{0};
        auto it = conds_.emplace(std::move(copy), state).first;
        if (auto res = static_cast<size_t>(std::distance(conds_.begin(), it)); res <= cond_max) {
            return res << 1;
        }
        throw std::range_error("maximum number of conditions exhausted");
    }

    //! Get a Tseitin literal for the condition with the given uid.
    //!
    //! The type parameter determines the kind of equivalence between condition
    //! and its Tseitin literal.
    //!
    //! @param uid the uid of the condition
    //! @param type the type of the Tseitin literal
    //! @return the resulting literal
    auto cond(size_t uid, CondType type) -> lit_t {
        auto it = CondMap::iterator{};
        if ((uid & 1) == 1) {
            return static_cast<lit_t>(static_cast<ssize_t>(uid) >> 1);
        }
        it = conds_.nth(uid >> 1);
        auto lit = static_cast<lit_t>(it.value() >> 2);
        auto cur = it.value() & 2;
        // add forward
        if (cur == 0) {
            lit = uid_to_lit(this->uid(UIDType::lit));
            it.value() = (static_cast<uint64_t>(static_cast<atom_t>(lit)) << 2) | 1;
            auto hd = std::array{lit_to_atom(lit)};
            backend_->rule(hd, it.key(), false);
        }
        // add backward
        if (cur == 1 && type == CondType::equivalence) {
            it.value() |= 2;
            for (auto const &clit : it.key()) {
                if (clit > 0) {
                    auto hd = std::array{lit_to_atom(clit)};
                    auto bd = std::array{lit};
                    backend_->rule(hd, bd, false);
                }
            }
        }
        return lit;
    }

    //! Negate the given literal.
    //!
    //! Introduces a Tseitin literal if the given literal is negative.
    //!
    //! @param lit the literal to negate
    //! @return the negated literal
    auto negate(lit_t lit) -> lit_t {
        // TODO: maybe store a map for double negated literals.
        if (lit > 0) {
            return -lit;
        }
        auto nlit = uid_to_lit(uid(UIDType::lit));
        backend_->rule(std::array{lit_to_atom(nlit)}, std::array{lit}, false);
        return -nlit;
    }

    [[nodiscard]] auto backend() -> Backend & { return *backend_; }

  private:
    // TODO: the vector can be stored more compactly.
    using CondMap = Util::ordered_map<LitVec, uint64_t>;
    Backend *backend_;
    CondMap conds_;
    size_t lit_uids_ = 0;
    size_t cond_uids_ = 0;
};

//! Output handling conditions.
//!
//! Each literal is mapped to a program literal and appended to a vector of
//! literals that can be retrieved via function literals.
//!
//! Only supports simple literals excluding aggregates, theory atoms and conditions.
class OutputCond : public OutputLit {
  public:
    OutputCond(OutputHelper &helper) : helper_{&helper} {}

    //! Get the literals of the body.
    //!
    //! Literals are added via the OutputLit interface.
    //!
    //! @return the literals
    [[nodiscard]] auto literals() const -> LitVec const & { return body_; }

    //! Get the helper of the output.
    //!
    //! @return the helper
    auto helper() -> OutputHelper & { return *helper_; };

    //! Get the backend of the output.
    //!
    //! @return the backend
    auto backend() -> Backend & { return helper_->backend(); };

    //! Append the atom with the given sign.
    //!
    //! @pre The uid must be a literal uid.
    //!
    //! @param sign the sign of the literal
    //! @param uid the uid
    auto append(Sign sign, size_t uid) {
        switch (sign) {
            case Sign::none: {
                body_.emplace_back(uid_to_lit(uid));
                return;
            }
            case Sign::once: {
                body_.emplace_back(-uid_to_lit(uid));
                return;
            }
            case Sign::twice: {
                body_.emplace_back(helper_->negate(-uid_to_lit(uid)));
                return;
            }
        }
        Util::unreachable();
    }

    //! Append the given uid to the body.
    //!
    //! Equivalent to `append(Sign::none, uid)`.
    //!
    //! @param uid the uid
    auto append(size_t uid) { body_.emplace_back(uid_to_lit(uid)); }

    //! Start a new condition/body clearing the underlying literals.
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

    OutputHelper *helper_;
    LitVec body_;
};

//! Output handling rule bodies.
//!
//! Each literal is mapped to a program literal and appended to a vector of
//! literals that can be retrieved via function literals.
//!
//! Supports the full range of clingo's body literals.
class OutputBody : public OutputCond {
  public:
    OutputBody(OutputHelper &helper) : OutputCond(helper) {}

  private:
    auto do_cond_lit(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = helper().uid(UIDType::cond);
        }
        append(*uid);
        return *uid;
    }

    auto do_bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = helper().uid(UIDType::aggr);
        }
        append(sign, *uid);
        return *uid;
    }

    auto do_bd_theory(Sign sign, std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = helper().uid(UIDType::theory);
        }
        append(sign, *uid);
        return *uid;
    }
};

class OutputBackend : public OutputStm, OutputTheory {
  public:
    OutputBackend(Backend &backend) : helper_{backend} {};

  private:
    void do_fact(Symbol sym, size_t uid) override {
        auto hd = std::array{uid_to_atom(uid)};
        auto bd = std::array{uid_to_lit(uid)};
        helper_.backend().rule(hd, LitSpan{}, false);
        helper_.backend().show(sym, bd);
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
        helper_.backend().rule(atoms_, body_.literals(), choice);
    }

    void do_show_term(Symbol term) override { helper_.backend().show(term, body_.literals()); }

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

    auto do_cond_id() -> size_t override { return helper_.cond(cond_.literals()); }

    auto do_lit_uid() -> size_t override { return helper_.uid(UIDType::lit); }

    void do_cond_lit(size_t uid, CondLits elems) override {
        // TODO: It might be possible to reduce the number of auxiliary program
        // literals by only introducing only one variable K for all conditions.
        // TODO: can be a member
        LitVec body;
        body.reserve(elems.size());
        for (auto const &elem : elems) {
            auto const &[conc, cond] = elem;
            if (conc) {
                // Note: conc is encoded as a condition, which is guaranteed to
                // be mapped to a positive literal.
                // Below, we us the following variable names:
                // - K: new uid replacing G : F in the body
                // - G: conc
                // - F: cond
                auto g = helper_.cond(*conc, CondType::equivalence);
                auto f = helper_.cond(cond, CondType::equivalence);
                auto k = uid_to_lit(helper_.uid(UIDType::lit));
                auto &bck = helper_.backend();
                bck.rule(std::array{lit_to_atom(k)}, std::array{g}, false);
                assert(g > 0);
                if (f > 0) {
                    // formula G : F is replaced by K
                    // K :- G.
                    // K :- not F.
                    // K | F :- not not G.
                    bck.rule(std::array{lit_to_atom(k)}, std::array{-f}, false);
                    bck.rule(std::array{lit_to_atom(k), lit_to_atom(f)}, std::array{helper_.negate(-g)}, false);
                } else {
                    // the above formulas can be simplified
                    // K :- G.
                    // K :- not F.
                    bck.rule(std::array{lit_to_atom(k)}, std::array{helper_.negate(f)}, false);
                }
                body.emplace_back(k);
            } else {
                body.emplace_back(helper_.negate(helper_.cond(uid, CondType::implication)));
            }
        }
        auto hd = std::array{uid_to_atom(uid)};
        helper_.backend().rule(hd, body, false);
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
        // tmp_ << fun << " { " << Util::p_range(elems, "; ", [prt](auto &buf, auto const &elem) { prt(buf, elem);
        // })
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

    auto do_fun(String name, IndexSpan args) -> size_t override {
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

    auto do_tup(TheoryTermTupleType type, IndexSpan args) -> size_t override {
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

    OutputHelper helper_;
    OutputBody body_{helper_};
    OutputCond cond_{helper_};
    AtomVec atoms_;
    // Util::OutputBuffer tmp_;
    // OutputCond cond_;
    // Util::ordered_set<std::string> strs_;
    // bool explicit_show_ = true;
};

} // namespace

auto make_backend_output(Backend &backend) -> UOutputStm { return std::make_unique<OutputBackend>(backend); }

} // namespace Clingo::Output
