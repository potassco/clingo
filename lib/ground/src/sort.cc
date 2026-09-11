#include <clingo/ground/sort.hh>

#include <clingo/util/print.hh>

#include <typeindex>

namespace CppClingo::Ground {

auto BaseSort::size() const -> size_t {
    return atoms_.size();
}
auto BaseSort::index(Key const &key) const -> size_t {
    return atoms_.find(key) - atoms_.begin();
}
auto BaseSort::nth(size_t index) const -> AtomSet::const_iterator {
    return atoms_.nth(index);
}
auto BaseSort::nth(size_t index) -> AtomSet::iterator {
    return atoms_.nth(index);
}
auto BaseSort::groups() -> GroupMap & {
    return groups_;
}
auto BaseSort::atoms() -> AtomSet & {
    return atoms_;
}

// NOLINTBEGIN
class StateSort::GroupKey {
  private:
    struct priv_tag {};

  public:
    GroupKey([[maybe_unused]] priv_tag tag, Assignment &ass, VariableVec const &global) {
        auto *it = symbols_;
        for (auto var : global) {
            *it++ = ass[var].value();
        }
    }
    static void construct(auto &mbr, Assignment &ass, VariableVec const &global, GroupKey *&target) {
        auto size = global.size() * sizeof(Symbol);
        if (target == nullptr) {
            target = static_cast<GroupKey *>(mbr.allocate(size, alignof(GroupKey)));
        } else {
            std::destroy_at(target);
        }
        std::construct_at(target, priv_tag{}, ass, global);
    }
    auto symbols() const -> Symbol const * { return symbols_; }

  private:
    CLINGO_IGNORE_ZERO_SIZED_ARRAY_B
    Symbol symbols_[0];
    CLINGO_IGNORE_ZERO_SIZED_ARRAY_E
};
// NOLINTEND

auto StateSort::global() const -> VariableVec const & {
    return global_;
}
auto StateSort::symbols() -> SymbolVec & {
    symbols_.resize(global_.size());
    return symbols_;
}
auto StateSort::prev() const -> Term const & {
    return *prev_;
}
auto StateSort::next() const -> Term const & {
    return *next_;
}
auto StateSort::base() -> BaseSort & {
    return base_;
}
auto StateSort::insert_member(EvalContext const &ctx, Term const &term) -> BaseSortMember::AtomSet::iterator {
    auto group = insert_group(ctx).first;
    auto value = term.eval(ctx);
    assert(value.has_value());
    return member_base_.atoms().try_emplace({group - base_.groups().begin(), *value}).first;
}
auto StateSort::insert_group(EvalContext const &ctx) -> std::pair<GroupMap::iterator, bool> {
    GroupKey::construct(*mbr_, ctx.ass(), global_, group_key_);
    auto [it, inserted] = base_.groups().try_emplace(group_key_->symbols());
    if (inserted) {
        group_key_ = nullptr;
    }
    return {it, inserted};
}
void StateSort::insert_value(EvalContext const &ctx, GroupMap::iterator group, Term const &term) {
    if (auto value = term.eval(ctx); value) {
        group.value().emplace_back(*value);
    }
}
void StateSort::propagate(GroupMap::iterator group) {
    auto group_index = group - base_.groups().begin();
    auto &values = group.value();
    std::ranges::sort(values);
    values.erase(std::ranges::unique(values).begin(), values.end());
    if (values.size() < 2) {
        return;
    }
    auto prev = values.begin();
    for (auto next = std::next(prev); next != values.end(); ++prev, ++next) {
        base_.atoms().try_emplace({group_index, *prev, *next}, invalid_offset);
    }
}
void StateSort::output([[maybe_unused]] Logger &log, [[maybe_unused]] SymbolStore &store, OutputStm &out) {
    for (size_t index = 0; index < member_base_.size(); ++index) {
        auto &atom = member_base_.nth(index).value();
        if (atom.fact) {
            atom.uid = out.uid(true);
            continue;
        }
        if (!atom.uid.has_value()) {
            atom.uid = out.uid();
        }
        auto elems = std::vector<OutputStm::BdElem>{};
        elems.reserve(atom.supports.size());
        auto tuple = std::array{SymbolStore::num_ref(1)};
        for (auto const &support : atom.supports) {
            elems.emplace_back(tuple, IndexSpan{&support, 1});
        }
        auto guards = std::array{OutputStm::Guard{Relation::greater_equal, SymbolStore::num_ref(1)}};
        out.bd_aggr(*atom.uid, AggregateFunction::sum, elems, guards);
    }
}

auto MatchSortMember::vars() const -> VariableSet {
    auto vars = VariableSet{state_->global().begin(), state_->global().end()};
    value_->vars(vars);
    return vars;
}
auto MatchSortMember::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) -> VariableVec {
    return {bound.begin(), bound.end()};
}
auto MatchSortMember::match(EvalContext const &ctx, Key const &key) const -> bool {
    auto const *symbol = state_->base().groups().nth(std::get<0>(key)).key();
    for (auto var : state_->global()) {
        if (auto &assigned = ctx.ass()[var]; assigned) {
            if (*assigned != *symbol) {
                return false;
            }
        } else {
            assigned = *symbol;
        }
        ++symbol;
    }
    return value_->match(ctx, std::get<1>(key));
}
auto MatchSortMember::eval(EvalContext const &ctx) const -> std::optional<Key> {
    auto globals = SymbolVec{};
    for (auto var : state_->global()) {
        globals.emplace_back(ctx.ass()[var].value());
    }
    auto group = state_->base().groups().find(globals.data());
    auto value = value_->eval(ctx);
    if (group == state_->base().groups().end() || !value) {
        return std::nullopt;
    }
    return Key{group - state_->base().groups().begin(), *value};
}
auto operator<<(std::ostream &out, MatchSortMember const &match) -> std::ostream & {
    return out << "#sort_member(" << match.value() << ")";
}

void LitSortMember::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        vars.insert(state().global().begin(), state().global().end());
    }
    if (mode != VarSelectMode::depend) {
        value().vars(vars);
    }
}
auto LitSortMember::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                               std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    auto index = type == MatcherType::new_atoms ? std::make_optional(state().member_index()) : std::nullopt;
    return {make_atom_matcher(mbr, bound, state().member_base(), static_cast<MatchSortMember &>(*this), type, offset_),
            index};
}
auto LitSortMember::do_score([[maybe_unused]] std::vector<bool> const &bound, [[maybe_unused]] double estimate) const
    -> double {
    return 1;
}
auto LitSortMember::do_domain_size([[maybe_unused]] EstimateSelector sel) const -> std::optional<double> {
    return std::nullopt;
}
void LitSortMember::do_print(std::ostream &out) const {
    out << "#sort_member(" << value() << ")";
}
auto LitSortMember::do_output(EvalContext const &ctx, OutputLit &out) const -> bool {
    auto key = eval(ctx);
    assert(key.has_value());
    auto &atom = state().member_base().atoms().find(*key).value();
    if (atom.fact) {
        return false;
    }
    atom.uid = out.bd_aggr(Sign::none, atom.uid);
    return true;
}
auto LitSortMember::do_copy() const -> ULit {
    return std::make_unique<LitSortMember>(state(), value().copy());
}
auto LitSortMember::do_hash() const -> size_t {
    return Util::value_hash_record<LitSortMember>(reinterpret_cast<uintptr_t>(&state()), value());
}
auto LitSortMember::do_equal_to(Lit const &other) const -> bool {
    auto const *member = dynamic_cast<LitSortMember const *>(&other);
    return member != nullptr && &state() == &member->state() && value() == member->value();
}
auto LitSortMember::do_compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *member = dynamic_cast<LitSortMember const *>(&other); member != nullptr) {
        return std::make_tuple(&state(), std::cref(value())) <=>
               std::make_tuple(&member->state(), std::cref(member->value()));
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

auto StmSortMember::do_important() const -> VariableSet {
    auto vars = VariableSet{state_->global().begin(), state_->global().end()};
    value_->vars(vars);
    return vars;
}
void StmSortMember::do_init(size_t gen) {
    state_->member_base().ensure(gen);
}
auto StmSortMember::do_report(EvalContext const &ctx) -> bool {
    auto atom = state_->insert_member(ctx, *value_);
    auto fact = true;
    for (auto const &lit : std::span{body_}.first(num_cond_)) {
        if (lit->output(ctx, ctx.out().cond())) {
            fact = false;
        }
    }
    atom.value().supports.emplace_back(ctx.out().cond_id());
    atom.value().fact = atom.value().fact || fact;
    return true;
}
void StmSortMember::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] OutputStm &out, Queue &queue) {
    if (state_->member_base().has_update() && state_->member_index() != stratified_index) {
        queue.propagate(state_->member_index());
    }
}
void StmSortMember::do_print_head(std::ostream &out) const {
    out << "#sort_member(" << *value_ << ")";
}
void StmSortMember::do_print(std::ostream &out) const {
    out << priority_ << ": ";
    print_head(out);
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &stream, auto const &lit) { stream << *lit; }) << ".";
}

auto MatchSort::vars() const -> VariableSet {
    auto vars = VariableSet{state_->global().begin(), state_->global().end()};
    state_->prev().vars(vars);
    state_->next().vars(vars);
    return vars;
}
auto MatchSort::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) -> VariableVec {
    return {bound.begin(), bound.end()};
}
auto MatchSort::match(EvalContext const &ctx, Key const &key) const -> bool {
    auto &ass = ctx.ass();
    auto const *symbol = state_->base().groups().nth(std::get<0>(key)).key();
    for (auto var : state_->global()) {
        if (auto &value = ass[var]; value) {
            if (*value != *symbol) {
                return false;
            }
        } else {
            value = *symbol;
        }
        ++symbol;
    }
    return state_->prev().match(ctx, std::get<1>(key)) && state_->next().match(ctx, std::get<2>(key));
}
auto MatchSort::eval(EvalContext const &ctx) const -> std::optional<Key> {
    auto globals = SymbolVec{};
    globals.reserve(state_->global().size());
    for (auto var : state_->global()) {
        globals.emplace_back(ctx.ass()[var].value());
    }
    auto group = state_->base().groups().find(globals.data());
    auto prev = state_->prev().eval(ctx);
    auto next = state_->next().eval(ctx);
    if (group == state_->base().groups().end() || !prev || !next) {
        return std::nullopt;
    }
    return Key{static_cast<size_t>(group - state_->base().groups().begin()), *prev, *next};
}
auto operator<<(std::ostream &out, [[maybe_unused]] MatchSort const &match) -> std::ostream & {
    return out << "#sort";
}

auto StmSortElem::do_body() const -> ULitVec const & {
    return body_;
}
auto StmSortElem::do_important() const -> VariableSet {
    auto vars = VariableSet{state_->global().begin(), state_->global().end()};
    value_->vars(vars);
    return vars;
}
auto StmSortElem::do_is_important(size_t index) const -> bool {
    return index < num_cond_;
}
void StmSortElem::do_init(size_t gen) {
    state_->base().ensure(gen);
}
auto StmSortElem::do_report(EvalContext const &ctx) -> bool {
    auto group = state_->insert_group(ctx).first;
    StateSort::insert_value(ctx, group, *value_);
    return true;
}
void StmSortElem::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] OutputStm &out,
                               [[maybe_unused]] Queue &queue) {
}
auto StmSortElem::do_priority() const -> size_t {
    return priority_;
}
void StmSortElem::do_print_head(std::ostream &out) const {
    out << "#sort_elem(" << *value_ << ")";
}
void StmSortElem::do_print(std::ostream &out) const {
    print_head(out);
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &stream, auto const &lit) { stream << *lit; }) << ".";
}

namespace {
class MatcherSortStrat : public Matcher {
  public:
    MatcherSortStrat(StateSort &state, InstantiatorVec insts, UMatcher matcher)
        : state_{&state}, insts_{std::move(insts)}, matcher_{std::move(matcher)} {}

  private:
    void do_init(InstantiationContext const &ctx, size_t gen) override {
        for (auto &inst : insts_) {
            inst.init(ctx, gen);
        }
        matcher_->init(ctx, gen);
    }
    void do_match(EvalContext const &ctx) override {
        auto [group, inserted] = state_->insert_group(ctx);
        if (inserted) {
            auto symbols = state_->symbols().begin();
            for (auto var : state_->global()) {
                *symbols++ = ctx.ass()[var].value();
            }
            for (auto &inst : insts_) {
                std::ignore = inst.instantiate(ctx.log(), ctx.store(), ctx.out(), nullptr);
            }
            state_->propagate(group);
            state_->base().update(0);
        }
        matcher_->match(ctx);
    }
    [[nodiscard]] auto do_next(EvalContext const &ctx) -> bool override { return matcher_->next(ctx); }
    void do_print(std::ostream &out) const override { matcher_->print(out); }
    [[nodiscard]] auto do_type() const -> MatcherType override { return matcher_->type(); }

    StateSort *state_;
    InstantiatorVec insts_;
    UMatcher matcher_;
};
} // namespace

void LitSortStrat::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        vars.insert(state().global().begin(), state().global().end());
    }
    if (mode != VarSelectMode::depend) {
        state().prev().vars(vars);
        state().next().vars(vars);
    }
}
auto LitSortStrat::do_domain() const -> bool {
    return true;
}
auto LitSortStrat::do_single_pass() const -> bool {
    return true;
}
auto LitSortStrat::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                              std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    offset_ = invalid_offset;
    auto linearizer = Linearizer{mbr, EstimateFunction::average, EstimateSelector::pred};
    auto queue = Queue{};
    linearizer.start(queue);
    for (auto &elem : elems_) {
        linearizer.prepare(elem, elem.body(), elem.important());
    }
    return {std::make_unique<MatcherSortStrat>(
                state(), queue.release(),
                make_atom_matcher(mbr, bound, state().base(), static_cast<MatchSort &>(*this), type, offset_)),
            std::nullopt};
}
auto LitSortStrat::do_score([[maybe_unused]] std::vector<bool> const &bound, [[maybe_unused]] double estimate) const
    -> double {
    return domain() ? 0 : std::numeric_limits<double>::max();
}
auto LitSortStrat::do_domain_size([[maybe_unused]] EstimateSelector sel) const -> std::optional<double> {
    return std::nullopt;
}
void LitSortStrat::do_print(std::ostream &out) const {
    out << static_cast<MatchSort const &>(*this);
}
auto LitSortStrat::do_output([[maybe_unused]] EvalContext const &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}
auto LitSortStrat::do_copy() const -> ULit {
    return std::make_unique<LitSortStrat>(state(), elems_);
}
auto LitSortStrat::do_hash() const -> size_t {
    return Util::value_hash_record<LitSortStrat>(reinterpret_cast<uintptr_t>(this));
}
auto LitSortStrat::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}
auto LitSortStrat::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

} // namespace CppClingo::Ground
