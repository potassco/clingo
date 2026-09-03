#include <clingo/ground/theory_atom.hh>

namespace CppClingo::Ground {

// definition of AtomTheory

void AtomTheory::add_elem(size_t idx) {
    elems_.emplace_back(idx);
}

auto AtomTheory::elems() const -> std::span<size_t const> {
    return elems_;
}

auto AtomTheory::uid() const -> std::optional<size_t> {
    return uid_ != invalid_offset ? std::make_optional(uid_) : std::nullopt;
}

void AtomTheory::uid(size_t uid) {
    assert(uid_ == invalid_offset || uid_ == uid);
    uid_ = uid;
}

// definition of BaseTheory

auto BaseTheory::add(Symbol const *sym, Symbol name, std::optional<size_t> rhs) -> std::pair<AtomMap::iterator, bool> {
    return atoms_.try_emplace(sym, name, rhs);
}

auto BaseTheory::size() const -> size_t {
    return atoms_.size();
}

auto BaseTheory::index(Symbol const *sym) const -> size_t {
    return atoms_.find(sym) - atoms_.begin();
}

auto BaseTheory::nth(size_t i) const -> AtomMap::const_iterator {
    return atoms_.nth(i);
}

auto BaseTheory::nth(size_t i) -> AtomMap::iterator {
    return atoms_.nth(i);
}

auto BaseTheory::atoms() -> AtomMap & {
    return atoms_;
}

// definition of StateBdTheory

StateTheory::ElementKey::ElementKey([[maybe_unused]] priv_tag tag, EvalContext const &ctx, OutputTheory &out,
                                    size_t atom_idx, UTheoryTermVec const &terms)
    : n_{terms.size()}, atom_idx_{atom_idx} {
    // NOLINTBEGIN
    auto *it = syms_;
    for (auto const &term : terms) {
        *it++ = term->output(ctx, out);
    }
    // NOLINTEND
}

void StateTheory::ElementKey::construct(std::pmr::monotonic_buffer_resource &mbr, EvalContext const &ctx,
                                        OutputTheory &out, size_t atom_idx, UTheoryTermVec const &terms,
                                        ElementKey *&target) {
    auto n = sizeof(ElementKey) + (terms.size() * sizeof(Symbol));
    if (target == nullptr) {
        target = static_cast<ElementKey *>(mbr.allocate(n, alignof(ElementKey)));
    } else {
        std::destroy_at(target);
    }
    std::construct_at(target, priv_tag{}, ctx, out, atom_idx, terms);
}

auto StateTheory::ElementKey::size() const -> size_t {
    return n_;
}

auto StateTheory::ElementKey::span() const -> std::span<size_t const> {
    // NOLINTBEGIN
    return {syms_, size()};
    // NOLINTEND
}

auto StateTheory::ElementKey::hash() const -> size_t {
    // NOLINTBEGIN
    return Util::value_hash_record<ElementKey>(atom_idx_, size(), span());
    // NOLINTEND
}

auto operator==(StateTheory::ElementKey const &a, StateTheory::ElementKey const &b) -> bool {
    return a.atom_idx_ == b.atom_idx_ && a.size() == b.size() &&
           std::equal(a.span().begin(), a.span().end(), b.span().begin());
}

void StateTheory::print(std::ostream &out) {
    out << "&" << *name_ << "(" << Util::p_range(global_, [](std::ostream &out, auto var) { out << "X_" << var; })
        << ")";
    if (guard_) {
        out << " " << guard_->first << " " << *guard_->second;
    }
}

auto StateTheory::global() const -> VariableVec const & {
    return global_;
}

auto StateTheory::name() const -> UTerm const & {
    return name_;
}

auto StateTheory::guard() const -> TheoryRGuard const & {
    return guard_;
}

auto StateTheory::base() -> BaseTheory & {
    return base_;
}

void StateTheory::elems(UStmVec elems) {
    elems_ = std::move(elems);
}

auto StateTheory::find_atom(Assignment &ass) -> AtomMap::iterator {
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
    auto jt = base_.atoms().find(atom_key_);
    assert(jt != base_.atoms().end());
    return jt;
}

auto StateTheory::insert_atom(Symbol name, std::optional<size_t> rhs, Assignment &ass)
    -> std::pair<AtomMap::iterator, bool> {
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
    auto res = base_.add(atom_key_, name, rhs);
    if (res.second) {
        atom_key_ = nullptr;
    }
    return res;
}

void StateTheory::insert_elem(EvalContext const &ctx, AtomMap::iterator it, UTheoryTermVec const &tuple,
                              ElementKey *&elem_key, auto const &get_cond) {
    ElementKey::construct(*mbr_, ctx, ctx.out().theory(), it - base().atoms().begin(), tuple, elem_key);
    auto [jt, jns] = tuples_.try_emplace(elem_key);
    if (jns) {
        elem_key = nullptr;
        it.value().add_elem(jt - tuples_.begin());
    }
    jt.value().emplace_back(get_cond());
}

void StateTheory::output(Logger &log, SymbolStore &store, OutputStm &out) {
    CLINGO_REPORT(log, debug) << "    delayed statements";
    for (auto const &stm : elems_) {
        CLINGO_REPORT(log, debug) << "      " << *stm;
    }
    auto ass = Assignment{};
    auto lin = Linearizer{*mbr_};
    auto queue = Queue{};
    lin.start(queue);
    for (auto &elem : elems_) {
        lin.prepare(*elem, elem->body(), elem->important());
    }
    std::ignore = queue.process(log, store, out);
    auto &thy = out.theory();
    std::vector<size_t> elems;
    auto guard = OutputTheory::OptGuard{};
    if (guard_) {
        guard.emplace(guard_->first, size_t{});
    }
    for (auto const &atm : base_.atoms()) {
        elems.clear();
        for (auto const &elem : atm.second.elems()) {
            auto const &[tuple, conds] = *tuples_.nth(elem);
            for (auto const &cond : conds) {
                elems.emplace_back(thy.elem(tuple->span(), cond));
            }
        }
        if (guard_) {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            guard->second = *atm.second.rhs();
        }
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        thy.atom(type_, *atm.second.uid(), atm.second.name(), elems, guard);
    }
}

// definition of MatchTheory

auto MatchTheory::vars() const -> VariableSet {
    return VariableSet{state_->global().begin(), state_->global().end()};
}

auto MatchTheory::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
    static_cast<void>(this);
    return {bound.begin(), bound.end()};
}

auto MatchTheory::match(EvalContext const &ctx, Symbol const *sym) const -> bool {
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

auto MatchTheory::eval(EvalContext const &ctx) const -> std::optional<Symbol const *> {
    eval_.clear();
    for (auto var : state_->global()) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        eval_.emplace_back(*ctx.ass()[var]);
    }
    return eval_.data();
}

auto operator<<(std::ostream &out, MatchTheory const &m) -> std::ostream & {
    m.state_->print(out);
    return out;
}

auto MatchTheory::state() const -> StateTheory & {
    return *state_;
}

// definition of LitMatchTheory

void LitMatchTheory::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::depend) {
        vars.insert(state().global().begin(), state().global().end());
    }
}

auto LitMatchTheory::do_domain() const -> bool {
    // this is an auxiliary literal for binding variables
    return true;
}

auto LitMatchTheory::do_single_pass() const -> bool {
    return true;
}

auto LitMatchTheory::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    offset_ = invalid_offset;
    auto &match = static_cast<MatchTheory &>(*this);
    auto index = std::optional<size_t>{};
    return {make_atom_matcher(mbr, bound, state().base(), match, type, offset_), index};
}

auto LitMatchTheory::do_score([[maybe_unused]] std::vector<bool> const &bound,
                              [[maybe_unused]] double recursive_estimate) const -> double {
    // Note: at the time of score computation the aggregate is still empty.
    // Scoring low should be fine here.
    return 0;
}

void LitMatchTheory::do_print(std::ostream &out) const {
    state().print(out);
}

auto LitMatchTheory::do_output([[maybe_unused]] EvalContext const &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitMatchTheory::do_copy() const -> ULit {
    return std::make_unique<LitMatchTheory>(state());
}

auto LitMatchTheory::do_hash() const -> size_t {
    // NOLINTNEXTLINE
    return Util::value_hash_record<LitMatchTheory>(reinterpret_cast<uintptr_t>(this));
}

auto LitMatchTheory::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitMatchTheory::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

// definition of StmTheoryElement

auto StmTheoryElement::do_body() const -> ULitVec const & {
    return body_;
}

auto StmTheoryElement::do_important() const -> VariableSet {
    auto res = VariableSet{};
    res.insert(state_->global().begin(), state_->global().end());
    for (auto const &term : tuple_) {
        term->vars(res);
    }
    return res;
}

void StmTheoryElement::do_init(size_t gen) {
    state_->base().update(gen);
}

auto StmTheoryElement::do_report(EvalContext const &ctx) -> bool {
    auto &ass = ctx.ass();
    auto it = state_->find_atom(ass);
    auto get_cond = [this, &ctx]() {
        // output the condition
        auto &out = ctx.out().cond();
        for (auto const &lit : body_) {
            std::ignore = lit->output(ctx, out);
        }
        return ctx.out().cond_id();
    };
    state_->insert_elem(ctx, it, tuple_, elem_key_, get_cond);
    return true;
}

void StmTheoryElement::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] OutputStm &out,
                                    [[maybe_unused]] Queue &queue) {
}

auto StmTheoryElement::do_priority() const -> size_t {
    return std::numeric_limits<size_t>::max();
}

void StmTheoryElement::do_print_head(std::ostream &out) const {
    auto p_var = [](std::ostream &out, auto const &x) { out << "X_" << x; };
    auto p_term = [](std::ostream &out, auto const &x) { out << *x; };
    out << "#elem(g(" << Util::p_range(state_->global(), p_var) << "),t(" << Util::p_range(tuple_, p_term) << "))";
}

void StmTheoryElement::do_print(std::ostream &out) const {
    out << "max: ";
    print_head(out);
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

// definition of LitBdTheory

void LitBdTheory::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        vars.insert(state_->global().begin(), state_->global().end());
    }
}

auto LitBdTheory::do_domain() const -> bool {
    return false;
}

auto LitBdTheory::do_single_pass() const -> bool {
    return true;
}

auto LitBdTheory::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr,
                             [[maybe_unused]] MatcherType type, [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    return {make_once_matcher(*state_->name(), name_), std::nullopt};
}

auto LitBdTheory::do_score([[maybe_unused]] std::vector<bool> const &bound,
                           [[maybe_unused]] double recursive_estimate) const -> double {
    return 0;
}

void LitBdTheory::do_print(std::ostream &out) const {
    out << sign_;
    state_->print(out);
}

auto LitBdTheory::do_output(EvalContext const &ctx, OutputLit &out) const -> bool {
    auto rhs = std::optional<size_t>{};
    if (auto const &guard = state_->guard()) {
        rhs.emplace(guard->second->output(ctx, ctx.out().theory()));
    }
    auto res = state_->insert_atom(name_, rhs, ctx.ass());
    auto &atm = res.first.value();
    atm.uid(out.bd_theory(sign_, atm.uid()));
    return true;
}

auto LitBdTheory::do_copy() const -> ULit {
    return std::make_unique<LitBdTheory>(*state_, sign_);
}

auto LitBdTheory::do_hash() const -> size_t {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return Util::value_hash_record<LitBdTheory>(reinterpret_cast<uintptr_t>(state_));
}

auto LitBdTheory::do_equal_to(Lit const &other) const -> bool {
    return &other == this;
}

auto LitBdTheory::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return &other <=> this;
}

// definition of StmHdTheory

auto StmHdTheory::do_body() const -> ULitVec const & {
    return body_;
}

auto StmHdTheory::do_important() const -> VariableSet {
    return VariableSet{state_->global().begin(), state_->global().end()};
}

void StmHdTheory::do_init([[maybe_unused]] size_t gen) {
    state_->base().update(gen);
}

auto StmHdTheory::do_report(EvalContext const &ctx) -> bool {
    if (auto name = state_->name()->eval(ctx)) {
        auto &out = ctx.out().body();
        for (auto const &lit : body_) {
            std::ignore = lit->output(ctx, out);
        }
        auto rhs = std::optional<size_t>{};
        if (auto const &guard = state_->guard()) {
            rhs.emplace(guard->second->output(ctx, ctx.out().theory()));
        }
        auto res = state_->insert_atom(*name, rhs, ctx.ass());
        auto &atm = res.first.value();
        atm.uid(ctx.out().theory_rule(atm.uid()));
    }
    return true;
}

void StmHdTheory::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] OutputStm &out,
                               [[maybe_unused]] Queue &queue) {
}

auto StmHdTheory::do_priority() const -> size_t {
    return std::numeric_limits<size_t>::max();
}

void StmHdTheory::do_print_head(std::ostream &out) const {
    state_->print(out);
}

void StmHdTheory::do_print(std::ostream &out) const {
    out << "max: ";
    print_head(out);
    out << " :- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

} // namespace CppClingo::Ground
