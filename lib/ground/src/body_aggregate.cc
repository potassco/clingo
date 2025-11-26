#include <clingo/ground/body_aggregate.hh>

#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>

// #define DEBUG_AGGR
#ifdef DEBUG_AGGR
#include <iostream>
#endif

namespace CppClingo::Ground {

// definition of AtomAggr

void AtomBdAggr::accumulate(AggregateFunction fun, SymbolSpan tup, bool fact) {
    assert(fun != AggregateFunction::count);
    if (!tup.empty()) {
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
        } else if (tup.front().type() == SymbolType::number) {
            auto const &num = tup.front().num();
            if (fun == AggregateFunction::sum || num > 0) {
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

[[nodiscard]] auto AtomBdAggr::state() const -> AtomBdAggrState {
    return state_;
}

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

void AtomBdAggr::add_elem(size_t idx) {
    elems_.emplace_back(idx);
}

auto AtomBdAggr::elems() const -> std::span<size_t const> {
    return std::span{elems_.begin(), elems_.end()};
}

auto AtomBdAggr::todo() -> std::span<size_t const> {
    return std::span{elems_.begin() + static_cast<std::ptrdiff_t>(propagated_), elems_.end()};
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
    // This function is called during propagate where the state is set to
    // something other than unknown.
    assert(it->second.state() != AtomBdAggrState::unknown);
    it.value().derived_idx(derived_.size());
    derived_.add(static_cast<size_t>(std::distance(atoms_.begin(), it)));
}

auto BaseBdAggr::size() const -> size_t {
    return derived_.size();
}

auto BaseBdAggr::index(Symbol const *sym) const -> size_t {
    if (auto it = atoms_.find(sym); it != atoms_.end() && it->second.state() != AtomBdAggrState::unknown) {
        return it->second.derived_idx();
    }
    return size();
}

auto BaseBdAggr::nth(size_t i) const -> AtomMap::const_iterator {
    return atoms_.nth(derived_[i]);
}

auto BaseBdAggr::nth(size_t i) -> AtomMap::iterator {
    return atoms_.nth(derived_[i]);
}

auto BaseBdAggr::atoms() -> AtomMap & {
    return atoms_;
}

// definition of StateAggr

// NOLINTBEGIN

//! Helper class to construct symbols arrays identifying aggregate atoms.
//!
//! The size of the array corresponds to the number of global variables and is
//! not stored here. The symbols are stored using a monotonic buffer resource.
class StateBdAggr::AtomKey {
  private:
    struct priv_tag {};

  public:
    //! Construct the atom key.
    AtomKey([[maybe_unused]] priv_tag tag, EvalContext const &ctx, VariableVec const &global, GuardVec &guards,
            bool &res) {
        auto *it = syms_;
        for (auto const &var : global) {
            *it++ = ctx.ass()[var].value();
        }
        for (auto const &guard : guards) {
            if (auto val = guard.second->eval(ctx); val) {
                *it++ = *val;
            } else {
                return;
            }
        }
        res = true;
    }
    //! Construct the atom key.
    AtomKey([[maybe_unused]] priv_tag tag, Symbol const *tuple, size_t n) { std::copy_n(tuple, n, syms_); }

    //! Construct an atom key from the global variables and guards.
    //!
    //! Returns false if evaluating the guards fails.
    static auto construct(auto &mbr, EvalContext const &ctx, VariableVec const &global, GuardVec &guards,
                          AtomKey *&target) -> bool {
        if (target == nullptr) {
            auto n = (global.size() + guards.size()) * sizeof(Symbol);
            target = static_cast<AtomKey *>(mbr.allocate(n, alignof(AtomKey)));
        } else {
            std::destroy_at(target);
        }
        bool res = false;
        std::construct_at(target, priv_tag{}, ctx, global, guards, res);
        return res;
    }
    //! Construct an atom key from the given symbols.
    //!
    //! This function might create keys for atoms without definitions, which
    //! can happen for negated atoms potentially derived later on.
    static void construct(auto &mbr, Symbol const *tuple, size_t n, AtomKey *&target) {
        if (target == nullptr) {
            target = static_cast<AtomKey *>(mbr.allocate(n * sizeof(Symbol), alignof(AtomKey)));
        } else {
            std::destroy_at(target);
        }
        std::construct_at(target, priv_tag{}, tuple, n);
    }

    //! Return the symbols representing the atom key.
    auto syms() -> Symbol const * { return syms_; }

  private:
    CLINGO_IGNORE_ZERO_SIZED_ARRAY_B
    Symbol syms_[0];
    CLINGO_IGNORE_ZERO_SIZED_ARRAY_E
};

StateBdAggr::ElementKey::ElementKey([[maybe_unused]] priv_tag tag, EvalContext const &ctx, AggregateFunction fun,
                                    size_t atom_idx, StmBdAggrElem &elem, bool &res)
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
        // evaluate the remaining terms
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

auto StateBdAggr::ElementKey::construct(auto &mbr, EvalContext const &ctx, AggregateFunction fun, size_t atom_idx,
                                        StmBdAggrElem &elem) -> bool {
    auto n = sizeof(ElementKey) + elem.tuple_.size() * sizeof(Symbol);
    if (elem.elem_key_ == nullptr) {
        elem.elem_key_ = static_cast<ElementKey *>(mbr.allocate(n, alignof(ElementKey)));
    } else {
        std::destroy_at(elem.elem_key_);
    }
    bool res = false;
    std::construct_at(elem.elem_key_, priv_tag{}, ctx, fun, atom_idx, elem, res);
    return res;
}

auto StateBdAggr::ElementKey::span() const -> SymbolSpan {
    return SymbolSpan{syms_, n_};
}

auto StateBdAggr::ElementKey::hash() const -> size_t {
    return Util::value_hash_record<ElementKey>(n_, atom_idx_, span());
}

auto operator==(StateBdAggr::ElementKey const &a, StateBdAggr::ElementKey const &b) -> bool {
    return a.atom_idx_ == b.atom_idx_ && a.n_ == b.n_ && std::equal(a.span().begin(), a.span().end(), b.span().begin());
}

// NOLINTEND

auto StateBdAggr::global() const -> VariableVec const & {
    return global_;
}

auto StateBdAggr::symbols() -> SymbolVec & {
    symbols_.resize(global_.size());
    return symbols_;
}

auto StateBdAggr::guards() const -> GuardVec const & {
    return guards_;
}

auto StateBdAggr::fun() const -> AggregateFunction {
    return fun_;
}

auto StateBdAggr::domain() const -> bool {
    return domain_;
}

auto StateBdAggr::monotone() const -> bool {
    return monotone_;
}

auto StateBdAggr::single_pass_elems() const -> bool {
    return single_pass_elems_;
}

auto StateBdAggr::index() const -> size_t {
    return index_;
}

auto StateBdAggr::propagate() -> bool {
    bool res = false;
    for (auto atom_idx : queue_) {
        auto it = base_.atoms().nth(atom_idx);
        auto &state = it.value();
        for (auto elem_idx : state.todo()) {
            auto elem = tuples_.nth(elem_idx);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            state.accumulate(fun_, elem.key()->span(), elem->second.empty());
#ifdef DEBUG_AGGR
            std::cerr << "accumulate: a: " << atom_idx << " e: " << elem_idx << " t:";
            for (auto val : elem.key()->span()) {
                std::cerr << " " << val;
            }
            if (elem->second.empty()) {
                std::cerr << " [f]";
            }
            std::cerr << "\n";
#endif
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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

void StateBdAggr::enqueue_(AtomMap::iterator it) {
    if (auto &state = it.value(); state.enqueue()) {
        queue_.emplace_back(atom_index_(it));
    }
}

auto StateBdAggr::insert_atom(EvalContext const &ctx) -> std::optional<std::pair<AtomMap::iterator, bool>> {
    if (AtomKey::construct(*mbr_, ctx, global_, guards_, atom_key_)) {
        auto [it, ins] = base_.atoms().try_emplace(atom_key_->syms(), fun_);
        if (ins) {
            atom_key_ = nullptr;
            enqueue_(it);
        }
        return std::make_optional<std::pair<AtomMap::iterator, bool>>(it, ins);
    }
    return std::nullopt;
}

auto StateBdAggr::insert_atom(Symbol const *tuple) -> AtomMap::iterator {
    AtomKey::construct(*mbr_, tuple, global_.size() + guards_.size(), atom_key_);
    auto [it, ins] = base_.atoms().try_emplace(atom_key_->syms(), fun_);
    if (ins) {
        atom_key_ = nullptr;
    }
    return it;
}

void StateBdAggr::insert_elem(EvalContext const &ctx, AtomMap::iterator it, StmBdAggrElem &elem) {
    if (ElementKey::construct(*mbr_, ctx, fun_, atom_index_(it), elem)) {
        auto [jt, jns] = tuples_.try_emplace(elem.elem_key_);
        if (jns) {
            elem.elem_key_ = nullptr;
            it.value().add_elem(jt - tuples_.begin());
            enqueue_(it);
        }

        auto [cond_id, fact] = elem.get_cond_(ctx);
        // we use an empty vector to indicate that one of the conditions is fact
        if (fact) {
            jt.value().clear();
        } else if (jns || !jt.value().empty()) {
            auto &cond = jt.value();
            cond.emplace_back(cond_id);
            std::ranges::sort(cond);
            cond.erase(std::ranges::unique(cond).begin(), cond.end());
        }
    }
}

auto StateBdAggr::atom_index_(AtomMap::iterator it) -> size_t {
    return it - base_.atoms().begin();
}

void StateBdAggr::print(std::ostream &out, bool print_index) {
    auto it = guards_.begin();
    if (guards_.size() > 1) {
        out << *it->second << " " << flip(it->first) << " ";
        ++it;
    }
    out << fun_ << "(" << Util::p_range(global_, [](std::ostream &out, auto var) { out << "X_" << var; }) << ")";
    if (print_index && index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
    for (auto ie = guards_.end(); it != ie; ++it) {
        out << " " << it->first << " " << *it->second;
    }
}

auto StateBdAggr::base() -> BaseBdAggr & {
    return base_;
}

void StateBdAggr::output([[maybe_unused]] Logger &log, [[maybe_unused]] SymbolStore &store, OutputStm &out) {
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
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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

auto MatchBdAggr::vars() const -> VariableSet {
    return VariableSet{state_->global().begin(), state_->global().end()};
}

auto MatchBdAggr::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
    static_cast<void>(this);
    return {bound.begin(), bound.end()};
}

auto MatchBdAggr::match(EvalContext const &ctx, Symbol const *sym) const -> bool {
    for (auto var : state_->global()) {
        if (auto &opt = ctx.ass()[var]; opt) {
            if (*opt != *sym) {
                return false;
            }
        } else {
            ctx.ass()[var] = *sym;
        }
        sym = std::next(sym);
    }
    return true;
}

auto MatchBdAggr::eval(EvalContext const &ctx) const -> std::optional<Symbol const *> {
    eval_.clear();
    for (auto var : state_->global()) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        eval_.emplace_back(ctx.ass()[var].value());
    }
    for (auto const &guard : state().guards()) {
        if (auto sym = guard.second->eval(ctx); sym) {
            eval_.emplace_back(*sym);
        } else {
            return std::nullopt;
        }
    }
    return eval_.data();
}

auto MatchBdAggr::state() const -> StateBdAggr & {
    return *state_;
}

auto operator<<(std::ostream &out, MatchBdAggr const &m) -> std::ostream & {
    m.state_->print(out, false);
    return out;
}

// definition of LitBdAggr

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

auto LitBdAggr::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type, std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
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
    return {make_atom_matcher(mbr, bound, state().base(), match, type, offset_), index};
}

auto LitBdAggr::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    // Note: at the time of score computation the aggregate is still empty.
    // Since we decided to split earlier, matching them should always be
    // better than using their body prefix.
    return 0;
}

void LitBdAggr::do_print(std::ostream &out) const {
    state().print(out, true);
}

auto LitBdAggr::do_output([[maybe_unused]] EvalContext const &ctx, OutputLit &out) const -> bool {
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

auto LitBdAggr::do_copy() const -> ULit {
    return std::make_unique<LitBdAggr>(state(), sign_);
}

auto LitBdAggr::do_hash() const -> size_t {
    // NOLINTNEXTLINE
    return Util::value_hash_record<LitBdAggr>(reinterpret_cast<uintptr_t>(this));
}

auto LitBdAggr::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitBdAggr::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

// definition of StmAggrElem

auto StmBdAggrElem::do_body() const -> ULitVec const & {
    return body_;
}

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

void StmBdAggrElem::do_init(size_t gen) {
    state_->base().ensure(gen);
}

auto StmBdAggrElem::get_cond_(EvalContext const &ctx) -> std::pair<size_t, bool> {
    bool fact = true;
    auto &out = ctx.out().cond();
    for (auto const &lit : std::span{body_}.subspan(0, num_cond_)) {
        if (lit->output(ctx, out)) {
            fact = false;
        }
    }
    return {ctx.out().cond_id(), fact};
}

auto StmBdAggrElem::do_report(EvalContext const &ctx) -> bool {
    if (auto it = state_->insert_atom(ctx)) {
        state_->insert_elem(ctx, it->first, *this);
    }
    return true;
}

void StmBdAggrElem::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] OutputStm &out, Queue &queue) {
    // This is called after all statements on the current priority have
    // been processed. Thus, all element aggregation rules have been
    // processed. Here, aggregates that can match are added to the base and
    // are enqueued.
    if (state_->propagate() && state_->index() != stratified_index) {
        queue.propagate(state_->index());
    }
}

auto StmBdAggrElem::do_priority() const -> size_t {
    return priority_;
}

void StmBdAggrElem::do_print_head(std::ostream &out) const {
    auto p_var = [](std::ostream &out, auto const &x) { out << "X_" << x; };
    auto p_term = [](std::ostream &out, auto const &x) { out << *x; };
    out << "#elem(g(" << Util::p_range(state_->global(), p_var) << "),t(" << Util::p_range(tuple_, p_term) << "))";
}

void StmBdAggrElem::do_print(std::ostream &out) const {
    out << priority_ << ": ";
    print_head(out);
    if (state_->index() != stratified_index) {
        out << "[" << state_->index() << "]";
    }
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

// definition of LitBdAggrStrat

namespace {

class MatcherBdAggrStrat : public OnceMatcher {
  public:
    MatcherBdAggrStrat(StateBdAggr &state, std::vector<Instantiator> insts, size_t &offset, bool positive)
        : state_{&state}, insts_{std::move(insts)}, offset_{&offset}, positive_{positive} {}

  private:
    void do_init(InstantiationContext const &ctx, size_t gen) override {
        for (auto &inst : insts_) {
            inst.init(ctx, gen);
        }
    }
    auto do_once(EvalContext const &ctx) -> bool override {
        if (auto it = state_->insert_atom(ctx)) {
            *offset_ = it->first - state_->base().atoms().begin();
            if (it->second) {
                // bind global variables
                auto &ass = ctx.ass();
                auto jt = state_->symbols().begin();
                for (auto const &var : state_->global()) {
                    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                    *jt++ = *ass[var];
                }
                // ground elems
                CLINGO_REPORT(ctx.log(), trace) << "<<< begin nested instantiation";
                for (auto &inst : insts_) {
                    std::ignore = inst.instantiate(ctx.log(), ctx.store(), ctx.out(), nullptr);
                }
                CLINGO_REPORT(ctx.log(), trace) << ">>> end nested instantiation";
                // propagate aggregate
                std::ignore = state_->propagate();
                // ensure that base comprises all atoms
                // (note that the call could be omitted as well)
                state_->base().update(0);
            }
            return it->first.value().state() != (positive_ ? AtomBdAggrState::unknown : AtomBdAggrState::fact);
        }
        return false;
    }
    void do_print(std::ostream &out) const override { state_->print(out, false); }

    StateBdAggr *state_;
    InstantiatorVec insts_;
    size_t *offset_;
    bool positive_;
};

} // namespace

void LitBdAggrStrat::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        vars.insert(state_->global().begin(), state_->global().end());
    }
}

auto LitBdAggrStrat::do_domain() const -> bool {
    assert(state_->single_pass_elems());
    return state_->domain();
}

auto LitBdAggrStrat::do_single_pass() const -> bool {
    assert(state_->single_pass_elems());
    return true;
}

auto LitBdAggrStrat::do_matcher(std::pmr::monotonic_buffer_resource &mbr, [[maybe_unused]] MatcherType type,
                                [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    auto lin = Linearizer{mbr};
    auto queue = Queue{};
    lin.start(queue);
    for (auto &elem : elems_) {
        lin.prepare(elem, elem.body(), elem.important());
    }
    return {std::make_unique<MatcherBdAggrStrat>(*state_, queue.release(), offset_, sign_ != Sign::once), std::nullopt};
}

auto LitBdAggrStrat::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    // Note: grounding the aggregate might be expensive. Maybe implement a
    // better estimate. An estimate is possible because elements are
    // stratified.
    // NOLINTNEXTLINE(readability-magic-numbers)
    return 100;
}

void LitBdAggrStrat::do_print(std::ostream &out) const {
    state_->print(out, true);
}

auto LitBdAggrStrat::do_output([[maybe_unused]] EvalContext const &ctx, OutputLit &out) const -> bool {
    if (domain()) {
        return false;
    }
    assert(offset_ != invalid_offset);
    // Note: here the atom index and not the derived index can be used
    auto it = state_->base().atoms().nth(offset_);
    if (it.value().state() == (sign_ != Sign::once ? AtomBdAggrState::fact : AtomBdAggrState::unknown)) {
        return false;
    }
    auto &state = it.value();
    state.uid(out.bd_aggr(sign_, state.uid()));
    return true;
}

auto LitBdAggrStrat::do_copy() const -> ULit {
    return std::make_unique<LitBdAggrStrat>(*state_, elems_, sign_);
}

auto LitBdAggrStrat::do_hash() const -> size_t {
    // NOLINTNEXTLINE
    return Util::value_hash_record<LitBdAggrStrat>(reinterpret_cast<uintptr_t>(this));
}

auto LitBdAggrStrat::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitBdAggrStrat::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

} // namespace CppClingo::Ground
