#include <clingo/output/backend.hh>

#include <clingo/util/checked_math.hh>
#include <clingo/util/ordered_map.hh>
#include <clingo/util/ordered_set.hh>
#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>

#include <iostream>

namespace Clingo::Output {

namespace {

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

/* TODO: maybe remove those
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
*/

//! Output handling conditions.
//!
//! Each literal is mapped to a program literal and appended to a vector of
//! literals that can be retrieved via function literals.
//!
//! Only supports simple literals excluding aggregates, theory atoms and conditions.
class OutputCond : public OutputLit {
  public:
    OutputCond(Backend &backend) : backend_{&backend} {}

    //! Get the literals of the body.
    //!
    //! Literals are added via the OutputLit interface.
    //!
    //! @return the literals
    [[nodiscard]] auto literals() const -> LitVec const & { return body_; }

    //! Get the backend of the output.
    //!
    //! @return the backend
    auto backend() -> Backend & { return *backend_; };

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
                body_.emplace_back(backend().negate(-uid_to_lit(uid)));
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

    Backend *backend_;
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
    OutputBody(Backend &backend) : OutputCond(backend) {}

  private:
    auto do_cond_lit(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = backend().next_lit();
        }
        append(*uid);
        return *uid;
    }

    auto do_bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = backend().next_lit();
        }
        append(sign, *uid);
        return *uid;
    }

    auto do_bd_theory(Sign sign, std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = backend().next_lit();
        }
        append(sign, *uid);
        return *uid;
    }
};

class OutputBackend : public OutputStm, OutputTheory {
  public:
    OutputBackend(Backend &backend) : backend_{&backend} {};

  private:
    void do_fact([[maybe_unused]] Symbol sym, size_t uid) override {
        backend().rule(std::array{uid_to_lit(uid)}, LitSpan{}, false);
    }

    [[nodiscard]] auto do_body() -> OutputLit & override {
        body_.start();
        return body_;
    }

    void do_rule(std::optional<std::tuple<Symbol, size_t, bool>> head) override {
        bool choice = false;
        if (head) {
            choice = get<2>(*head);
            auto lit = uid_to_lit(get<1>(*head));
            backend().rule(std::array{lit}, body_.literals(), choice);
        } else {
            backend().rule({}, body_.literals(), choice);
        }
    }

    void do_show_term(Symbol term) override { backend().show(term, body_.literals()); }

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
        if (!uid) {
            uid = backend_->next_lit();
        }
        backend().rule(std::array{uid_to_lit(*uid)}, body_.literals(), false);
        return *uid;
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

    auto do_cond_id() -> size_t override { return backend().cond(cond_.literals()); }

    auto do_uid() -> size_t override { return backend().next_lit(); }

    void do_cond_lit(size_t uid, CondLits elems) override { backend_->cond_lit(uid_to_lit(uid), elems); }

    void do_bd_aggr(size_t uid, AggregateFunction fun, BdElems elems, Guards guards) override {
        backend_->bd_aggr(uid_to_lit(uid), fun, elems, guards);
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

    void do_end_step() override { backend_->end_step(); }

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

    auto backend() -> Backend & { return *backend_; }

    LitVec lits_;
    Backend *backend_;
    OutputBody body_{*backend_};
    OutputCond cond_{*backend_};
    // Util::OutputBuffer tmp_;
    // OutputCond cond_;
    // Util::ordered_set<std::string> strs_;
    // bool explicit_show_ = true;
};

} // namespace

auto Backend::info_(lit_t lit) -> LitInfo & {
    assert(lit > 0);
    return lits_[lit - 1];
}

auto Backend::next_lit() -> Output::lit_t {
    if (lit_ < lit_max) {
        lits_.emplace_back();
        return ++lit_;
    }
    throw std::range_error("literals number of literals exhausted");
}

auto Backend::negate(lit_t lit) -> lit_t {
    assert(lit != 0 && lit >= lit_min);
    if (lit > 0) {
        return -lit;
    }
    auto &neg = info_(lit).neg;
    if (neg == 0) {
        neg = next_lit();
        rule(std::array{neg}, std::array{lit}, false);
    }
    return -neg;
}

//! Analyze a sum aggregate.
//!
//! Constructs a new aggregate element vector removing unnecessary elements,
//! computes an interval for largest and smallest value of the aggregate, and
//! represents the guards of the aggreagte as an interval set. This interval set
//! is additionally intersected with the range.
auto Backend::analyze_sum_(BdAggrElemSpan elems, GuardSpan guards)
    -> std::tuple<BdSumAggrElemVec, NumberSet::interval, NumberSet, CycleType> {
    // simplify the aggregate
    auto fixed = Number(0);
    auto elem_vec = BdSumAggrElemVec{};
    auto cond_map = Util::unordered_map<IndexSpan, size_t>{};
    elem_vec.reserve(elems.size());
    cond_map.reserve(elems.size());
    for (auto const &[tup, conds] : elems) {
        if (!tup.empty() && tup.front().type() == SymbolType::number && tup.front().num() != 0) {
            auto num = tup.front().num();
            if (conds.empty()) {
                fixed += num;
            } else {
                if (auto [it, ins] = cond_map.try_emplace(conds, elem_vec.size()); !ins) {
                    get<0>(elem_vec[it.value()]) += num;
                } else {
                    aux2_.assign(conds.begin(), conds.end());
                    elem_vec.emplace_back(std::move(num),
                                          clause_({aux2_.begin(), aux2_.end()}, ClauseType::disjunctive));
                }
            }
        }
    }
    elem_vec.erase(std::ranges::remove_if(elem_vec, [](auto const &x) { return std::get<0>(x) == 0; }).end(),
                   elem_vec.end());
    elem_vec.shrink_to_fit();
    // comput the range of the aggregate
    auto lower = Number(0);
    auto upper = Number(0);
    for (auto const &[num, clit] : elem_vec) {
        if (num > 0) {
            upper += num;
        } else {
            lower += num;
        }
    }
    // compute the bounds of the aggregate
    auto bounds = Util::interval_set<Number>{};
    auto range = Util::interval_set<Number>::interval{{lower, true}, {upper, true}};
    bounds.add(range);
    for (auto const &[rel, guard] : guards) {
        // Note: workaround for buggy clang-tidy diagnostic
        auto adjust = [&, &g = guard]() {
            if (g.type() == SymbolType::number) {
                return g.num() - fixed;
            }
            if (g > upper) {
                return upper + 1;
            }
            assert(g < lower);
            return lower - 1;
        }();
        switch (rel) {
            case Relation::greater_equal: {
                bounds.remove({{lower, true}, {adjust, false}});
                break;
            }
            case Relation::greater: {
                bounds.remove({{lower, true}, {adjust + 1, false}});
                break;
            }
            case Relation::less_equal: {
                bounds.remove({{adjust, false}, {upper, true}});
                break;
            }
            case Relation::less: {
                bounds.remove({{adjust - 1, false}, {upper, true}});
                break;
            }
            case Relation::equal: {
                bounds.remove({{lower, true}, {adjust, false}});
                bounds.remove({{adjust, false}, {upper, true}});
                break;
            }
            case Relation::not_equal: {
                bounds.remove({{adjust - 1, false}, {adjust + 1, false}});
                break;
            }
        }
    }
    // classify which types of cycles have to be considered
    auto type = bounds.size() > 1 ? CycleType::both : CycleType::none;
    if (type != CycleType::both && bounds.size() == 1 && !bounds.contains(range)) {
        auto const &sub = bounds.front();
        if (lower < 0 && upper > 0) {
            // the aggregate can switch arbitrarily between true and false
            type = CycleType::both;
        } else if (lower < 0 && sub < upper) {
            // the aggregate can go from false to true by adding negative weights
            type |= CycleType::negative;
        } else if (upper > 0 && lower < sub) {
            // the aggregate can go from false to true by adding positive weights
            type |= CycleType::positive;
        }
    }
    return {std::move(elem_vec), std::move(range), std::move(bounds), type};
}

void Backend::bd_aggr(lit_t lit, AggregateFunction fun, BdAggrElemSpan elems, GuardSpan guards) {
    assert(fun != AggregateFunction::count);
    if (fun == AggregateFunction::sum || fun == AggregateFunction::sump) {
        auto [elem_vec, range, bounds, type] = analyze_sum_(elems, guards);
        if (bounds.contains(range)) {
            rule(std::array{lit}, std::array<lit_t, 0>{}, false);
            return;
        }
        if (bounds.empty()) {
            rule(std::array{negate(lit)}, LitSpan{}, false);
            return;
        }
        // add edges based on the monotonicity of the aggregate
        if (type != CycleType::none) {
            for (auto const &[num, clit] : elem_vec) {
                if (clit > 0 &&
                    ((test(type, CycleType::positive) && num > 0) || (test(type, CycleType::negative) && num < 0))) {
                    graph_.add_edge(lit, clit);
                }
            }
        }

#define GRINGO_DEBUG_AGGREGATES
#ifdef GRINGO_DEBUG_AGGREGATES
        std::cerr << "handle aggregate: \n";
        std::cerr << "  fun: " << fun << "\n";
        std::cerr << "  range: " << (range.left.inclusive ? "[" : "(") << range.left.bound << "," << range.right.bound
                  << (range.right.inclusive ? "]" : ")") << "\n";
        std::cerr << "  bounds:";
        for (auto const &[left, right] : bounds) {
            std::cerr << (left.inclusive ? "[" : "(") << left.bound << "," << right.bound
                      << (right.inclusive ? "]" : ")");
        }
        std::cerr << "\n";
        // Note: workaround for buggy clang-tidy diagnostic
        std::cerr << "  monotonicity: " << [&t = type]() {
            if (t == CycleType::both) {
                return "nonmonotone";
            }
            if (t != CycleType::none) {
                return "convex";
            }
            return "antimonotone";
        }() << "\n";
#endif
        sum_aggrs_.emplace_back(lit, std::move(elem_vec), std::move(range), std::move(bounds));
    } else {
        throw std::logic_error("implement me: add support for remaining aggregates");
    }
}

void Backend::rule(LitSpan head, LitSpan body, bool choice) {
    for (auto const &hlit : head) {
        if (hlit > 0) {
            for (auto const &blit : body) {
                if (blit > 0) {
                    graph_.add_edge(hlit, blit);
                }
            }
        }
    }
    do_rule(head, body, choice);
}

auto Backend::clause_(LitSpan lits, ClauseType type) -> lit_t {
    aux1_.assign(lits.begin(), lits.end());
    std::ranges::sort(aux1_);
    aux1_.erase(std::ranges::unique(aux1_).begin(), aux1_.end());
    if (aux1_.size() == 1) {
        return aux1_.front();
    }
    auto [it, ins] = (type == ClauseType::conjunctive ? conds_ : disjunctions_).emplace(std::move(aux1_), 0);
    if (ins) {
        it.value() = next_lit();
        for (auto const &lit : it->first) {
            if (lit > 0) {
                graph_.add_edge(it.value(), lit);
            }
        }
    }
    return it.value();
}

auto Backend::cond(LitSpan lits) -> lit_t { return clause_(lits, ClauseType::conjunctive); }

void Backend::tr_disjunctions() {
    for (auto const &[clause, lit] : disjunctions_) {
        assert(lit > 0);
        auto const &lit_info = info_(lit);
        if (lit_info.type != EQType::none) {
            for (auto const &clit : clause) {
                if (clit > 0) {
                    mark_(clit, lit_info.type);
                }
                do_rule(std::array{lit}, std::array{clit}, false);
            }
        }
        if (lit_info.type == EQType::equivalence) {
            // TODO: non-cyclic literals better be shifted
            do_rule(clause, std::array{lit}, false);
        }
    }
}

void Backend::tr_conjunctions_() {
    for (auto const &[cond, lit] : conds_) {
        assert(lit > 0);
        auto const &lit_info = info_(lit);
        if (lit_info.type != EQType::none) {
            do_rule(std::array{lit}, cond, false);
        }
        if (lit_info.type == EQType::equivalence && lit_info.scc > 0) {
            for (auto const &clit : cond) {
                if (clit > 0 && info_(clit).scc == lit_info.scc) {
                    do_rule(std::array{clit}, std::array{lit}, false);
                }
            }
        }
    }
}

void Backend::cond_lit(lit_t lit, CondLitSpan elems) {
    for (auto const &elem : elems) {
        if (auto const &[id_conc, id_prem] = elem; id_conc && lit > 0 && *id_conc > 0) {
            graph_.add_edge(lit, *id_conc);
        }
    }
    cond_lits_.emplace_back(std::piecewise_construct, std::forward_as_tuple(lit),
                            std::forward_as_tuple(elems.begin(), elems.end()));
}

void Backend::mark_(lit_t lit, EQType type) {
    // maybe mark literal to have type
    if (lit > 0) {
        info_(lit).type = std::max(info_(lit).type, type);
    }
}

void Backend::tr_cond_lits_() {
    for (auto const &[lit, elems] : cond_lits_) {
        aux2_.clear();
        aux2_.reserve(elems.size());
        // Below, we us the following variable names:
        // - K: new uid replacing G : F in the body
        // - G: captures the conclusion
        // - F: catures the premise
        for (auto const &[g, f] : elems) {
            mark_(f, EQType::implication);
            if (g) {
                auto rec = lit > 0 && f > 0 && info_(lit).scc > 0 && info_(lit).scc == info_(f).scc;
                auto k = next_lit();
                // formula G : F is replaced by K
                // K :- G.
                // K :- not F.
                mark_(*g, EQType::implication);
                do_rule(std::array{k}, std::array{*g}, false);
                do_rule(std::array{k}, std::array{negate(f)}, false);
                if (rec) {
                    // K | F :- not not G.
                    mark_(f, EQType::equivalence);
                    do_rule(std::array{k, f}, std::array{negate(negate(*g))}, false);
                }
                aux2_.emplace_back(k);
            } else {
                aux2_.emplace_back(negate(f));
            }
        }
        do_rule(std::array{lit}, aux2_, false);
    }
}

void Backend::tr_aggr_() {
    for (auto &[lit, elems, range, bounds] : sum_aggrs_) {
        // check which kind of literals are cyclic
        auto is_recursive = [lit, this](lit_t clit) {
            return lit > 0 && clit > 0 && info_(clit).scc == info_(lit).scc;
        };
        auto has_pos_cycle = false; // cycle through atom with *positive* weight
        auto has_neg_cycle = false; // cycle through atom with *negative* weight
        if (lit > 0 && info_(lit).scc > 0) {
            for (auto const &[num, clit] : elems) {
                if (is_recursive(clit)) {
                    if (bounds.size() == 1) {
                        has_pos_cycle = has_pos_cycle || num > 0;
                        has_neg_cycle = has_neg_cycle || num < 0;
                    } else {
                        has_pos_cycle = has_neg_cycle = true;
                    }
                }
            }
        }
        auto flip = [](BdSumAggrElemVec const &elems) {
            auto res = BdSumAggrElemVec{};
            res.reserve(elems.size());
            for (auto const &[num, cond] : elems) {
                res.emplace_back(-num, cond);
            }
            return res;
        };

        auto to_int = [](Number const &num) {
            if (auto val = num.as_int()) {
                return *val;
            }
            throw std::range_error("number out of range");
        };

        // translate aggregate in lower bound form
        auto nlits = std::vector<lit_t>(elems.size(), 0);
        auto translate = [&](lit_t lit, BdSumAggrElemVec const &elems, Number bound) {
            auto wlits = std::vector<std::pair<lit_t, weight_t>>{};
            wlits.reserve(elems.size());
            auto it = nlits.begin();
            for (auto const &[weight, clit] : elems) {
                auto &nlit = *it++;
                if (weight > 0) {
                    wlits.emplace_back(clit, to_int(weight));
                } else {
                    if (!is_recursive(clit)) {
                        wlits.emplace_back(negate(clit), to_int(-weight));
                    } else {
                        if (nlit == 0) {
                            nlit = next_lit();
                            rule(std::array{nlit}, std::array{negate(clit)}, false);
                        }
                        wlits.emplace_back(nlit, to_int(-weight));
                    }
                    bound -= weight;
                }
            }
            do_bd_aggr(lit, wlits, bound.as_int().value());
        };

        // translate all bounds of an aggregate
        for (auto const &bound : bounds) {
            bool has_lower = range.left.bound < bound;
            bool has_upper = bound < range.right.bound;
            auto lit_lower = has_lower && has_upper ? next_lit() : lit;
            auto lit_upper = has_lower && has_upper ? next_lit() : lit;
            if (has_lower) {
                if (has_neg_cycle && !has_pos_cycle) {
                    if (lit == lit_lower) {
                        lit_lower = next_lit();
                    }
                    translate(lit_lower, flip(elems), -bound.left.bound + 1);
                    lit_lower = -lit_lower;
                } else {
                    translate(lit_lower, elems, bound.left.bound);
                }
            }
            if (has_upper) {
                if (!has_neg_cycle) {
                    if (lit == lit_upper) {
                        lit_upper = next_lit();
                    }
                    translate(lit_upper, elems, bound.right.bound + 1);
                    lit_upper = -lit_upper;
                } else {
                    translate(lit_upper, flip(elems), -bound.right.bound);
                }
            }
            if (lit != lit_lower && lit != lit_upper) {
                rule(std::array{lit}, std::array{lit_lower, lit_upper}, false);
            } else if (lit != lit_lower) {
                rule(std::array{lit}, std::array{lit_lower}, false);
            } else if (lit != lit_upper) {
                rule(std::array{lit}, std::array{lit_upper}, false);
            }
        }

        // add disjunctions for recursive literals
        auto it = elems.begin();
        for (auto const &nlit : nlits) {
            auto clit = it++->second;
            mark_(clit, nlit > 0 ? EQType::equivalence : EQType::implication);
            if (nlit > 0) {
                // nlit :- lit.                % saturate
                rule(std::array{nlit}, std::array{lit}, false);
                // nlit | clit :- not not lit. % guess
                rule(std::array{nlit, clit, negate(lit)}, {}, false);
            }
        }
    }
}

void Backend::end_step() {
    size_t idx_scc = 0;
    graph_.tarjan([&, this](std::vector<size_t> const &scc) {
        assert(!scc.empty());
        if (scc.size() > 1 || graph_.has_loop(scc.front())) {
            ++idx_scc;
            std::ranges::for_each(scc, [&](auto const &lit) { info_(uid_to_lit(lit)).scc = idx_scc; });
        }
    });
    tr_cond_lits_();
    tr_aggr_();
    tr_disjunctions();
    tr_conjunctions_();
    graph_.clear();
}

auto make_backend_output(Backend &backend) -> UOutputStm { return std::make_unique<OutputBackend>(backend); }

} // namespace Clingo::Output
