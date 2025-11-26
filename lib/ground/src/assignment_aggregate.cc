#include <clingo/ground/assignment_aggregate.hh>

#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>

// #define DEBUG_AGGR
#ifdef DEBUG_AGGR
#include <iostream>
#endif

namespace CppClingo::Ground {

// definition of AtomAssignAggr

auto AtomAssignAggr::init_(AggregateFunction fun) -> Values {
    return std::visit(
        []<class T>(T &&x) -> Values {
            Util::small_vector<std::remove_cvref_t<T>> ret;
            ret.emplace_back(std::forward<T>(x));
            return ret;
        },
        neutral_num(fun));
}

auto AtomAssignAggr::is_fact() const {
    return std::visit([](auto const &x) { return x.size() == 1; }, values_);
}

void AtomAssignAggr::accumulate(AggregateFunction fun, SymbolSpan tup, bool fact) {
    assert(fun != AggregateFunction::count);
    if (tup.empty()) {
        return;
    }
    auto const &val = tup.front();
    switch (fun) {
        case AggregateFunction::min:
        case AggregateFunction::max: {
            auto &vals = std::get<Util::small_vector<Symbol>>(values_);
            auto cmp = [lt = fun == AggregateFunction::min](auto const &a, auto const &b) {
                return lt ? a < b : b < a;
            };
            if (fact) {
                if (auto *it = std::ranges::lower_bound(vals, val, cmp); it != vals.end()) {
                    vals.erase(it, vals.end());
                    propagated_vals_ = std::min(vals.size(), propagated_vals_);
                    vals.emplace_back(val);
                }
            } else {
                if (auto *it = std::ranges::lower_bound(vals, val, cmp); it != vals.end() && *it != val) {
                    propagated_vals_ = std::min(static_cast<size_t>(it - vals.begin()), propagated_vals_);
                    vals.emplace(it, val);
                }
            }
            break;
        }
        default: {
            if (tup.front().type() != SymbolType::number) {
                return;
            }
            auto const &num = tup.front().num();
            if (num == 0 || (fun == AggregateFunction::sump && num < 0)) {
                return;
            }
            auto &vals = std::get<Util::small_vector<Number>>(values_);
            if (fact) {
                propagated_vals_ = 0;
                for (auto &val : vals) {
                    val += num;
                }
            } else {
                auto m = std::ssize(vals);
                auto n = std::ssize(vals);
                auto p = static_cast<std::ptrdiff_t>(propagated_vals_);
                for (auto i = std::ptrdiff_t{0}; i < n; ++i) {
                    if (i == p) {
                        m = std::ssize(vals);
                    }
                    // the value to insert
                    auto iv = vals[i] + num;
                    // there are 4 sorted ranges:
                    // - [ib, ip) : values already propagated
                    // - [ip, in) : values previously inserted
                    // - [in, im) : fresh values from propagated ones
                    // - [im, ie) : fresh values from not yet propagated ones
                    auto const *ib = vals.begin();
                    auto const *ip = std::next(ib, p);
                    auto const *in = std::next(ib, n);
                    auto const *im = std::next(ib, m);
                    if (!std::binary_search(ib, ib, iv) && !std::binary_search(ip, in, iv) &&
                        !std::binary_search(in, im, iv) && vals.back() != iv) {
                        vals.emplace_back(std::move(iv));
                    }
                }
                // sort the range [ip, ie]
                auto *ib = vals.begin();
                auto *ie = vals.end();
                std::inplace_merge(std::next(ib, n), std::next(ib, m), ie);
                std::inplace_merge(std::next(ib, p), std::next(ib, n), ie);
            }
        }
    }
}

auto AtomAssignAggr::todo_values() -> std::variant<NumberSpan, SymbolSpan> {
    return std::visit(
        [p = static_cast<std::ptrdiff_t>(propagated_vals_)](auto const &x) -> std::variant<NumberSpan, SymbolSpan> {
            return std::span{std::next(x.begin(), p), x.end()};
        },
        values_);
}

void AtomAssignAggr::add_elem(size_t idx) {
    elems_.emplace_back(idx);
}

auto AtomAssignAggr::elems() const -> std::span<size_t const> {
    return std::span{elems_.begin(), elems_.end()};
}

auto AtomAssignAggr::num_values_() const -> size_t {
    return std::visit([](auto const &x) { return x.size(); }, values_);
}

auto AtomAssignAggr::enqueue() -> bool {
    if (!enqueued_ && (propagated_ < elems_.size() || propagated_vals_ < num_values_())) {
        enqueued_ = true;
        return true;
    }
    return false;
}

void AtomAssignAggr::dequeue() {
    assert(enqueued_);
    std::visit([](auto &x) { std::ranges::sort(x); }, values_);
    propagated_vals_ = num_values_();
    propagated_ = elems_.size();
    enqueued_ = false;
}

auto AtomAssignAggr::todo() -> std::span<size_t const> {
    return std::span{elems_.begin() + static_cast<std::ptrdiff_t>(propagated_), elems_.end()};
}

// definition of BaseAssignAggr

auto BaseAssignAggr::is_fact(Key sym) const -> bool {
    return single_pass_elems_ && atoms_.nth(sym.first).value().is_fact();
}

auto BaseAssignAggr::add(size_t idx, Symbol val) -> bool {
    return derived_.emplace(Key{idx, val}, invalid_offset).second;
}

auto BaseAssignAggr::size() const -> size_t {
    return derived_.size();
}

auto BaseAssignAggr::index(Key sym) const -> size_t {
    return derived_.find(sym) - derived_.begin();
}

auto BaseAssignAggr::nth(size_t i) const -> AtomSet::const_iterator {
    return derived_.nth(i);
}

auto BaseAssignAggr::nth(size_t i) -> AtomSet::iterator {
    return derived_.nth(i);
}

auto BaseAssignAggr::atoms() -> AtomMap & {
    return atoms_;
}

auto BaseAssignAggr::derived() -> AtomSet & {
    return derived_;
}

auto BaseAssignAggr::domain_elems() const -> bool {
    return domain_elems_;
}

auto BaseAssignAggr::single_pass_elems() const -> bool {
    return single_pass_elems_;
}

// definition of StateAssignAggr

// NOLINTBEGIN

class StateAssignAggr::AtomKey {
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

StateAssignAggr::ElementKey::ElementKey([[maybe_unused]] priv_tag tag, EvalContext const &ctx, AggregateFunction fun,
                                        size_t atom_idx, StmAssignAggrElem &elem, bool &res)
    : n_{elem.tuple_.size()}, atom_idx_{atom_idx} {
    assert(fun != AggregateFunction::count);
    auto *it = syms_;
    if (auto jt = elem.tuple_.begin(), je = elem.tuple_.end(); jt != je) {
        // check the weight of the tuple
        if (auto val = (*jt)->eval(ctx); val) {
            if (relevant_val(fun, *val)) {
                *it = *val;
            } else {
                neutral_val(fun) !=
                    *val &&expect(ctx, elem.loc_weight_, elem.logged_, "non-negative number expected (got ", *val, ")");
                return;
            }
        } else {
            return;
        }
        for (++jt, ++it; jt != je; ++jt, ++it) {
            if (auto val = (*jt)->eval(ctx); val) {
                *it = *val;
            } else {
                return;
            }
        }
    } else {
        return;
    }
    res = true;
}

auto StateAssignAggr::ElementKey::construct(auto &mbr, EvalContext const &ctx, AggregateFunction fun, size_t atom_idx,
                                            StmAssignAggrElem &elem) -> bool {
    if (elem.key_ == nullptr) {
        auto n = sizeof(ElementKey) + elem.tuple_.size() * sizeof(Symbol);
        elem.key_ = static_cast<StateAssignAggr::ElementKey *>(mbr.allocate(n, alignof(ElementKey)));
    } else {
        std::destroy_at(elem.key_);
    }
    bool res = false;
    std::construct_at(elem.key_, priv_tag{}, ctx, fun, atom_idx, elem, res);
    return res;
}

auto StateAssignAggr::ElementKey::span() const -> SymbolSpan {
    return SymbolSpan{syms_, n_};
}

auto StateAssignAggr::ElementKey::hash() const -> size_t {
    return Util::value_hash_record<ElementKey>(n_, atom_idx_, span());
}

auto operator==(StateAssignAggr::ElementKey const &a, StateAssignAggr::ElementKey const &b) -> bool {
    return a.atom_idx_ == b.atom_idx_ && a.n_ == b.n_ && std::equal(a.span().begin(), a.span().end(), b.span().begin());
}

// NOLINTEND

auto StateAssignAggr::global() const -> VariableVec const & {
    return global_;
}

auto StateAssignAggr::symbols() -> SymbolVec & {
    symbols_.resize(global_.size());
    return symbols_;
}

auto StateAssignAggr::term() const -> Term const & {
    return *term_;
}

auto StateAssignAggr::fun() const -> AggregateFunction {
    return fun_;
}

auto StateAssignAggr::domain_elems() const -> bool {
    return base_.domain_elems();
}

auto StateAssignAggr::single_pass_elems() const -> bool {
    return base_.single_pass_elems();
}

auto StateAssignAggr::index() const -> size_t {
    return index_;
}

auto StateAssignAggr::propagate(SymbolStore &store) -> bool {
    bool res = false;
    for (auto atom_idx : queue_) {
        auto it = base_.atoms().nth(atom_idx);
        auto &state = it.value();
        for (auto elem_idx : state.todo()) {
            assert(elem_idx < tuples_.size());
            auto elem = tuples_.nth(elem_idx);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            state.accumulate(fun_, elem.key()->span(), elem->second.empty());
#ifdef DEBUG_AGGR
            std::cerr << "accumulate: a: " << atom_idx << " e: " << elem_idx << " t:";
            for (auto const &val : elem.key()->span()) {
                std::cerr << " " << val;
            }
            if (elem->second.empty()) {
                std::cerr << " [f]";
            }
            std::cerr << "\n";
#endif
        }

        std::visit(
            [&, this]<class T>(T const &values) {
                for (auto const &val : values) {
                    if constexpr (Util::matches<typename T::value_type, Number>) {
                        res = base().add(atom_idx, store.num_ref(val)) || res;
                    } else {
                        res = base().add(atom_idx, val) || res;
                    }
#ifdef DEBUG_AGGR
                    std::cerr << "propagate: a: " << atom_idx << " v: " << val << (state.is_fact() ? " [f]" : "")
                              << "\n";
#endif
                }
            },
            state.todo_values());
        state.dequeue();
    }
    queue_.clear();
    return res;
}

void StateAssignAggr::enqueue_(AtomMap::iterator it) {
    if (auto &state = it.value(); state.enqueue()) {
        queue_.emplace_back(atom_index(it));
    }
}

auto StateAssignAggr::insert_atom(EvalContext const &ctx) -> std::pair<AtomMap::iterator, bool> {
    AtomKey::construct(*mbr_, ctx.ass(), global_, atom_key_);
    auto [it, ins] = base_.atoms().try_emplace(atom_key_->syms(), fun_);
    if (ins) {
        atom_key_ = nullptr;
        enqueue_(it);
    }
    return {it, ins};
}

void StateAssignAggr::insert_elem(EvalContext const &ctx, AtomMap::iterator it, StmAssignAggrElem &elem) {
    if (ElementKey::construct(*mbr_, ctx, fun_, atom_index(it), elem)) {
        auto [jt, jns] = tuples_.try_emplace(elem.key_);
        if (jns) {
            elem.key_ = nullptr;
            it.value().add_elem(jt - tuples_.begin());
            enqueue_(it);
        }

        auto [cond_id, fact] = elem.get_cond_(ctx);
        // we use an empty vector to indicate that one of the conditions is fact
        if (fact) {
            jt.value().clear();
        } else if (jns || !jt.value().empty()) {
            jt.value().emplace_back(cond_id);
        }
    }
}

auto StateAssignAggr::atom_index(AtomMap::iterator it) -> size_t {
    return it - base_.atoms().begin();
}

void StateAssignAggr::print(std::ostream &out, bool print_index) {
    out << fun_ << "(" << Util::p_range(global_, [](std::ostream &out, auto var) { out << "X_" << var; }) << ")";
    if (print_index && index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
    out << " = " << *term_;
}

auto StateAssignAggr::base() -> BaseAssignAggr & {
    return base_;
}

void StateAssignAggr::output([[maybe_unused]] Logger &log, [[maybe_unused]] SymbolStore &store, OutputStm &out) {
    std::vector<std::pair<SymbolSpan, std::span<size_t const>>> elems;
    std::vector<std::pair<Relation, Symbol>> guards;
    for (auto const &[key, uid] : base_.derived()) {
        if (uid != invalid_offset) {
            auto &atom = base_.atoms().nth(key.first).value();
            elems.clear();
            guards.clear();
            for (auto const &elem_idx : atom.elems()) {
                auto const &[tuple, conds] = *tuples_.nth(elem_idx);
                elems.emplace_back(tuple->span(), conds);
            }
            guards.emplace_back(Relation::equal, key.second);
            out.bd_aggr(uid, fun_, elems, guards);
        }
    }
}

// definition of MatchAssignAggr

auto MatchAssignAggr::vars() const -> VariableSet {
    VariableSet res{state_->global().begin(), state_->global().end()};
    state_->term().vars(res);
    return res;
}

auto MatchAssignAggr::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const
    -> VariableVec {
    static_cast<void>(this);
    return {bound.begin(), bound.end()};
}

auto MatchAssignAggr::match(EvalContext const &ctx, Key key) const -> bool {
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

auto MatchAssignAggr::eval(EvalContext const &ctx) const -> std::optional<Key> {
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

auto operator<<(std::ostream &out, MatchAssignAggr const &m) -> std::ostream & {
    m.state_->print(out, false);
    return out;
}

auto MatchAssignAggr::state() const -> StateAssignAggr & {
    return *state_;
}

// definition of LitAssignAggr

void LitAssignAggr::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::depend) {
        vars.insert(state().global().begin(), state().global().end());
        state().term().vars(vars);
    }
}

auto LitAssignAggr::do_domain() const -> bool {
    return state().domain_elems() && state().single_pass_elems();
}

auto LitAssignAggr::do_single_pass() const -> bool {
    return state().index() == stratified_index || state().single_pass_elems();
}

auto LitAssignAggr::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                               std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    offset_ = invalid_offset;
    auto &match = static_cast<MatchAssignAggr &>(*this);
    auto index = std::optional<size_t>{};
    if (state().index() != stratified_index && type == MatcherType::new_atoms) {
        index = state().index();
    }
    return {make_atom_matcher(mbr, bound, state().base(), match, type, offset_), index};
}

auto LitAssignAggr::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    // Note: at the time of score computation the aggregate is still empty.
    // Since we decided to split earlier, matching them should always be
    // better than using their body prefix.
    return 0;
}

void LitAssignAggr::do_print(std::ostream &out) const {
    state().print(out, true);
}

auto LitAssignAggr::do_output([[maybe_unused]] EvalContext const &ctx, OutputLit &out) const -> bool {
    if (domain()) {
        return false;
    }
    auto &base = state().base();
    auto it = base.nth(offset_);
    auto jt = base.atoms().nth(it.key().first);
    auto const &state_aggr = jt.value();
    if (state().single_pass_elems() && state_aggr.is_fact()) {
        return false;
    }
    auto &state_elem = it.value();
    state_elem = out.bd_aggr(Sign::none, state_elem != invalid_offset ? std::make_optional(state_elem) : std::nullopt);
    return true;
}

auto LitAssignAggr::do_copy() const -> ULit {
    return std::make_unique<LitAssignAggr>(state());
}

auto LitAssignAggr::do_hash() const -> size_t {
    // NOLINTNEXTLINE
    return Util::value_hash_record<LitAssignAggr>(reinterpret_cast<uintptr_t>(this));
}

auto LitAssignAggr::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitAssignAggr::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

// definition of LitAssignAggr

auto StmAssignAggrElem::do_body() const -> ULitVec const & {
    return body_;
}

auto StmAssignAggrElem::do_important() const -> VariableSet {
    auto res = VariableSet{};
    res.insert(state_->global().begin(), state_->global().end());
    for (auto const &term : tuple_) {
        term->vars(res);
    }
    return res;
}

auto StmAssignAggrElem::do_is_important(size_t index) const -> bool {
    // Only the literals gathered by do_important and the ones in the
    // condition are important. The remaining ones in the body can be
    // backtracked.
    return index < num_cond_;
}

void StmAssignAggrElem::do_init(size_t gen) {
    state_->base().ensure(gen);
}

auto StmAssignAggrElem::get_cond_(EvalContext const &ctx) -> std::pair<size_t, bool> {
    bool fact = true;
    auto &out = ctx.out().cond();
    for (auto const &lit : std::span{body_}.subspan(0, num_cond_)) {
        if (lit->output(ctx, out)) {
            fact = false;
        }
    }
    return {ctx.out().cond_id(), fact};
}

auto StmAssignAggrElem::do_report(EvalContext const &ctx) -> bool {
    auto it = state_->insert_atom(ctx).first;
    state_->insert_elem(ctx, it, *this);
    return true;
}

void StmAssignAggrElem::do_propagate(SymbolStore &store, [[maybe_unused]] OutputStm &out, Queue &queue) {
    // This is called after all statements on the current priority have
    // been processed. Thus, all element aggregation rules have been
    // processed. Here, aggregates that can match are added to the base and
    // are enqueued.
    if (state_->propagate(store) && state_->index() != stratified_index) {
        queue.propagate(state_->index());
    }
}

auto StmAssignAggrElem::do_priority() const -> size_t {
    return priority_;
}

void StmAssignAggrElem::do_print_head(std::ostream &out) const {
    auto p_var = [](std::ostream &out, auto const &x) { out << "X_" << x; };
    auto p_term = [](std::ostream &out, auto const &x) { out << *x; };
    out << "#elem(g(" << Util::p_range(state_->global(), p_var) << "),t(" << Util::p_range(tuple_, p_term) << "))";
}

void StmAssignAggrElem::do_print(std::ostream &out) const {
    out << priority_ << ": ";
    print_head(out);
    if (state_->index() != stratified_index) {
        out << "[" << state_->index() << "]";
    }
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

// definition of LitAssignAggrStrat

namespace {

class MatcherAssignAggrStrat : public Matcher {
  public:
    MatcherAssignAggrStrat(StateAssignAggr &state, std::vector<Instantiator> insts, UMatcher matcher)
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

    StateAssignAggr *state_;
    InstantiatorVec insts_;
    UMatcher matcher_;
};

} // namespace

void LitAssignAggrStrat::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        vars.insert(state().global().begin(), state().global().end());
    }
    if (mode != VarSelectMode::depend) {
        state().term().vars(vars);
    }
}

auto LitAssignAggrStrat::do_domain() const -> bool {
    assert(state().single_pass_elems());
    return state().domain_elems();
}

auto LitAssignAggrStrat::do_single_pass() const -> bool {
    assert(state().single_pass_elems());
    return true;
}

auto LitAssignAggrStrat::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                    std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    offset_ = invalid_offset;
    auto lin = Linearizer{mbr};
    auto queue = Queue{};
    lin.start(queue);
    for (auto &elem : elems_) {
        lin.prepare(elem, elem.body(), elem.important());
    }
    auto &match = static_cast<MatchAssignAggr &>(*this);
    return {std::make_unique<MatcherAssignAggrStrat>(
                state(), queue.release(), make_atom_matcher(mbr, bound, state().base(), match, type, offset_)),
            std::nullopt};
}

auto LitAssignAggrStrat::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    // Note: at the time of score computation the aggregate is still empty.
    // Since we decided to split earlier, matching them should always be
    // better than using their body prefix.
    return domain() ? 0 : std::numeric_limits<double>::max();
}

void LitAssignAggrStrat::do_print(std::ostream &out) const {
    state().print(out, true);
}

auto LitAssignAggrStrat::do_output([[maybe_unused]] EvalContext const &ctx, OutputLit &out) const -> bool {
    assert(state().single_pass_elems());
    if (domain()) {
        return false;
    }
    auto &base = state().base();
    auto it = base.nth(offset_);
    auto jt = base.atoms().nth(it.key().first);
    auto const &state_aggr = jt.value();
    if (state_aggr.is_fact()) {
        return false;
    }
    auto &state_elem = it.value();
    state_elem = out.bd_aggr(Sign::none, state_elem != invalid_offset ? std::make_optional(state_elem) : std::nullopt);
    return true;
}

auto LitAssignAggrStrat::do_copy() const -> ULit {
    return std::make_unique<LitAssignAggrStrat>(state(), elems_);
}

auto LitAssignAggrStrat::do_hash() const -> size_t {
    // NOLINTNEXTLINE
    return Util::value_hash_record<LitAssignAggrStrat>(reinterpret_cast<uintptr_t>(this));
}

auto LitAssignAggrStrat::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitAssignAggrStrat::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

} // namespace CppClingo::Ground
