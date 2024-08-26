#include <gringo/ground/body_aggregate.hh>

#include <gringo/util/print.hh>
#include <gringo/util/type_traits.hh>

// #define DEBUG_AGGR
#ifdef DEBUG_AGGR
#include <iostream>
#endif

namespace Gringo::Ground {

namespace {

auto valid_weight(AggregateFunction fun, Symbol sym) -> bool {
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

} // namespace

// definition of AtomAggr

void AtomBdAggr::accumulate(AggregateFunction fun, SymbolSpan tup, bool fact) {
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

auto AtomBdAggr::propagate(GuardVec const &guards, Symbol const *vals) -> std::pair<bool, bool> {
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

[[nodiscard]] auto AtomBdAggr::derived_idx() const -> size_t {
    assert(state_ != AtomBdAggrState::unknown);
    return derived_idx_;
}

void AtomBdAggr::derived_idx(size_t idx) {
    assert(state_ != AtomBdAggrState::unknown);
    derived_idx_ = idx;
}

[[nodiscard]] auto AtomBdAggr::state() const -> AtomBdAggrState { return state_; }

void AtomBdAggr::state(AtomBdAggrState state) {
    assert(state_ == AtomBdAggrState::unknown);
    state_ = state;
}

auto AtomBdAggr::enqueue() -> bool {
    if (!enqueued_ && state_ == AtomBdAggrState::unknown && (propagated_ < elems_.size() || elems_.empty())) {
        enqueued_ = true;
        return true;
    }
    return false;
}

void AtomBdAggr::dequeue() {
    assert(enqueued_);
    propagated_ = elems_.size();
    enqueued_ = false;
}

void AtomBdAggr::add_elem(size_t idx) { elems_.emplace_back(idx); }

auto AtomBdAggr::elems() const -> std::span<size_t const> { return std::span{elems_.begin(), elems_.end()}; }

auto AtomBdAggr::todo() -> std::span<size_t const> {
    return std::span{elems_.begin() + static_cast<ssize_t>(propagated_), elems_.end()};
}

auto AtomBdAggr::uid() const -> std::optional<size_t> {
    return uid_ != invalid_offset ? std::make_optional(uid_) : std::nullopt;
}

void AtomBdAggr::uid(size_t uid) {
    assert(uid_ == invalid_offset || uid_ == uid);
    uid_ = uid;
}

auto AtomBdAggr::init_(AggregateFunction fun) -> Bound {
    if (fun == AggregateFunction::min) {
        return Bound{std::in_place_index<1>, SymbolStore::sup(), SymbolStore::sup()};
    }
    if (fun == AggregateFunction::max) {
        return Bound{std::in_place_index<1>, SymbolStore::inf(), SymbolStore::inf()};
    }
    return Bound{std::in_place_index<0>, 0, 0};
}

// definition of BaseAggr

auto BaseBdAggr::is_fact(Symbol const *sym) const -> bool {
    auto it = atoms_.find(sym);
    return it != atoms_.end() && it->second.state() == AtomBdAggrState::fact;
}

void BaseBdAggr::add(AtomMap::iterator it) {
    assert(it->second.state() != AtomBdAggrState::unknown);
    it.value().derived_idx(derived_.size());
    derived_.add(atom_index_(it));
}

auto BaseBdAggr::size() const -> size_t { return derived_.size(); }

auto BaseBdAggr::index(Symbol const *sym) const -> size_t {
    if (auto it = atoms_.find(sym); it != atoms_.end() && it->second.state() != AtomBdAggrState::unknown) {
        return it->second.derived_idx();
    }
    return size();
}

auto BaseBdAggr::nth(size_t i) const -> AtomMap::const_iterator { return atoms_.nth(derived_[i]); }

auto BaseBdAggr::nth(size_t i) -> AtomMap::iterator { return atoms_.nth(derived_[i]); }

auto BaseBdAggr::atoms() -> AtomMap & { return atoms_; }

auto BaseBdAggr::atom_index_(AtomMap::const_iterator it) const -> size_t {
    return static_cast<size_t>(std::distance(atoms_.cbegin(), it));
}

// definition of StateAggr

// NOLINTBEGIN

StateBdAggr::ElementKey::ElementKey(SymbolStore &store, Assignment &ass, AggregateFunction fun, size_t atom_idx,
                                    UTermVec const &tuple, bool &res)
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

auto StateBdAggr::ElementKey::span() const -> SymbolSpan { return SymbolSpan{syms, n}; }

auto StateBdAggr::ElementKey::hash() const -> size_t {
    return Util::value_hash_record<ElementKey>(n, atom_idx, span());
}

auto operator==(StateBdAggr::ElementKey const &a, StateBdAggr::ElementKey const &b) -> bool {
    return a.atom_idx == b.atom_idx && a.n == b.n && std::equal(a.span().begin(), a.span().end(), b.span().begin());
}

StateBdAggr::AtomKey::AtomKey(SymbolStore &store, Assignment &ass, VariableVec const &global, GuardVec &guards,
                              bool &res) {
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

StateBdAggr::AtomKey::AtomKey(Symbol const *tuple, size_t n) { std::copy_n(tuple, n, syms); }

// NOLINTEND

auto StateBdAggr::global() const -> VariableVec const & { return global_; }

auto StateBdAggr::guards() const -> GuardVec const & { return guards_; }

auto StateBdAggr::fun() const -> AggregateFunction { return fun_; }

auto StateBdAggr::domain() const -> bool { return domain_; }

auto StateBdAggr::monotone() const -> bool { return monotone_; }

auto StateBdAggr::single_pass_elems() const -> bool { return single_pass_elems_; }

auto StateBdAggr::index() const -> size_t { return index_; }

auto StateBdAggr::propagate() -> bool {
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
            if (fact && (monotone() || single_pass_elems())) {
                state.state(AtomBdAggrState::fact);
#ifdef DEBUG_AGGR
                std::cerr << "propagate: a: " << atom_idx << " [f]\n";
#endif
            } else {
                state.state(AtomBdAggrState::derived);
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

void StateBdAggr::enqueue(AtomMap::iterator it) {
    if (auto &state = it.value(); state.enqueue()) {
        queue_.emplace_back(index(it));
    }
}

auto StateBdAggr::insert_atom(SymbolStore &store, Assignment &ass) -> std::optional<AtomMap::iterator> {
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

auto StateBdAggr::insert_atom(Symbol const *tuple) -> AtomMap::iterator {
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

void StateBdAggr::insert_elem(SymbolStore &store, Assignment &ass, AtomMap::iterator it, UTermVec const &tuple,
                              auto const &get_cond) {
    auto n = sizeof(StateBdAggr::ElementKey) + tuple.size() * sizeof(Symbol);
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

auto StateBdAggr::index(AtomMap::iterator it) -> size_t { return it - base_.atoms().begin(); }

auto StateBdAggr::index(ElementMap::iterator it) -> size_t { return it - tuples_.begin(); }

void StateBdAggr::print(std::ostream &out) {
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

auto StateBdAggr::base() -> BaseBdAggr & { return base_; }

void StateBdAggr::output(OutputStm &out) {
    std::vector<std::pair<SymbolSpan, std::span<size_t const>>> elems;
    std::vector<std::pair<Relation, Symbol>> guards;
    for (auto const &[tuple, atom] : base_.atoms()) {
        if (auto uid = atom.uid(); uid) {
            elems.clear();
            guards.clear();
            for (auto const &elem_idx : atom.elems()) {
                auto const &[tuple, conds] = *tuples_.nth(elem_idx);
                elems.emplace_back(tuple->span(), conds);
            }
            auto const *it = tuple + global_.size();
            for (auto &guard : guards_) {
                guards.emplace_back(guard.first, *it);
                it = std::next(it);
            }
            out.bd_aggr(*uid, fun_, elems, guards);
        }
    }
}

// definition of MatchAggr

auto MatchBdAggr::vars() const -> VariableSet { return VariableSet{state_->global().begin(), state_->global().end()}; }

auto MatchBdAggr::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
    static_cast<void>(this);
    return {bound.begin(), bound.end()};
}

auto MatchBdAggr::match([[maybe_unused]] SymbolStore &store, Symbol const *sym, Assignment &ass) const -> bool {
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

auto MatchBdAggr::eval(SymbolStore &store, Assignment &ass) const -> std::optional<Symbol const *> {
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

auto MatchBdAggr::state() const -> StateBdAggr & { return *state_; }

auto operator<<(std::ostream &out, MatchBdAggr const &m) -> std::ostream & {
    m.state_->print(out);
    return out;
}

// definition of AggrLit

void LitBdAggr::do_vars(VariableSet &vars, VarSelectMode mode) const {
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

auto LitBdAggr::do_domain() const -> bool {
    return state().domain() && ((sign_ == Sign::none && state().monotone()) || state().single_pass_elems());
}

auto LitBdAggr::do_single_pass() const -> bool {
    return state().index() == stratified_index || sign_ == Sign::once || state().single_pass_elems();
}

auto LitBdAggr::do_matcher(MatcherType type,
                           std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    symbol_ = nullptr;
    offset_ = invalid_offset;
    auto &match = static_cast<MatchBdAggr &>(*this);
    if (sign_ == Sign::once) {
        return {make_non_fact_matcher(state().base(), match, symbol_), std::nullopt};
    }
    // Note that double-negated single-pass aggregates are treated like
    // positive aggregates.
    if (sign_ == Sign::twice && !state().single_pass_elems()) {
        return {make_once_matcher(match, symbol_), std::nullopt};
    }
    auto index = std::optional<size_t>{};
    if (state().index() != stratified_index && type == MatcherType::new_atoms) {
        index = state().index();
    }
    return {make_atom_matcher(bound, state().base(), match, type, offset_), index};
}

auto LitBdAggr::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    // Note: at the time of score computation the aggregate is still empty.
    // Since we decided to split earlier, matching them should always be
    // better than using their body prefix.
    return 0;
}

void LitBdAggr::do_print(std::ostream &out) const { state().print(out); }

auto LitBdAggr::do_output([[maybe_unused]] InstantiationContext &ctx, OutputLit &out) const -> bool {
    if (domain()) {
        return false;
    }
    auto it = StateBdAggr::AtomMap::iterator{};
    if (sign_ != Sign::once) {
        if (offset_ != invalid_offset) {
            // the atom was used for matching
            it = state().base().nth(offset_);
            if (it.value().state() == AtomBdAggrState::fact) {
                return false;
            }
        } else {
            // the atom could not be used for matching
            assert(symbol_ != nullptr && sign_ == Sign::twice);
            // ensure presence of atom for output
            it = state().insert_atom(symbol_);
        }
    } else {
        assert(symbol_ != nullptr);
        if (state().single_pass_elems()) {
            // Note: could be transformed to the non-negated case.
            // (best done before the ground representation)
            auto &atoms = state().base().atoms();
            it = atoms.find(symbol_);
            if (it == atoms.end() || it.value().state() == AtomBdAggrState::unknown) {
                return false;
            }
        } else {
            // ensure presence of atom for output
            it = state().insert_atom(symbol_);
        }
    }
    auto &state = it.value();
    state.uid(out.bd_aggr(sign_, state.uid()));
    return true;
}

auto LitBdAggr::do_copy() const -> ULit { return std::make_unique<LitBdAggr>(state(), sign_); }

auto LitBdAggr::do_hash() const -> size_t {
    // NOLINTNEXTLINE
    return Util::value_hash_record<LitBdAggr>(reinterpret_cast<uintptr_t>(this));
}

auto LitBdAggr::do_equal_to(Lit const &other) const -> bool { return this == &other; }

auto LitBdAggr::do_compare_to(Lit const &other) const -> std::weak_ordering { return this <=> &other; }

// definition of StmAggrElem

auto StmBdAggrElem::do_body() const -> ULitVec const & { return body_; }

auto StmBdAggrElem::do_important() const -> VariableSet {
    auto res = VariableSet{};
    res.insert(state_->global().begin(), state_->global().end());
    for (auto const &term : tuple_) {
        term->vars(res);
    }
    return res;
}

auto StmBdAggrElem::do_is_important(size_t index) const -> bool {
    // Only the literals gathered by do_important and the ones in the
    // condition are important. The remaining ones in the body can be
    // backtracked.
    return index < num_cond_;
}

void StmBdAggrElem::do_init([[maybe_unused]] size_t gen) {
    // by construction, this statement does not increment the generation
}

auto StmBdAggrElem::do_report(InstantiationContext &ctx) -> bool {
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

void StmBdAggrElem::do_propagate(Queue &queue) {
    // This is called after all statements on the current priority have
    // been processed. Thus, all element aggregation rules have been
    // processed. Here, aggregates that can match are added to the base and
    // are enqueued.
    if (state_->propagate() && state_->index() != stratified_index) {
        queue.propagate(state_->index());
    }
}

auto StmBdAggrElem::do_priority() const -> size_t { return priority_; }

void StmBdAggrElem::do_print_head(std::ostream &out) const {
    auto p_var = [](std::ostream &out, auto const &x) { out << "X_" << x; };
    auto p_term = [](std::ostream &out, auto const &x) { out << *x; };
    out << "#elem(g(" << Util::p_range{state_->global(), p_var} << "),t(" << Util::p_range{tuple_, p_term} << "))";
}

void StmBdAggrElem::do_print(std::ostream &out) const {
    out << priority_ << ": ";
    print_head(out);
    if (state_->index() != stratified_index) {
        out << "[" << state_->index() << "]";
    }
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

} // namespace Gringo::Ground
