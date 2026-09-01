#include <clingo/output/backend.hh>

#include <clingo/util/checked_math.hh>
#include <clingo/util/optional.hh>
#include <clingo/util/ordered_map.hh>
#include <clingo/util/ordered_set.hh>
#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>
#include <utility>

// #define CLINGO_DEBUG_AGGREGATES
#ifdef CLINGO_DEBUG_AGGREGATES
#include <iostream>
#endif

namespace CppClingo::Output {

namespace {

//! Available clause equivalence types.
//!
//! Clauses are associated with literals. The equivalence between
//! the literal and the clause is either established with an implication (a
//! single rule) or an equivalence (several rules).
enum class EQType : uint8_t {
    none,        //!< the clause is not used
    implication, //!< only forward direction is necessary
    equivalence, //!< forward and backward directions are necessary
};

//! Available clause types.
enum class ClauseType : uint8_t {
    disjunctive, //!< disjunctive clauses
    conjunctive, //!< conjunctive clauses (also sometimes conditions)
};

//! Sentinel to mark invalid ids.
constexpr auto invalid_id = std::numeric_limits<size_t>::max();

//! Per literal information.
struct LitInfo {
    size_t scc = 0;             //!< component number
    size_t clause = invalid_id; //!< associated clause index
    prg_lit_t neg = 0;          //!< associated negative literal
    EQType type = EQType::none; //!< equivalence type of the associated clause
};

//! Convert a uid to an integer literal.
//!
//! @pre uid != 0
//!
//! @param uid the uid to convert
//! @return the resulting literal
auto uid_to_lit(size_t uid) -> prg_lit_t {
    assert(uid != 0);
    return static_cast<int32_t>(uid);
}

//! Convert a uid to an integer atom.
//!
//! @pre static_cast<int32_t>(uid) > 0
//!
//! @param uid the uid to convert
//! @return the resulting literal
auto uid_to_atom(size_t uid) -> prg_lit_t {
    assert(static_cast<int32_t>(uid) > 0);
    return static_cast<int32_t>(uid);
}

//! Convert a uid to an id.
//!
//! @param uid the uid to convert
//! @return the resulting id
auto uid_to_id(size_t uid) -> prg_id_t {
    return static_cast<prg_id_t>(uid);
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

//! Helper to compare symbols in forward order.
//!
//! Extends the symbol with a neutral value.
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

//! Helper to compare symbols in backward order.
//!
//! Extends the symbol with a neutral value.
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

//! Helper storing the backend and information relevant for translation.
class BuilderBase {
  public:
    //! Construct the builder.
    //!
    //! @param store the symbol storage
    //! @param backend the backend
    BuilderBase(SymbolStore &store, ProgramBackend &backend) : store_{&store}, backend_{&backend} {};

    //! Return a fresh literal.
    //!
    //! @return the fresh literal
    auto next_lit() -> prg_lit_t { return backend_->next_lit(); }

    //! Negate the given literal.
    //!
    //! Introduces a Tseitin literal if the given literal is negative. Flag rec
    //! can be set to false if the literal occurrence does not occur in a
    //! positive cycle.
    //!
    //! @param lit the literal to negate
    //! @return the negated literal
    auto negate(prg_lit_t lit) -> prg_lit_t {
        assert(lit != 0 && lit >= prg_lit_min);
        if (lit > 0) {
            return -lit;
        }
        auto &neg = info(-lit).neg;
        if (neg == 0) {
            neg = next_lit();
            backend_->rule(std::array{neg}, std::array{lit}, false);
        }
        return -neg;
    }

    //! Get a Tseitin literal for the conjunction of the given literals.
    //!
    //! @param lits the literals
    //! @return an equivalent literal
    auto cond(PrgLitSpan lits) -> prg_lit_t { return clause(lits, ClauseType::conjunctive); }

    //! Get the conjunction of literals equivalent to the given literal.
    //!
    //! @note If the literal itself represents the conjunction, a one
    //! elementary span pointing to the given literal is returned.
    //!
    //! @param lit the literal
    //! @return the associated literals
    auto cond(prg_lit_t const &lit) -> PrgLitSpan {
        if (lit > 0) {
            if (auto &li = info(lit); li.clause != invalid_id) {
                auto it = clauses_.nth(li.clause);
                if (auto const &[clause, type] = it.key(); type == ClauseType::conjunctive) {
                    return clause;
                }
            }
        }
        return std::span{&lit, 1};
    }

    //! Cond must not be called with rvalues.
    auto cond(prg_lit_t &&) -> PrgLitSpan = delete;

    //! Compute the strongly connected components of the positive dependency graph.
    //!
    //! Component numbers are stored in the lit info vector. Trivial components
    //! have scc number zero.
    void compute_sccs() {
        size_t idx_scc = 0;
        graph_.tarjan([&, this](std::vector<size_t> const &scc) {
            assert(!scc.empty());
            if (scc.size() > 1 || graph_.has_loop(scc.front())) {
                ++idx_scc;
                std::ranges::for_each(scc, [&](auto const &lit) { info(uid_to_lit(lit)).scc = idx_scc; });
            }
        });
        graph_.clear();
    }

    //! Translate stored clauses.
    //!
    //! Since clauses can have other clauses as elements, those are marked with
    //! the same equivalence type as the parent. For negative literals, some
    //! rules are omitted.
    //!
    //! This function should be the last translation function called so that
    //! prior ones can add additional clauses if necessary.
    auto tr() {
        auto todo = PrgLitVec{};
        auto hd = PrgLitVec{};
        auto bd = PrgLitVec{};
        auto mark = [&todo, this](prg_lit_t lit) {
            assert(lit > 0);
            if (auto &li = info(lit); li.type != EQType::none) {
                for (auto const &clit : clauses_.nth(li.clause).key().first) {
                    if (this->mark(Literal::from_rep(clit), li.type)) {
                        todo.emplace_back(static_cast<prg_lit_t>(Literal::from_rep(clit).atom().index()));
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
            auto &li = info(lit);
            auto const &[lits, type] = clause;
            if (type == ClauseType::conjunctive) {
                if (li.type != EQType::none) {
                    backend_->rule(std::array{lit}, lits, false);
                }
                if (li.type == EQType::equivalence && li.scc > 0) {
                    for (auto const &clit : lits) {
                        if (clit > 0 && info(clit).scc == li.scc) {
                            backend_->rule(std::array{clit}, std::array{lit}, false);
                        }
                    }
                }
            } else {
                if (li.type != EQType::none) {
                    for (auto const &clit : lits) {
                        backend_->rule(std::array{lit}, std::array{clit}, false);
                    }
                }
                if (li.type == EQType::equivalence) {
                    hd.clear();
                    hd.emplace_back(lit);
                    bd.clear();
                    for (auto const &clit : lits) {
                        if (clit < 0) {
                            bd.emplace_back(negate(clit));
                        } else if (in_cycle(clit, lit)) {
                            bd.emplace_back(-clit);
                        } else {
                            hd.emplace_back(clit);
                        }
                    }
                    backend_->rule(hd, bd, false);
                }
            }
        }
        // NOTE: an offset could be maintained
        for (auto &info : infos_) {
            info.clause = invalid_id;
            info.type = EQType::none;
            info.scc = 0;
        }
        clauses_.clear();
    }

    //! Get the underlying backend.
    //!
    //! @return the backend
    auto backend() -> ProgramBackend & { return *backend_; }

    //! Get the underlying symbol store.
    //!
    //! @return the symbol store
    auto store() -> SymbolStore & { return *store_; }

    //! Mark a literal with the given equivalence type.
    //!
    //! If the literal is associated with a clause, the equivalence between
    //! literal and clause is established accordingly.
    //!
    //! @param lit the literal to mark
    //! @param type the type of the equivalence
    //! @return whether the literal has been marked with a stricter type
    auto mark(Literal lit, EQType type) -> bool {
        auto atm = lit.atom();
        if (lit.sign()) {
            type = std::max(type, EQType::implication);
        }
        if (auto &li = info(static_cast<prg_lit_t>(atm.index())); li.clause != invalid_id && li.type < type) {
            li.type = type;
            return true;
        }
        return false;
    }

    //! Add a Tseitin literal for the given clause.
    //!
    //! Uses a map to uniquely associate literals. For the singleton clauses,
    //! the single literal is returned.
    //!
    //! @param lits the literals
    //! @param type the type of the clause
    //! @param add_edges whether to add dependency edges
    //! @return the Tseitin literal
    auto clause(PrgLitSpan lits, ClauseType type, bool add_edges = true) -> prg_lit_t {
        lits_.assign(lits.begin(), lits.end());
        std::ranges::sort(lits_);
        lits_.erase(std::ranges::unique(lits_).begin(), lits_.end());
        if (lits_.size() == 1) {
            return lits_.front();
        }
        auto [it, ins] = clauses_.emplace(std::pair{lits_, type}, 0);
        if (ins) {
            it.value() = next_lit();
            info(it.value()).clause = std::distance(clauses_.begin(), it);
            for (auto const &lit : it->first.first) {
                if (add_edges && lit > 0) {
                    graph_.add_edge(it.value(), lit);
                }
            }
        }
        return it.value();
    }

    //! Map the given symbol to a unique id.
    //!
    //! @param name the name
    //! @return the id of the vertex
    auto vertex(Symbol name) -> prg_id_t {
        auto [it, ins] = vertices_.emplace(name, static_cast<prg_id_t>(vertices_.size()));
        if (ins && vertices_.size() > 1 && it.value() == 0) {
            throw std::range_error("maximum number of vertices exceeded");
        }
        return it.value();
    }

    //! Add an edge to the underlying graph.
    //!
    //! @param u a "head" atom
    //! @param v a "body" atom
    void add_edge(size_t u, size_t v) { graph_.add_edge(u, v); }

    //! Get the literal info for an atom.
    //!
    //! @pre lit > 0
    //!
    //! @param lit the literal
    //! @return a reference to the info
    [[nodiscard]] auto info(prg_lit_t lit) -> LitInfo & {
        assert(lit > 0);
        while (std::cmp_less(infos_.size(), lit)) {
            infos_.emplace_back();
        }
        return infos_[lit - 1];
    }

    //! Check if the literals occur in the same positive cycle.
    //!
    //! Returns false if one of the literals is negativ.
    //!
    //! @return the Boolean result
    [[nodiscard]] auto in_cycle(prg_lit_t a, prg_lit_t b) -> bool {
        if (a > 0 && b > 0) {
            auto scc = info(a).scc;
            return scc > 0 && scc == info(b).scc;
        }
        return false;
    }

  private:
    using LitInfoVec = std::vector<LitInfo>;
    using ClauseLitMap = Util::ordered_map<std::pair<PrgLitVec, ClauseType>, prg_lit_t>;
    using VertexMap = Util::unordered_map<SharedSymbol, prg_id_t>;

    SymbolStore *store_;
    ProgramBackend *backend_;
    PrgLitVec lits_;
    Util::Graph graph_;
    LitInfoVec infos_;
    ClauseLitMap clauses_;
    VertexMap vertices_;
};

//! Builder for rules.
class BuilderRule {
  public:
    //! Add a disjunctive or choice rule.
    //!
    //! Note that negative literals in the head are supported. They are shifted
    //! before passing them to the backend.
    //!
    //! @param head the literals forming the head
    //! @param body the literals forming the body
    //! @param choice whether the rule is a choice or disjunctive rule
    void add(BuilderBase &bld, PrgLitSpan head, PrgLitSpan body, bool choice) {
        hd_.clear();
        hd_.reserve(head.size());
        bd_.clear();
        bd_.reserve(body.size());
        for (auto const &hlit : head) {
            if (hlit > 0) {
                hd_.emplace_back(hlit);
                for (auto const &blit : body) {
                    if (blit > 0) {
                        bld.add_edge(hlit, blit);
                    }
                }
            } else if (!choice) {
                bd_.emplace_back(bld.negate(hlit));
            }
        }
        bd_.insert(bd_.end(), body.begin(), body.end());
        bld.backend().rule(hd_, bd_, choice);
    }

  private:
    PrgLitVec hd_;
    PrgLitVec bd_;
};

//! Builder for min and max aggregates.
template <class Sym> class BuilderMinMax {
  public:
    //! Delays or translates min and max aggregates based on monotonicity.
    //!
    //! Converts aggregates into sequences of form
    //!
    //!     F(TF)*T?
    //!     T(FT)*F?
    //!
    //! where all but the last T and F in the sequence are non-empty.
    //!
    //! The aggregate is true if for each true literal in F there is one true
    //! literal in the preceeding Ts. Here the empty F is considered to contain
    //! a true literal.
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
    //!
    //! @param bld the base builder
    //! @param lit the Tseitin literal of the aggregate
    //! @param elems the aggregates elements
    //! @param guards the guards of the aggregate
    void add(BuilderBase &bld, prg_lit_t lit, OutputStm::BdElemSpan elems, OutputStm::GuardSpan guards) {
        assert(lit > 0);
        auto [lower, upper, bounds] = analyze_(bld, elems, guards);
        lits_.clear();
        ids_.clear();
        ids_.emplace_back(0);
        auto start = bounds.contains(lower);
        auto state = start;
        for (auto const &[weight, lit] : elems_) {
            auto prev = state;
            state = bounds.contains(weight);
            if (prev != state) {
                ids_.emplace_back(lits_.size());
            }
            lits_.emplace_back(lit);
        }
        if (state != bounds.contains(upper)) {
            ids_.emplace_back(lits_.size());
        }
        lits_.resize(ids_.back());
        for (auto const &clit : lits_) {
            bld.mark(Literal::from_rep(clit), EQType::implication);
        }
        auto get_span = [this](auto it) {
            return std::span{std::next(lits_.begin(), *it), std::next(lits_.begin(), *(it + 1))};
        };
        if (ids_.size() <= 2 || (ids_.size() == 3 && !start)) {
            // translate constant, monotone, antimontone, and convex cases
            tr_(bld, lit, start, lits_, ids_, false);
        } else {
            // delay nonmonotone case
            bool state = start;
            for (auto it = ids_.begin(), ie = ids_.end(); it + 1 != ie; ++it) {
                if (state) {
                    for (auto const &clit : get_span(it)) {
                        if (clit > 0) {
                            bld.add_edge(lit, clit);
                        }
                    }
                }
                state = !state;
            }
            delayed_.emplace_back(lit, start, lits_, ids_);
        }
    }

    //! Translate delayed min and max aggregates.
    //!
    //! @param bld the base builder
    void tr(BuilderBase &bld) {
        for (auto const &[lit, start, lits, ids] : delayed_) {
            tr_(bld, lit, start, lits, ids, true);
        }
        delayed_.clear();
    }

  private:
    //! A vector of sum aggregates.
    using DelayedVec = std::vector<std::tuple<prg_lit_t, bool, PrgLitVec, IndexVec>>;

    //! Simplify and analyze min and max aggregates.
    //!
    //! Returns relevant elements, the range, and bounds of the aggregate.
    //!
    //! @warning Relevant elements are stored in member `elems_`.
    //!
    //! @param bld the base builder
    //! @param elems the elements of the aggregate
    //! @param guards the guards of the aggregate
    //! @return the lower bound, upper bound, and bounds
    auto analyze_(BuilderBase &bld, OutputStm::BdElemSpan elems, OutputStm::GuardSpan guards)
        -> std::tuple<Sym, Sym, Util::interval_set<Sym>> {
        // simplify the elements
        elems_.clear();
        elems_.reserve(elems.size());
        cond_map_.clear();
        cond_map_.reserve(elems.size());
        auto upper = Sym::neutral();
        for (auto const &[tup, conds] : elems) {
            if (!tup.empty() && tup.front() < upper && conds.empty()) {
                // adjust upper bound
                upper = tup.front();
            }
        }
        for (auto const &[tup, conds] : elems) {
            if (!tup.empty() && tup.front() < upper) {
                assert(!conds.empty());
                if (auto [it, ins] = cond_map_.try_emplace(conds, elems_.size()); !ins) {
                    // drop tuples with larger weights
                    get<0>(elems_[it.value()]) = std::min<Sym>(get<0>(elems_[it.value()]), tup.front());
                } else {
                    // add weight literal pairs
                    Util::into_vec(lits_, conds, uid_to_id);
                    elems_.emplace_back(tup.front(), bld.clause({lits_.begin(), lits_.end()}, ClauseType::disjunctive));
                }
            }
        }
        std::ranges::sort(elems_);
        auto lower = elems_.empty() ? upper : get<0>(elems_.front());
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
        return {lower, upper, bounds};
    }

    //! Translates min and max aggregates.
    //!
    //! Min aggregates are translated as indicated in the example below:
    //!
    //!     lit :- 2 != #min { 1:a; 2:b; 3:c; 4:d } !=4
    //!     [         ]
    //!     [ ] [ ] [ ]
    //!      a b c d
    //!      T F T F T
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
    //! Rules of form (*) are chained by introducing Tseitin literals.
    //!
    //! Convex aggregates are translated right away. In which case no cycle
    //! information is available yet and the delayed flag is set to false.
    //!
    //! @param bld the base builder
    //! @param lit the Tseitin literal of the aggregate
    //! @param start whether the sequences starts with a T or an F
    //! @param lits the literals in the sequence
    //! @param ids the start indices of the alternating T/F in the sequence
    //! @param delayed whether translation was delayed
    void tr_(BuilderBase &bld, prg_lit_t lit, bool start, PrgLitVec const &lits, IndexVec const &ids, bool delayed) {
        assert(!ids.empty() && lits.size() == ids.back() && lit > 0);
        auto get_span = [&lits = lits](auto it) {
            return std::span{std::next(lits.begin(), *it), std::next(lits.begin(), *(it + 1))};
        };
        tr_lits_.clear();
        tr_lits_.reserve(lits.size());
        auto state = start;
        for (auto it = ids.begin(), ie = ids.end(); it + 1 != ie; ++it) {
            for (auto const &clit : get_span(it)) {
                if (state) {
                    // lit :- clit, tr_lits_.
                    if (tr_lits_.size() > 1) {
                        auto nlit = bld.clause(tr_lits_, ClauseType::conjunctive, false);
                        bld.mark(Literal::from_rep(nlit), EQType::implication);
                        tr_lits_.clear();
                        tr_lits_.emplace_back(nlit);
                    }
                    tr_lits_.emplace_back(clit);
                    if (!delayed && clit > 0) {
                        bld.add_edge(lit, clit);
                    }
                    bld.backend().rule(std::array{lit}, tr_lits_, false);
                    tr_lits_.pop_back();
                } else {
                    if (delayed && bld.in_cycle(lit, clit)) {
                        // nlit :- not clit.
                        // nlit :- lit.
                        // nlit | clit :- not not l.
                        bld.mark(Literal::from_rep(clit), EQType::equivalence);
                        auto nlit = bld.next_lit();
                        bld.backend().rule(std::array{nlit}, std::array{bld.negate(clit)}, false);
                        bld.backend().rule(std::array{nlit}, std::array{bld.negate(lit)}, false);
                        bld.backend().rule(std::array{nlit, clit}, std::array{bld.negate(bld.negate(lit))}, false);
                        tr_lits_.emplace_back(nlit);
                    } else {
                        tr_lits_.emplace_back(bld.negate(clit));
                    }
                }
            }
            state = !state;
        }
        if (state) {
            bld.backend().rule(std::array{lit}, tr_lits_, false);
        }
    }

    //! Aggregates for translation once dependency info is available.
    DelayedVec delayed_;
    //! Vector produced as a side-effect by analyze_ to avoid allocations.
    std::vector<std::pair<Sym, prg_lit_t>> elems_;
    //! Map used by `analyze_` to avoid allocations.
    Util::unordered_map<IndexSpan, size_t> cond_map_;
    //! Vector used by `add` and `analyze_` to avoid allocations.
    PrgLitVec lits_;
    //! Vector used by `add` to avoid allocations.
    IndexVec ids_;
    //! Vector used by `tr_` to avoid allocations.
    PrgLitVec tr_lits_;
};

//! A builder for sum aggregates taking dependency info into account.
class BuilderSum {
  public:
    //! Add a sum aggregate.
    //!
    //! @param bld the base builder
    //! @param lit the Tseitin literal of the aggregate
    //! @param elems the elements of the aggregate
    //! @param guards the guards of the aggregate
    void add(BuilderBase &bld, prg_lit_t lit, OutputStm::BdElemSpan elems, OutputStm::GuardSpan guards) {
        auto [range, bounds, type] = analyze_(bld, elems, guards);
        if (bounds.contains(range)) {
            rule_.add(bld, std::array{lit}, {}, false);
            return;
        }
        if (bounds.empty()) {
            rule_.add(bld, {}, std::array{lit}, false);
            return;
        }
        // add edges based on the monotonicity of the aggregate
        if (type != CycleType::none) {
            for (auto const &[num, clit] : elems_) {
                if (clit > 0 && ((intersects(type, CycleType::positive) && num > 0) ||
                                 (intersects(type, CycleType::negative) && num < 0))) {
                    bld.add_edge(lit, clit);
                }
            }
        }

#ifdef CLINGO_DEBUG_AGGREGATES
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
        // Note: convex aggregates could be translated right away.
        delayed_.emplace_back(lit, elems_, std::move(range), std::move(bounds));
    }

    //! Translates stored aggregate literals.
    //!
    //! This function iterates over all stored aggregate literals and
    //! translates them into a form understood by the backend.
    //!
    //! @param bld the base builder
    void tr(BuilderBase &bld) {
        auto nlits = std::vector<prg_lit_t>{};
        auto wlits = std::vector<std::pair<prg_lit_t, prg_weight_t>>{};
        auto flits = SumElemVec{};

        for (auto &[lit, elems, range, bounds] : delayed_) {
            assert(lit > 0);
            // check which kind of literals are cyclic
            auto is_recursive = [lit, &bld](prg_lit_t clit) { return bld.in_cycle(lit, clit); };
            auto has_pos_cycle = false; // cycle through atom with *positive* weight
            auto has_neg_cycle = false; // cycle through atom with *negative* weight
            if (bld.info(lit).scc > 0) {
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
            // translate aggregate in lower bound form
            auto flip = [&flits](SumElemVec const &elems) -> decltype(auto) {
                flits.clear();
                flits.reserve(elems.size());
                for (auto const &[num, cond] : elems) {
                    flits.emplace_back(-num, cond);
                }
                return flits;
            };
            nlits.assign(elems.size(), 0);
            auto translate = [&](prg_lit_t lit, SumElemVec const &elems, Number bound) {
                assert(lit > 0);
                wlits.clear();
                wlits.reserve(elems.size());
                auto it = nlits.begin();
                for (auto const &[weight, clit] : elems) {
                    auto &nlit = *it++;
                    if (weight > 0) {
                        wlits.emplace_back(clit, num_to_int(weight));
                    } else {
                        if (!is_recursive(clit)) {
                            wlits.emplace_back(bld.negate(clit), num_to_int(-weight));
                        } else {
                            if (nlit == 0) {
                                nlit = bld.next_lit();
                                bld.backend().rule(std::array{nlit}, std::array{bld.negate(clit)}, false);
                            }
                            wlits.emplace_back(nlit, num_to_int(-weight));
                        }
                        bound -= weight;
                    }
                }
                bld.backend().bd_aggr(std::array{lit}, wlits, bound.as_int().value(), false);
            };

            // translate all bounds of an aggregate
            for (auto const &bound : bounds) {
                bool has_lower = range.left.bound < bound;
                bool has_upper = bound < range.right.bound;
                auto lit_lower = has_lower && has_upper ? bld.next_lit() : lit;
                auto lit_upper = has_lower && has_upper ? bld.next_lit() : lit;
                if (has_lower) {
                    // Note: that the conditions could be weakend dropping
                    // `has_neg_cycle`. I am not sure which variant would work
                    // better in practice.
                    if (has_neg_cycle && !has_pos_cycle) {
                        if (lit == lit_lower) {
                            lit_lower = bld.next_lit();
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
                            lit_upper = bld.next_lit();
                        }
                        translate(lit_upper, elems, bound.right.bound + 1);
                        lit_upper = -lit_upper;
                    } else {
                        translate(lit_upper, flip(elems), -bound.right.bound);
                    }
                }
                if (lit != lit_lower && lit != lit_upper) {
                    bld.backend().rule(std::array{lit}, std::array{lit_lower, lit_upper}, false);
                } else if (lit != lit_lower) {
                    bld.backend().rule(std::array{lit}, std::array{lit_lower}, false);
                } else if (lit != lit_upper) {
                    bld.backend().rule(std::array{lit}, std::array{lit_upper}, false);
                }
            }

            // add disjunctions for recursive literals
            auto it = elems.begin();
            for (auto const &nlit : nlits) {
                auto clit = it++->second;
                bld.mark(Literal::from_rep(clit), nlit > 0 ? EQType::equivalence : EQType::implication);
                if (nlit > 0) {
                    // nlit :- lit.                % saturate
                    bld.backend().rule(std::array{nlit}, std::array{lit}, false);
                    // nlit | clit :- not not lit. % guess
                    bld.backend().rule(std::array{nlit, clit}, std::array{bld.negate(bld.negate(lit))}, false);
                }
            }
        }
        delayed_.clear();
    }

  private:
    using NumberSet = Util::interval_set<Number>;
    //! A sum aggregate element.
    using SumElem = std::pair<Number, prg_lit_t>;
    //! A vector of sum aggregate elements.
    using SumElemVec = std::vector<SumElem>;
    //! A vector of sum aggregates.
    using SumVec = std::vector<std::tuple<prg_lit_t, SumElemVec, NumberSet::interval, NumberSet>>;

    //! Which weights have to be considered for cycle computation.
    enum class CycleType : uint8_t { none, positive, negative, both };
    // NOLINTNEXTLINE
    CLINGO_ENABLE_BITSET_ENUM(CycleType, friend);

    //! Analyze a sum aggregate.
    //!
    //! Constructs a new aggregate element vector removing unnecessary elements,
    //! computes an interval for largest and smallest value of the aggregate, and
    //! represents the guards of the aggreagte as an interval set. This interval set
    //! is additionally intersected with the range.
    //!
    //! @warning Vector `elems_` is produced as a side-effect.
    //!
    //! @param bld the base builder
    //! @param elems the elements of the aggregate
    //! @param guards the guards of the aggregate
    //! @return the range, bounds, and cycles to consider for depencies
    auto analyze_(BuilderBase &bld, OutputStm::BdElemSpan elems, OutputStm::GuardSpan guards)
        -> std::tuple<NumberSet::interval, NumberSet, CycleType> {
        // simplify the aggregate
        auto fixed = Number(0);
        elems_.clear();
        elems_.reserve(elems.size());
        cond_map_.clear();
        cond_map_.reserve(elems.size());
        for (auto const &[tup, conds] : elems) {
            if (!tup.empty() && tup.front().type() == SymbolType::number && tup.front().num() != 0) {
                auto num = tup.front().num();
                if (conds.empty()) {
                    fixed += num;
                } else {
                    if (auto [it, ins] = cond_map_.try_emplace(conds, elems_.size()); !ins) {
                        get<0>(elems_[it.value()]) += num;
                    } else {
                        Util::into_vec(lits_, conds, uid_to_id);
                        elems_.emplace_back(std::move(num),
                                            bld.clause({lits_.begin(), lits_.end()}, ClauseType::disjunctive));
                    }
                }
            }
        }
        elems_.erase(std::ranges::remove_if(elems_, [](auto const &x) { return std::get<0>(x) == 0; }).end(),
                     elems_.end());
        elems_.shrink_to_fit();
        // comput the range of the aggregate
        auto lower = Number(0);
        auto upper = Number(0);
        for (auto const &[num, clit] : elems_) {
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
        return {std::move(range), std::move(bounds), type};
    }

    SumVec delayed_;
    BuilderRule rule_;
    SumElemVec elems_;
    Util::unordered_map<IndexSpan, size_t> cond_map_;
    PrgLitVec lits_;
};

//! Builder for conditional literals in rule bodies.
class BuilderCondLit {
  public:
    //! Define a conjunction of conditional literal.
    //!
    //! The given literal is derived by the given conditional literals.
    //!
    //! @pre lit > 0
    //!
    //! @param bld the base builder
    //! @param lit the Tseitin literal of the conditional literal
    //! @param elems the elements forming the conditional literal
    void add(BuilderBase &bld, prg_lit_t lit, OutputStm::CondLitSpan elems) {
        assert(lit > 0);
        elems_.clear();
        bool simple = true;
        for (auto const &elem : elems) {
            if (auto const &[id_conc, id_prem] = elem; id_conc && lit > 0 && *id_conc > 0) {
                bld.add_edge(lit, *id_conc);
                simple = false;
            }
            elems_.emplace_back(Util::transform(elem.first, uid_to_lit), uid_to_lit(elem.second));
        }
        if (simple) {
            tr_(bld, lit, elems_);
        } else {
            delayed_.emplace_back(lit, elems_);
        }
    }

    //! Translate stored conditional literals.
    //!
    //! @param bld the base builder
    void tr(BuilderBase &bld) {
        for (auto const &[lit, elems] : delayed_) {
            tr_(bld, lit, elems);
        }
        delayed_.clear();
    }

  private:
    using CondLitElem = std::pair<std::optional<prg_lit_t>, prg_lit_t>;
    using CondLitElemVec = std::vector<CondLitElem>;
    using CondLitVec = std::vector<std::pair<prg_lit_t, CondLitElemVec>>;

    //! Translate a single conditional literal.
    //!
    //! @param bld the base builder
    //! @param lit the Tseitin literal of the conditional literal
    //! @param elems the elements of the conditional literal
    void tr_(BuilderBase &bld, prg_lit_t lit, CondLitElemVec const &elems) {
        lits_.clear();
        lits_.reserve(elems.size());
        // Below, we us the following variable names:
        // - K: new uid replacing G : F in the body
        // - G: captures the conclusion
        // - F: catures the premise
        for (auto const &[g, f] : elems) {
            bld.mark(Literal::from_rep(f), EQType::implication);
            if (g) {
                auto rec = bld.in_cycle(lit, f);
                auto k = bld.next_lit();
                // formula G : F is replaced by K
                // K :- G.
                // K :- not F.
                bld.mark(Literal::from_rep(*g), EQType::implication);
                bld.backend().rule(std::array{k}, std::array{*g}, false);
                bld.backend().rule(std::array{k}, std::array{bld.negate(f)}, false);
                if (rec) {
                    // K | F :- not not G.
                    bld.mark(Literal::from_rep(f), EQType::equivalence);
                    bld.backend().rule(std::array{k, f}, std::array{bld.negate(bld.negate(*g))}, false);
                } else {
                }
                lits_.emplace_back(k);
            } else {
                lits_.emplace_back(bld.negate(f));
            }
        }
        bld.backend().rule(std::array{lit}, lits_, false);
    }

    CondLitVec delayed_;
    PrgLitVec lits_;
    CondLitElemVec elems_;
};

//! Builder for disjunctions in rule heads.
class BuilderDisjunction {
  public:
    using DisjElemSpan = OutputStm::DisjElemSpan;

    //! Define a disjunction of (conditional) literals.
    //!
    //! The given literal derives the given literals.
    //!
    //! We use a shifting of conditions into rule bodies as semantic guideline
    //! for the translation applied in this function.
    //!
    //! @param bld the base builder
    //! @param lit the Tseitin literal of the rule body
    //! @param elems the literals to derive
    void add(BuilderBase &bld, prg_lit_t lit, DisjElemSpan elems) {
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
            bld.backend().rule({}, std::array{lit}, false);
            return;
        }
        for (auto const &[sym, auid, conds] : elems) {
            if (auid == 0 && conds.empty()) {
                return;
            }
        }
        elems_.clear();
        bd_.clear();
        hd_.clear();
        bd_.emplace_back(lit);
        for (auto const &[sym, auid, conds] : elems) {
            if (auid != 0) {
                auto a = uid_to_lit(auid);
                assert(a > 0);
                if (conds.empty()) {
                    hd_.emplace_back(a);
                } else {
                    auto x = bld.next_lit();
                    auto y = bld.next_lit();
                    Util::into_vec(lits_, conds, uid_to_atom);
                    auto c = bld.clause(lits_, ClauseType::disjunctive);
                    bld.mark(Literal::from_rep(c), EQType::implication);
                    hd_.emplace_back(x);
                    bd_.emplace_back(y);
                    // a :- x.
                    rule_.add(bld, std::array{a}, std::array{x}, false);
                    //   :- x, not c.
                    rule_.add(bld, {}, std::array{x, bld.negate(c)}, false);
                    // y :- c.
                    rule_.add(bld, std::array{y}, std::array{c}, false);
                    // y :- not c.
                    rule_.add(bld, std::array{y}, std::array{bld.negate(c)}, false);
                    // x :- a, not not c.
                    elems_.emplace_back(a, x, c);
                }
            } else {
                Util::into_vec(lits_, conds, uid_to_atom);
                bd_.emplace_back(bld.negate(bld.clause(lits_, ClauseType::disjunctive)));
                bld.mark(Literal::from_rep(bd_.back()), EQType::implication);
            }
        }
        rule_.add(bld, hd_, bd_, false);
        if (!elems_.empty()) {
            hd_.clear();
            for (auto const &elem : elems) {
                if (auto auid = get<1>(elem); auid != 0) {
                    hd_.emplace_back(uid_to_lit(auid));
                }
            }
            delayed_.emplace_back(std::move(hd_), std::move(elems_));
        }
    }

    //! Translate rule (*) from add() based on head cycles.
    //!
    //! @param bld the base builder
    void tr(BuilderBase &bld) {
        auto hd_counts = Util::unordered_map<size_t, size_t>{};
        auto in_head_cycle = [&hd_counts, &bld](prg_lit_t lit) {
            auto scc = bld.info(lit).scc;
            return scc > 0 && hd_counts.find(scc).value() > 0;
        };
        auto init_head_cycle = [&hd_counts, &bld](PrgLitVec const &hd) {
            hd_counts.clear();
            for (auto lit : hd) {
                if (auto scc = bld.info(lit).scc; scc > 0) {
                    ++hd_counts[scc];
                }
            }
        };
        for (auto &[hd, elems] : delayed_) {
            init_head_cycle(hd);
            for (auto const &[a, x, c] : elems) {
                bld.backend().rule({}, std::array{x, bld.negate(c)}, false);
                if (in_head_cycle(a)) {
                    bld.backend().rule(std::array{x}, std::array{a, bld.negate(bld.negate(c))}, false);
                } else {
                    bld.backend().rule({}, std::array{a, c, bld.negate(x)}, false);
                }
            }
        }
        delayed_.clear();
    }

  private:
    using DisjVec = std::vector<std::tuple<PrgLitVec, std::vector<std::tuple<prg_lit_t, prg_lit_t, prg_lit_t>>>>;

    DisjVec delayed_;
    BuilderRule rule_;
    std::vector<std::tuple<prg_lit_t, prg_lit_t, prg_lit_t>> elems_;
    PrgLitVec lits_;
    PrgLitVec hd_;
    PrgLitVec bd_;
};

//! Builder incrementally extending a minimize constraint.
//!
//! The builder stores a map from tuples to a disjunction of literals, which is
//! incrementally exended by literals representing conjunctive clauses. An
//! optional is used for the disjunctive clause, which is not engaged if the
//! clause is true. Furthermore, a literal is stored for the disjunctive
//! clause, which is set at the end of a step when the disjunctive clause is
//! complete.
class BuilderMinimize {
  public:
    //! Extend the current minimize constraint.
    //!
    //! @param bld the base builder
    //! @param lits the condition
    //! @param weight the weight
    //! @param prio the priority
    //! @param terms the tuple (without weight and priority)
    void add(BuilderBase &bld, PrgLitSpan lits, prg_weight_t weight, prg_weight_t prio, SymbolSpan terms) {
        auto [it, ins] = tuples_.try_emplace(std::tuple(weight, prio, SharedSymbolVec{terms.begin(), terms.end()}));
        if (!it.value()) {
            if (!ins) {
                // skip factual tuples marked below
                return;
            }
            it.value().emplace();
        }
        auto &[old, conds] = *it.value(); // NOLINT
        if (lits.empty()) {
            if (old != 0) {
                auto lit = bld.negate(old);
                bld.backend().minimize(prio, std::array{std::pair{lit, weight}});
            } else {
                auto lit = bld.clause({}, ClauseType::conjunctive);
                bld.mark(Literal::from_rep(lit), EQType::implication);
                bld.backend().minimize(prio, std::array{std::pair{lit, weight}});
            }
            // mark tuple as fact (ignored if enqueued)
            it.value().reset();
            return;
        }
        lits_.assign(lits.begin(), lits.end());
        if (old != 0) {
            lits_.emplace_back(bld.negate(old));
        }
        if (ins || conds.empty()) {
            delayed_.emplace_back(std::distance(tuples_.begin(), it));
        }
        conds.emplace_back(bld.clause(lits_, ClauseType::conjunctive));
    }

    //! Translate tuples from the current step.
    //!
    //! @param bld the base builder
    void tr(BuilderBase &bld) {
        for (auto const &idx : delayed_) {
            auto it = tuples_.nth(idx);
            if (it.value()) {
                auto const &[weight, prio, terms] = it.key();
                auto &[old, conds] = *it.value(); // NOLINT
                assert(!conds.empty());
                for (auto const &lit : conds) {
                    bld.mark(Literal::from_rep(lit), EQType::implication);
                }
                old = bld.clause(conds, ClauseType::disjunctive);
                conds.clear();
                bld.mark(Literal::from_rep(old), EQType::implication);
                bld.backend().minimize(prio, std::array{std::pair{old, weight}});
            }
        }
        delayed_.clear();
    }

    //! Simplify (disjunctive) clauses of stored tuples.
    void simplify(std::function<TruthValue(prg_lit_t)> const &pred) {
        // remove tuple with false DNF
        erase_if(tuples_, [&](auto const &item) { return item.second && pred(item.second->first) == TruthValue::bot; });
        for (auto it = tuples_.begin(), ie = tuples_.end(); it != ie; ++it) {
            if (auto &cond = it.value(); cond) {
                if (pred(cond->first) == TruthValue::top) {
                    // mark tuple with true disjunctive clause
                    cond.reset();
                } else {
                    // remove false literals from disjunctive clause
                    erase_if(cond->second, [&](prg_lit_t const &lit) { return pred(lit) == TruthValue::bot; });
                }
            }
        }
    }

  private:
    IndexVec delayed_;
    Util::ordered_map<std::tuple<prg_weight_t, prg_weight_t, SharedSymbolVec>,
                      std::optional<std::pair<prg_lit_t, PrgLitVec>>>
        tuples_;
    PrgLitVec lits_;
};

//! Output handling conditions.
//!
//! Each literal is mapped to a program literal and appended to a vector of
//! literals that can be retrieved via function literals.
//!
//! Only supports simple literals excluding aggregates, theory atoms and conditions.
class OutputCond : public OutputLit {
  public:
    //! Construct the output.
    //!
    //! @param bld the base builder
    OutputCond(BuilderBase &bld) : bld_{&bld} {}

    //! Get the literals of the body.
    //!
    //! Literals are added via the OutputLit interface.
    //!
    //! @return the literals
    [[nodiscard]] auto literals() const -> PrgLitVec const & { return body_; }

    //! Get the base builder of the output.
    //!
    //! @return the base builder
    auto builder() -> BuilderBase & { return *bld_; };

    //! Append the atom with the given sign.
    //!
    //! @pre The uid must be a literal uid.
    //!
    //! @param sign the sign of the literal
    //! @param uid the uid
    void append(Sign sign, size_t uid) {
        switch (sign) {
            case Sign::none: {
                body_.emplace_back(uid_to_lit(uid));
                return;
            }
            case Sign::once: {
                body_.emplace_back(builder().negate(uid_to_lit(uid)));
                return;
            }
            case Sign::twice: {
                body_.emplace_back(builder().negate(builder().negate(uid_to_lit(uid))));
                return;
            }
        }
        Util::unreachable();
    }

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

    BuilderBase *bld_;
    PrgLitVec body_;
};

//! Output handling rule bodies.
//!
//! Each literal is mapped to a program literal and appended to a vector of
//! literals that can be retrieved via function literals.
//!
//! Supports the full range of clingo's body literals.
class OutputBody : public OutputCond {
  public:
    //! Construct the output.
    //! @param bld the base builder
    OutputBody(BuilderBase &bld) : OutputCond(bld) {}

  private:
    //! Delay building by introducing a fresh literal if necessary and adding
    //! it to the current body.
    //!
    //! @param sign the sign of the body literal
    //! @param uid the current literal (stored as an id)
    //! @return the literal in form of an id
    auto delay_(Sign sign, std::optional<size_t> uid) -> size_t {
        if (!uid) {
            uid = builder().next_lit();
        }
        append(sign, *uid);
        return *uid;
    }

    auto do_cond_lit(std::optional<size_t> uid) -> size_t override { return delay_(Sign::none, uid); }
    auto do_bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t override { return delay_(sign, uid); }
    auto do_bd_theory(Sign sign, std::optional<size_t> uid) -> size_t override { return delay_(sign, uid); }
};

//! Output for statements and theory.
//!
//! This class more or relays calls to the responsible builders.
class OutputBackend : public OutputStm, OutputTheory {
  public:
    //! Construct the output.
    //!
    //! @param store the symbol store
    //! @param backend the backend
    OutputBackend(SymbolStore &store, ProgramBackend &backend, TheoryData &theory)
        : bld_{store, backend}, theory_{&theory} {};

  private:
    void do_project_atom(size_t p_atom, size_t atom) override {
        rule_.add(bld_, std::array{uid_to_atom(p_atom)}, std::array{uid_to_lit(atom)}, false);
    }

    auto do_body() -> OutputLit & override {
        body_.start();
        return body_;
    }

    void do_fact([[maybe_unused]] Symbol sym, size_t uid) override {
        if (uid_to_lit(uid) != fact_) {
            rule_.add(bld_, std::array{uid_to_lit(uid)}, PrgLitSpan{}, false);
        }
    }

    void do_rule(std::optional<std::tuple<Symbol, size_t, bool>> head) override {
        if (head) {
            auto choice = get<2>(*head);
            auto lit = uid_to_lit(get<1>(*head));
            if (lit != fact_) {
                rule_.add(bld_, std::array{lit}, body_.literals(), choice);
            }
        } else {
            rule_.add(bld_, {}, body_.literals(), false);
        }
    }

    void do_show_atom(Symbol atom, size_t uid) override { bld_.backend().show_atom(atom, uid_to_atom(uid)); }

    void do_show_term(Symbol term) override { bld_.backend().show_term(term, body_.literals()); }

    void do_external([[maybe_unused]] Symbol atom, size_t uid, ExternalType type) override {
        bld_.backend().external(uid_to_atom(uid), type);
    }

    void do_project([[maybe_unused]] Symbol atom, size_t uid) override {
        bld_.backend().project(std::array{uid_to_atom(uid)});
    }

    auto do_aggr_rule(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = bld_.next_lit();
        }
        rule_.add(bld_, std::array{uid_to_lit(*uid)}, body_.literals(), false);
        return *uid;
    }

    auto do_theory_rule(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = bld_.next_lit();
        }
        rule_.add(bld_, std::array{uid_to_lit(*uid)}, body_.literals(), false);
        return *uid;
    }

    auto do_disjunctive_rule(std::optional<size_t> uid) -> size_t override {
        if (!uid) {
            uid = bld_.next_lit();
        }
        rule_.add(bld_, std::array{uid_to_lit(*uid)}, body_.literals(), false);
        return *uid;
    }

    void do_weak_constraint(Number const &weight, Number const *prio, SymbolSpan terms) override {
        minimize_.add(bld_, body_.literals(), num_to_int(weight), prio != nullptr ? num_to_int(*prio) : 0, terms);
    }

    void do_heuristic([[maybe_unused]] Symbol atom, size_t uid, Number const &weight, Number const *prio,
                      HeuristicType type) override {
        bld_.backend().heuristic(uid_to_lit(uid), num_to_int(weight), prio != nullptr ? num_to_int(*prio) : 0, type,
                                 body_.literals());
    }

    void do_edge(Symbol src, Symbol dst) override {
        auto id_src = bld_.vertex(src);
        auto id_dst = bld_.vertex(dst);
        bld_.backend().edge(id_src, id_dst, body_.literals());
    }

    auto do_cond() -> OutputLit & override {
        cond_.start();
        return cond_;
    }

    auto do_cond_id() -> size_t override { return bld_.cond(cond_.literals()); }

    auto do_uid(bool fact) -> size_t override {
        if (fact) {
            if (fact_ == 0) {
                fact_ = bld_.next_lit();
                bld_.backend().rule(std::array{fact_}, {}, false);
            }
            return fact_;
        }
        return bld_.next_lit();
    }

    void do_cond_lit(size_t uid, CondLitSpan elems) override { cond_lit_.add(bld_, uid_to_lit(uid), elems); }

    void do_bd_aggr(size_t uid, AggregateFunction fun, BdElemSpan elems, GuardSpan guards) override {
        auto lit = uid_to_lit(uid);
        assert(lit > 0 && fun != AggregateFunction::count);
        switch (fun) {
            case AggregateFunction::sum:
            case AggregateFunction::sump: {
                // Note: assumse that negative weights of sum+ have already been removed
                sum_.add(bld_, lit, elems, guards);
                break;
            }
            case AggregateFunction::min: {
                min_.add(bld_, lit, elems, guards);
                break;
            }
            case AggregateFunction::max: {
                max_.add(bld_, lit, elems, guards);
                break;
            }
            case CppClingo::AggregateFunction::count: {
                assert(false);
            }
        }
    }

    void do_hd_aggr(size_t uid, AggregateFunction fun, HdElemSpan elems, GuardSpan guards) override {
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
        auto blit = uid_to_lit(uid);
        auto bd_conds = std::vector<IndexVec>{};
        auto bd_elems = std::vector<BdElem>{};
        bd_elems.reserve(elems.size());
        bd_conds.reserve(elems.size());
        for (auto const &[tuple, conds] : elems) {
            if (conds.empty()) {
                bd_elems.emplace_back(tuple, IndexSpan{});
            } else {
                bd_conds.emplace_back();
                bd_conds.back().reserve(conds.size());
                for (auto const &[sym, huid, cuid] : conds) {
                    auto clit = uid_to_lit(cuid);
                    if (huid > 0) {
                        auto hlit = uid_to_lit(huid);
                        auto elit = bld_.clause(std::array{hlit, clit}, ClauseType::conjunctive);
                        bd_conds.back().emplace_back(elit);
                        // elit is a fresh positive clause literal
                        bld_.mark(Literal::from_rep(uid_to_lit(static_cast<size_t>(elit))), EQType::implication);
                        bld_.mark(Literal::from_rep(clit), EQType::implication);
                        rule_.add(bld_, std::array{hlit}, std::array{blit, clit}, true);
                    } else {
                        bd_conds.back().emplace_back(clit);
                    }
                }
                bd_elems.emplace_back(tuple, bd_conds.back());
            }
        }
        auto lit = bld_.next_lit();
        bd_aggr(lit, fun, bd_elems, guards);
        rule_.add(bld_, {}, std::array{-lit, blit}, false);
    }

    void do_disjunction(size_t uid, DisjElemSpan elems) override { disjunction_.add(bld_, uid_to_lit(uid), elems); }

    auto do_theory() -> OutputTheory & override { return *this; }

    void do_flush() override {}

    void do_classical_negation(size_t atom_a, size_t atom_b) override {
        bld_.backend().rule({}, std::array{uid_to_atom(atom_a), uid_to_atom(atom_b)}, false);
    }

    void do_end_ground() override {
        bld_.compute_sccs();
        minimize_.tr(bld_);
        cond_lit_.tr(bld_);
        disjunction_.tr(bld_);
        min_.tr(bld_);
        max_.tr(bld_);
        sum_.tr(bld_);
        bld_.tr();
    }

    void do_mark([[maybe_unused]] SymbolCollector &gc) override {}

    auto do_str(String val) -> size_t override { return theory_->str(val); }

    auto do_num(Number const &val) -> size_t override { return theory_->num(num_to_int(val)); }

    auto do_fun(String name, IndexSpan args) -> size_t override {
        return theory_->fun(name, Util::to_vec<TheoryData::IdVec>(args.begin(), args.end(), uid_to_id));
    }

    auto do_tup(TheoryTermTupleType type, IndexSpan args) -> size_t override {
        return theory_->tup(type, Util::to_vec(args, uid_to_id));
    }

    auto do_sym(Symbol sym) -> size_t override { return theory_->sym(sym); }

    auto do_elem(IndexSpan tuple, size_t cond) -> size_t override {
        auto lit = uid_to_lit(cond);
        auto lits = bld_.cond(lit);
        return theory_->elem(Util::to_vec<TheoryData::IdVec>(tuple, uid_to_id), {lits.begin(), lits.end()});
    }

    void do_atom(OutputTheory::AtomType type, size_t atom_uid, Symbol name, IndexSpan elems, OptGuard guard) override {
        auto new_lit = type != OutputTheory::AtomType::directive ? static_cast<prg_lit_t>(atom_uid) : 0;
        auto old_lit = theory_->atom([atom_uid]() { return static_cast<prg_lit_t>(atom_uid); }, name,
                                     Util::to_vec<TheoryData::IdVec>(elems, uid_to_id),
                                     Util::transform(guard, [](auto const &guard) {
                                         return std::pair{guard.first, static_cast<prg_id_t>(guard.second)};
                                     }));
        // handle directives
        if (new_lit == 0 || old_lit == 0) {
            bld_.backend().rule(std::array{uid_to_atom(atom_uid)}, {}, false);
        } else if (new_lit != old_lit) {
            if (type == OutputTheory::AtomType::body) {
                bld_.backend().rule(std::array{new_lit}, std::array{old_lit}, false);
            } else {
                bld_.backend().rule(std::array{old_lit}, std::array{new_lit}, false);
            }
        }
    }

    void do_simplify(std::function<TruthValue(prg_lit_t)> const &pred) override { minimize_.simplify(pred); }

    PrgLitVec lits_;
    BuilderBase bld_;
    BuilderRule rule_;
    BuilderMinMax<FwdSym> min_;
    BuilderMinMax<BwdSym> max_;
    BuilderSum sum_;
    BuilderCondLit cond_lit_;
    BuilderDisjunction disjunction_;
    BuilderMinimize minimize_;
    TheoryData *theory_;
    OutputBody body_{bld_};
    OutputCond cond_{bld_};
    prg_lit_t fact_ = 0;
};

} // namespace

auto TheoryData::num(prg_weight_t num) -> prg_id_t {
    auto [it, ins] = insert_(nums_, num);
    if (ins) {
        backend_->num(it.value(), it.key());
    }
    return it.value();
}

auto TheoryData::str(String str) -> prg_id_t {
    auto [it, ins] = insert_(strings_, str);
    if (ins) {
        backend_->str(it.value(), it.key()->c_str());
    }
    return it.value();
}

auto TheoryData::fun(prg_id_t name, IdVec args) -> prg_id_t {
    if (args.empty()) {
        return name;
    }
    auto [it, ins] = insert_(funs_, std::pair{name, std::move(args)});
    if (ins) {
        backend_->fun(it.value(), it.key().first, it.key().second);
    }
    return it.value();
}

auto TheoryData::tup(TheoryTermTupleType type, IdVec args) -> prg_id_t {
    auto [it, ins] = insert_(tups_, std::pair{type, std::move(args)});
    if (ins) {
        backend_->tup(it.value(), it.key().first, it.key().second);
    }
    return it.value();
}

auto TheoryData::sym(Symbol sym) -> prg_id_t {
    switch (sym.type()) {
        case SymbolType::inf: {
            return str(*store_->string("#inf"));
        }
        case SymbolType::sup: {
            return str(*store_->string("#sup"));
        }
        case SymbolType::string: {
            static thread_local auto buf = Util::OutputBuffer{};
            buf.reset();
            buf << Util::p_quoted(sym.str().view());
            return str(*store_->string(buf.view()));
        }
        case SymbolType::function: {
            auto args = IdVec{};
            args.reserve(sym.args().size());
            for (auto const &arg : sym.args()) {
                args.emplace_back(this->sym(arg));
            }
            auto ret = fun(sym.name(), args);
            if (sym.has_sign()) {
                ret = fun(*store_->string("-"), {&ret, 1});
            }
            return ret;
        }
        case SymbolType::tuple: {
            auto args = IdVec{};
            args.reserve(sym.args().size());
            for (auto const &arg : sym.args()) {
                args.emplace_back(this->sym(arg));
            }
            return tup(TheoryTermTupleType::tuple, args);
        }
        case SymbolType::number: {
            return num(num_to_int(sym.num()));
        }
    }
    Util::unreachable();
}

auto TheoryData::elem(IdVec tuple, LitVec cond) -> prg_id_t {
    auto [it, ins] = insert_(elems_, std::pair{std::move(tuple), std::move(cond)});
    if (ins) {
        backend_->elem(it.value(), it.key().first, it.key().second);
    }
    return it.value();
}

auto TheoryData::atom(std::function<prg_lit_t()> const &atom, prg_id_t name, IdVec elems,
                      std::optional<std::pair<prg_id_t, prg_id_t>> guard) -> prg_lit_t {
    auto [it, ins] = atoms_.emplace(std::tuple{name, std::move(elems), guard}, 0);
    if (ins) {
        it.value() = atom();
        backend_->atom(it.value(), get<0>(it.key()), get<1>(it.key()), get<2>(it.key()));
    }
    return it.value();
}
auto TheoryData::atom(std::function<prg_lit_t()> const &atom, Symbol name, IdVec elems,
                      std::optional<std::pair<String, prg_id_t>> guard) -> prg_lit_t {
    auto name_id = sym(name);
    auto guard_id = Util::transform(
        guard, [this](auto const &guard) { return std::pair{str(guard.first), static_cast<prg_id_t>(guard.second)}; });
    return this->atom(atom, name_id, std::move(elems), guard_id);
}

void TheoryData::reset() noexcept {
    strings_.clear();
    nums_.clear();
    funs_.clear();
    tups_.clear();
    elems_.clear();
    atoms_.clear();
    ids_ = 0;
}

template <class M, class V> auto TheoryData::insert_(M &map, V &&val) -> std::pair<typename M::iterator, bool> {
    auto [it, ins] = map.try_emplace(std::forward<V>(val), ids_);
    if (ins) {
        ++ids_;
        if (ids_ == std::numeric_limits<prg_id_t>::max()) {
            throw std::range_error("theory ids exhausted");
        }
    }
    return {it, ins};
}

auto make_backend_output(SymbolStore &store, ProgramBackend &backend, TheoryData &theory) -> UOutputStm {
    return std::make_unique<OutputBackend>(store, backend, theory);
}

} // namespace CppClingo::Output
