#include <gringo/ground/condlit.hh>
#include <gringo/ground/matcher.hh>

#include <gringo/core/logger.hh>

#include <gringo/util/print.hh>

#include <typeindex>

namespace Gringo::Ground {

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

auto StateCondLit::vars_global() const -> VariableVec const & { return global_; }

auto StateCondLit::vars_local() const -> VariableVec const & { return local_; }

auto StateCondLit::index() const -> size_t { return index_; }

auto StateCondLit::add_empty(Assignment const &ass) -> std::pair<MapAtomCondLit::iterator, bool> {
    auto const syms = syms_atoms_.push_map(global_, [&ass](auto var) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        return ass[var].value();
    });
    auto [it, ins] = atoms_.try_emplace(syms.data());
    if (ins) {
        if (it.value().enqueue(elems_)) {
            propagate_.emplace_back(std::distance(atoms_.begin(), it));
        }
    } else {
        syms_atoms_.pop();
    }
    return {it, ins};
}

void StateCondLit::add_premise(Assignment const &ass, bool fact) {
    auto it = atom_find(ass);
    // no further elements have to be accumulated if the literal is false
    if (it.value().is_false()) {
        return;
    }
    auto syms_elem = syms_elems_.push_map(Util::enumerate{local_.size() + 1}, [this, it, &ass](size_t i) {
        if (i == 0) {
            return Symbol::from_rep(std::distance(atoms_.begin(), it));
        }
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        return ass[local_[i - 1]].value();
    });

    auto [jt, ins] = elems_.try_emplace(syms_elem.data(), fact, has_conclusion_);
    // an element can only be added once
    assert(ins);

    auto &atom = it.value();
    auto &elem = jt.value();

    atom.add_elem(std::distance(elems_.begin(), jt));
    if (elem.is_blocked()) {
        if (!fact || has_conclusion_) {
            base_premise_.add(jt);
        }
    } else if (atom.enqueue(elems_)) {
        propagate_.emplace_back(atom_index(it));
    }
}

void StateCondLit::add_conclusion(Assignment const &ass, bool fact) {
    auto it = atom_find(ass);
    assert(it != atoms_.end());
    auto jt = elem_find(ass, it);
    assert(jt != elems_.end());
    auto &atom = it.value();
    auto &elem = jt.value();
    elem.mark_conclusion(fact);
    if (atom.enqueue(elems_)) {
        propagate_.emplace_back(atom_index(it));
    }
}

auto StateCondLit::propagate() -> bool {
    bool res = false;
    for (auto atom_index : propagate_) {
        auto it = atoms_.nth(atom_index);
        auto &atom = it.value();
        if (atom.propagate(elems_)) {
            base_lit_.add(it);
            res = true;
        }
    }
    propagate_.clear();
    return res;
}

auto StateCondLit::domain() const -> bool { return domain_ && !rec_premise_; }

auto StateCondLit::base_empty() -> BaseCondLitEmpty & { return base_empty_; }

auto StateCondLit::base_premise() -> BaseCondLitPremise & { return base_premise_; }

auto StateCondLit::base_lit() -> BaseCondLit & { return base_lit_; }

auto StateCondLit::lit_is_fact(Assignment const &ass) -> bool {
    if (rec_premise_) {
        return false;
    }
    auto it = atom_find(ass);
    assert(it != atoms_.end());
    return it->second.is_fact(elems_);
}

auto StateCondLit::atom_index(Assignment &ass) const -> std::optional<size_t> {
    if (auto it = atom_find(ass); it != atoms_.end()) {
        return atom_index(it);
    }
    return std::nullopt;
}

auto StateCondLit::atom_nth(size_t index) -> MapAtomCondLit::iterator { return atoms_.nth(index); }

auto StateCondLit::atom_index(MapAtomCondLit::const_iterator it) const -> size_t {
    return std::distance(atoms_.begin(), it);
}

auto StateCondLit::atom_find(Assignment const &ass) const -> MapAtomCondLit::const_iterator {
    temp_syms_.clear();
    for (auto var : global_) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        temp_syms_.emplace_back(ass[var].value());
    }
    return atoms_.find(temp_syms_.data());
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

// MatchCondLit

auto MatchCondLit::vars() const -> VariableSet { return state_->vars(type_ == LitCondLitType::premise); }

auto MatchCondLit::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
    static_cast<void>(this);
    return {bound.begin(), bound.end()};
};

auto MatchCondLit::match([[maybe_unused]] SymbolStore &store, Symbol const *sym, Assignment &ass) const -> bool {
    if (type_ == LitCondLitType::premise) {
        auto atom = state_->atom_nth(Symbol::to_rep(*sym));
        return match_(ass, atom->first, state_->vars_global()) && match_(ass, std::next(sym), state_->vars_local());
    }
    return match_(ass, sym, state_->vars_global());
};

auto MatchCondLit::eval([[maybe_unused]] SymbolStore &store, Assignment &ass) const -> std::optional<Symbol const *> {
    eval_.clear();
    bool is_premise = type_ == LitCondLitType::premise;
    if (is_premise) {
        if (auto index = state_->atom_index(ass); index) {
            eval_.emplace_back(Symbol::from_rep(*index));
        } else {
            return std::nullopt;
        }
    }
    for (auto var : is_premise ? state_->vars_local() : state_->vars_global()) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        eval_.emplace_back(ass[var].value());
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

auto MatchCondLit::state() const -> StateCondLit & { return *state_; }

auto MatchCondLit::type() const -> LitCondLitType { return type_; }

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
    // TODO: domain if elements are domain
    if (type() == LitCondLitType::lit) {
        return state().domain();
    }
    return true;
}

auto LitCondLit::do_recursive() const -> bool { return index_ != stratified_index; }

auto LitCondLit::do_matcher(MatcherType type,
                            std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    auto index = std::optional<size_t>{};
    if (index_ != std::numeric_limits<size_t>::max() && type == MatcherType::new_atoms) {
        index = index_;
    }
    auto &match = static_cast<MatchCondLit &>(*this);
    if (this->type() == LitCondLitType::empty) {
        return {make_atom_matcher(bound, state().base_empty(), match, type), index};
    }
    if (this->type() == LitCondLitType::premise) {
        return {make_atom_matcher(bound, state().base_premise(), match, type), index};
    }
    return {make_atom_matcher(bound, state().base_lit(), match, type), index};
}

auto LitCondLit::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return 1; }

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

auto LitCondLit::do_output(InstantiationContext &ctx, OutputLit &out) const -> bool {
    if (type() == LitCondLitType::lit && !state().lit_is_fact(ctx.ass())) {
        // TODO: fix once there is a proper output
        out.cond_lit(0);
        return true;
    }
    return false;
}

auto LitCondLit::do_copy() const -> ULit { return std::make_unique<LitCondLit>(type(), state(), index_); }

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
    auto do_once(InstantiationContext &ctx) -> bool override {
        if (init_) {
            state_->base_empty().update(0);
        } else {
            init_ = true;
            inst_.init(ctx.store(), 0);
        }
        auto [it, ins] = state_->add_empty(ctx.ass());
        if (ins) {
            GRINGO_REPORT(ctx.log(), trace) << "<<< begin nested instantiation";
            state_->base_empty().update(1);
            std::ignore = inst_.instantiate(ctx.log(), ctx.store(), ctx.out());
            GRINGO_REPORT(ctx.log(), trace) << ">>> end nested instantiation";
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

void LitCondLitStrat::do_init([[maybe_unused]] size_t gen) {}

auto LitCondLitStrat::do_report(InstantiationContext &ctx) -> bool {
    // TODO: get uid
    auto &out = ctx.out().cond_lit_premise(0);
    bool fact = true;
    for (auto const &lit : premise_) {
        if (lit->output(ctx, out)) {
            fact = false;
        }
    }
    out.end();
    state_->add_premise(ctx.ass(), fact);
    // In the stratified case, the conclusion is always false. Furthermore,
    // exactly one literal is bound. Thus, we can exit instantiation early
    // here.
    return !fact;
}

void LitCondLitStrat::do_propagate([[maybe_unused]] Queue &queue) {}

auto LitCondLitStrat::do_priority() const -> size_t { return 0; }

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

auto LitCondLitStrat::do_domain() const -> bool { return state_->domain(); }

auto LitCondLitStrat::do_recursive() const -> bool { return false; }

auto LitCondLitStrat::do_matcher([[maybe_unused]] MatcherType type, [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    Queue queue;
    Linearizer lin;
    lin.start(queue, state_->domain());
    lin.prepare(static_cast<InstanceCallback &>(*this), premise_, state_->vars(true));
    auto insts = queue.release();
    assert(insts.size() == 1);
    return {std::make_unique<MatcherCondLitStrat>(*state_, std::move(insts.front())), std::nullopt};
}

auto LitCondLitStrat::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return 1; }

void LitCondLitStrat::do_print(std::ostream &out) const {
    out << "#cond_lit(lit";
    for (auto var : state_->vars_global()) {
        out << ","
            << "X_" << var;
    }
    out << ")";
}

auto LitCondLitStrat::do_output(InstantiationContext &ctx, OutputLit &out) const -> bool {
    if (!state_->lit_is_fact(ctx.ass())) {
        out.cond_lit(0);
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
    for (auto var : base_->vars_global()) {
        out << ","
            << "X_" << var;
    }
    if (type_ != StmCondLitType::empty) {
        for (auto var : base_->vars_local()) {
            out << ","
                << "X_" << var;
        }
    }
    out << ")";
    if (index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
}
void StmCondLit::do_print(std::ostream &out) const {
    out << prio_ << ": ";
    print_head(out);
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

auto StmCondLit::do_body() const -> ULitVec const & { return body_; }

auto StmCondLit::do_important() const -> VariableSet { return base_->vars(type_ != StmCondLitType::empty); }

void StmCondLit::do_init([[maybe_unused]] size_t gen) {
    // by construction, this statement does not increment the generation
}

auto StmCondLit::do_report(InstantiationContext &ctx) -> bool {
    switch (type_) {
        case StmCondLitType::empty: {
            base_->add_empty(ctx.ass());
            break;
        }
        case StmCondLitType::premise: {
            // TODO: get uid
            bool fact = true;
            auto &out = ctx.out().cond_lit_premise(0);
            for (auto const &lit : body_) {
                if (lit->output(ctx, out)) {
                    fact = false;
                }
            }
            out.end();
            base_->add_premise(ctx.ass(), fact);
            break;
        }
        case StmCondLitType::conclusion: {
            // TODO: fix once there is a proper output
            bool fact = true;
            auto &out = ctx.out().cond_lit_conclusion(0);
            for (auto const &lit : body_) {
                if (lit->output(ctx, out)) {
                    fact = false;
                }
            }
            out.end();
            base_->add_conclusion(ctx.ass(), fact);
            break;
        }
    }
    return true;
}

void StmCondLit::do_propagate([[maybe_unused]] Queue &queue) {
    switch (type_) {
        case StmCondLitType::empty: {
            if (base_->base_empty().has_update()) {
                if (index_ != stratified_index) {
                    queue.propagate(index_);
                }
            }
            break;
        }
        case StmCondLitType::premise: {
            if (base_->base_premise().has_update()) {
                if (index_ != stratified_index) {
                    queue.propagate(index_);
                }
            }
            // note that atoms not blocked at this point are not added to the premise base
            // thus, we have to propagate here already
            if (base_->propagate() && base_->index() != stratified_index) {
                queue.propagate(base_->index());
            }
            break;
        }
        case StmCondLitType::conclusion: {
            if (base_->base_lit().has_update()) {
                if (index_ != stratified_index) {
                    queue.propagate(index_);
                }
            }
            // propagate further conditional literals
            if (base_->propagate() && base_->index() != stratified_index) {
                queue.propagate(base_->index());
            }
            break;
        }
    }
}

auto StmCondLit::do_priority() const -> size_t { return prio_; }

} // namespace Gringo::Ground
