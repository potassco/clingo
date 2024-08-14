#pragma once

#include <gringo/ground/literal.hh>
#include <gringo/ground/matcher.hh>
#include <gringo/ground/statement.hh>

#include <gringo/util/print.hh>
#include <gringo/util/small_vector.hh>
#include <gringo/util/type_traits.hh>

#include <ostream>

// #define DEBUG_AGGR
#ifdef DEBUG_AGGR
#include <iostream>
#endif

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

    //! Initialize for the given aggregate function.
    AtomAggr(AggregateFunction fun) : bound_{init_(fun)} {}

    //! Accumulate a tuple.
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

    //! Check if the aggregate matches the guards (first) and is a fact
    //! (second).
    //!
    //! Only the relation of the given non-ground guards is accessed; the
    //! values for the terms are stored in the aggregate atom.
    auto propagate(GuardVec const &guards, Symbol const *vals) -> std::pair<bool, bool> {
        const auto *it = vals;
        bool fact = true;
        for (auto const &guard : guards) {
            auto rel = guard.first;
            auto res = std::visit(
                [it, rel]<class T>(T const &x) -> std::pair<bool, bool> {
                    switch (rel) {
                        case Relation::less: {
                            return {x.first < *it, x.second < *it};
                        }
                        case Relation::less_equal: {
                            return {x.first <= *it, x.second <= *it};
                        }
                        case Relation::greater: {
                            return {x.second > *it, x.first > *it};
                        }
                        case Relation::greater_equal: {
                            return {x.second >= *it, x.first >= *it};
                        }
                        case Relation::equal: {
                            return {x.first <= *it && *it <= x.second, *it == x.first && *it == x.second};
                        }
                        case Relation::not_equal: {
                            return {*it != x.first || *it != x.second, x.first > *it || *it > x.second};
                        }
                    }
                    Util::unreachable();
                },
                bound_);
            if (!res.first) {
                return {false, false};
            }
            fact = fact && res.second;
            it = std::next(it);
        }
        return {true, fact};
    }

    //! Get the index of the aggregate.
    //!
    //! It must be derived first.
    [[nodiscard]] auto derived_idx() const -> size_t {
        assert(state_ != AtomAggrState::unknown);
        return derived_idx_;
    }
    //! Set the derived index of the aggregate.
    //!
    //! It must be derived first.
    void derived_idx(size_t idx) {
        assert(state_ != AtomAggrState::unknown);
        derived_idx_ = idx;
    }

    //! Get the derived state of the aggregate atom.
    [[nodiscard]] auto state() const -> AtomAggrState { return state_; }
    //! Set the derived state of the aggregate atom.
    //!
    //! It must only be derived once.
    void state(AtomAggrState state) {
        assert(state_ == AtomAggrState::unknown);
        state_ = state;
    }

    //! Enqueue the atom for propagation.
    auto enqueue() -> bool {
        if (!enqueued_ && state_ == AtomAggrState::unknown && (propagated_ < elems_.size() || elems_.empty())) {
            enqueued_ = true;
            return true;
        }
        return false;
    }
    //! Dequeue the atom after propagation.
    //!
    //! Also marks elements as propagated.
    void dequeue() {
        assert(enqueued_);
        propagated_ = elems_.size();
        enqueued_ = false;
    }

    //! Add a new element.
    auto add_elem(size_t idx) { elems_.emplace_back(idx); }
    //! Get the aggregate elements.
    [[nodiscard]] auto elems() const -> std::span<size_t const> { return std::span{elems_.begin(), elems_.end()}; }
    //! Get the aggregate elements to propagate.
    [[nodiscard]] auto todo() -> std::span<size_t const> {
        return std::span{elems_.begin() + static_cast<ssize_t>(propagated_), elems_.end()};
    }

  private:
    static auto init_(AggregateFunction fun) -> Bound {
        if (fun == AggregateFunction::min) {
            return Bound{std::in_place_index<1>, SymbolStore::sup(), SymbolStore::sup()};
        }
        if (fun == AggregateFunction::max) {
            return Bound{std::in_place_index<1>, SymbolStore::inf(), SymbolStore::inf()};
        }
        return Bound{std::in_place_index<0>, 0, 0};
    }

    std::vector<size_t> elems_;
    std::variant<std::pair<Number, Number>, std::pair<Symbol, Symbol>> bound_;
    size_t propagated_ = 0;
    size_t derived_idx_ = 0;
    AtomAggrState state_ = AtomAggrState::unknown;
    bool enqueued_ = false;
};

class BaseAggr : public BaseImpl<Symbol const *, BaseAggr> {
  public:
    using BaseImpl::contains;
    //! Map containing the atoms.
    using AtomMap = Util::ordered_map<Symbol const *, AtomAggr, Util::SpanHash, Util::SpanEqualTo>;

    BaseAggr(size_t size) : atoms_{0, size, size} {}

    //! Check if the given atom is a fact.
    //!
    //! This function does not take into account to which generation an atom belongs.
    //! It can also return true for atoms added to upcoming generations.
    auto is_fact(Symbol const *sym) const -> bool {
        auto it = atoms_.find(sym);
        return it != atoms_.end() && it->second.state() == AtomAggrState::fact;
    }
    //! Add an atom to the base.
    //!
    //! This function should be called during propagation if an aggregate can match.
    void add(AtomMap::iterator it) {
        assert(it->second.state() != AtomAggrState::unknown);
        it.value().derived_idx(derived_.size());
        derived_.add(atom_index_(it));
    }

    //! Get the number of derived atoms.
    [[nodiscard]] auto size() const -> size_t { return derived_.size(); }

    //! Get the atom index of the given symbol.
    //!
    //! Note that only derived atoms have indices.
    auto index(Symbol const *sym) const -> size_t {
        if (auto it = atoms_.find(sym); it != atoms_.end() && it->second.state() != AtomAggrState::unknown) {
            return it->second.derived_idx();
        }
        return size();
    }
    //! Get the i-th atom in the base.
    auto nth(size_t i) const -> AtomMap::const_iterator { return atoms_.nth(derived_[i]); }
    //! Get the i-th atom in the base.
    auto nth(size_t i) -> AtomMap::iterator { return atoms_.nth(derived_[i]); }

    //! Get the underlying atom map (includes atoms not yet derived).
    [[nodiscard]] auto atoms() -> AtomMap & { return atoms_; }

  private:
    [[nodiscard]] auto atom_index_(AtomMap::const_iterator it) const -> size_t {
        return static_cast<size_t>(std::distance(atoms_.cbegin(), it));
    }

    AtomMap atoms_;
    Util::index_sequence<size_t> derived_;
};

auto valid_weight(AggregateFunction fun, Symbol sym) {
    switch (fun) {
        case AggregateFunction::min: {
            return sym.type() != SymbolType::sup;
        }
        case AggregateFunction::max: {
            return sym.type() != SymbolType::inf;
        }
        case AggregateFunction::sum: {
            return sym.type() == SymbolType::number && sym.num() != 0;
        }
        case AggregateFunction::sump: {
            return sym.type() == SymbolType::number && sym.num() > 0;
        }
        case AggregateFunction::count: {
            return true;
        }
    }
    Util::unreachable();
}

class StateAggr {
  public:
    struct ElementKey {
        // NOLINTBEGIN
        ElementKey(SymbolStore &store, Assignment &ass, AggregateFunction fun, size_t atom_idx, UTermVec const &tuple,
                   bool &res)
            : n{tuple.size()}, atom_idx{atom_idx} {
            auto *it = syms;
            if (auto jt = tuple.begin(), je = tuple.end(); jt != je) {
                // check the weight of the tuple
                if (auto val = (*jt)->eval(store, ass); val && valid_weight(fun, *val)) {
                    *it = *val;
                } else {
                    res = false;
                    return;
                }
                for (++jt, ++it; jt != je; ++jt, ++it) {
                    if (auto val = (*jt)->eval(store, ass); val) {
                        *it = *val;
                    } else {
                        res = false;
                        return;
                    }
                }
            } else if (fun != AggregateFunction::count) {
                res = false;
                return;
            }
        }
        auto span() const -> SymbolSpan { return SymbolSpan{syms, n}; }
        auto hash() const -> size_t { return Util::value_hash_record<ElementKey>(n, atom_idx, span()); }
        friend auto operator==(ElementKey const &a, ElementKey const &b) -> bool {
            return a.atom_idx == b.atom_idx && a.n == b.n &&
                   std::equal(a.span().begin(), a.span().end(), b.span().begin());
        }
        // Note that these two could be combined to safe a little bit of memory.
        size_t n;
        size_t atom_idx;
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
        Symbol syms[0];
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
        // NOLINTEND
    };
    struct AtomKey {
        // NOLINTBEGIN
        AtomKey(SymbolStore &store, Assignment &ass, VariableVec const &global, GuardVec &guards, bool &res) {
            auto *it = syms;
            for (auto const &var : global) {
                *it++ = ass[var].value();
            }
            for (auto const &guard : guards) {
                if (auto val = guard.second->eval(store, ass); val) {
                    *it++ = *val;
                } else {
                    res = false;
                    break;
                }
            }
        }
        AtomKey(Symbol const *tuple, size_t n) { std::copy_n(tuple, n, syms); }
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
        Symbol syms[0];
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
        // NOLINTEND
    };
    using AtomMap = BaseAggr::AtomMap;
    using ElementMap = Util::ordered_map<ElementKey *, Util::small_vector<size_t>>;

    //! Initialize an aggregate state.
    StateAggr(VariableVec global, GuardVec guards, AggregateFunction fun, size_t index, bool domain, bool monotone,
              bool recursive)
        : base_{global.size()}, global_{std::move(global)}, guards_{std::move(guards)}, index_{index}, fun_{fun},
          domain_{domain}, monotone_{monotone}, recursive_{recursive} {}

    //! Get the global variables in the aggregate.
    [[nodiscard]] auto global() const -> VariableVec const & { return global_; }
    //! Get the non-ground guards of the aggregate.
    [[nodiscard]] auto guards() const -> GuardVec const & { return guards_; }
    //! Get the aggregate function.
    [[nodiscard]] auto fun() const -> AggregateFunction { return fun_; }
    //! Indicates that all aggregate elements are domain.
    //!
    //! That is, all the bases of literals in conditions are domain and all
    //! negative literals are stratified.
    //!
    //! Only considers the elements of the aggregate.
    [[nodiscard]] auto domain() const -> bool { return domain_; }
    //! Indicates that the aggregate is monotone.
    //!
    //! Neither takes the sign of the aggregate nor its elements into account.
    [[nodiscard]] auto monotone() const -> bool { return monotone_; }
    //! Indicates that there is recursion through one of the literals in the
    //! conditions of elements.
    //!
    //! This does not take into account the body prefix of elements.
    [[nodiscard]] auto recursive() const -> bool { return recursive_; }
    //! Get the update index.
    [[nodiscard]] auto index() const -> size_t { return index_; }

    //! Propagate equeued aggregates.
    auto propagate() -> bool {
        bool res = false;
        for (auto atom_idx : queue_) {
            auto it = base_.atoms().nth(atom_idx);
            auto &state = it.value();
            for (auto elem_idx : state.todo()) {
                auto elem = tuples_.nth(elem_idx);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                auto tup = std::span(elem.key()->syms, elem.key()->n);
                state.accumulate(fun_, tup, elem->second.empty());
#ifdef DEBUG_AGGR
                std::cerr << "accumulate: a: " << atom_idx << " e: " << elem_idx << " t:";
                for (auto val : tup) {
                    std::cerr << " " << val;
                }
                if (elem->second.empty()) {
                    std::cerr << " [f]";
                }
                std::cerr << "\n";
#endif
            }

            if (auto [match, fact] = state.propagate(guards_, it.key() + global_.size()); match) {
                if (fact && (monotone_ || !recursive_)) {
                    state.state(AtomAggrState::fact);
#ifdef DEBUG_AGGR
                    std::cerr << "propagate: a: " << atom_idx << " [f]\n";
#endif
                } else {
                    state.state(AtomAggrState::unknown);
#ifdef DEBUG_AGGR
                    std::cerr << "propagate: a: " << atom_idx << "\n";
#endif
                }
                res = true;
                base_.add(it);
            }
            state.dequeue();
        }
        queue_.clear();
        return res;
    }

    //! Enequeue an atom for propgation.
    void enqueue(AtomMap::iterator it) {
        if (auto &state = it.value(); state.enqueue()) {
            queue_.emplace_back(index(it));
        }
    }

    //! Insert an aggregate atom (stemming from an aggregate element).
    //!
    //! This function also enqueues freshly inserted atoms to cover the case
    //! that the aggregate matches the empty element set.
    auto insert_atom(SymbolStore &store, Assignment &ass) -> std::optional<AtomMap::iterator> {
        auto n = (global_.size() + guards_.size()) * sizeof(Symbol);
        auto res = true;
        auto &tup = node_store_.construct<AtomKey>(n, store, ass, global_, guards_, res);
        if (!res) {
            node_store_.reclaim(n, tup);
            return std::nullopt;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        auto [it, ins] = base_.atoms().try_emplace(tup.syms, fun_);
        if (ins) {
            enqueue(it);
        } else {
            node_store_.reclaim(n, tup);
        }
        return it;
    }

    //! Insert a previously evaluated atom.
    //!
    //! This functions can be used to ensure the presence of an atom that has not yet been derived.
    auto insert_atom(Symbol const *tuple) -> AtomMap::iterator {
        auto n = (global_.size() + guards_.size()) * sizeof(Symbol);
        auto &tup = node_store_.construct<AtomKey>(n, tuple, global_.size());

        auto [it, ins] =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            base_.atoms().try_emplace(tup.syms, fun_);
        if (!ins) {
            node_store_.reclaim(n, tup);
        }
        return it;
    }

    //! Insert an aggregate element.
    void insert_elem(SymbolStore &store, Assignment &ass, AtomMap::iterator it, UTermVec const &tuple,
                     auto const &get_cond) {
        auto n = sizeof(StateAggr::ElementKey) + tuple.size() * sizeof(Symbol);
        bool res = true;
        auto &tup = node_store_.construct<ElementKey>(n, store, ass, fun_, index(it), tuple, res);
        if (!res) {
            node_store_.reclaim(n, tup);
            return;
        }

        auto [jt, jns] = tuples_.try_emplace(&tup);
        if (!jns) {
            node_store_.reclaim(n, tup);
        }
        it.value().add_elem(jt - tuples_.begin());

        auto [cond_id, fact] = get_cond();
        // we use an empty vector to indicate that one of the conditions is fact
        if (fact) {
            jt.value().clear();
        } else if (jns || !jt.value().empty()) {
            jt.value().emplace_back(cond_id);
        }
        // enque the aggregate for propgation
        enqueue(it);
    }

    //! Get the index of an aggregate atom.
    //!
    //! This index also captures not yet derived atoms.
    auto index(AtomMap::iterator it) -> size_t { return it - base_.atoms().begin(); }

    //! Get the index of an aggregate element.
    auto index(ElementMap::iterator it) -> size_t { return it - tuples_.begin(); }

    //! Print a non-ground representation of the aggregate.
    void print(std::ostream &out) {
        auto it = guards_.begin();
        if (guards_.size() > 1) {
            out << *it->second << " " << flip(it->first) << " ";
            ++it;
        }
        out << fun_ << "(" << Util::p_range(global_, [](std::ostream &out, auto var) { out << "X_" << var; }) << ")";
        if (index_ != stratified_index) {
            out << "[" << index_ << "]";
        }
        for (auto ie = guards_.end(); it != ie; ++it) {
            out << " " << it->first << " " << *it->second;
        }
    }

    //! Get the underlying atom base.
    [[nodiscard]] auto base() -> BaseAggr & { return base_; }

  private:
    Util::NodeStore<alignof(Symbol)> node_store_;
    BaseAggr base_;
    ElementMap tuples_;
    VariableVec global_;
    GuardVec guards_;
    std::vector<size_t> queue_;
    size_t index_;
    AggregateFunction fun_;
    bool domain_;
    bool monotone_;
    bool recursive_;
};

//! A term like object used to match conditional literals and their elements.
class MatchAggrLit {
  public:
    //! The key to match against.
    using Key = Symbol const *;

    //! Construct the matcher.
    MatchAggrLit(StateAggr &state) : state_{&state} { eval_.reserve(state_->global().size()); }

    //! Get the variables of the matcher.
    [[nodiscard]] auto vars() const -> VariableSet {
        return VariableSet{state_->global().begin(), state_->global().end()};
    }

    //! Get the signature of the matcher.
    [[nodiscard]] auto signature(VariableSet const &bound,
                                 [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
        static_cast<void>(this);
        return {bound.begin(), bound.end()};
    }

    //! Match a span of symbols representing an atom or element with the assignment.
    [[nodiscard]] auto match([[maybe_unused]] SymbolStore &store, Symbol const *sym, Assignment &ass) const -> bool {
        for (auto var : state_->global()) {
            if (auto &opt = ass[var]; opt) {
                if (*opt != *sym) {
                    return false;
                }
            } else {
                ass[var] = *sym;
            }
            sym = std::next(sym);
        }
        return true;
    }

    //! Evaluate w.r.t. the given assignment and return a span representing an atom or element.
    [[nodiscard]] auto eval(SymbolStore &store, Assignment &ass) const -> std::optional<Symbol const *> {
        eval_.clear();
        for (auto var : state_->global()) {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            eval_.emplace_back(ass[var].value());
        }
        for (auto const &guard : state().guards()) {
            if (auto sym = guard.second->eval(store, ass); sym) {
                eval_.emplace_back(*sym);
            } else {
                return std::nullopt;
            }
        }
        return eval_.data();
    }

    //! Print a string representation of the matcher.
    friend auto operator<<(std::ostream &out, MatchAggrLit const &m) -> std::ostream & {
        m.state_->print(out);
        return out;
    }

    //! Get the associated state.
    [[nodiscard]] auto state() const -> StateAggr & { return *state_; }

  private:
    std::vector<Symbol> mutable eval_;
    StateAggr *state_;
};

//! Literal representing an aggregate.
class LitAggr : public Lit, private MatchAggrLit {
  public:
    LitAggr(StateAggr &state, Sign sign) : MatchAggrLit{state}, sign_{sign} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override {
        switch (mode) {
            case VarSelectMode::all: {
                break;
            }
            case VarSelectMode::provide: {
                if (sign_ == Sign::none || (sign_ == Sign::twice && state().index() == stratified_index)) {
                    break;
                }
                return;
            }
            case VarSelectMode::depend: {
                if (sign_ == Sign::once || (sign_ == Sign::twice && state().index() != stratified_index)) {
                    break;
                }
                return;
            }
        }
        vars.insert(state().global().begin(), state().global().end());
    }

    //! Returns true if matching aggregates are always facts.
    //!
    //! The function can only return true if all literals in elements are
    //! domain. Furthermore, the aggregate must be either monotone or there is
    //! no recursion through it.
    //!
    //! Note that we do not need a stratified index for the latter case. There
    //! can be recursion through the body prefix.
    [[nodiscard]] auto do_domain() const -> bool override {
        return state().domain() && ((sign_ == Sign::none && state().monotone()) || !state().recursive());
    }

    //! Returns true if the aggregate needs only one grounding pass.
    [[nodiscard]] auto do_single_pass() const -> bool override {
        return state().index() == stratified_index || sign_ == Sign::once || !state().recursive();
    }

    [[nodiscard]] auto do_matcher(MatcherType type, std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override {
        symbol_ = nullptr;
        offset_ = invalid_offset;
        auto &match = static_cast<MatchAggrLit &>(*this);
        if (sign_ == Sign::once) {
            return {make_non_fact_matcher(state().base(), match, symbol_), std::nullopt};
        }
        // Note that double-negated non-recursive aggregates are treated like
        // positive aggregates.
        if (sign_ == Sign::twice && state().recursive()) {
            return {make_once_matcher(match, symbol_), std::nullopt};
        }
        auto index = std::optional<size_t>{};
        if (state().index() != stratified_index && type == MatcherType::new_atoms) {
            index = state().index();
        }
        return {make_atom_matcher(bound, state().base(), match, type, offset_), index};
    }

    [[nodiscard]] auto do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double override {
        // Note: at the time of score computation the aggregate is still empty.
        // Since we decided to split earlier, matching them should always be
        // better than using their body prefix.
        return 0;
    }

    void do_print(std::ostream &out) const override { state().print(out); }

    auto do_output(InstantiationContext &ctx, OutputLit &out) const -> bool override {
        auto it = StateAggr::AtomMap::const_iterator{};
        if (sign_ != Sign::once) {
            if (offset_ != invalid_offset) {
                // the atom was used for matching
                it = state().base().nth(offset_);
                if (it.value().state() == AtomAggrState::fact) {
                    return false;
                }
            } else {
                // the atom could not be used for matching
                // (can only be the case for recursive double negation with the match_once_matcher)
                assert(symbol_ != nullptr);
                // ensure presence of atom for output
                it = state().insert_atom(symbol_);
            }
        } else {
            assert(symbol_ != nullptr);
            if (!state().recursive()) {
                // Note: could be transformed to the non-negated case.
                // (best done before the ground representation)
                auto &atoms = state().base().atoms();
                it = atoms.find(symbol_);
                if (it == atoms.end() || it.value().state() == AtomAggrState::unknown) {
                    return false;
                }
            } else {
                // ensure presence of atom for output
                it = state().insert_atom(symbol_);
            }
        }
        static_cast<void>(ctx);
        static_cast<void>(out);
        throw std::logic_error("implement me: output aggregate");
    }

    [[nodiscard]] auto do_copy() const -> ULit override { return std::make_unique<LitAggr>(state(), sign_); }

    [[nodiscard]] auto do_hash() const -> size_t override {
        // NOLINTNEXTLINE
        return Util::value_hash_record<LitAggr>(reinterpret_cast<uintptr_t>(this));
    }

    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override { return this == &other; }

    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override { return this <=> &other; }

    size_t offset_ = invalid_offset;
    Symbol const *symbol_ = nullptr;
    Sign sign_;
};

//! Gather aggregate elements.
//!
//! This class can also be used to derive empty aggregates. A tuple with a
//! neutral element has to be used, which is 0/\#sum/\#sup depending on the
//! type of the aggregate.
class StmAggrElem : public Stm {
  public:
    StmAggrElem(StateAggr &state, UTermVec tuple, ULitVec body, size_t num_cond, size_t priority)
        : state_{&state}, tuple_{std::move(tuple)}, body_{std::move(body)}, num_cond_{num_cond}, priority_{priority} {}

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
        auto &ass = ctx.ass();
        // insert aggregate atom
        if (auto it = state_->insert_atom(ctx.store(), ass)) {

            auto get_cond = [this, &ctx]() {
                // output the condition
                bool fact = true;
                auto &out = ctx.out().cond();
                for (auto const &lit : body_) {
                    if (lit->output(ctx, out)) {
                        fact = false;
                    }
                }
                return std::make_pair(ctx.out().cond_id(), fact);
            };
            // insert the element
            state_->insert_elem(ctx.store(), ass, *it, tuple_, get_cond);
        }
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
        out << "#elem(g(" << Util::p_range{state_->global(), p_var} << "),t(" << Util::p_range{tuple_, p_term} << "))";
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
};

} // namespace Gringo::Ground
