#include <clingo/ground/disjunction.hh>

#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>

// #define DEBUG_AGGR
#ifdef DEBUG_AGGR
#include <iostream>
#endif

namespace CppClingo::Ground {

// definition of AtomAggr

auto AtomDisjunction::is_fact() const -> bool {
    return fact_ == 1;
}

void AtomDisjunction::mark_fact() {
    fact_ = 1;
}

auto AtomDisjunction::enqueue() -> bool {
    if (enqueued_ == 0 && propagated_ < elems_.size()) {
        enqueued_ = 1;
        return true;
    }
    return false;
}

void AtomDisjunction::dequeue() {
    assert(enqueued_);
    propagated_ = elems_.size();
    enqueued_ = 0;
}

void AtomDisjunction::add_elem(size_t idx) {
    elems_.emplace_back(idx);
}

auto AtomDisjunction::elems() const -> std::span<size_t const> {
    return std::span{elems_.begin(), elems_.end()};
}

auto AtomDisjunction::todo() -> std::span<size_t const> {
    return std::span{std::next(elems_.begin(), static_cast<std::ptrdiff_t>(propagated_)), elems_.end()};
}

auto AtomDisjunction::uid() const -> std::optional<size_t> {
    return uid_ != invalid_offset ? std::make_optional(uid_) : std::nullopt;
}

void AtomDisjunction::uid(size_t uid) {
    assert(uid_ == invalid_offset || uid_ == uid);
    uid_ = uid;
}

// definition of BaseDisjunction

auto BaseDisjunction::add(Symbol const *sym) -> std::pair<AtomMap::iterator, bool> {
    return atoms_.try_emplace(sym);
}

auto BaseDisjunction::size() const -> size_t {
    return atoms_.size();
}

auto BaseDisjunction::index(Symbol const *sym) const -> size_t {
    return static_cast<size_t>(atoms_.find(sym) - atoms_.begin());
}

auto BaseDisjunction::nth(size_t i) const -> AtomMap::const_iterator {
    return atoms_.nth(i);
}

auto BaseDisjunction::nth(size_t i) -> AtomMap::iterator {
    return atoms_.nth(i);
}

auto BaseDisjunction::atoms() -> AtomMap & {
    return atoms_;
}

// definition of StateAggr

auto StateDisjunction::global() const -> VariableVec const & {
    return global_;
}

auto StateDisjunction::symbols() -> SymbolVec & {
    symbols_.resize(global_.size());
    return symbols_;
}

auto StateDisjunction::single_pass_body() const -> bool {
    return single_pass_body_;
}

auto StateDisjunction::index() const -> size_t {
    return index_;
}

auto StateDisjunction::indices() const -> std::vector<size_t> {
    std::vector<size_t> res;
    for (auto const &[sig, base, indices] : bases_) {
        res.insert(res.end(), indices.begin(), indices.end());
    }
    std::ranges::sort(res);
    res.erase(std::ranges::unique(res).begin(), res.end());
    return res;
}

auto StateDisjunction::base() -> BaseDisjunction & {
    return base_;
}

void StateDisjunction::enqueue(Queue &queue) {
    if (index_ != stratified_index && base_.has_update()) {
        queue.propagate(index_);
    }
}

void StateDisjunction::propagate(OutputStm &out, Queue &queue) {
    // process enqueued atoms
    for (auto atom_idx : queue_) {
        auto it = base_.nth(atom_idx);
        auto &state = it.value();
        // accumulate the elements
        if (!it.value().is_fact()) {
            for (auto elem_idx : state.todo()) {
                auto elem = elems_.nth(elem_idx);
                if (elem.value().second.empty()) {
                    auto sym = elem.key().first;
                    auto sig = std::make_tuple(sym.name(), sym.args().size(), sym.has_classical_sign());
                    auto jt = std::ranges::lower_bound(bases_, sig, std::less<>{},
                                                       [](auto const &a) -> decltype(auto) { return std::get<0>(a); });
                    if (std::get<1>(*jt)->is_fact(sym)) {
                        state.mark_fact();
                    }
                }
#ifdef DEBUG_AGGR
                std::cerr << "accumulate: a: " << atom_idx << " e: " << elem_idx << " h:" << " " << elem.key().first;
                if (it.value().is_fact()) {
                    std::cerr << " [f]";
                }
                std::cerr << "\n";
#endif
            }
        }
        // propagate the elements
        if (!it.value().is_fact()) {
            for (auto elem_idx : state.todo()) {
                auto elem = elems_.nth(elem_idx);
                auto sym = elem.key().first;
                auto sig = std::make_tuple(sym.name(), sym.args().size(), sym.has_classical_sign());
                auto jt = std::ranges::lower_bound(bases_, sig, std::less<>{},
                                                   [](auto const &a) -> decltype(auto) { return std::get<0>(a); });
                assert(jt != bases_.end());
                elem.value().first =
                    std::get<1>(*jt)->add(sym, StateAtom::derived, [&out]() { return out.uid(); }).first.value().id;
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

void StateDisjunction::enqueue_(AtomMap::iterator it) {
    if (auto &state = it.value(); state.enqueue()) {
        queue_.emplace_back(atom_index_(it));
    }
}

auto StateDisjunction::insert_atom(Assignment &ass) -> std::pair<AtomMap::iterator, bool> {
    auto n = global_.size() * sizeof(Symbol);
    if (atom_key_ == nullptr) {
        atom_key_ = static_cast<Symbol *>(mbr_->allocate(n, alignof(Symbol)));
    } else {
        std::destroy_n(atom_key_, n);
    }
    auto *it = atom_key_;
    for (auto const &var : global_) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        std::construct_at(it, ass[var].value());
        it = std::next(it);
    }
    auto res = base_.add(atom_key_);
    if (res.second) {
        atom_key_ = nullptr;
        enqueue_(res.first);
    }
    return res;
}

void StateDisjunction::insert_elem(EvalContext const &ctx, AtomMap::iterator it, UTerm const &head,
                                   auto const &get_cond) {
    if (auto opt = head->eval(ctx); opt) {
        auto [jt, jns] = elems_.try_emplace(ElementKey{*opt, atom_index_(it)});
        if (jns) {
            it.value().add_elem(static_cast<size_t>(jt - elems_.begin()));
            enqueue_(it);
        }
        auto [cond_id, fact] = get_cond();
        auto &conds = jt.value().second;
        if (fact) {
            conds.clear();
        } else if (jns || !jt.value().second.empty()) {
            auto kt = std::ranges::lower_bound(conds, cond_id);
            if (kt == conds.end() || *kt != cond_id) {
                conds.emplace(kt, cond_id);
            }
        }
    }
}

auto StateDisjunction::atom_index_(AtomMap::iterator it) -> size_t {
    return static_cast<size_t>(it - base_.atoms().begin());
}

void StateDisjunction::print(std::ostream &out, bool print_index) {
    out << "#disjunction(" << Util::p_range(global_, [](std::ostream &out, auto var) { out << "X_" << var; }) << ")";
    if (print_index && index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
}

void StateDisjunction::output([[maybe_unused]] Logger &log, [[maybe_unused]] SymbolStore &store, OutputStm &out) {
    std::vector<OutputStm::DisjElem> elems;
    for (auto const &[tuple, atom] : base_.atoms()) {
        if (auto uid = atom.uid(); uid) {
            elems.clear();
            if (atom.is_fact()) {
                elems.emplace_back(SymbolStore::sup(), 0, std::span<size_t const>{});
            } else {
                for (auto const &elem_idx : atom.elems()) {
                    auto const &[head, cond] = *elems_.nth(elem_idx);
                    elems.emplace_back(head.first, cond.first, IndexSpan{cond.second});
                }
            }
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            out.disjunction(*uid, elems);
        }
    }
}

// definition of StmDisjunction

auto StmDisjunction::do_body() const -> ULitVec const & {
    return body_;
}

auto StmDisjunction::do_important() const -> VariableSet {
    auto res = VariableSet{};
    res.insert(state_->global().begin(), state_->global().end());
    return res;
}

void StmDisjunction::do_init([[maybe_unused]] size_t gen) {
    state_->base().ensure(gen);
}

auto StmDisjunction::do_report(EvalContext const &ctx) -> bool {
    auto &lit = state_->insert_atom(ctx.ass()).first.value();
    auto &out = ctx.out().body();
    for (auto const &lit : body_) {
        std::ignore = lit->output(ctx, out);
    }
    lit.uid(ctx.out().disjunctive_rule(lit.uid()));
    // Note: in principle, it would be possible to detect false disjunctions
    // in the stratified case.
    return true;
}

void StmDisjunction::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] OutputStm &out, Queue &queue) {
    // enqueue the aggregate element statements for propagation
    state_->enqueue(queue);
}

auto StmDisjunction::do_priority() const -> size_t {
    return priority_;
}

void StmDisjunction::do_print_head(std::ostream &out) const {
    state_->print(out, false);
}

void StmDisjunction::do_print(std::ostream &out) const {
    out << priority_ << ": ";
    state_->print(out, true);
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

// definition of StmDisjunctionElem

auto StmDisjunctionElem::do_body() const -> ULitVec const & {
    return body_;
}

auto StmDisjunctionElem::do_important() const -> VariableSet {
    auto res = VariableSet{};
    res.insert(state_->global().begin(), state_->global().end());
    head_->vars(res);
    return res;
}

void StmDisjunctionElem::do_init(size_t gen) {
    if (base_ != nullptr) {
        base_->update(gen);
    }
}

auto StmDisjunctionElem::do_report(EvalContext const &ctx) -> bool {
    auto &ass = ctx.ass();
    // insert aggregate atom
    if (auto it = state_->insert_atom(ass).first; !it.value().is_fact()) {
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
        state_->insert_elem(ctx, it, head_, get_cond);
    }
    return true;
}

void StmDisjunctionElem::do_propagate([[maybe_unused]] SymbolStore &store, OutputStm &out, Queue &queue) {
    // This is called after all statements on the current priority have been
    // processed. Thus, all element aggregation rules have been processed.
    // Here, literals are derived by the disjunction are propagated.
    state_->propagate(out, queue);
}

auto StmDisjunctionElem::do_priority() const -> size_t {
    return std::numeric_limits<size_t>::max();
}

void StmDisjunctionElem::do_print_head(std::ostream &out) const {
    auto p_var = [](std::ostream &out, auto const &x) { out << "X_" << x; };
    out << "#elem(g(" << Util::p_range(state_->global(), p_var) << "),";
    if (head_ != nullptr) {
        out << *head_;
    } else {
        out << "#true";
    }
    out << ",h(" << *head_ << "))";
}

void StmDisjunctionElem::do_print(std::ostream &out) const {
    out << "max: ";
    print_head(out);
    if (auto indices = state_->indices(); !indices.empty()) {
        out << "[" << Util::p_range(indices, ",") << "]";
    }
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

// definition of MatchDisjunction

auto MatchDisjunction::vars() const -> VariableSet {
    return VariableSet{state_->global().begin(), state_->global().end()};
}

auto MatchDisjunction::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const
    -> VariableVec {
    static_cast<void>(this);
    return {bound.begin(), bound.end()};
}

auto MatchDisjunction::match(EvalContext const &ctx, Symbol const *sym) const -> bool {
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

auto MatchDisjunction::eval(EvalContext const &ctx) const -> std::optional<Symbol const *> {
    eval_.clear();
    for (auto var : state_->global()) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        eval_.emplace_back(ctx.ass()[var].value());
    }
    return eval_.data();
}

auto MatchDisjunction::state() const -> StateDisjunction & {
    return *state_;
}

auto operator<<(std::ostream &out, MatchDisjunction const &m) -> std::ostream & {
    m.state_->print(out, false);
    return out;
}

// definition of LitDisjunction

void LitDisjunction::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::depend) {
        vars.insert(state().global().begin(), state().global().end());
    }
}

auto LitDisjunction::do_domain() const -> bool {
    // this is an auxiliary literal for binding variables
    return true;
}

auto LitDisjunction::do_single_pass() const -> bool {
    return state().single_pass_body();
}

auto LitDisjunction::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    offset_ = invalid_offset;
    auto &match = static_cast<MatchDisjunction &>(*this);
    auto index = std::optional<size_t>{};
    if (state().index() != stratified_index && type == MatcherType::new_atoms) {
        index = state().index();
    }
    return {make_atom_matcher(mbr, bound, state().base(), match, type, offset_), index};
}

auto LitDisjunction::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    // Note: at the time of score computation the aggregate is still empty.
    // Scoring low should be fine here.
    return 0;
}

void LitDisjunction::do_print(std::ostream &out) const {
    state().print(out, true);
}

auto LitDisjunction::do_output([[maybe_unused]] EvalContext const &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitDisjunction::do_copy() const -> ULit {
    return std::make_unique<LitDisjunction>(state());
}

auto LitDisjunction::do_hash() const -> size_t {
    // NOLINTNEXTLINE
    return Util::value_hash_record<LitDisjunction>(reinterpret_cast<uintptr_t>(this));
}

auto LitDisjunction::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitDisjunction::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

} // namespace CppClingo::Ground
