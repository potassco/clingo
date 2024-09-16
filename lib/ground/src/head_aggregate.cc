#include <gringo/ground/head_aggregate.hh>

#include <gringo/util/print.hh>
#include <gringo/util/type_traits.hh>

// #define DEBUG_AGGR
#ifdef DEBUG_AGGR
#include <iostream>
#endif

namespace Gringo::Ground {

// definition of AtomAggr

void AtomHdAggr::accumulate(AggregateFunction fun, SymbolSpan tup, bool fact) {
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

void AtomHdAggr::add_elem(size_t idx) { elems_.emplace_back(idx); }

auto AtomHdAggr::elems() const -> std::span<size_t const> { return std::span{elems_.begin(), elems_.end()}; }

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

auto BaseHdAggr::size() const -> size_t { return atoms_.size(); }

auto BaseHdAggr::index(Symbol const *sym) const -> size_t { return atoms_.find(sym) - atoms_.begin(); }

auto BaseHdAggr::nth(size_t i) const -> AtomMap::const_iterator { return atoms_.nth(i); }

auto BaseHdAggr::nth(size_t i) -> AtomMap::iterator { return atoms_.nth(i); }

auto BaseHdAggr::atoms() -> AtomMap & { return atoms_; }

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
    AtomKey([[maybe_unused]] priv_tag tag, SymbolStore &store, Assignment &ass, VariableVec const &global,
            GuardVec &guards, bool &res) {
        auto *it = syms_;
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
    //! Private constructor.
    AtomKey([[maybe_unused]] priv_tag tag, Symbol const *tuple, size_t n) { std::copy_n(tuple, n, syms_); }

    //! Construct an atom key from the global variables and guards.
    //!
    //! Returns false if evaluating the guards fails.
    static auto construct(auto &mbr, SymbolStore &store, Assignment &ass, VariableVec const &global, GuardVec &guards,
                          AtomKey *&target) -> bool {
        if (target == nullptr) {
            auto n = (global.size() + guards.size()) * sizeof(Symbol);
            target = static_cast<AtomKey *>(mbr.allocate(n, alignof(AtomKey)));
        } else {
            std::destroy_at(target);
        }
        bool res = true;
        std::construct_at(target, priv_tag{}, store, ass, global, guards, res);
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
    GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
    Symbol syms_[0];
    GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
};

StateHdAggr::ElementKey::ElementKey([[maybe_unused]] priv_tag tag, SymbolStore &store, Assignment &ass,
                                    AggregateFunction fun, size_t atom_idx, UTermVec const &tuple, bool &res)
    : n_{tuple.size() << 1}, atom_idx_{atom_idx} {
    auto *it = syms_;
    if (auto jt = tuple.begin(), je = tuple.end(); jt != je) {
        // check the weight of the tuple
        if (auto val = (*jt)->eval(store, ass); val && relevant_val(fun, *val)) {
            *it = *val;
        } else {
            res = false;
            return;
        }
        // evaluate the remaining terms
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

auto StateHdAggr::ElementKey::construct(auto &mbr, SymbolStore &store, Assignment &ass, AggregateFunction fun,
                                        size_t atom_idx, UTermVec const &tuple, ElementKey *&target) -> bool {
    bool res = true;
    auto n = sizeof(ElementKey) + tuple.size() * sizeof(Symbol);
    if (target == nullptr) {
        target = static_cast<ElementKey *>(mbr.allocate(n, alignof(ElementKey)));
    } else {
        std::destroy_at(target);
    }
    std::construct_at(target, priv_tag{}, store, ass, fun, atom_idx, tuple, res);
    return res;
}

void StateHdAggr::ElementKey::mark_fact() const { n_ |= 1; }

auto StateHdAggr::ElementKey::fact() const -> bool { return n_ & 1; }

auto StateHdAggr::ElementKey::size() const -> size_t { return n_ >> 1; }

auto StateHdAggr::ElementKey::span() const -> SymbolSpan { return SymbolSpan{syms_, size()}; }

auto StateHdAggr::ElementKey::hash() const -> size_t {
    return Util::value_hash_record<ElementKey>(size(), atom_idx_, span());
}

auto operator==(StateHdAggr::ElementKey const &a, StateHdAggr::ElementKey const &b) -> bool {
    return a.atom_idx_ == b.atom_idx_ && a.size() == b.size() &&
           std::equal(a.span().begin(), a.span().end(), b.span().begin());
}

// NOLINTEND

auto StateHdAggr::global() const -> VariableVec const & { return global_; }

auto StateHdAggr::symbols() -> SymbolVec & {
    symbols_.resize(global_.size());
    return symbols_;
}

auto StateHdAggr::guards() const -> GuardVec const & { return guards_; }

auto StateHdAggr::fun() const -> AggregateFunction { return fun_; }

auto StateHdAggr::single_pass_elems() const -> bool { return single_pass_elems_; }

auto StateHdAggr::index() const -> size_t { return index_; }

void StateHdAggr::propagate(Queue &queue) {
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
                for (auto const &[sym, cond] : tuples_.nth(elem_idx).value()) {
                    auto sig = std::make_tuple(sym.name(), sym.args().size(), sym.has_classical_sign());
                    auto it = std::lower_bound(bases_.begin(), bases_.end(), sig,
                                               [](auto const &a, auto const &b) { return std::get<0>(a) < b; });
                    assert(it != bases_.end());
                    auto *base = std::get<1>(*it);
                    base->add(sym, StateAtom::derived);
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
    if (index_ != stratified_index) {
        for (auto const &[sig, base, indices] : bases_) {
            if (!indices.empty() && base->has_update()) {
                for (auto const &index : indices) {
                    queue.propagate(index);
                }
            }
        }
    }
}

void StateHdAggr::enqueue_(AtomMap::iterator it) {
    if (auto &state = it.value(); state.enqueue()) {
        queue_.emplace_back(atom_index_(it));
    }
}

auto StateHdAggr::insert_atom(SymbolStore &store,
                              Assignment &ass) -> std::optional<std::pair<AtomMap::iterator, bool>> {
    if (AtomKey::construct(*mbr_, store, ass, global_, guards_, atom_key_)) {
        auto [it, ins] = base_.add(atom_key_->syms(), fun_);
        if (ins) {
            atom_key_ = nullptr;
            enqueue_(it);
        }
        return std::make_optional<std::pair<AtomMap::iterator, bool>>(it, ins);
    }
    return std::nullopt;
}

auto StateHdAggr::insert_atom(Symbol const *tuple) -> AtomMap::iterator {
    AtomKey::construct(*mbr_, tuple, global_.size() + guards_.size(), atom_key_);
    auto [it, ins] = base_.add(atom_key_->syms(), fun_);
    if (ins) {
        atom_key_ = nullptr;
    }
    return it;
}

void StateHdAggr::insert_elem(SymbolStore &store, Assignment &ass, AtomMap::iterator it, UTerm const &head,
                              UTermVec const &tuple, ElementKey *&elem_key, auto const &get_cond) {
    auto sym = SymbolStore::sup();
    if (head != nullptr) {
        if (auto opt = head->eval(store, ass)) {
            sym = *opt;
        } else {
            return;
        }
    }
    if (ElementKey::construct(*mbr_, store, ass, fun_, atom_index_(it), tuple, elem_key)) {
        auto [jt, jns] = tuples_.try_emplace(elem_key);
        if (jns) {
            elem_key = nullptr;
            it.value().add_elem(jt - tuples_.begin());
            enqueue_(it);
        }

        auto [cond_id, fact] = get_cond();
        if (head != nullptr && fact) {
            jt.key()->mark_fact();
        }
        auto &cond = jt.value();
        cond.emplace_back(sym, cond_id);
        std::sort(cond.begin(), cond.end());
        cond.erase(std::unique(cond.begin(), cond.end()), cond.end());
    }
}

auto StateHdAggr::atom_index_(AtomMap::iterator it) -> size_t { return it - base_.atoms().begin(); }

void StateHdAggr::print(std::ostream &out) {
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

void StateHdAggr::output(OutputStm &out) {
    static_cast<void>(out);
    static_cast<void>(this);
    throw std::logic_error("implement me!!!");
    /*
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
    */
}

// definition of StmAggrElem

auto StmHdAggrElem::do_body() const -> ULitVec const & { return body_; }

auto StmHdAggrElem::do_important() const -> VariableSet {
    auto res = VariableSet{};
    res.insert(state_->global().begin(), state_->global().end());
    for (auto const &term : tuple_) {
        term->vars(res);
    }
    return res;
}

auto StmHdAggrElem::do_is_important(size_t index) const -> bool {
    // Only the literals gathered by do_important and the ones in the
    // condition are important. The remaining ones in the body can be
    // backtracked.
    return index < num_cond_;
}

void StmHdAggrElem::do_init([[maybe_unused]] size_t gen) {
    // by construction, this statement does not increment the generation
}

auto StmHdAggrElem::do_report(InstantiationContext &ctx) -> bool {
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
        state_->insert_elem(ctx.store(), ass, it->first, head_, tuple_, elem_key_, get_cond);
    }
    return true;
}

void StmHdAggrElem::do_propagate([[maybe_unused]] SymbolStore &store, Queue &queue) {
    // This is called after all statements on the current priority have
    // been processed. Thus, all element aggregation rules have been
    // processed. Here, aggregates that can match are added to the base and
    // are enqueued.
    state_->propagate(queue);
}

auto StmHdAggrElem::do_priority() const -> size_t { return priority_; }

void StmHdAggrElem::do_print_head(std::ostream &out) const {
    auto p_var = [](std::ostream &out, auto const &x) { out << "X_" << x; };
    auto p_term = [](std::ostream &out, auto const &x) { out << *x; };
    out << "#elem(g(" << Util::p_range{state_->global(), p_var} << "),";
    if (head_ != nullptr) {
        out << *head_;
    } else {
        out << "#true";
    }
    out << ",t(" << Util::p_range{tuple_, p_term} << "))";
}

void StmHdAggrElem::do_print(std::ostream &out) const {
    out << priority_ << ": ";
    print_head(out);
    if (state_->index() != stratified_index) {
        out << "[" << state_->index() << "]";
    }
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

} // namespace Gringo::Ground
