#include <clingo/ground/condlit.hh>
#include <clingo/ground/matcher.hh>

#include <clingo/core/logger.hh>

#include <clingo/util/print.hh>

#include <typeindex>

namespace CppClingo::Ground {

// StateAtomCondLit

[[nodiscard]] auto StateAtomCondLit::enqueue(MapElemCondLit const &elems) -> bool {
    if (enqueued_ == 0 && propagated_ == 0 &&
        (elems_propagated_ == elems_.size() || !elems.nth(elems_[elems_propagated_]).value().is_blocked())) {
        enqueued_ = 1;
        return true;
    }
    return false;
}

[[nodiscard]] auto StateAtomCondLit::propagate(MapElemCondLit const &elems) -> bool {
    assert(propagated_ == 0 && enqueued_ != 0);
    enqueued_ = 0;
    for (auto n = elems_.size(); elems_propagated_ < n; ++elems_propagated_) {
        auto const elem_it = elems.nth(elems_[elems_propagated_]);
        auto const &elem = elem_it.value();
        if (elem.is_blocked()) {
            if (elem.is_false()) {
                false_ = 1;
            }
            return false;
        }
    }
    propagated_ = 1;
    return true;
}

[[nodiscard]] auto StateAtomCondLit::is_fact(MapElemCondLit const &elems) const -> bool {
    return std::ranges::all_of(elems_, [&elems](auto idx) { return elems.nth(idx).value().is_fact(); });
}

// StateCondLit

void StateCondLit::vars(VariableSet &res, bool all) const {
    if (all) {
        res.insert(local_.begin(), local_.end());
    }
    res.insert(global_.begin(), global_.end());
}

auto StateCondLit::vars(bool all) const -> VariableSet {
    VariableSet res;
    res.reserve(all ? global_.size() + local_.size() : global_.size());
    vars(res, all);
    return res;
}

auto StateCondLit::vars_global() const -> VariableVec const & {
    return global_;
}

auto StateCondLit::vars_local() const -> VariableVec const & {
    return local_;
}

auto StateCondLit::index() const -> size_t {
    return index_;
}

auto StateCondLit::add_empty(Assignment const &ass) -> std::pair<MapAtomCondLit::iterator, bool> {
    if (syms_atom_ == nullptr) {
        syms_atom_ = static_cast<Symbol *>(mbr_->allocate(global_.size() * sizeof(Symbol), alignof(Symbol)));
    }
    std::ranges::transform(global_, syms_atom_, [&ass](auto const &var) { return *ass[var]; });
    auto [it, ins] = atoms_.try_emplace(syms_atom_);
    if (ins) {
        syms_atom_ = nullptr;
        if (it.value().enqueue(elems_)) {
            propagate_.emplace_back(std::distance(atoms_.begin(), it));
        }
    }
    return {it, ins};
}

auto StateCondLit::add_premise(EvalContext const &ctx, ULitVec const &premise) -> bool {
    // no further elements have to be accumulated if the literal is false
    auto it = atom_find(ctx.ass());
    if (it.value().is_false()) {
        return false;
    }

    bool fact = true;
    auto &out = ctx.out().cond();
    for (auto const &lit : premise) {
        if (lit->output(ctx, out)) {
            fact = false;
        }
    }

    auto &ass = ctx.ass();
    auto *syms_elem = static_cast<Symbol *>(mbr_->allocate((local_.size() + 1) * sizeof(Symbol), alignof(Symbol)));
    auto *kt = syms_elem;
    *kt = Symbol::from_rep(std::distance(atoms_.begin(), it));
    for (auto var : local_) {
        kt = std::next(kt);
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        *kt = *ass[var];
    }

    auto [jt, ins] = elems_.try_emplace(syms_elem, ctx.out().cond_id(), fact, has_conclusion_);
    // an element can only be added once
    assert(ins);

    auto &atom = it.value();
    auto &elem = jt.value();

    atom.add_elem(std::distance(elems_.begin(), jt));
    // Note: in principle, we only need to ground if the conclusion is a fact.
    // We still add to the base here to gather the conclusion for the output.
    if (has_conclusion_) {
        base_premise_.add(jt);
    }
    if (!elem.is_blocked() && atom.enqueue(elems_)) {
        propagate_.emplace_back(atom_index(it));
    }
    return has_conclusion_ || !fact;
}

void StateCondLit::add_conclusion(Assignment const &ass, MapAtomCondLit::iterator it, size_t conclusion, bool fact) {
    assert(it != atoms_.end());
    auto jt = elem_find(ass, it);
    assert(jt != elems_.end());
    jt.value().set_conclusion(conclusion, fact);
    if (it.value().enqueue(elems_)) {
        propagate_.emplace_back(atom_index(it));
    }
}

auto StateCondLit::propagate() -> bool {
    bool res = false;
    for (auto atom_index : propagate_) {
        auto it = atoms_.nth(atom_index);
        if (it.value().propagate(elems_)) {
            base_lit_.add(it);
            res = true;
        }
    }
    propagate_.clear();
    return res;
}

auto StateCondLit::domain() const -> bool {
    return domain_;
}

auto StateCondLit::base_empty() -> BaseCondLitEmpty & {
    return base_empty_;
}

auto StateCondLit::base_premise() -> BaseCondLitPremise & {
    return base_premise_;
}

auto StateCondLit::base_lit() -> BaseCondLit & {
    return base_lit_;
}

auto StateCondLit::atom_is_fact(MapAtomCondLit::iterator it) -> bool {
    if (!sp_premise_) {
        return false;
    }
    assert(it != atoms_.end());
    return it->second.is_fact(elems_);
}

auto StateCondLit::atom_index(Assignment const &ass) -> std::optional<size_t> {
    if (auto it = atom_find(ass); it != atoms_.end()) {
        return atom_index(it);
    }
    return std::nullopt;
}

auto StateCondLit::atom_nth(size_t index) -> MapAtomCondLit::iterator {
    return atoms_.nth(index);
}

auto StateCondLit::atom_index(MapAtomCondLit::const_iterator it) const -> size_t {
    return std::distance(atoms_.begin(), it);
}

auto StateCondLit::elem_index(MapElemCondLit::const_iterator it) const -> size_t {
    return std::distance(elems_.begin(), it);
}

auto StateCondLit::atom_find(Assignment const &ass) -> MapAtomCondLit::iterator {
    temp_syms_.clear();
    for (auto var : global_) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        temp_syms_.emplace_back(ass[var].value());
    }
    return atoms_.find(temp_syms_.data());
}

auto StateCondLit::elem_find(Assignment const &ass, MapAtomCondLit::iterator it) -> MapElemCondLit::iterator {
    temp_syms_.clear();
    temp_syms_.emplace_back(Symbol::from_rep(atom_index(it)));
    for (auto var : local_) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        temp_syms_.emplace_back(ass[var].value());
    }
    return elems_.find(temp_syms_.data());
}

void StateCondLit::output([[maybe_unused]] Logger &log, [[maybe_unused]] SymbolStore &store, OutputStm &out) {
    std::vector<std::pair<std::optional<size_t>, size_t>> elems;
    for (auto const &[tuple, atom] : atoms_) {
        if (auto uid = atom.uid(); uid) {
            elems.clear();
            for (auto const &elem_idx : atom.elems()) {
                auto const &[tuple, elem] = *elems_.nth(elem_idx);
                if (!elem.is_fact()) {
                    elems.emplace_back(elem.conclusion(), elem.premise());
                }
            }
            out.cond_lit(*uid, elems);
        }
    }
}

// MatchCondLit

auto MatchCondLit::vars() const -> VariableSet {
    return state_->vars(type_ == LitCondLitType::premise);
}

auto MatchCondLit::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
    static_cast<void>(this);
    return {bound.begin(), bound.end()};
};

auto MatchCondLit::match(EvalContext const &ctx, Symbol const *sym) const -> bool {
    if (type_ == LitCondLitType::premise) {
        auto it = state_->atom_nth(Symbol::to_rep(*sym));
        return match_(ctx.ass(), it.key(), state_->vars_global()) &&
               match_(ctx.ass(), std::next(sym), state_->vars_local());
    }
    return match_(ctx.ass(), sym, state_->vars_global());
};

auto MatchCondLit::eval(EvalContext const &ctx) const -> std::optional<Symbol const *> {
    eval_.clear();
    bool is_premise = type_ == LitCondLitType::premise;
    if (is_premise) {
        if (auto index = state_->atom_index(ctx.ass()); index) {
            eval_.emplace_back(Symbol::from_rep(*index));
        } else {
            return std::nullopt;
        }
    }
    for (auto var : is_premise ? state_->vars_local() : state_->vars_global()) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        eval_.emplace_back(ctx.ass()[var].value());
    }
    return eval_.data();
};

auto operator<<(std::ostream &out, MatchCondLit const &m) -> std::ostream & {
    out << "#cond_lit(" << m.type_;
    for (auto var : m.state_->vars_global()) {
        out << ",X_" << var;
    }
    if (m.type_ == LitCondLitType::premise) {
        for (auto var : m.state_->vars_local()) {
            out << ",X_" << var;
        }
    }
    out << ")";
    return out;
}

auto MatchCondLit::state() const -> StateCondLit & {
    return *state_;
}

auto MatchCondLit::type() const -> LitCondLitType {
    return type_;
}

auto MatchCondLit::match_(Assignment &ass, Symbol const *sym, VariableVec const &vars) -> bool {
    for (auto var : vars) {
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

// LitCondLit

auto operator<<(std::ostream &out, LitCondLitType type) -> std::ostream & {
    switch (type) {
        case LitCondLitType::empty: {
            out << "empty";
            break;
        }
        case LitCondLitType::premise: {
            out << "premise";
            break;
        }
        case LitCondLitType::lit: {
            out << "condlit";
            break;
        }
    }
    return out;
}

void LitCondLit::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::depend) {
        state().vars(vars, type() == LitCondLitType::premise);
    }
}

auto LitCondLit::do_domain() const -> bool {
    return type() != LitCondLitType::lit || state().domain();
}

auto LitCondLit::do_single_pass() const -> bool {
    return index_ == stratified_index;
}

auto LitCondLit::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type, std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    auto index = std::optional<size_t>{};
    if (index_ != std::numeric_limits<size_t>::max() && type == MatcherType::new_atoms) {
        index = index_;
    }
    auto &match = static_cast<MatchCondLit &>(*this);
    if (this->type() == LitCondLitType::empty) {
        return {make_atom_matcher(mbr, bound, state().base_empty(), match, type, offset_), index};
    }
    if (this->type() == LitCondLitType::premise) {
        return {make_atom_matcher(mbr, bound, state().base_premise(), match, type, offset_), index};
    }
    return {make_atom_matcher(mbr, bound, state().base_lit(), match, type, offset_), index};
}

auto LitCondLit::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    return 1;
}

void LitCondLit::do_print(std::ostream &out) const {
    out << "#cond_lit(" << type();
    for (auto var : state().vars_global()) {
        out << ","
            << "X_" << var;
    }
    if (type() == LitCondLitType::premise) {
        for (auto var : state().vars_local()) {
            out << ","
                << "X_" << var;
        }
    }
    out << ")";
    if (index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
}

auto LitCondLit::do_output(EvalContext const &ctx, OutputLit &out) const -> bool {
    if (type() == LitCondLitType::lit) {
        auto it = state().atom_find(ctx.ass());
        if (state().atom_is_fact(it)) {
            return false;
        }
        it.value().uid(out.cond_lit(it.value().uid()));
        return true;
    }
    return false;
}

auto LitCondLit::do_copy() const -> ULit {
    return std::make_unique<LitCondLit>(type(), state(), index_);
}

auto LitCondLit::do_hash() const -> size_t {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return Util::value_hash_record<LitCondLit>(type(), reinterpret_cast<uintptr_t>(&state()));
}

auto LitCondLit::do_equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitCondLit const *>(&other);
    return x != nullptr && std::make_tuple(type(), &state()) == std::make_tuple(x->type(), &x->state());
}

auto LitCondLit::do_compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *x = dynamic_cast<LitCondLit const *>(&other); x != nullptr) {
        return std::make_tuple(type(), &state()) <=> std::make_tuple(x->type(), &x->state());
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// LitCondLitStrat

namespace {

class MatcherCondLitStrat : public OnceMatcher {
  public:
    MatcherCondLitStrat(StateCondLit &state, Instantiator inst) : state_{&state}, inst_{std::move(inst)} {}

  private:
    auto do_once(EvalContext const &ctx) -> bool override {
        if (init_) {
            state_->base_empty().update(0);
        } else {
            init_ = true;
            inst_.init(ctx, 0);
        }
        auto [it, ins] = state_->add_empty(ctx.ass());
        if (ins) {
            CLINGO_REPORT(ctx.log(), trace) << "<<< begin nested instantiation";
            state_->base_empty().update(1);
            std::ignore = inst_.instantiate(ctx.log(), ctx.store(), ctx.out());
            CLINGO_REPORT(ctx.log(), trace) << ">>> end nested instantiation";
            std::ignore = state_->propagate();
        }
        return !it.value().is_false();
    }
    void do_print(std::ostream &out) const override {
        out << "#cond_lit(lit";
        for (auto var : state_->vars_global()) {
            out << ","
                << "X_" << var;
        }
        out << ")";
    }

    StateCondLit *state_;
    Instantiator inst_;
    bool init_ = false;
};

} // namespace

void LitCondLitStrat::do_init([[maybe_unused]] size_t gen) {
}

auto LitCondLitStrat::do_report(EvalContext const &ctx) -> bool {
    // In the stratified case, the conclusion is always false. Furthermore,
    // exactly one literal is bound. Thus, we can exit instantiation early
    // here.
    return state_->add_premise(ctx, premise_);
}

void LitCondLitStrat::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] OutputStm &out,
                                   [[maybe_unused]] Queue &queue) {
}

auto LitCondLitStrat::do_priority() const -> size_t {
    return 0;
}

void LitCondLitStrat::do_print_head(std::ostream &out) const {
    out << "#cond_lit(premise";
    for (auto var : state_->vars_global()) {
        out << ","
            << "X_" << var;
    }
    for (auto var : state_->vars_local()) {
        out << ","
            << "X_" << var;
    }
    out << ")";
}

void LitCondLitStrat::do_vars(VariableSet &vars, [[maybe_unused]] VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        state_->vars(vars, false);
    }
}

auto LitCondLitStrat::do_domain() const -> bool {
    return state_->domain();
}

auto LitCondLitStrat::do_single_pass() const -> bool {
    return true;
}

auto LitCondLitStrat::do_matcher(std::pmr::monotonic_buffer_resource &mbr, [[maybe_unused]] MatcherType type,
                                 [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    auto lin = Linearizer{mbr};
    auto queue = Queue{};
    lin.start(queue);
    lin.prepare(static_cast<InstanceCallback &>(*this), premise_, state_->vars(true));
    auto insts = queue.release();
    assert(insts.size() == 1);
    return {std::make_unique<MatcherCondLitStrat>(*state_, std::move(insts.front())), std::nullopt};
}

auto LitCondLitStrat::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    return 1;
}

void LitCondLitStrat::do_print(std::ostream &out) const {
    out << "#cond_lit(lit";
    for (auto var : state_->vars_global()) {
        out << ","
            << "X_" << var;
    }
    out << ")";
}

auto LitCondLitStrat::do_output(EvalContext const &ctx, OutputLit &out) const -> bool {
    if (auto it = state_->atom_find(ctx.ass()); !state_->atom_is_fact(it)) {
        it.value().uid(out.cond_lit(it.value().uid()));
        return true;
    }
    return false;
}

auto LitCondLitStrat::do_copy() const -> ULit {
    ULitVec premise;
    premise.reserve(premise_.size());
    for (auto const &lit : premise_) {
        premise.emplace_back(lit->copy());
    }
    return std::make_unique<LitCondLitStrat>(*state_, std::move(premise));
}

auto LitCondLitStrat::do_hash() const -> size_t {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return Util::value_hash_record<LitCondLitStrat>(reinterpret_cast<uintptr_t>(state_));
}

auto LitCondLitStrat::do_equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitCondLitStrat const *>(&other);
    return x != nullptr && state_ == x->state_;
}

auto LitCondLitStrat::do_compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *x = dynamic_cast<LitCondLitStrat const *>(&other); x != nullptr) {
        return state_ <=> x->state_;
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// StmCondLit

auto operator<<(std::ostream &out, StmCondLitType type) -> std::ostream & {
    switch (type) {
        case StmCondLitType::empty: {
            out << "empty";
            break;
        }
        case StmCondLitType::premise: {
            out << "premise";
            break;
        }
        case StmCondLitType::conclusion: {
            out << "conclusion";
            break;
        }
    }
    return out;
}

void StmCondLit::do_print_head(std::ostream &out) const {
    out << "#cond_lit(" << type_;
    for (auto var : state_->vars_global()) {
        out << ","
            << "X_" << var;
    }
    if (type_ != StmCondLitType::empty) {
        for (auto var : state_->vars_local()) {
            out << ","
                << "X_" << var;
        }
    }
    out << ")";
}
void StmCondLit::do_print(std::ostream &out) const {
    out << prio_ << ": ";
    print_head(out);
    if (index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

auto StmCondLit::do_body() const -> ULitVec const & {
    return body_;
}

auto StmCondLit::do_important() const -> VariableSet {
    return state_->vars(type_ != StmCondLitType::empty);
}

void StmCondLit::do_init(size_t gen) {
    switch (type_) {
        case StmCondLitType::empty: {
            state_->base_empty().ensure(gen);
            break;
        }
        case StmCondLitType::premise: {
            state_->base_premise().ensure(gen);
            break;
        }
        case StmCondLitType::conclusion: {
            state_->base_lit().ensure(gen);
            break;
        }
    }
}

auto StmCondLit::do_report(EvalContext const &ctx) -> bool {
    switch (type_) {
        case StmCondLitType::empty: {
            state_->add_empty(ctx.ass());
            break;
        }
        case StmCondLitType::premise: {
            std::ignore = state_->add_premise(ctx, body_);
            break;
        }
        case StmCondLitType::conclusion: {
            if (auto it = state_->atom_find(ctx.ass()); !it.value().is_false()) {
                // By construction, the literal at the end of the body is the
                // conclusion. Using a condition here is the easiest way to
                // obtain an id for the conclusion. It would also be possible
                // to implement something to get an id for a single literal.
                // However, it is also possible to optimize small size
                // conditions, which should provide similar storage benefits
                // and keeps the interface simpler.
                bool fact = true;
                auto &out = ctx.out().cond();
                for (auto const &lit : body_) {
                    if (lit->output(ctx, out)) {
                        fact = false;
                    }
                }
                state_->add_conclusion(ctx.ass(), it, ctx.out().cond_id(), fact);
            }
            break;
        }
    }
    return true;
}

void StmCondLit::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] OutputStm &out, Queue &queue) {
    switch (type_) {
        case StmCondLitType::empty: {
            if (state_->base_empty().has_update()) {
                if (index_ != stratified_index) {
                    queue.propagate(index_);
                }
            }
            break;
        }
        case StmCondLitType::premise: {
            if (state_->base_premise().has_update()) {
                if (index_ != stratified_index) {
                    queue.propagate(index_);
                }
            }
            // note that atoms not blocked at this point are not added to the premise base
            // thus, we have to propagate here already
            if (state_->propagate() && state_->index() != stratified_index) {
                queue.propagate(state_->index());
            }
            break;
        }
        case StmCondLitType::conclusion: {
            if (state_->base_lit().has_update()) {
                if (index_ != stratified_index) {
                    queue.propagate(index_);
                }
            }
            // propagate further conditional literals
            if (state_->propagate() && state_->index() != stratified_index) {
                queue.propagate(state_->index());
            }
            break;
        }
    }
}

auto StmCondLit::do_priority() const -> size_t {
    return prio_;
}

} // namespace CppClingo::Ground
