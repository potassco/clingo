#include <clingo/ground/head_aggregate.hh>

#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>

#include <functional>

// #define DEBUG_AGGR
#ifdef DEBUG_AGGR
#include <iostream>
#endif

namespace CppClingo::Ground {

// definition of AtomAggr

void AtomHdAggr::accumulate(AggregateFunction fun, SymbolSpan tup, bool fact) {
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

auto AtomHdAggr::propagate(GuardVec const &guards, Symbol const *vals) -> bool {
    if (matched_) {
        return true;
    }
    const auto *it = vals;
    for (auto const &guard : guards) {
        auto rel = guard.first;
        auto res = std::visit(
            [it, rel]<class T>(T const &x) -> bool {
                switch (rel) {
                    case Relation::less: {
                        return x.first < *it;
                    }
                    case Relation::less_equal: {
                        return x.first <= *it;
                    }
                    case Relation::greater: {
                        return x.second > *it;
                    }
                    case Relation::greater_equal: {
                        return x.second >= *it;
                    }
                    case Relation::equal: {
                        return x.first <= *it && *it <= x.second;
                    }
                    case Relation::not_equal: {
                        return *it != x.first || *it != x.second;
                    }
                }
                Util::unreachable();
            },
            bound_);
        if (!res) {
            return false;
        }
        it = std::next(it);
    }
    matched_ = true;
    return true;
}

auto AtomHdAggr::enqueue() -> bool {
    if (!enqueued_ && propagated_ < elems_.size()) {
        enqueued_ = true;
        return true;
    }
    return false;
}

void AtomHdAggr::dequeue() {
    assert(enqueued_);
    propagated_ = elems_.size();
    enqueued_ = false;
}

void AtomHdAggr::add_elem(size_t idx) {
    elems_.emplace_back(idx);
}

auto AtomHdAggr::elems() const -> std::span<size_t const> {
    return std::span{elems_.begin(), elems_.end()};
}

auto AtomHdAggr::todo() -> std::span<size_t const> {
    return std::span{elems_.begin() + static_cast<ssize_t>(propagated_), elems_.end()};
}

auto AtomHdAggr::uid() const -> std::optional<size_t> {
    return uid_ != invalid_offset ? std::make_optional(uid_) : std::nullopt;
}

void AtomHdAggr::uid(size_t uid) {
    assert(uid_ == invalid_offset || uid_ == uid);
    uid_ = uid;
}

auto AtomHdAggr::init_(AggregateFunction fun) -> Bound {
    if (fun == AggregateFunction::min) {
        return Bound{std::in_place_index<1>, SymbolStore::sup(), SymbolStore::sup()};
    }
    if (fun == AggregateFunction::max) {
        return Bound{std::in_place_index<1>, SymbolStore::inf(), SymbolStore::inf()};
    }
    return Bound{std::in_place_index<0>, 0, 0};
}

// definition of BaseHdAggr

auto BaseHdAggr::add(Symbol const *sym, AggregateFunction fun) -> std::pair<AtomMap::iterator, bool> {
    return atoms_.try_emplace(sym, fun);
}

auto BaseHdAggr::size() const -> size_t {
    return atoms_.size();
}

auto BaseHdAggr::index(Symbol const *sym) const -> size_t {
    return atoms_.find(sym) - atoms_.begin();
}

auto BaseHdAggr::nth(size_t i) const -> AtomMap::const_iterator {
    return atoms_.nth(i);
}

auto BaseHdAggr::nth(size_t i) -> AtomMap::iterator {
    return atoms_.nth(i);
}

auto BaseHdAggr::atoms() -> AtomMap & {
    return atoms_;
}

// definition of StateAggr

// NOLINTBEGIN

//! Helper class to construct symbols arrays identifying aggregate atoms.
//!
//! The size of the array corresponds to the number of global variables and is
//! not stored here. The symbols are stored using a monotonic buffer resource.
class StateHdAggr::AtomKey {
  private:
    struct priv_tag {};

  public:
    //! Private constructor.
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

    //! Return the symbols representing the atom key.
    auto syms() -> Symbol const * { return syms_; }

  private:
    CLINGO_IGNORE_ZERO_SIZED_ARRAY_B
    Symbol syms_[0];
    CLINGO_IGNORE_ZERO_SIZED_ARRAY_E
};

StateHdAggr::ElementKey::ElementKey([[maybe_unused]] priv_tag tag, EvalContext const &ctx, AggregateFunction fun,
                                    size_t atom_idx, StmHdAggrElem &elem, bool &res)
    : n_{elem.tuple_.size() << 1}, atom_idx_{atom_idx} {
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

auto StateHdAggr::ElementKey::construct(auto &mbr, EvalContext const &ctx, AggregateFunction fun, size_t atom_idx,
                                        StmHdAggrElem &elem) -> bool {
    if (elem.elem_key_ == nullptr) {
        auto n = sizeof(ElementKey) + elem.tuple_.size() * sizeof(Symbol);
        elem.elem_key_ = static_cast<ElementKey *>(mbr.allocate(n, alignof(ElementKey)));
    } else {
        std::destroy_at(elem.elem_key_);
    }
    bool res = false;
    std::construct_at(elem.elem_key_, priv_tag{}, ctx, fun, atom_idx, elem, res);
    return res;
}

void StateHdAggr::ElementKey::mark_fact() const {
    n_ |= 1;
}

auto StateHdAggr::ElementKey::fact() const -> bool {
    return n_ & 1;
}

auto StateHdAggr::ElementKey::size() const -> size_t {
    return n_ >> 1;
}

auto StateHdAggr::ElementKey::span() const -> SymbolSpan {
    return SymbolSpan{syms_, size()};
}

auto StateHdAggr::ElementKey::hash() const -> size_t {
    return Util::value_hash_record<ElementKey>(size(), atom_idx_, span());
}

auto operator==(StateHdAggr::ElementKey const &a, StateHdAggr::ElementKey const &b) -> bool {
    return a.atom_idx_ == b.atom_idx_ && a.size() == b.size() &&
           std::equal(a.span().begin(), a.span().end(), b.span().begin());
}

// NOLINTEND

auto StateHdAggr::global() const -> VariableVec const & {
    return global_;
}

auto StateHdAggr::symbols() -> SymbolVec & {
    symbols_.resize(global_.size());
    return symbols_;
}

auto StateHdAggr::guards() const -> GuardVec const & {
    return guards_;
}

auto StateHdAggr::fun() const -> AggregateFunction {
    return fun_;
}

auto StateHdAggr::single_pass_body() const -> bool {
    return single_pass_body_;
}

auto StateHdAggr::index() const -> size_t {
    return index_;
}

auto StateHdAggr::indices() const -> std::vector<size_t> {
    std::vector<size_t> res;
    for (auto const &[sig, base, indices] : bases_) {
        res.insert(res.end(), indices.begin(), indices.end());
    }
    std::ranges::sort(res);
    res.erase(std::ranges::unique(res).begin(), res.end());
    return res;
}

auto StateHdAggr::base() -> BaseHdAggr & {
    return base_;
}

void StateHdAggr::enqueue(Queue &queue) {
    if (index_ != stratified_index && base_.has_update()) {
        queue.propagate(index_);
    }
}

void StateHdAggr::propagate(OutputStm &out, Queue &queue) {
    // process enqueued atoms
    for (auto atom_idx : queue_) {
        auto it = base_.nth(atom_idx);
        auto &state = it.value();
        // accumulate the elements
        for (auto elem_idx : state.todo()) {
            auto elem = tuples_.nth(elem_idx);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            state.accumulate(fun_, elem.key()->span(), elem.key()->fact());
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
        // propagate the elements
        if (state.propagate(guards_, it.key() + global_.size())) {
            for (auto elem_idx : state.todo()) {
                for (auto &[sym, head, cond] : tuples_.nth(elem_idx).value()) {
                    if (auto sig = sym.signature()) {
                        auto it =
                            std::ranges::lower_bound(bases_, *sig, std::less<>{},
                                                     [](auto const &a) -> decltype(auto) { return std::get<0>(a); });
                        assert(it != bases_.end());
                        auto *base = std::get<1>(*it);
                        head = base->add(sym, StateAtom::derived, [&out]() { return out.uid(); }).first.value().id;
                    }
                }
            }
#ifdef DEBUG_AGGR
            std::cerr << "propagate: a: " << atom_idx << "\n";
#endif
        }
        state.dequeue();
    }
    queue_.clear();
    // propagate modified bases
    for (auto const &[sig, base, indices] : bases_) {
        if (!indices.empty() && base->has_update()) {
            for (auto const &index : indices) {
                queue.propagate(index);
            }
        }
    }
}

void StateHdAggr::enqueue_(AtomMap::iterator it) {
    if (auto &state = it.value(); state.enqueue()) {
        queue_.emplace_back(atom_index_(it));
    }
}

auto StateHdAggr::insert_atom(EvalContext const &ctx) -> std::optional<std::pair<AtomMap::iterator, bool>> {
    if (AtomKey::construct(*mbr_, ctx, global_, guards_, atom_key_)) {
        auto [it, ins] = base_.add(atom_key_->syms(), fun_);
        if (ins) {
            atom_key_ = nullptr;
            enqueue_(it);
        }
        return std::make_optional<std::pair<AtomMap::iterator, bool>>(it, ins);
    }
    return std::nullopt;
}

void StateHdAggr::insert_elem(EvalContext const &ctx, AtomMap::iterator it, StmHdAggrElem &elem) {
    auto sym = SymbolStore::sup();
    if (elem.head_ != nullptr) {
        if (auto opt = elem.head_->eval(ctx)) {
            sym = *opt;
        } else {
            return;
        }
    }
    if (ElementKey::construct(*mbr_, ctx, fun_, atom_index_(it), elem)) {
        auto [jt, jns] = tuples_.try_emplace(elem.elem_key_);
        if (jns) {
            elem.elem_key_ = nullptr;
            it.value().add_elem(jt - tuples_.begin());
            enqueue_(it);
        }

        auto [cond_id, fact] = elem.get_cond_(ctx);
        if (elem.head_ == nullptr && fact) {
            jt.key()->mark_fact();
        }
        auto &cond = jt.value();
        cond.emplace_back(sym, 0, cond_id);
        // Note: we prefer conditions that already have a literal
        auto lt = [](auto &a, auto &b) {
            if (auto cmp = get<0>(a) <=> get<0>(b); cmp != 0) {
                return cmp < 0;
            }
            if (auto cmp = get<2>(a) <=> get<2>(b); cmp != 0) {
                return cmp < 0;
            }
            return get<1>(b) < get<1>(a);
        };
        auto eq = [](auto &a, auto &b) { return get<0>(a) == get<0>(b) && get<2>(a) == get<2>(b); };
        std::ranges::sort(cond, lt);
        cond.erase(std::ranges::unique(cond, eq).begin(), cond.end());
    }
}

auto StateHdAggr::atom_index_(AtomMap::iterator it) -> size_t {
    return it - base_.atoms().begin();
}

void StateHdAggr::print(std::ostream &out, bool print_index) {
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

void StateHdAggr::output([[maybe_unused]] Logger &log, [[maybe_unused]] SymbolStore &store, OutputStm &out) {
    std::vector<std::pair<SymbolSpan, std::span<std::tuple<Symbol, size_t, size_t> const>>> elems;
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
            out.hd_aggr(*uid, fun_, elems, guards);
        }
    }
}

// definition of StmHdAggr

auto StmHdAggr::do_body() const -> ULitVec const & {
    return body_;
}

auto StmHdAggr::do_important() const -> VariableSet {
    auto res = VariableSet{};
    res.insert(state_->global().begin(), state_->global().end());
    return res;
}

void StmHdAggr::do_init(size_t gen) {
    state_->base().ensure(gen);
}

auto StmHdAggr::do_report(EvalContext const &ctx) -> bool {
    if (auto res = state_->insert_atom(ctx)) {
        auto &aggr = res->first.value();
        auto &out = ctx.out().body();
        for (auto const &lit : body_) {
            std::ignore = lit->output(ctx, out);
        }
        aggr.uid(ctx.out().aggr_rule(aggr.uid()));
    }
    // Note: in principle, it would be possible to detect false head aggregates
    // in the stratified case.
    return true;
}

void StmHdAggr::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] OutputStm &out, Queue &queue) {
    // enqueue the aggregate element statements for propagation
    state_->enqueue(queue);
}

auto StmHdAggr::do_priority() const -> size_t {
    return priority_;
}

void StmHdAggr::do_print_head(std::ostream &out) const {
    state_->print(out, false);
}

void StmHdAggr::do_print(std::ostream &out) const {
    out << priority_ << ": ";
    state_->print(out, true);
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

// definition of StmHdAggrElem

auto StmHdAggrElem::do_body() const -> ULitVec const & {
    return body_;
}

auto StmHdAggrElem::do_important() const -> VariableSet {
    auto res = VariableSet{};
    res.insert(state_->global().begin(), state_->global().end());
    for (auto const &term : tuple_) {
        term->vars(res);
    }
    if (head_ != nullptr) {
        head_->vars(res);
    }
    return res;
}

void StmHdAggrElem::do_init(size_t gen) {
    if (base_ != nullptr) {
        base_->update(gen);
    }
}

auto StmHdAggrElem::get_cond_(EvalContext const &ctx) -> std::pair<size_t, bool> {
    bool fact = true;
    auto &out = ctx.out().cond();
    for (auto const &lit : body_) {
        if (lit->output(ctx, out)) {
            fact = false;
        }
    }
    return {ctx.out().cond_id(), fact};
}

auto StmHdAggrElem::do_report(EvalContext const &ctx) -> bool {
    if (auto it = state_->insert_atom(ctx)) {
        state_->insert_elem(ctx, it->first, *this);
    }
    return true;
}

void StmHdAggrElem::do_propagate([[maybe_unused]] SymbolStore &store, OutputStm &out, Queue &queue) {
    // This is called after all statements on the current priority have been
    // processed. Thus, all element aggregation rules have been processed.
    // Here, literals are derived by the head aggregate are propagated.
    state_->propagate(out, queue);
}

auto StmHdAggrElem::do_priority() const -> size_t {
    return std::numeric_limits<size_t>::max();
}

void StmHdAggrElem::do_print_head(std::ostream &out) const {
    auto p_var = [](std::ostream &out, auto const &x) { out << "X_" << x; };
    auto p_term = [](std::ostream &out, auto const &x) { out << *x; };
    out << "#elem(g(" << Util::p_range(state_->global(), p_var) << "),";
    if (head_ != nullptr) {
        out << *head_;
    } else {
        out << "#true";
    }
    out << ",t(" << Util::p_range(tuple_, p_term) << "))";
}

void StmHdAggrElem::do_print(std::ostream &out) const {
    out << "max: ";
    print_head(out);
    if (auto indices = state_->indices(); !indices.empty()) {
        out << "[" << Util::p_range(indices, ",") << "]";
    }
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

// definition of MatchHdAggr

auto MatchHdAggr::vars() const -> VariableSet {
    return VariableSet{state_->global().begin(), state_->global().end()};
}

auto MatchHdAggr::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
    static_cast<void>(this);
    return {bound.begin(), bound.end()};
}

auto MatchHdAggr::match(EvalContext const &ctx, Symbol const *sym) const -> bool {
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

auto MatchHdAggr::eval(EvalContext const &ctx) const -> std::optional<Symbol const *> {
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

auto MatchHdAggr::state() const -> StateHdAggr & {
    return *state_;
}

auto operator<<(std::ostream &out, MatchHdAggr const &m) -> std::ostream & {
    m.state_->print(out, false);
    return out;
}

// definition of LitHdAggr

void LitHdAggr::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::depend) {
        vars.insert(state().global().begin(), state().global().end());
    }
}

auto LitHdAggr::do_domain() const -> bool {
    // this is an auxiliary literal for binding variables
    return true;
}

auto LitHdAggr::do_single_pass() const -> bool {
    return state().single_pass_body();
}

auto LitHdAggr::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type, std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    offset_ = invalid_offset;
    auto &match = static_cast<MatchHdAggr &>(*this);
    auto index = std::optional<size_t>{};
    if (state().index() != stratified_index && type == MatcherType::new_atoms) {
        index = state().index();
    }
    return {make_atom_matcher(mbr, bound, state().base(), match, type, offset_), index};
}

auto LitHdAggr::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    // Note: at the time of score computation the aggregate is still empty.
    // Scoring low should be fine here.
    return 0;
}

void LitHdAggr::do_print(std::ostream &out) const {
    state().print(out, true);
}

auto LitHdAggr::do_output([[maybe_unused]] EvalContext const &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitHdAggr::do_copy() const -> ULit {
    return std::make_unique<LitHdAggr>(state());
}

auto LitHdAggr::do_hash() const -> size_t {
    // NOLINTNEXTLINE
    return Util::value_hash_record<LitHdAggr>(reinterpret_cast<uintptr_t>(this));
}

auto LitHdAggr::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitHdAggr::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

} // namespace CppClingo::Ground
