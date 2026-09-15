#include <clingo/ground/sort.hh>

#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>

// #define DEBUG_AGGR
#ifdef DEBUG_AGGR
#include <iostream>
#endif

namespace CppClingo::Ground {

// definition of AtomSortAggr

auto AtomSortAggr::is_fact(Symbol sym) const -> bool {
    if (fact_) {
        return true;
    }
    assert(sym.type() == SymbolType::tuple);
    auto args = sym.args();
    assert(args.size() == 2);
    auto a = values_.find(args[0]);
    auto b = values_.find(args[1]);
    return std::all_of(a, std::next(b), [](auto &&key) { return key.second.fact; });
}

void AtomSortAggr::accumulate(Symbol sym, bool fact) {
    auto [it, ins] = values_.try_emplace(sym, ValueState{fact, false});
    if (!ins) {
        it->second.fact = it->second.fact || fact;
    }
    fact_ = fact_ && fact;
    dirty_ = true;
}

auto AtomSortAggr::todo_values(SymbolStore &store) -> std::span<Symbol const> {
    for (auto it = values_.begin(); it != values_.end(); ++it) {
        auto &[sym, state] = *it;
        if (!state.propagated) {
            for (auto jt = std::next(it); jt != values_.end(); ++jt) {
                auto &[jsym, jstate] = *jt;
                if (jstate.propagated) {
                    pairs_.emplace_back(store.tup_ref(std::array{sym, jsym}));
                }
                if (jstate.fact) {
                    break;
                }
            }
            for (auto jt = std::make_reverse_iterator(it); jt != values_.rend(); ++jt) {
                auto &[jsym, jstate] = *jt;
                pairs_.emplace_back(store.tup_ref(std::array{jsym, sym}));
                if (jstate.fact) {
                    break;
                }
            }
            state.propagated = true;
        }
    }
    auto ib = std::next(pairs_.begin(), propagated_pairs_);
    auto ie = std::next(pairs_.begin(), std::ssize(pairs_));
    return std::span{ib, ie};
}

void AtomSortAggr::add_elem(size_t idx) {
    dirty_ = true;
    elems_.emplace_back(idx);
}

auto AtomSortAggr::elems() const -> std::span<size_t const> {
    return std::span{elems_.begin(), elems_.end()};
}

auto AtomSortAggr::enqueue() -> bool {
    if (!enqueued_ && dirty_) {
        enqueued_ = true;
        return true;
    }
    return false;
}

void AtomSortAggr::dequeue() {
    assert(enqueued_);
    propagated_pairs_ = std::ssize(pairs_);
    propagated_elems_ = std::ssize(elems_);
    enqueued_ = false;
    dirty_ = false;
}

auto AtomSortAggr::todo_elems() -> std::span<size_t const> {
    // NOLINTNEXTLINE
    auto ib = std::next(elems_.begin(), static_cast<std::ptrdiff_t>(propagated_elems_));
    return std::span{ib, elems_.end()};
}

// definition of BaseSortAggr

auto BaseSortAggr::is_fact(Key sym) const -> bool {
    return single_pass_elems_ && atoms_.nth(sym.first).value().is_fact(sym.second);
}

auto BaseSortAggr::add(size_t idx, Symbol val) -> bool {
    return derived_.emplace(Key{idx, val}, invalid_offset).second;
}

auto BaseSortAggr::size() const -> size_t {
    return derived_.size();
}

auto BaseSortAggr::index(Key sym) const -> size_t {
    return derived_.find(sym) - derived_.begin();
}

auto BaseSortAggr::nth(size_t i) const -> AtomSet::const_iterator {
    return derived_.nth(i);
}

auto BaseSortAggr::nth(size_t i) -> AtomSet::iterator {
    return derived_.nth(i);
}

auto BaseSortAggr::atoms() -> AtomMap & {
    return atoms_;
}

auto BaseSortAggr::derived() -> AtomSet & {
    return derived_;
}

auto BaseSortAggr::domain_elems() const -> bool {
    return domain_elems_;
}

auto BaseSortAggr::single_pass_elems() const -> bool {
    return single_pass_elems_;
}

// definition of StateSortAggr

// NOLINTBEGIN

class StateSortAggr::AtomKey {
  private:
    struct priv_tag {};

  public:
    AtomKey([[maybe_unused]] priv_tag tag, Assignment &ass, VariableVec const &global) {
        auto *it = syms_;
        for (auto const &var : global) {
            *it++ = ass[var].value();
        }
    }

    static void construct(auto &mbr, Assignment &ass, VariableVec const &global, AtomKey *&target) {
        auto n = global.size() * sizeof(Symbol);
        if (target == nullptr) {
            target = static_cast<AtomKey *>(mbr.allocate(n, alignof(AtomKey)));
        } else {
            std::destroy_at(target);
        }
        std::construct_at(target, priv_tag{}, ass, global);
    }

    auto syms() -> Symbol const * { return syms_; }

  private:
    CLINGO_IGNORE_ZERO_SIZED_ARRAY_B
    Symbol syms_[0];
    CLINGO_IGNORE_ZERO_SIZED_ARRAY_E
};

// NOLINTEND

auto StateSortAggr::global() const -> VariableVec const & {
    return global_;
}

auto StateSortAggr::symbols() -> SymbolVec & {
    symbols_.resize(global_.size());
    return symbols_;
}

auto StateSortAggr::term() const -> Term const & {
    return *term_;
}

auto StateSortAggr::domain_elems() const -> bool {
    return base_.domain_elems();
}

auto StateSortAggr::single_pass_elems() const -> bool {
    return base_.single_pass_elems();
}

auto StateSortAggr::index() const -> size_t {
    return index_;
}

auto StateSortAggr::propagate(SymbolStore &store) -> bool {
    bool res = false;
    for (auto atom_idx : queue_) {
        auto it = base_.atoms().nth(atom_idx);
        auto &state = it.value();
        for (auto elem_idx : state.todo_elems()) {
            assert(elem_idx < tuples_.size());
            auto elem = tuples_.nth(elem_idx);
            state.accumulate(elem.key().second, elem->second.empty());
#ifdef DEBUG_AGGR
            std::cerr << "accumulate: a: " << atom_idx << " e: " << elem_idx << " t:";
            std::cerr << " " << elem.key().second;
            if (elem->second.empty()) {
                std::cerr << " [f]";
            }
            std::cerr << "\n";
#endif
        }

        for (auto const &val : state.todo_values(store)) {
            res = base().add(atom_idx, val) || res;
#ifdef DEBUG_AGGR
            std::cerr << "propagate: a: " << atom_idx << " v: " << val << (state.is_fact(val) ? " [f]" : "") << "\n";
#endif
        }
        state.dequeue();
    }
    queue_.clear();
    return res;
}

void StateSortAggr::enqueue_(AtomMap::iterator it) {
    if (auto &state = it.value(); state.enqueue()) {
        queue_.emplace_back(atom_index(it));
    }
}

auto StateSortAggr::insert_atom(EvalContext const &ctx) -> std::pair<AtomMap::iterator, bool> {
    AtomKey::construct(*mbr_, ctx.ass(), global_, atom_key_);
    auto [it, ins] = base_.atoms().try_emplace(atom_key_->syms());
    if (ins) {
        atom_key_ = nullptr;
        enqueue_(it);
    }
    return {it, ins};
}

void StateSortAggr::insert_elem(EvalContext const &ctx, AtomMap::iterator it, StmSortAggrElem &elem) {
    auto jt = elem.tuple_.begin();
    auto ie = elem.tuple_.end();
    if (jt == ie) {
        return;
    }
    auto sym = (*jt)->eval(ctx);
    if (!sym) {
        return;
    }
    for (++jt; jt != ie; ++jt) {
        if (!(*jt)->eval(ctx)) {
            return;
        }
    }
    auto [kt, kns] = tuples_.try_emplace(std::pair{atom_index(it), *sym});
    if (kns) {
        it.value().add_elem(kt - tuples_.begin());
        enqueue_(it);
    }

    auto [cond_id, fact] = elem.get_cond_(ctx);
    // we use an empty vector to indicate that one of the conditions is fact
    if (fact) {
        kt.value().clear();
    } else if (kns || !kt.value().empty()) {
        kt.value().emplace_back(cond_id);
    }
}

auto StateSortAggr::atom_index(AtomMap::iterator it) -> size_t {
    return it - base_.atoms().begin();
}

void StateSortAggr::print(std::ostream &out, bool print_index) {
    out << "#sort" << "(" << Util::p_range(global_, [](std::ostream &out, auto var) { out << "X_" << var; }) << ")";
    if (print_index && index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
    out << " = " << *term_;
}

auto StateSortAggr::base() -> BaseSortAggr & {
    return base_;
}

void StateSortAggr::output([[maybe_unused]] Logger &log, [[maybe_unused]] SymbolStore &store, OutputStm &out) {
    std::vector<std::pair<Symbol, std::span<size_t const>>> elems;
    for (auto const &[key, uid] : base_.derived()) {
        if (uid != invalid_offset) {
            auto args = key.second.args();
            auto a = args[0];
            auto b = args[1];
            auto &atom = base_.atoms().nth(key.first).value();
            elems.clear();
            for (auto const &elem_idx : atom.elems()) {
                auto const &[tuple, conds] = *tuples_.nth(elem_idx);
                if (tuple.second >= a && tuple.second <= b) {
                    elems.emplace_back(tuple.second, conds);
                }
            }
            out.bd_sort(uid, elems, key.second);
        }
    }
}

// definition of MatchSortAggr

auto MatchSortAggr::vars() const -> VariableSet {
    VariableSet res{state_->global().begin(), state_->global().end()};
    state_->term().vars(res);
    return res;
}

auto MatchSortAggr::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
    static_cast<void>(this);
    return {bound.begin(), bound.end()};
}

auto MatchSortAggr::match(EvalContext const &ctx, Key key) const -> bool {
    auto &ass = ctx.ass();
    auto const *sym = state().base().atoms().nth(key.first).key();
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
    return state().term().match(ctx, key.second);
}

auto MatchSortAggr::eval(EvalContext const &ctx) const -> std::optional<Key> {
    eval_.clear();
    for (auto var : state_->global()) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        eval_.emplace_back(ctx.ass()[var].value());
    }
    auto &atoms = state().base().atoms();
    auto it = atoms.find(eval_.data());
    if (it == atoms.end()) {
        // It is fine to return nullopt here because assignment aggregates can
        // only occur positively in rule bodies. Hence, failure to evaluate
        // here corresponds to not matching.
        return std::nullopt;
    }
    auto sym = state().term().eval(ctx);
    if (!sym) {
        return std::nullopt;
    }
    return std::make_optional<Key>(state().atom_index(it), *sym);
}

auto operator<<(std::ostream &out, MatchSortAggr const &m) -> std::ostream & {
    m.state_->print(out, false);
    return out;
}

auto MatchSortAggr::state() const -> StateSortAggr & {
    return *state_;
}

// definition of LitSortAggr

void LitSortAggr::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::depend) {
        vars.insert(state().global().begin(), state().global().end());
        state().term().vars(vars);
    }
}

auto LitSortAggr::do_domain() const -> bool {
    return state().domain_elems() && state().single_pass_elems();
}

auto LitSortAggr::do_single_pass() const -> bool {
    return state().index() == stratified_index || state().single_pass_elems();
}

auto LitSortAggr::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type, std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    offset_ = invalid_offset;
    auto &match = static_cast<MatchSortAggr &>(*this);
    auto index = std::optional<size_t>{};
    if (state().index() != stratified_index && type == MatcherType::new_atoms) {
        index = state().index();
    }
    return {make_atom_matcher(mbr, bound, state().base(), match, type, offset_), index};
}

auto LitSortAggr::do_score([[maybe_unused]] std::vector<bool> const &bound, [[maybe_unused]] double estimate) const
    -> double {
    // Note: at the time of score computation the aggregate is still empty.
    // Since we decided to split earlier, matching them should always be
    // better than using their body prefix.
    return 0;
}

auto LitSortAggr::do_domain_size([[maybe_unused]] EstimateSelector sel) const -> std::optional<double> {
    return std::nullopt;
}

void LitSortAggr::do_print(std::ostream &out) const {
    state().print(out, true);
}

auto LitSortAggr::do_output([[maybe_unused]] EvalContext const &ctx, OutputLit &out) const -> bool {
    if (domain()) {
        return false;
    }
    auto &base = state().base();
    auto it = base.nth(offset_);
    auto jt = base.atoms().nth(it.key().first);
    auto const &aggr = jt.value();
    auto sym = it.key().second;
    if (state().single_pass_elems() && aggr.is_fact(sym)) {
        return false;
    }
    auto &state_elem = it.value();
    state_elem = out.delayed(Sign::none, state_elem != invalid_offset ? std::make_optional(state_elem) : std::nullopt);
    return true;
}

auto LitSortAggr::do_copy() const -> ULit {
    return std::make_unique<LitSortAggr>(state());
}

auto LitSortAggr::do_hash() const -> size_t {
    // NOLINTNEXTLINE
    return Util::value_hash_record<LitSortAggr>(reinterpret_cast<uintptr_t>(this));
}

auto LitSortAggr::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitSortAggr::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

// definition of LitSortAggr

auto StmSortAggrElem::do_body() const -> ULitVec const & {
    return body_;
}

auto StmSortAggrElem::do_important() const -> VariableSet {
    auto res = VariableSet{};
    res.insert(state_->global().begin(), state_->global().end());
    for (auto const &term : tuple_) {
        term->vars(res);
    }
    return res;
}

auto StmSortAggrElem::do_is_important(size_t index) const -> bool {
    // Only the literals gathered by do_important and the ones in the
    // condition are important. The remaining ones in the body can be
    // backtracked.
    return index < num_cond_;
}

void StmSortAggrElem::do_init(size_t gen) {
    state_->base().ensure(gen);
}

auto StmSortAggrElem::get_cond_(EvalContext const &ctx) -> std::pair<size_t, bool> {
    bool fact = true;
    auto &out = ctx.out().cond();
    for (auto const &lit : std::span{body_}.subspan(0, num_cond_)) {
        if (lit->output(ctx, out)) {
            fact = false;
        }
    }
    return {ctx.out().cond_id(), fact};
}

auto StmSortAggrElem::do_report(EvalContext const &ctx) -> bool {
    auto it = state_->insert_atom(ctx).first;
    state_->insert_elem(ctx, it, *this);
    return true;
}

void StmSortAggrElem::do_propagate(SymbolStore &store, [[maybe_unused]] OutputStm &out, Queue &queue) {
    // This is called after all statements on the current priority have
    // been processed. Thus, all element aggregation rules have been
    // processed. Here, aggregates that can match are added to the base and
    // are enqueued.
    if (state_->propagate(store) && state_->index() != stratified_index) {
        queue.propagate(state_->index());
    }
}

auto StmSortAggrElem::do_priority() const -> size_t {
    return priority_;
}

void StmSortAggrElem::do_print_head(std::ostream &out) const {
    auto p_var = [](std::ostream &out, auto const &x) { out << "X_" << x; };
    auto p_term = [](std::ostream &out, auto const &x) { out << *x; };
    out << "#elem(g(" << Util::p_range(state_->global(), p_var) << "),t(" << Util::p_range(tuple_, p_term) << "))";
}

void StmSortAggrElem::do_print(std::ostream &out) const {
    out << priority_ << ": ";
    print_head(out);
    if (state_->index() != stratified_index) {
        out << "[" << state_->index() << "]";
    }
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

// definition of LitSortAggrStrat

namespace {

class MatcherSortAggrStrat : public Matcher {
  public:
    MatcherSortAggrStrat(StateSortAggr &state, std::vector<Instantiator> insts, UMatcher matcher)
        : state_{&state}, insts_{std::move(insts)}, matcher_{std::move(matcher)} {}

  private:
    void do_init(InstantiationContext const &ctx, size_t gen) override {
        for (auto &inst : insts_) {
            inst.init(ctx, gen);
        }
        matcher_->init(ctx, gen);
    }
    void do_match(EvalContext const &ctx) override {
        auto [it, ins] = state_->insert_atom(ctx);
        if (ins) {
            // bind global variables
            auto jt = state_->symbols().begin();
            for (auto const &var : state_->global()) {
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                *jt++ = *ctx.ass()[var];
            }
            // ground elems
            CLINGO_REPORT(ctx.log(), trace) << "<<< begin nested instantiation";
            for (auto &inst : insts_) {
                std::ignore = inst.instantiate(ctx.log(), ctx.store(), ctx.out(), nullptr);
            }
            CLINGO_REPORT(ctx.log(), trace) << ">>> end nested instantiation";
            // propagate aggregate
            std::ignore = state_->propagate(ctx.store());
            // ensure that base comprises all atoms
            state_->base().update(0);
        }
        matcher_->match(ctx);
    }
    [[nodiscard]] auto do_next(EvalContext const &ctx) -> bool override { return matcher_->next(ctx); }
    void do_print(std::ostream &out) const override { matcher_->print(out); }
    [[nodiscard]] auto do_type() const -> MatcherType override { return matcher_->type(); }

    StateSortAggr *state_;
    InstantiatorVec insts_;
    UMatcher matcher_;
};

} // namespace

void LitSortAggrStrat::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        vars.insert(state().global().begin(), state().global().end());
    }
    if (mode != VarSelectMode::depend) {
        state().term().vars(vars);
    }
}

auto LitSortAggrStrat::do_domain() const -> bool {
    assert(state().single_pass_elems());
    return state().domain_elems();
}

auto LitSortAggrStrat::do_single_pass() const -> bool {
    assert(state().single_pass_elems());
    return true;
}

auto LitSortAggrStrat::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                  std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    offset_ = invalid_offset;
    auto lin = Linearizer{mbr, EstimateFunction::average, EstimateSelector::pred};
    auto queue = Queue{};
    lin.start(queue);
    for (auto &elem : elems_) {
        lin.prepare(elem, elem.body(), elem.important());
    }
    auto &match = static_cast<MatchSortAggr &>(*this);
    return {std::make_unique<MatcherSortAggrStrat>(state(), queue.release(),
                                                   make_atom_matcher(mbr, bound, state().base(), match, type, offset_)),
            std::nullopt};
}

auto LitSortAggrStrat::do_score([[maybe_unused]] std::vector<bool> const &bound, [[maybe_unused]] double estimate) const
    -> double {
    // Note: at the time of score computation the aggregate is still empty.
    // Since we decided to split earlier, matching them should always be
    // better than using their body prefix.
    return domain() ? 0 : std::numeric_limits<double>::max();
}

void LitSortAggrStrat::do_print(std::ostream &out) const {
    state().print(out, true);
}

auto LitSortAggrStrat::do_output([[maybe_unused]] EvalContext const &ctx, OutputLit &out) const -> bool {
    assert(state().single_pass_elems());
    if (domain()) {
        return false;
    }
    auto &base = state().base();
    auto it = base.nth(offset_);
    auto jt = base.atoms().nth(it.key().first);
    auto const &state_aggr = jt.value();
    auto sym = it.key().second;
    if (state_aggr.is_fact(sym)) {
        return false;
    }
    auto &state_elem = it.value();
    state_elem = out.delayed(Sign::none, state_elem != invalid_offset ? std::make_optional(state_elem) : std::nullopt);
    return true;
}

auto LitSortAggrStrat::do_copy() const -> ULit {
    return std::make_unique<LitSortAggrStrat>(state(), elems_);
}

auto LitSortAggrStrat::do_domain_size([[maybe_unused]] EstimateSelector sel) const -> std::optional<double> {
    return std::nullopt;
}

auto LitSortAggrStrat::do_hash() const -> size_t {
    // NOLINTNEXTLINE
    return Util::value_hash_record<LitSortAggrStrat>(reinterpret_cast<uintptr_t>(this));
}

auto LitSortAggrStrat::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitSortAggrStrat::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

} // namespace CppClingo::Ground
