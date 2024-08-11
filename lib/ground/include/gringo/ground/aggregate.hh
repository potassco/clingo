#pragma once

#include <gringo/ground/literal.hh>
#include <gringo/ground/matcher.hh>
#include <gringo/ground/statement.hh>

#include <gringo/util/print.hh>
#include <gringo/util/small_vector.hh>
#include <gringo/util/type_traits.hh>

#include <iostream>

namespace Gringo::Ground {

using GuardVec = std::vector<std::pair<Relation, UTerm>>;

enum class AtomAggrState : uint8_t {
    unknown = 0,
    derived = 1,
    fact = 2,
};

class AtomAggr {
  public:
    using Bound = std::variant<std::pair<Number, Number>, std::pair<Symbol, Symbol>>;

    AtomAggr(AggregateFunction fun) : bound_{init_(fun)} {}

    void accumulate(AggregateFunction fun, SymbolSpan tup, bool fact) {
        if (!tup.empty() && tup.front().type() == SymbolType::number) {
            if (fun == AggregateFunction::min) {
                auto &val = std::get<1>(bound_);
                val.first = std::min(tup.front(), val.first);
                if (fact) {
                    val.second = std::min(tup.front(), val.second);
                }
            } else if (fun == AggregateFunction::max) {
                auto &val = std::get<1>(bound_);
                val.second = std::max(tup.front(), val.second);
                if (fact) {
                    val.first = std::max(tup.front(), val.first);
                }
            } else {
                if (tup.front().type() == SymbolType::number) {
                    auto const &num = tup.front().num();
                    auto &val = std::get<0>(bound_);
                    if (fact) {
                        val.first += num;
                        val.second += num;
                    } else if (num < 0) {
                        val.first += num;
                    } else {
                        val.second += num;
                    }
                }
            }
        }
    }

    auto propagate(GuardVec const &guards) -> bool {
        auto it = guards.begin();
        for (auto const &val : this->guards) {
            auto rel = it++->first;
            auto res = std::visit(
                [&val, &rel]<class T>(T const &x) {
                    std::cerr << "  check: " << val << rel << x.first << " and " << x.second << rel << val << "\n";
                    std::cerr << "  ^- This is not how to check intervals.";
                    // match:
                    //   <, <= : lower
                    //   >, >= : upper
                    //   =     : between
                    //   !=    : not (lower == upper and equal)
                    // fact:
                    //   <, <= : upper
                    //   >, >= : lower
                    //   =     : lower == upper and equal
                    //   !=    : not between
                    return evaluate(val, rel, x.first) && evaluate(x.second, rel, val);
                },
                bound_);
            if (!res) {
                return false;
            }
        }
        return true;
    }

    static auto init_(AggregateFunction fun) -> Bound {
        if (fun == AggregateFunction::min) {
            return Bound{std::in_place_index<1>, SymbolStore::sup(), SymbolStore::sup()};
        }
        if (fun == AggregateFunction::max) {
            return Bound{std::in_place_index<1>, SymbolStore::inf(), SymbolStore::inf()};
        }
        return Bound{std::in_place_index<0>, 0, 0};
    }

    std::vector<size_t> elems;
    std::variant<std::pair<Number, Number>, std::pair<Symbol, Symbol>> bound_;
    Util::small_vector<Symbol> guards;
    size_t propagated = 0;
    AtomAggrState state = AtomAggrState::unknown;
    bool enqueued = false;
};

class BaseAggr : public BaseImpl<Symbol const *, BaseAggr> {
  public:
    using BaseImpl::contains;
    //! Map containing the atoms.
    using AtomMap = Util::ordered_map<Symbol const *, AtomAggr, Util::SpanHash, Util::SpanEqualTo>;

    BaseAggr(size_t size) : atoms_{0, size, size} {}

    //! Check if the base is domain.
    //!
    //! A base is domain if it contains facts only.
    [[nodiscard]] auto domain() const {
        for (auto n = derived_.size(); domain_offset_ < n; ++domain_offset_) {
            if (atoms_.nth(derived_[domain_offset_])->second.state != AtomAggrState::fact) {
                return false;
            }
        }
        return true;
    }
    //! Check if the given atom is a fact.
    //!
    //! This function does not take into account to which generation an atom belongs.
    //! It can also return true for atoms added to upcoming generations.
    auto is_fact(Symbol const *sym) const -> bool {
        auto it = atoms_.find(sym);
        return it != atoms_.end() && it->second.state == AtomAggrState::fact;
    }
    //! Check if the base contains the given atom.
    //!
    //! This might include atoms that have not (yet) been derived.
    [[nodiscard]] auto contains(Symbol const *sym) const -> bool {
        auto it = atoms_.find(sym);
        return it != atoms_.end();
    }

    //! Add an atom to the base.
    //!
    //! This function should be called during propagation if an aggregate can match.
    auto add(AtomMap::const_iterator it) -> bool {
        auto idx = atom_index_(it);
        if (it->second.state == AtomAggrState::unknown) {
            derived_.add(idx);
            return true;
        }
        return false;
    }

    //! Get the number of derived atoms.
    [[nodiscard]] auto size() const -> size_t { return derived_.size(); }

    //! Get the atom index of the given symbol.
    //!
    //! Note that only derived atoms have indices.
    auto index(Symbol const *sym) const -> size_t {
        // TODO: the index in derived could be stored in the atom to avoid the lineoar lookup
        if (auto it = atoms_.find(sym); it != atoms_.end() && it->second.state != AtomAggrState::unknown) {
            return derived_.find(atom_index_(it));
        }
        return size();
    }
    //! Get the i-th atom in the base.
    auto nth(size_t i) const -> AtomMap::const_iterator { return atoms_.nth(derived_[i]); }
    //! Get the i-th atom in the base.
    auto nth(size_t i) -> AtomMap::iterator { return atoms_.nth(derived_[i]); }

    [[nodiscard]] auto atoms() -> AtomMap & { return atoms_; }

  private:
    [[nodiscard]] auto atom_index_(AtomMap::const_iterator it) const -> size_t {
        return static_cast<size_t>(std::distance(atoms_.cbegin(), it));
    }

    AtomMap atoms_;
    Util::index_sequence<size_t> derived_;
    size_t mutable domain_offset_ = 0;
};

class StateAggr {
  public:
    struct Tuple {
        // NOLINTBEGIN
        Tuple(size_t atom_idx, UTermVec const &tuple, SymbolStore &store, Assignment &ass)
            : n{tuple.size()}, atom_idx{atom_idx} {
            auto *it = syms;
            for (auto const &term : tuple) {
                *it++ = term->eval(store, ass).value();
            }
        }
        auto span() const -> SymbolSpan { return SymbolSpan{syms, n}; }
        auto hash() const -> size_t { return Util::value_hash_record<Tuple>(n, atom_idx, span()); }
        friend auto operator==(Tuple const &a, Tuple const &b) -> bool {
            return a.atom_idx == b.atom_idx && a.n == b.n &&
                   std::equal(a.span().begin(), a.span().end(), b.span().begin());
        }
        // Note: in practice these two could be combined to safe a little bit of memory
        size_t n;
        size_t atom_idx;
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
        Symbol syms[0];
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
        // NOLINTEND
    };
    using AtomMap = BaseAggr::AtomMap;
    using TupleMap = Util::ordered_map<Tuple *, Util::small_vector<size_t>>;

    StateAggr(VariableVec global, GuardVec guards, AggregateFunction fun, size_t index, bool monotone, bool recursive)
        : base_{global.size()}, global_{std::move(global)}, guards_{std::move(guards)}, index_{index}, fun_{fun},
          monotone_{monotone}, recursive_{recursive} {}

    [[nodiscard]] auto global() const -> VariableVec const & { return global_; }
    [[nodiscard]] auto guards() const -> GuardVec const & { return guards_; }
    [[nodiscard]] auto fun() const -> AggregateFunction { return fun_; }
    [[nodiscard]] auto monotone() const -> bool { return monotone_; }
    [[nodiscard]] auto recursive() const -> bool { return recursive_; }
    [[nodiscard]] auto index() const -> size_t { return index_; }

    auto propagate() -> bool {
        std::cerr << "propagate aggregate atoms:\n";
        for (auto atom_idx : queue_) {
            auto it = base_.atoms().nth(atom_idx);
            auto &state = it.value();
            state.enqueued = false;
            std::cerr << " atom: " << atom_idx << "\n";
            for (auto jt = state.elems.begin() + static_cast<ssize_t>(state.propagated), je = state.elems.end();
                 jt != je; ++jt) {
                auto elem = tuples_.nth(*jt);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                auto tup = std::span(elem.key()->syms, elem.key()->n);
                state.accumulate(fun_, tup, elem->second.empty());
                std::cerr << "  elem: " << *jt << "\n";
                std::cerr << "   accumulate";
                for (auto val : tup) {
                    std::cerr << " " << val;
                }
                if (elem->second.empty()) {
                    std::cerr << " as fact";
                }
                std::cerr << "\n";
            }
            if (state.propagate(guards_)) {
                std::cerr << " propagate: " << atom_idx << "\n";
            }
        }
        queue_.clear();
        std::cerr << '\n';
        return false;
    }

    auto insert_global(SymbolStore &store, Assignment &ass) -> AtomMap::iterator {
        struct GTup {
            // NOLINTBEGIN
            GTup(VariableVec const &global, Assignment &ass) {
                auto *it = syms;
                for (auto const &var : global) {
                    *it++ = ass[var].value();
                }
            }
            GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
            Symbol syms[0];
            GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
            // NOLINTEND
        };
        auto n = global_.size() * sizeof(Symbol);
        auto &tup = node_store_.construct<GTup>(n, global_, ass);

        auto [it, ins] =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            base_.atoms().try_emplace(tup.syms, fun_);
        if (!ins) {
            node_store_.reclaim(n, tup);
        } else {
            for (auto const &guard : guards_) {
                if (auto val = guard.second->eval(store, ass)) {
                    it.value().guards.emplace_back(*val);
                } else {
                    throw std::logic_error("implement me: handle undefined guards");
                }
            }
        }
        return it;
    }

    auto insert_tuple(AtomMap::iterator it, UTermVec const &tuple, SymbolStore &store,
                      Assignment &ass) -> std::pair<TupleMap::iterator, bool> {
        auto n = sizeof(StateAggr::Tuple) + tuple.size() * sizeof(Symbol);
        auto &tup = node_store_.construct<Tuple>(n, index(it), tuple, store, ass);

        auto [jt, jns] = tuples_.try_emplace(&tup);
        if (!jns) {
            node_store_.reclaim(n, tup);
        }
        it.value().elems.emplace_back(jt - tuples_.begin());
        return {jt, jns};
    }

    auto index(AtomMap::iterator it) -> size_t { return it - base_.atoms().begin(); }

    auto index(TupleMap::iterator it) -> size_t { return it - tuples_.begin(); }

    void enqueue(AtomMap::iterator it) {
        auto &state = it.value();
        if (!state.enqueued && state.propagated < state.elems.size()) {
            state.enqueued = true;
            queue_.emplace_back(index(it));
        }
    }

  private:
    Util::NodeStore<alignof(Symbol)> node_store_;
    BaseAggr base_;
    TupleMap tuples_;
    VariableVec global_;
    GuardVec guards_;
    std::vector<size_t> queue_;
    size_t index_;
    AggregateFunction fun_;
    bool monotone_;
    bool recursive_;
};

class MatcherAggr : public OnceMatcher {
  private:
    auto do_once([[maybe_unused]] InstantiationContext &ctx) -> bool override {
        throw std::logic_error("implement aggregate matcher");
    }
};

//! Literal representing an aggregate.
class LitAggr : public Lit {
  public:
    LitAggr(StateAggr &state) : state_{&state} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override {
        static_cast<void>(vars);
        static_cast<void>(mode);
    }

    [[nodiscard]] auto do_domain() const -> bool override {
        // TODO: maybe the state will know
        return true;
    }

    [[nodiscard]] auto do_recursive() const -> bool override {
        // TODO: maybe the state will know
        return false;
    }

    [[nodiscard]] auto do_matcher(MatcherType type, std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override {
        static_cast<void>(type);
        static_cast<void>(bound);
        // TODO: create proper matcher
        return {std::make_unique<MatcherAggr>(),
                state_->index() != stratified_index ? std::make_optional(state_->index()) : std::nullopt};
    }

    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override {
        static_cast<void>(bound);
        return 0;
    }

    void do_print(std::ostream &out) const override {
        auto const &guards = state_->guards();
        auto it = guards.begin();
        if (guards.size() > 1) {
            out << *it->second << " " << flip(it->first) << " ";
            ++it;
        }
        out << state_->fun() << "("
            << Util::p_range(state_->global(), [](std::ostream &out, auto var) { out << "X_" << var; }) << ")";
        if (state_->index() != stratified_index) {
            out << "[" << state_->index() << "]";
        }
        for (auto ie = guards.end(); it != ie; ++it) {
            out << " " << it->first << " " << *it->second;
        }
    }

    auto do_output(InstantiationContext &ctx, OutputLit &out) const -> bool override {
        static_cast<void>(ctx);
        static_cast<void>(out);
        return true;
    }

    [[nodiscard]] auto do_copy() const -> ULit override { return std::make_unique<LitAggr>(*state_); }

    [[nodiscard]] auto do_hash() const -> size_t override {
        // NOLINTNEXTLINE
        return Util::value_hash_record<LitAggr>(reinterpret_cast<uintptr_t>(this));
    }

    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override { return this == &other; }

    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override { return this <=> &other; }

    StateAggr *state_;
};

//! Gather aggregate elements.
//!
//! This class can also be used to derive empty aggregates. A tuple with a
//! neutral element has to be used, which is 0/\#sum/\#sup depending on the
//! type of the aggregate.
class StmAggrElem : public Stm {
  public:
    StmAggrElem(StateAggr &state, UTermVec tuple, ULitVec body, size_t num_cond, size_t priority, bool dom, bool rec)
        : state_{&state}, tuple_{std::move(tuple)}, body_{std::move(body)}, num_cond_{num_cond}, priority_{priority},
          dom_{dom}, rec_{rec} {
        static_cast<void>(num_cond_);
        static_cast<void>(dom_);
        static_cast<void>(rec_);
    }

  private:
    [[nodiscard]] auto do_body() const -> ULitVec const & override { return body_; }

    [[nodiscard]] auto do_important() const -> VariableSet override {
        auto res = VariableSet{};
        res.insert(state_->global().begin(), state_->global().end());
        for (auto const &term : tuple_) {
            term->vars(res);
        }
        return res;
    }
    [[nodiscard]] auto do_is_important(size_t index) const -> bool override {
        // Only the literals gathered by do_important and the ones in the
        // condition are important. The remaining ones in the body can be
        // backtracked.
        return index < num_cond_;
    }

    void do_init([[maybe_unused]] size_t gen) override {
        // by construction, this statement does not increment the generation
    }

    auto index() -> uint64_t {
        static_assert(sizeof(uintptr_t) <= sizeof(uint64_t));
        // NOLINTNEXTLINE
        return reinterpret_cast<uintptr_t>(this);
    }

    [[nodiscard]] auto do_report(InstantiationContext &ctx) -> bool override {
        // output the condition
        bool fact = true;
        auto &out = ctx.out().cond();
        for (auto const &lit : body_) {
            if (lit->output(ctx, out)) {
                fact = false;
            }
        }
        auto cond_id = ctx.out().cond_id();

        // insert aggregate, tuple, and append condition
        auto &ass = ctx.ass();
        auto it = state_->insert_global(ctx.store(), ass);
        auto [jt, ins] = state_->insert_tuple(it, tuple_, ctx.store(), ass);
        // we use an empty vector to indicate that one of the conditions is fact
        if (fact) {
            jt.value().clear();
        } else if (ins || !jt.value().empty()) {
            // Note that there is some optimization potential here. Typical
            // tuples have exactly one condition. A forward list using the node
            // store for allocation could be used here, where the firt element
            // is stored right in the tuple.
            //
            jt.value().emplace_back(cond_id);
        }
        state_->enqueue(it);

        // TODO: the element should also be output here

        std::cerr << "accumulate:";
        std::cerr << "\n  fact: " << fact;
        std::cerr << "\n  atom id: " << state_->index(it);
        std::cerr << "\n  element id: " << state_->index(jt);
        std::cerr << "\n  condition id: " << cond_id;
        std::cerr << "\n  global:";
        for (auto const &var : state_->global()) {
            if (auto sym = ctx.ass()[var]) {
                std::cerr << " " << *sym;
            }
        }
        std::cerr << "\n  tuple:";
        for (auto const &term : tuple_) {
            if (auto sym = term->eval(ctx.store(), ctx.ass())) {
                std::cerr << " " << *sym;
            }
        }
        std::cerr << "\n";
        return true;
    }

    void do_propagate(Queue &queue) override {
        // This is called after all statements on the current priority have
        // been processed. Thus, all element aggregation rules have been
        // processed. Here, aggregates that can match are added to the base and
        // are enqueued.
        if (state_->propagate() && state_->index() != stratified_index) {
            queue.propagate(state_->index());
        }
    }

    [[nodiscard]] auto do_priority() const -> size_t override { return priority_; }

    void do_print_head(std::ostream &out) const override {
        auto p_var = [](std::ostream &out, auto const &x) { out << "X_" << x; };
        auto p_term = [](std::ostream &out, auto const &x) { out << *x; };
        out << "#elem(";
        out << "g(" << Util::p_range{state_->global(), p_var} << ")";
        out << ",t(" << Util::p_range{tuple_, p_term} << ")";
        out << ")";
    }

    void do_print(std::ostream &out) const override {
        out << priority_ << ": ";
        print_head(out);
        if (state_->index() != stratified_index) {
            out << "[" << state_->index() << "]";
        }
        out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
    }

    StateAggr *state_;
    UTermVec tuple_;
    ULitVec body_;
    size_t num_cond_;
    size_t priority_;
    bool dom_;
    bool rec_;
};

} // namespace Gringo::Ground
