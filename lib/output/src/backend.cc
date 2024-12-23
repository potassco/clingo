#include <clingo/output/backend.hh>

#include <clingo/util/checked_math.hh>
#include <clingo/util/ordered_map.hh>
#include <clingo/util/ordered_set.hh>
#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>

#ifdef GRINGO_DEBUG_AGGREGATES
#include <iostream>
#endif

namespace Clingo::Output {

namespace {

//! Convert a uid to an integer literal.
//!
//! @pre uid != 0
//!
//! @param uid the uid to convert
//! @return the resulting literal
auto uid_to_lit(size_t uid) -> lit_t {
    assert(uid != 0);
    return static_cast<int32_t>(uid);
}

//! Convert a number into an integer.
//!
//! Throws a range error if the number cannot be converted into a 32 bit
//! integer.
//!
//! @param num the number to convert
//! @return the resulting integer
auto num_to_int(Number const &num) -> int32_t {
    if (auto val = num.as_int()) {
        return *val;
    }
    throw std::range_error("number out of range");
};

class FwdSym {
  public:
    FwdSym(Symbol sym) : sym_{sym} {}
    // NOLINTNEXTLINE
    friend auto operator==(FwdSym const &a, FwdSym const &b) -> bool = default;
    friend auto operator<=>(FwdSym const &a, FwdSym const &b) = default;
    static auto neutral() { return FwdSym{SymbolStore::sup()}; }

  private:
    Symbol sym_;
};

class BwdSym {
  public:
    BwdSym(Symbol sym) : sym_{sym} {}
    // NOLINTNEXTLINE
    friend auto operator==(BwdSym const &a, BwdSym const &b) -> bool = default;
    friend auto operator<=>(BwdSym const &a, BwdSym const &b) { return b.sym_ <=> a.sym_; }
    static auto neutral() { return BwdSym{SymbolStore::inf()}; }

  private:
    Symbol sym_;
};

//! Abstract class connecting grounder and solver.
//!
//! The backend is repsonsible for passig grounded statements to the solver (or
//! other forms of backends).
class Translator {
  public:
    using CondLit = OutputStm::CondLit;
    using CondLitSpan = OutputStm::CondLitSpan;
    using DijsElem = OutputStm::DisjElem;
    using DisjElemSpan = OutputStm::DisjElemSpan;
    using Guard = OutputStm::Guard;
    using GuardSpan = OutputStm::GuardSpan;
    using BdElem = OutputStm::BdElem;
    using BdElemSpan = OutputStm::BdElemSpan;
    using HdElem = OutputStm::HdElem;
    using HdElemSpan = OutputStm::HdElemSpan;

    Translator(Backend &backend) : backend_{&backend} {};

    //! Return a fresh literal.
    //!
    //! @return the fresh literal
    auto next_lit() -> Output::lit_t { return backend_->next_lit(); }

    //! Negate the given literal.
    //!
    //! Introduces a Tseitin literal if the given literal is negative. Flag rec
    //! can be set to false if the literal occurrence does not occur in a
    //! positive cycle.
    //!
    //! @param lit the literal to negate
    //! @return the negated literal
    auto negate(lit_t lit) -> lit_t {
        assert(lit != 0 && lit >= lit_min);
        if (lit > 0) {
            return -lit;
        }
        auto &neg = info_(-lit).neg;
        if (neg == 0) {
            neg = next_lit();
            backend_->rule(std::array{neg}, std::array{lit}, false);
        }
        return -neg;
    }

    //! Get a literal equivalent to the conjunction of the given literals.
    //!
    //! @param lits the literals
    auto cond(LitSpan lits) -> lit_t { return clause_(lits, ClauseType::conjunctive); }

    //! Define a conjunction of conditional literal.
    //!
    //! The given literal is derived by the given conditional literals.
    //!
    //! @pre lit > 0
    //!
    //! @param lit the literal that is derived
    //! @param elems the elements forming the conditional literal
    void cond_lit(lit_t lit, CondLitSpan elems) {
        for (auto const &elem : elems) {
            if (auto const &[id_conc, id_prem] = elem; id_conc && lit > 0 && *id_conc > 0) {
                graph_.add_edge(lit, *id_conc);
            }
        }
        cond_lits_.emplace_back(std::piecewise_construct, std::forward_as_tuple(lit),
                                std::forward_as_tuple(elems.begin(), elems.end()));
    }

    //! Define a disjunction of (conditional) literals.
    //!
    //! The given literal derives the given literals.
    //!
    //! We use a shifting of conditions into rule bodies as semantic guideline
    //! for the translation applied in this function.
    //!
    //! @param lit the literal representing the body
    //! @param elems the literals to derive
    void disjunction(lit_t lit, DisjElemSpan elems) {
        // example:
        //   a : c | b :- B.
        // shift:
        //       b :- not c, B.
        //   a | b :-     c, B.
        // factor:
        //   (a & ~~c) | b :- (c | not c), B.
        // replace:
        //   x | b :- y, B.
        //   (a & ~~c) :- x.
        //   x :- (a & ~~c).
        //   y :- (c | not c).
        // transform:
        //   x | b :- y, B.
        //   a :- x.
        //     :- x, not     c.
        //   x :- a, not not c. (*)
        //   y :-     c.
        //   y :- not c.
        // simplify (*) if a does not occur in a head cycle:
        //   :- a, c, not x.
        if (elems.empty()) {
            backend_->rule({}, std::array{lit}, false);
            return;
        }
        for (auto const &[sym, auid, conds] : elems) {
            if (auid == 0 && conds.empty()) {
                return;
            }
        }
        auto hd = LitVec{};
        auto bd = LitVec{};
        auto celems = std::vector<std::tuple<lit_t, lit_t, lit_t>>{};
        bd.emplace_back(lit);
        for (auto const &[sym, auid, conds] : elems) {
            if (auid != 0) {
                auto a = uid_to_lit(auid);
                assert(a > 0);
                if (conds.empty()) {
                    hd.emplace_back(a);
                } else {
                    auto x = next_lit();
                    auto y = next_lit();
                    aux_cond_.assign(conds.begin(), conds.end());
                    auto c = clause_(aux_cond_, ClauseType::disjunctive);
                    mark_(c, EQType::implication);
                    hd.emplace_back(x);
                    bd.emplace_back(y);
                    // a :- x.
                    rule(std::array{a}, std::array{x}, false);
                    //   :- x, not c.
                    rule({}, std::array{x, negate(c)}, false);
                    // y :- c.
                    rule(std::array{y}, std::array{c}, false);
                    // y :- not c.
                    rule(std::array{y}, std::array{negate(c)}, false);
                    // x :- a, not not c.
                    celems.emplace_back(a, x, c);
                }
            } else {
                aux_cond_.assign(conds.begin(), conds.end());
                bd.emplace_back(negate(clause_(aux_cond_, ClauseType::disjunctive)));
                mark_(bd.back(), EQType::implication);
            }
        }
        rule(hd, bd, false);
        if (!celems.empty()) {
            hd.clear();
            for (auto const &elem : elems) {
                if (auto auid = get<1>(elem); auid != 0) {
                    hd.emplace_back(uid_to_lit(auid));
                }
            }
            disjs_.emplace_back(std::move(hd), std::move(celems));
        }
    }

    // Translate rule (*) from disjunction() based on head cycles.
    void tr_disjs_() {
        auto hd_counts = Util::unordered_map<size_t, size_t>{};
        auto in_head_cycle = [&hd_counts, this](lit_t lit) {
            auto scc = info_(lit).scc;
            return scc > 0 && hd_counts.find(scc).value() > 0;
        };
        auto init_head_cycle = [&hd_counts, this](LitVec const &hd) {
            hd_counts.clear();
            for (auto lit : hd) {
                if (auto scc = info_(lit).scc; scc > 0) {
                    ++hd_counts[scc];
                }
            }
        };
        for (auto &[hd, elems] : disjs_) {
            init_head_cycle(hd);
            for (auto const &[a, x, c] : elems) {
                backend_->rule({}, std::array{x, negate(c)}, false);
                if (in_head_cycle(a)) {
                    backend_->rule(std::array{x}, std::array{a, negate(negate(c))}, false);
                } else {
                    backend_->rule({}, std::array{a, c, negate(x)}, false);
                }
            }
        }
    }

    //! Define a sum aggregate.
    //!
    //! The condition ids of the aggregate elements can be obtained using
    //! function `cond()`.
    //!
    //! @param lit the literal that is derived
    //! @param fun the aggregate function
    //! @param elems the elements of the aggregate
    //! @param guard the aggregate guards
    void bd_aggr(lit_t lit, AggregateFunction fun, BdElemSpan elems, GuardSpan guards) {
        assert(lit > 0 && fun != AggregateFunction::count);
        switch (fun) {
            case AggregateFunction::sum:
            case AggregateFunction::sump: {
                // Note: assumse that negative weights of sum+ have already been removed
                delay_sum_(lit, elems, guards);
                break;
            }
            case AggregateFunction::min: {
                delay_mm_<FwdSym>(lit, elems, guards);
                break;
            }
            case AggregateFunction::max: {
                delay_mm_<BwdSym>(lit, elems, guards);
                break;
            }
            case Clingo::AggregateFunction::count: {
                assert(false);
            }
        }
    }

    //! Define a head aggregate.
    //!
    //! The given literal represents the body associated with the aggregate.
    //!
    //! This functions reuses the translation of body aggregates.
    //!
    //! @param blit the body literal
    //! @param fun the aggregate's function
    //! @param elems the elements of the aggregate
    //! @param guards the guards of the aggregate
    void hd_aggr(lit_t blit, AggregateFunction fun, HdElemSpan elems, GuardSpan guards) {
        // The head aggregate
        //
        //     #sum { tuple: lit: cond } >= 1 :- body.
        //
        // is rewritten into
        //
        //     { lit } :- body, cond.
        //     :- not #sum { tuple: lit, cond } >= 1, body.
        //
        // With uid there is already an abbreviation for the body.
        auto bd_conds = std::vector<IndexVec>{};
        auto bd_elems = std::vector<BdElem>{};
        for (auto const &[tuple, conds] : elems) {
            if (conds.empty()) {
                bd_elems.emplace_back(tuple, IndexSpan{});
            } else {
                bd_conds.emplace_back();
                for (auto const &[sym, huid, cuid] : conds) {
                    auto clit = uid_to_lit(cuid);
                    if (huid > 0) {
                        auto hlit = uid_to_lit(huid);
                        bd_conds.back().emplace_back(clause_(std::array{hlit, clit}, ClauseType::conjunctive));
                        mark_(clit, EQType::implication);
                        rule(std::array{hlit}, std::array{blit, clit}, true);
                    } else {
                        bd_conds.back().emplace_back(clit);
                    }
                }
                // Note: that we can create a span here even though the vector
                // might be moved due to reallocation.
                bd_elems.emplace_back(tuple, bd_conds.back());
            }
        }
        auto lit = next_lit();
        bd_aggr(lit, fun, bd_elems, guards);
        rule({}, std::array{-lit, blit}, false);
    }

    //! Add a disjunctive or choice rule.
    //!
    //! Note that negative literals in the head are supported. They are shifted
    //! before passing them to the backend.
    //!
    //! @param head the literals forming the head
    //! @param body the literals forming the body
    //! @param choice whether the rule is a choice or disjunctive rule
    void rule(LitSpan head, LitSpan body, bool choice) {
        aux_hd_.clear();
        aux_bd_.clear();
        for (auto const &hlit : head) {
            if (hlit > 0) {
                aux_hd_.emplace_back(hlit);
                for (auto const &blit : body) {
                    if (blit > 0) {
                        graph_.add_edge(hlit, blit);
                    }
                }
            } else if (!choice) {
                aux_bd_.emplace_back(negate(hlit));
            }
        }
        aux_bd_.insert(aux_bd_.end(), body.begin(), body.end());
        backend_->rule(aux_hd_, aux_bd_, choice);
    }

    //! Add an edge directive.
    //!
    //! A map from symbols to unsigned integers is stored in the translator.
    //!
    //! @param src name of the source vertex
    //! @param dst name of the target vertex
    //! @param body the body
    void edge(Symbol src, Symbol dst, LitSpan body) { backend_->edge(vertex_(src), vertex_(dst), body); }

    //! Add a heuristic directive.
    //!
    //! @pre atom > 0
    //!
    //! @param atom the atom index
    //! @param weight the weight
    //! @param prio the optional priority
    //! @param type the heuristic type
    //! @param body the body of the directive
    void heuristic(lit_t atom, Number const &weight, Number const *prio, HeuristicType type, LitSpan body) {
        backend_->heuristic(atom, num_to_int(weight), prio != nullptr ? num_to_int(*prio) : 0, type, body);
    }

    //! Add an external directive.
    //!
    //! @pre atom > 0
    //!
    //! @param atom the atom to declare external
    //! @param type the type of the external
    void external(lit_t atom, ExternalType type) {
        assert(atom > 0);
        backend_->external(atom, type);
    }

    //! Finish a grounding step.
    //!
    //! Some language constructs require additional translation.
    //! Such translations are applied here.
    void end_step() {
        size_t idx_scc = 0;
        graph_.tarjan([&, this](std::vector<size_t> const &scc) {
            assert(!scc.empty());
            if (scc.size() > 1 || graph_.has_loop(scc.front())) {
                ++idx_scc;
                std::ranges::for_each(scc, [&](auto const &lit) { info_(uid_to_lit(lit)).scc = idx_scc; });
            }
        });
        tr_disjs_();
        tr_cond_lits_();
        tr_sum_();
        tr_mms_();
        tr_clauses();
        graph_.clear();
    }

    //! She the given symbol if the given conditon is true.
    void show(Symbol sym, LitSpan body) { backend_->show(sym, body); }

  private:
    //! Available condition types.
    //!
    //! Conditions are clauses associated with a literal. The equivalence between
    //! the literal and the clause is either established with an implication (a
    //! rule) or an equivalence.
    enum class EQType : uint8_t {
        none,        //!< the condition is not used
        implication, //!< only forward direction is necessary
        equivalence, //!< forward and backward directions are necessary
    };

    enum class ClauseType : uint8_t {
        disjunctive,
        conjunctive,
    };

    //! Which weights have to be considered for cycle computation.
    enum class CycleType : uint8_t { none, positive, negative, both };

    // NOLINTNEXTLINE
    friend void is_bit_set_enum(CycleType type);

    static constexpr auto invalid_id = std::numeric_limits<size_t>::max();

    struct LitInfo {
        size_t scc = 0;
        size_t clause = 0;
        lit_t neg = 0;
        EQType type = EQType::none;
    };

    using NumberSet = Util::interval_set<Number>;
    //! A sum aggregate element.
    using SumElem = std::pair<Number, lit_t>;
    //! A vector of sum aggregate elements.
    using SumElemVec = std::vector<SumElem>;
    //! A vector of sum aggregates.
    using SumVec = std::vector<std::tuple<lit_t, SumElemVec, NumberSet::interval, NumberSet>>;
    //! A vector of sum aggregates.
    using MinVec = std::vector<std::tuple<lit_t, bool, LitVec, IndexVec>>;

    using LitInfoVec = std::vector<LitInfo>;
    using ClauseLitMap = Util::ordered_map<std::pair<Output::LitVec, ClauseType>, lit_t>;

    using CondLitElem = std::pair<std::optional<lit_t>, lit_t>;
    using CondLitElemVec = std::vector<CondLitElem>;
    using CondLitVec = std::vector<std::pair<lit_t, CondLitElemVec>>;
    using DisjVec = std::vector<std::tuple<LitVec, std::vector<std::tuple<lit_t, lit_t, lit_t>>>>;

    using VertexMap = Util::unordered_map<SharedSymbol, Output::id_t>;

    //! Get the literal info for an atom.
    [[nodiscard]] auto info_(lit_t lit) -> LitInfo & {
        assert(lit > 0);
        while (static_cast<lit_t>(lits_.size()) < lit) {
            lits_.emplace_back();
        }
        return lits_[lit - 1];
    }

    //! Mark a literal with the given equivalence type.
    //!
    //! If the literal is associated with a clause, the equivalence between
    //! literal and clause is established accordingly.
    auto mark_(lit_t lit, EQType type) -> bool {
        auto atm = std::abs(lit);
        if (lit < 0) {
            type = std::max(type, EQType::implication);
        }
        if (auto &info = info_(atm); info.clause != invalid_id && info.type < type) {
            info.type = type;
            return true;
        }
        return false;
    }

    //! Map the given symbol to a unique id.
    //!
    //! @param name the name
    auto vertex_(Symbol name) -> id_t {
        auto [it, ins] = vertices_.emplace(name, vertices_.size());
        if (ins && vertices_.size() > 1 && it.value() == 0) {
            throw std::range_error("maximum number of vertices exceeded");
        }
        return it.value();
    }

    //! Translate stored clauses.
    //!
    //! Since clauses can have other clauses as elements, those are marked with
    //! the same equivalence type as the parent. For negative literals, some
    //! rules are omitted.
    //!
    //! This function should be the last translation function called so that
    //! prior ones can add additional clauses if necessary.
    void tr_clauses() {
        auto todo = LitVec{};
        auto mark = [&todo, this](lit_t lit) {
            assert(lit > 0);
            if (auto &info = info_(lit); info.type != EQType::none) {
                for (auto const &clit : clauses_.nth(info.clause).key().first) {
                    if (mark_(clit, info.type)) {
                        todo.emplace_back(std::abs(clit));
                    }
                }
            }
        };
        for (auto const &[clause, lit] : clauses_) {
            mark(lit);
        }
        while (!todo.empty()) {
            auto lit = todo.back();
            todo.pop_back();
            mark(lit);
        }
        for (auto const &[clause, lit] : clauses_) {
            assert(lit > 0);
            auto const &info = info_(lit);
            auto const &[lits, type] = clause;
            if (type == ClauseType::conjunctive) {
                if (info.type != EQType::none) {
                    backend_->rule(std::array{lit}, lits, false);
                }
                if (info.type == EQType::equivalence && info.scc > 0) {
                    for (auto const &clit : lits) {
                        if (clit > 0 && info_(clit).scc == info.scc) {
                            backend_->rule(std::array{clit}, std::array{lit}, false);
                        }
                    }
                }
            } else {
                if (info.type != EQType::none) {
                    for (auto const &clit : lits) {
                        backend_->rule(std::array{lit}, std::array{clit}, false);
                    }
                }
                if (info.type == EQType::equivalence) {
                    aux_hd_.clear();
                    aux_hd_.emplace_back(lit);
                    aux_bd_.clear();
                    for (auto const &clit : lits) {
                        if (clit < 0) {
                            aux_bd_.emplace_back(negate(clit));
                        } else if (clit > 0 && info_(lit).scc != info_(clit).scc) {
                            aux_bd_.emplace_back(-clit);
                        } else {
                            aux_hd_.emplace_back(clit);
                        }
                    }
                    backend_->rule(aux_hd_, aux_bd_, false);
                }
            }
        }
    }

    //! Translate stored conditional literals.
    void tr_cond_lits_() {
        for (auto const &[lit, elems] : cond_lits_) {
            assert(lit > 0);
            aux_bd_.clear();
            aux_bd_.reserve(elems.size());
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
                    backend_->rule(std::array{k}, std::array{*g}, false);
                    backend_->rule(std::array{k}, std::array{negate(f)}, false);
                    if (rec) {
                        // K | F :- not not G.
                        mark_(f, EQType::equivalence);
                        backend_->rule(std::array{k, f}, std::array{negate(negate(*g))}, false);
                    }
                    aux_bd_.emplace_back(k);
                } else {
                    aux_bd_.emplace_back(negate(f));
                }
            }
            backend_->rule(std::array{lit}, aux_bd_, false);
        }
    }

    //! Simplify and analyze min and max aggregates.
    //!
    //! Returns relevant elements, the range, and bounds of the aggregate.
    template <class Sym>
    auto analyze_mm_(BdElemSpan elems, GuardSpan guards)
        -> std::tuple<Sym, Sym, Util::interval_set<Sym>, std::vector<std::pair<Sym, lit_t>>> {
        // simplify the elements
        auto upper = Sym::neutral();
        auto elem_vec = std::vector<std::pair<Sym, lit_t>>{};
        auto cond_map = Util::unordered_map<IndexSpan, size_t>{};
        elem_vec.reserve(elems.size());
        cond_map.reserve(elems.size());
        for (auto const &[tup, conds] : elems) {
            if (!tup.empty() && tup.front() < upper && conds.empty()) {
                // adjust upper bound
                upper = tup.front();
            }
        }
        for (auto const &[tup, conds] : elems) {
            if (!tup.empty() && tup.front() < upper) {
                assert(!conds.empty());
                if (auto [it, ins] = cond_map.try_emplace(conds, elem_vec.size()); !ins) {
                    // drop tuples with larger weights
                    get<0>(elem_vec[it.value()]) = std::min<Sym>(get<0>(elem_vec[it.value()]), tup.front());
                } else {
                    // add weight literal pairs
                    aux_cond_.assign(conds.begin(), conds.end());
                    elem_vec.emplace_back(tup.front(),
                                          clause_({aux_cond_.begin(), aux_cond_.end()}, ClauseType::disjunctive));
                }
            }
        }
        std::ranges::sort(elem_vec);
        auto lower = elem_vec.empty() ? upper : get<0>(elem_vec.front());
        // compute the bounds of the aggregate
        auto bounds = Util::interval_set<Sym>{};
        auto range = typename Util::interval_set<Sym>::interval{{lower, true}, {upper, true}};
        bounds.add(range);
        for (auto const &[rel, guard] : guards) {
            switch (rel) {
                case Relation::greater_equal: {
                    bounds.remove({{lower, true}, {guard, false}});
                    break;
                }
                case Relation::greater: {
                    bounds.remove({{lower, true}, {guard, true}});
                    break;
                }
                case Relation::less_equal: {
                    bounds.remove({{guard, false}, {upper, true}});
                    break;
                }
                case Relation::less: {
                    bounds.remove({{guard, true}, {upper, true}});
                    break;
                }
                case Relation::equal: {
                    bounds.remove({{lower, true}, {guard, false}});
                    bounds.remove({{guard, false}, {upper, true}});
                    break;
                }
                case Relation::not_equal: {
                    bounds.remove({{guard, true}, {guard, true}});
                    break;
                }
            }
        }
        return {lower, upper, bounds, std::move(elem_vec)};
    }

    //! Delays or translates min and max aggregates based on monotonicity.
    //!
    //! Converts aggregates into sequences of form
    //!
    //!     F(TF)*T?
    //!     T(FT)*F?
    //!
    //! where all but the last T and F in the sequence are non-empty.
    //!
    //! The sequences T and F correspond to true and false, respectively.
    //!
    //! The sequences TF, FT, FTF correspond to monotone, antimonotone, and
    //! convex aggregates, respectively.
    //!
    //! Edges have to be added for all literals in T.
    //!
    //! Nonmonotone aggregates are delayed for later translation. Otherwise,
    //! aggregates are translated right away.
    template <class Sym> void delay_mm_(lit_t lit, BdElemSpan elems, GuardSpan guards) {
        assert(lit > 0);
        auto [lower, upper, bounds, elem_vec] = analyze_mm_<Sym>(elems, guards);
        auto lits = LitVec{};
        auto ids = IndexVec{};
        lits.reserve(elem_vec.size());
        ids.emplace_back(0);
        auto start = bounds.contains(lower);
        auto state = start;
        for (auto const &[weight, lit] : elem_vec) {
            auto prev = state;
            state = bounds.contains(weight);
            if (prev != state) {
                ids.emplace_back(lits.size());
            }
            lits.emplace_back(lit);
        }
        if (state != bounds.contains(upper)) {
            ids.emplace_back(lits.size());
        }
        lits.resize(ids.back());
        for (auto const &clit : lits) {
            mark_(clit, EQType::implication);
        }
        auto get_span = [&lits](auto it) {
            return std::span{std::next(lits.begin(), *it), std::next(lits.begin(), *(it + 1))};
        };
        if (ids.size() <= 2 || (ids.size() == 3 && !start)) {
            // translate constant, monotone, antimontone, and convex cases
            tr_mm_(lit, start, lits, ids, false);
        } else {
            // delay nonmonotone case
            bool state = start;
            for (auto it = ids.begin(), ie = ids.end(); it + 1 != ie; ++it) {
                if (state) {
                    for (auto const &clit : get_span(it)) {
                        if (clit > 0) {
                            graph_.add_edge(lit, clit);
                        }
                    }
                }
                state = !state;
            }
            min_aggrs_.emplace_back(lit, start, std::move(lits), std::move(ids));
        }
    }

    //! Translates min and max aggregates.
    //!
    //! Min aggregates are translated as indicated in the example below:
    //!
    //!     lit :- 2 != #min { 1:a; 2:b; 3:c; 4:d } !=4
    //!     [         ]
    //!     [ ] [ ] [ ]
    //!      1 2 3 4 e
    //!     lit :- a.
    //!     lit :- c, nb.
    //!     lit :- nb, nd. % (*)
    //!     nb :- not b.
    //!     nb :- lit.
    //!     nb | b :- not not lit.
    //!     nd :- not d.
    //!     nd :- lit.
    //!     nd | d :- not not lit.
    //!
    //! Rules of form (*) are shortened by introducing auxiliary literals.
    void tr_mm_(lit_t lit, bool start, LitVec const &lits, IndexVec const &ids, bool delayed) {
        assert(!ids.empty() && lits.size() == ids.back() && lit > 0);
        auto get_span = [&lits = lits](auto it) {
            return std::span{std::next(lits.begin(), *it), std::next(lits.begin(), *(it + 1))};
        };
        auto scc = info_(lit).scc;
        aux_cond_.clear();
        auto state = start;
        for (auto it = ids.begin(), ie = ids.end(); it + 1 != ie; ++it) {
            for (auto const &clit : get_span(it)) {
                if (state) {
                    // lit :- clit, aux_cond_.
                    if (aux_cond_.size() > 1) {
                        auto nlit = clause_(aux_cond_, ClauseType::conjunctive, false);
                        mark_(nlit, EQType::implication);
                        aux_cond_.clear();
                        aux_cond_.emplace_back(nlit);
                    }
                    aux_cond_.emplace_back(clit);
                    if (!delayed && clit > 0) {
                        graph_.add_edge(lit, clit);
                    }
                    backend_->rule(std::array{lit}, aux_cond_, false);
                    aux_cond_.pop_back();
                } else {
                    if (delayed && clit > 0 && scc != 0 && scc == info_(clit).scc) {
                        // nlit :- not clit.
                        // nlit :- lit.
                        // nlit | clit :- not not l.
                        mark_(clit, EQType::equivalence);
                        auto nlit = next_lit();
                        backend_->rule(std::array{nlit}, std::array{negate(clit)}, false);
                        backend_->rule(std::array{nlit}, std::array{negate(lit)}, false);
                        backend_->rule(std::array{nlit, clit}, std::array{negate(negate(lit))}, false);
                        aux_cond_.emplace_back(nlit);
                    } else {
                        aux_cond_.emplace_back(negate(clit));
                    }
                }
            }
            state = !state;
        }
        if (state) {
            backend_->rule(std::array{lit}, aux_cond_, false);
        }
    }

    //! Translate delayed min and max aggregates.
    void tr_mms_() {
        for (auto const &[lit, start, lits, ids] : min_aggrs_) {
            tr_mm_(lit, start, lits, ids, true);
        }
    }

    //! Analyze a sum aggregate.
    //!
    //! Constructs a new aggregate element vector removing unnecessary elements,
    //! computes an interval for largest and smallest value of the aggregate, and
    //! represents the guards of the aggreagte as an interval set. This interval set
    //! is additionally intersected with the range.
    auto analyze_sum_(BdElemSpan elems,
                      GuardSpan guards) -> std::tuple<SumElemVec, NumberSet::interval, NumberSet, CycleType> {
        // simplify the aggregate
        auto fixed = Number(0);
        auto elem_vec = SumElemVec{};
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
                        aux_cond_.assign(conds.begin(), conds.end());
                        elem_vec.emplace_back(std::move(num),
                                              clause_({aux_cond_.begin(), aux_cond_.end()}, ClauseType::disjunctive));
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

    void delay_sum_(lit_t lit, BdElemSpan elems, GuardSpan guards) {
        auto [elem_vec, range, bounds, type] = analyze_sum_(elems, guards);
        if (bounds.contains(range)) {
            rule(std::array{lit}, {}, false);
            return;
        }
        if (bounds.empty()) {
            rule({}, std::array{lit}, false);
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

#ifdef GRINGO_DEBUG_AGGREGATES
        std::cerr << "handle aggregate: \n";
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
    }

    //! Translates stored aggregate literals.
    //!
    //! This function iterates over all stored aggregate literals and
    //! translates them into a form understood by the backend.
    void tr_sum_() {
        for (auto &[lit, elems, range, bounds] : sum_aggrs_) {
            assert(lit > 0);
            // check which kind of literals are cyclic
            auto is_recursive = [lit, this](lit_t clit) {
                return lit > 0 && clit > 0 && info_(clit).scc == info_(lit).scc;
            };
            auto has_pos_cycle = false; // cycle through atom with *positive* weight
            auto has_neg_cycle = false; // cycle through atom with *negative* weight
            if (info_(lit).scc > 0) {
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
            auto flip = [](SumElemVec const &elems) {
                auto res = SumElemVec{};
                res.reserve(elems.size());
                for (auto const &[num, cond] : elems) {
                    res.emplace_back(-num, cond);
                }
                return res;
            };

            // translate aggregate in lower bound form
            auto nlits = std::vector<lit_t>(elems.size(), 0);
            auto translate = [&](lit_t lit, SumElemVec const &elems, Number bound) {
                assert(lit > 0);
                auto wlits = std::vector<std::pair<lit_t, weight_t>>{};
                wlits.reserve(elems.size());
                auto it = nlits.begin();
                for (auto const &[weight, clit] : elems) {
                    auto &nlit = *it++;
                    if (weight > 0) {
                        wlits.emplace_back(clit, num_to_int(weight));
                    } else {
                        if (!is_recursive(clit)) {
                            wlits.emplace_back(negate(clit), num_to_int(-weight));
                        } else {
                            if (nlit == 0) {
                                nlit = next_lit();
                                backend_->rule(std::array{nlit}, std::array{negate(clit)}, false);
                            }
                            wlits.emplace_back(nlit, num_to_int(-weight));
                        }
                        bound -= weight;
                    }
                }
                backend_->bd_aggr(lit, wlits, bound.as_int().value());
            };

            // translate all bounds of an aggregate
            for (auto const &bound : bounds) {
                bool has_lower = range.left.bound < bound;
                bool has_upper = bound < range.right.bound;
                auto lit_lower = has_lower && has_upper ? next_lit() : lit;
                auto lit_upper = has_lower && has_upper ? next_lit() : lit;
                if (has_lower) {
                    // Note: that the conditions could be weakend dropping
                    // `has_neg_cycle`. I am not sure which variant would work
                    // better in practice.
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
                // symmetric case for upper bounds
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
                    backend_->rule(std::array{lit}, std::array{lit_lower, lit_upper}, false);
                } else if (lit != lit_lower) {
                    backend_->rule(std::array{lit}, std::array{lit_lower}, false);
                } else if (lit != lit_upper) {
                    backend_->rule(std::array{lit}, std::array{lit_upper}, false);
                }
            }

            // add disjunctions for recursive literals
            auto it = elems.begin();
            for (auto const &nlit : nlits) {
                auto clit = it++->second;
                mark_(clit, nlit > 0 ? EQType::equivalence : EQType::implication);
                if (nlit > 0) {
                    // nlit :- lit.                % saturate
                    backend_->rule(std::array{nlit}, std::array{lit}, false);
                    // nlit | clit :- not not lit. % guess
                    backend_->rule(std::array{nlit, clit}, std::array{negate(negate(lit))}, false);
                }
            }
        }
    }

    //! Get a literal equivalent to the given clause.
    auto clause_(LitSpan lits, ClauseType type, bool add_edges = true) -> lit_t {
        aux_bd_.assign(lits.begin(), lits.end());
        std::ranges::sort(aux_bd_);
        aux_bd_.erase(std::ranges::unique(aux_bd_).begin(), aux_bd_.end());
        if (aux_bd_.size() == 1) {
            return aux_bd_.front();
        }
        auto [it, ins] = clauses_.emplace(std::pair{aux_bd_, type}, 0);
        if (ins) {
            it.value() = next_lit();
            info_(it.value()).clause = std::distance(clauses_.begin(), it);
            for (auto const &lit : it->first.first) {
                if (add_edges && lit > 0) {
                    graph_.add_edge(it.value(), lit);
                }
            }
        }
        return it.value();
    }

    Backend *backend_;
    Output::LitVec aux_hd_;
    Output::LitVec aux_bd_;
    Output::LitVec aux_cond_;
    Util::Graph graph_;
    LitInfoVec lits_;
    ClauseLitMap clauses_;
    VertexMap vertices_;

    CondLitVec cond_lits_;
    DisjVec disjs_;
    SumVec sum_aggrs_;
    MinVec min_aggrs_;
};

//! Output handling conditions.
//!
//! Each literal is mapped to a program literal and appended to a vector of
//! literals that can be retrieved via function literals.
//!
//! Only supports simple literals excluding aggregates, theory atoms and conditions.
class OutputCond : public OutputLit {
  public:
    OutputCond(Translator &translator) : translator_{&translator} {}

    //! Get the literals of the body.
    //!
    //! Literals are added via the OutputLit interface.
    //!
    //! @return the literals
    [[nodiscard]] auto literals() const -> LitVec const & { return body_; }

    //! Get the translator of the output.
    //!
    //! @return the translator
    auto translator() -> Translator & { return *translator_; };

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
                body_.emplace_back(translator().negate(-uid_to_lit(uid)));
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

    Translator *translator_;
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
    OutputBody(Translator &translator) : OutputCond(translator) {}

  private:
    auto do_cond_lit(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = translator().next_lit();
        }
        append(*uid);
        return *uid;
    }

    auto do_bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = translator().next_lit();
        }
        append(sign, *uid);
        return *uid;
    }

    auto do_bd_theory(Sign sign, std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = translator().next_lit();
        }
        append(sign, *uid);
        return *uid;
    }
};

class OutputBackend : public OutputStm, OutputTheory {
  public:
    OutputBackend(Backend &backend) : translator_{backend} {};

  private:
    void do_fact([[maybe_unused]] Symbol sym, size_t uid) override {
        translator().rule(std::array{uid_to_lit(uid)}, LitSpan{}, false);
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
            translator().rule(std::array{lit}, body_.literals(), choice);
        } else {
            translator().rule({}, body_.literals(), choice);
        }
    }

    void do_show_term(Symbol term) override { translator().show(term, body_.literals()); }

    void do_external([[maybe_unused]] Symbol atom, size_t uid, ExternalType type) override {
        translator().external(uid_to_lit(uid), type);
    }

    void do_project(Symbol atom) override {
        static_cast<void>(atom);
        throw std::logic_error{"implement me: project"};
    }

    auto do_aggr_rule(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = translator_.next_lit();
        }
        translator().rule(std::array{uid_to_lit(*uid)}, body_.literals(), false);
        return *uid;
    }

    auto do_theory_rule(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = translator_.next_lit();
        }
        translator().rule(std::array{uid_to_lit(*uid)}, body_.literals(), false);
        return *uid;
    }

    auto do_disjunctive_rule(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = translator_.next_lit();
        }
        translator().rule(std::array{uid_to_lit(*uid)}, body_.literals(), false);
        return *uid;
    }

    void do_weak_constraint(Number const &weight, std::optional<Symbol> prio, SymbolSpan terms) override {
        static_cast<void>(weight);
        static_cast<void>(prio);
        static_cast<void>(terms);
        throw std::logic_error{"implement me: weak_constraint"};
    }

    void do_heuristic([[maybe_unused]] Symbol atom, size_t uid, Number const &weight, Number const *prio,
                      HeuristicType type) override {
        translator().heuristic(uid_to_lit(uid), weight, prio, type, body_.literals());
    }

    void do_edge(Symbol src, Symbol dst) override { translator().edge(src, dst, body_.literals()); }

    auto do_cond() -> OutputLit & override {
        cond_.start();
        return cond_;
    }

    auto do_cond_id() -> size_t override { return translator().cond(cond_.literals()); }

    auto do_uid() -> size_t override { return translator().next_lit(); }

    void do_cond_lit(size_t uid, CondLitSpan elems) override { translator_.cond_lit(uid_to_lit(uid), elems); }

    void do_bd_aggr(size_t uid, AggregateFunction fun, BdElemSpan elems, GuardSpan guards) override {
        translator().bd_aggr(uid_to_lit(uid), fun, elems, guards);
    }

    void do_hd_aggr(size_t uid, AggregateFunction fun, HdElemSpan elems, GuardSpan guards) override {
        translator().hd_aggr(uid_to_lit(uid), fun, elems, guards);
    }

    void do_disjunction(size_t uid, DisjElemSpan elems) override { translator().disjunction(uid_to_lit(uid), elems); }

    auto do_theory() -> OutputTheory & override { return *this; }

    void do_flush() override {}

    void do_end_step() override { translator_.end_step(); }

    void do_mark([[maybe_unused]] SymbolCollector &gc) override {}

    auto do_str(String val) -> size_t override {
        static_cast<void>(val);
        throw std::logic_error{"implement me: theory str"};
    }

    auto do_num(Number const &val) -> size_t override {
        static_cast<void>(val);
        throw std::logic_error{"implement me: theory num"};
    }

    auto do_fun(String name, IndexSpan args) -> size_t override {
        static_cast<void>(name);
        static_cast<void>(args);
        throw std::logic_error{"implement me: theory fun"};
    }

    auto do_tup(TheoryTermTupleType type, IndexSpan args) -> size_t override {
        static_cast<void>(type);
        static_cast<void>(args);
        throw std::logic_error{"implement me: theory tup"};
    }

    auto do_elem(IndexSpan tuple, size_t cond) -> size_t override {
        static_cast<void>(tuple);
        static_cast<void>(cond);
        throw std::logic_error{"implement me: theory elem"};
    }

    void do_atm(size_t atom_uid, Symbol name, IndexSpan elems, OptGuard guard) override {
        static_cast<void>(atom_uid);
        static_cast<void>(name);
        static_cast<void>(elems);
        static_cast<void>(guard);
        throw std::logic_error{"implement me: theory atom"};
    }

    auto translator() -> Translator & { return translator_; }

    LitVec lits_;
    Translator translator_;
    OutputBody body_{translator_};
    OutputCond cond_{translator_};
};

} // namespace

auto make_backend_output(Backend &backend) -> UOutputStm { return std::make_unique<OutputBackend>(backend); }

} // namespace Clingo::Output
