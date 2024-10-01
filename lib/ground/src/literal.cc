#include <gringo/ground/literal.hh>
#include <gringo/ground/matcher.hh>

#include <gringo/util/print.hh>

#include <typeindex>

namespace Gringo::Ground {

void LitInterval::do_print(std::ostream &out) const { out << *lhs_ << "=" << *lower_ << ".." << *upper_; }

auto LitInterval::do_output([[maybe_unused]] InstantiationContext &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitInterval::do_copy() const -> ULit {
    return std::make_unique<LitInterval>(lhs_->copy(), lower_->copy(), upper_->copy());
}

auto LitInterval::do_domain() const -> bool { return true; }

auto LitInterval::do_single_pass() const -> bool { return true; }

void LitInterval::do_vars(VariableSet &vars, VarSelectMode mode) const {
    switch (mode) {
        case VarSelectMode::all: {
            lhs_->vars(vars);
            lower_->vars(vars);
            upper_->vars(vars);
            break;
        }
        case VarSelectMode::provide: {
            lhs_->vars(vars);
            break;
        }
        case VarSelectMode::depend: {
            lower_->vars(vars);
            upper_->vars(vars);
            break;
        }
    }
}

auto LitInterval::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr,
                             [[maybe_unused]] MatcherType type,
                             std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    return {make_interval_matcher(bound, *lhs_, *lower_, *upper_), std::nullopt};
}

auto LitInterval::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    if (auto *l = dynamic_cast<TermSymbol *>(lower_.get()), *r = dynamic_cast<TermSymbol *>(lower_.get());
        l != nullptr && r != nullptr) {
        VariableSet vars;
        lhs_->vars(vars);
        if (std::all_of(vars.begin(), vars.end(), [&bound](auto var) { return bound[var]; })) {
            return -1;
        }
        auto sl = l->symbol();
        auto sr = r->symbol();
        if (sl.type() != SymbolType::number || sr.type() != SymbolType::number) {
            return -1;
        }
        auto const &nl = sl.num();
        auto const &nr = sr.num();
        if (nl > nr) {
            return -1;
        }
        auto d = nr - nl;
        if (auto id = d.as_int(); id) {
            return *id;
        }
        return std::numeric_limits<double>::max();
    }
    // NOLINTNEXTLINE(readability-magic-numbers)
    return 100;
}

auto LitInterval::do_hash() const -> size_t { return Util::value_hash_record<LitInterval>(lhs_, lower_, upper_); }

auto LitInterval::do_equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitInterval const *>(&other);
    if (x != nullptr) {
        return std::tie(*lhs_, *lower_, *upper_) == std::tie(*x->lhs_, *x->lower_, *x->upper_);
    }
    return false;
}

auto LitInterval::do_compare_to(Lit const &other) const -> std::weak_ordering {
    auto const *x = dynamic_cast<LitInterval const *>(&other);
    if (x != nullptr) {
        return std::tie(*lhs_, *lower_, *upper_) <=> std::tie(*x->lhs_, *x->lower_, *x->upper_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// LitBool

void LitBool::do_print(std::ostream &out) const { out << (value_ ? "#true" : "#false"); }

auto LitBool::do_output([[maybe_unused]] InstantiationContext &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitBool::do_copy() const -> ULit { return std::make_unique<LitBool>(value_); }

auto LitBool::do_domain() const -> bool { return true; }

auto LitBool::do_single_pass() const -> bool { return true; }

void LitBool::do_vars([[maybe_unused]] VariableSet &vars, [[maybe_unused]] VarSelectMode mode) const {}

auto LitBool::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr, [[maybe_unused]] MatcherType type,
                         [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    if (value_) {
        return {make_once_matcher(), std::nullopt};
    }
    // note: for completeness; should not happen
    class NeverMatcher : public Matcher {
      public:
        NeverMatcher() = default;

      private:
        void do_init([[maybe_unused]] SymbolStore &store, [[maybe_unused]] size_t gen) override {}
        void do_match([[maybe_unused]] InstantiationContext &ctx) override {}
        auto do_next([[maybe_unused]] InstantiationContext &ctx) -> bool override { return false; }
        void do_print(std::ostream &out) const override { out << "#never"; }
    };
    return {std::make_unique<NeverMatcher>(), std::nullopt};
}

auto LitBool::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return -1; }

auto LitBool::do_hash() const -> size_t { return Util::value_hash_record<LitBool>(value_); }

auto LitBool::do_equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitBool const *>(&other);
    if (x != nullptr) {
        return std::tie(value_) == std::tie(x->value_);
    }
    return false;
}

auto LitBool::do_compare_to(Lit const &other) const -> std::weak_ordering {
    auto const *x = dynamic_cast<LitBool const *>(&other);
    if (x != nullptr) {
        return static_cast<int>(value_) <=> static_cast<int>(x->value_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// LitComparison

void LitComparison::do_print(std::ostream &out) const { out << *lhs_ << cmp_ << *rhs_; }

auto LitComparison::do_output([[maybe_unused]] InstantiationContext &ctx,
                              [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitComparison::do_copy() const -> ULit {
    return std::make_unique<LitComparison>(lhs_->copy(), cmp_, rhs_->copy());
}

auto LitComparison::do_domain() const -> bool { return true; }

auto LitComparison::do_single_pass() const -> bool { return true; }

void LitComparison::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (cmp_ != Relation::equal) {
        mode = VarSelectMode::all;
    }
    switch (mode) {
        case VarSelectMode::all: {
            lhs_->vars(vars);
            rhs_->vars(vars);
            break;
        }
        case VarSelectMode::provide: {
            lhs_->vars(vars, true);
            break;
        }
        case VarSelectMode::depend: {
            // Note: the rewriting ensures that if variables can be provided,
            //       then all of them can be provided.
            VariableSet provide;
            lhs_->vars(provide, true);
            if (provide.empty()) {
                lhs_->vars(vars);
            }
            rhs_->vars(vars);
            break;
        }
    }
}

auto LitComparison::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr,
                               [[maybe_unused]] MatcherType type, [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    return {make_comp_matcher(bound, *lhs_, cmp_, *rhs_), std::nullopt};
}

auto LitComparison::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return -1; }

auto LitComparison::do_hash() const -> size_t {
    if (cmp_ == Relation::equal && *rhs_ < *lhs_) {
        return Util::value_hash_record<LitComparison>(rhs_, cmp_, lhs_);
    }
    return Util::value_hash_record<LitComparison>(lhs_, cmp_, rhs_);
}

auto LitComparison::do_equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitComparison const *>(&other);
    if (x != nullptr) {
        if (cmp_ == Relation::equal && x->cmp_ == Relation::equal) {
            return std::tie(*lhs_, *rhs_) == std::tie(*x->lhs_, *x->rhs_) ||
                   std::tie(*lhs_, *rhs_) == std::tie(*x->rhs_, *x->lhs_);
        }
        return std::tie(*lhs_, cmp_, *rhs_) == std::tie(*x->lhs_, x->cmp_, *x->rhs_);
    }
    return false;
}

auto LitComparison::do_compare_to(Lit const &other) const -> std::weak_ordering {
    auto const *x = dynamic_cast<LitComparison const *>(&other);
    if (x != nullptr) {
        if (cmp_ == Relation::equal && x->cmp_ == Relation::equal && ((*rhs_ < *lhs_) != (*x->rhs_ < *x->lhs_))) {
            return std::tie(*lhs_, *rhs_) <=> std::tie(*x->rhs_, *x->lhs_);
        }
        return std::tie(*lhs_, cmp_, *rhs_) <=> std::tie(*x->lhs_, x->cmp_, *x->rhs_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// LitFactCheck

void LitFactCheck::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        atom_->vars(vars);
    }
}

auto LitFactCheck::do_domain() const -> bool { return true; }

auto LitFactCheck::do_single_pass() const -> bool { return true; }

auto LitFactCheck::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr,
                              [[maybe_unused]] MatcherType type, [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    return {make_non_fact_matcher(*base_, *atom_, *target_), std::nullopt};
}

auto LitFactCheck::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return 0; }

void LitFactCheck::do_print(std::ostream &out) const { out << "#not_fact " << *atom_; }

auto LitFactCheck::do_output([[maybe_unused]] InstantiationContext &ctx,
                             [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitFactCheck::do_copy() const -> ULit { return std::make_unique<LitFactCheck>(*base_, *atom_, *target_); }

auto LitFactCheck::do_hash() const -> size_t { return Util::value_hash_record<LitFactCheck>(*atom_); }

auto LitFactCheck::do_equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitFactCheck const *>(&other);
    return x != nullptr && *atom_ == *x->atom_;
}

auto LitFactCheck::do_compare_to(Lit const &other) const -> std::weak_ordering {
    auto const *x = dynamic_cast<LitFactCheck const *>(&other);
    if (x != nullptr) {
        return *atom_ <=> *x->atom_;
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// LitSymbolic

void LitSymbolic::do_print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
}

auto LitSymbolic::do_output(InstantiationContext &ctx, OutputLit &out) const -> bool {
    if ((index_ == stratified_index || sign_ == Sign::none) && base_->domain()) {
        return false;
    }
    auto get_symbol = [&, this]() -> std::optional<Symbol> {
        if (offset_ != invalid_offset) {
            return base_->nth(offset_).key();
        }
        if (sign_ == Sign::once) {
            return symbol_;
        }
        return atom_->eval(ctx.store(), ctx.ass());
    };
    if (auto sym = get_symbol(); sym) {
        if (sign_ == Sign::once ? index_ == stratified_index && !base_->contains(*sym) : base_->is_fact(*sym)) {
            return false;
        }
        out.lit(sign_, *sym);
    } else {
        // note: cannot happen by construction
        out.boolean(false);
    }
    return true;
}

auto LitSymbolic::do_copy() const -> ULit {
    return std::make_unique<LitSymbolic>(*base_, sign_, atom_->copy(), index_, domain_);
}

auto LitSymbolic::do_domain() const -> bool { return domain_ || (index_ == stratified_index && base_->domain()); }

auto LitSymbolic::do_single_pass() const -> bool { return sign_ != Sign::none || index_ == stratified_index; }

void LitSymbolic::do_vars(VariableSet &vars, VarSelectMode mode) const {
    switch (mode) {
        case VarSelectMode::all: {
            atom_->vars(vars);
            break;
        }
        case VarSelectMode::provide: {
            if (sign_ == Sign::none || (sign_ == Sign::twice && index_ == stratified_index)) {
                atom_->vars(vars);
            }
            break;
        }
        case VarSelectMode::depend: {
            if (sign_ == Sign::once || (sign_ == Sign::twice && index_ != stratified_index)) {
                atom_->vars(vars);
            }
            break;
        }
    }
}

auto LitSymbolic::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                             std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    offset_ = invalid_offset;
    if (sign_ == Sign::once) {
        return {make_non_fact_matcher(*base_, *atom_, symbol_), std::nullopt};
    }
    if (sign_ == Sign::twice && index_ != stratified_index) {
        return {make_once_matcher(), std::nullopt};
    }

    auto index = std::optional<size_t>{};
    if (index_ != stratified_index && type == MatcherType::new_atoms) {
        index = index_;
    }
    return {make_atom_matcher(mbr, bound, *base_, *atom_, type, offset_), index};
}

auto LitSymbolic::do_score(std::vector<bool> const &bound) const -> double {
    if (sign_ != Sign::once) {
        // TODO: Somehow gringo previously added 10,000,000 if all variables were
        // bound. I don't see the point of this?
        return atom_->score(static_cast<double>(base_->size()), bound);
    }
    return 0;
}

auto LitSymbolic::do_hash() const -> size_t { return Util::value_hash_record<LitSymbolic>(sign_, atom_); }

auto LitSymbolic::do_equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitSymbolic const *>(&other);
    return x != nullptr && std::tie(sign_, *atom_) == std::tie(x->sign_, *x->atom_);
}

auto LitSymbolic::do_compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *x = dynamic_cast<LitSymbolic const *>(&other); x != nullptr) {
        return std::tie(sign_, *atom_) <=> std::tie(x->sign_, *x->atom_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// LitProject
// TODO:
// - quite a bit of c&p
// - composition...

void LitProject::State::init(SymbolStore &store, size_t gen) {
    base_->update(gen);
    for (size_t n = base_->end(MatcherType::all_atoms); imported_ != n; ++imported_) {
        auto atom = base_->nth(imported_);
        for (auto &sym : ass_) {
            sym = std::nullopt;
        }
        if (p_body_->match(store, atom->first, ass_)) {
            if (auto sym = p_head_->eval(store, ass_); sym) {
                p_base_.add(*sym, atom->second.state);
            }
        }
    }
    p_base_.update(gen);
}

void LitProject::do_print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
}

auto LitProject::do_output(InstantiationContext &ctx, OutputLit &out) const -> bool {
    // Note: eval can be avoided for lookup matchers
    if ((index_ == stratified_index || sign_ == Sign::none) && state_->p_base().domain()) {
        return false;
    }
    auto get_symbol = [&, this]() -> std::optional<Symbol> {
        if (offset_ != invalid_offset) {
            return state_->p_base().nth(offset_).key();
        }
        if (sign_ == Sign::once) {
            return symbol_;
        }
        return p_atom_->eval(ctx.store(), ctx.ass());
    };
    if (auto p_sym = get_symbol()) {
        if (sign_ == Sign::once ? index_ == stratified_index && !state_->p_base().contains(*p_sym)
                                : state_->p_base().is_fact(*p_sym)) {
            return false;
        }
    }
    if (auto sym = atom_->eval(ctx.store(), ctx.ass())) {
        out.lit(sign_, *sym);
    } else {
        // note: cannot happen by construction
        out.boolean(false);
    }
    return true;
}

auto LitProject::do_copy() const -> ULit {
    return std::make_unique<LitProject>(*state_, sign_, atom_->copy(), p_atom_->copy(), index_, domain_);
}

auto LitProject::do_domain() const -> bool {
    return domain_ || (index_ == stratified_index && state_->base().domain());
}

auto LitProject::do_single_pass() const -> bool { return sign_ != Sign::none || index_ == stratified_index; }

void LitProject::do_vars(VariableSet &vars, VarSelectMode mode) const {
    switch (mode) {
        case VarSelectMode::all: {
            atom_->vars(vars);
            break;
        }
        case VarSelectMode::provide: {
            if (sign_ == Sign::none || (sign_ == Sign::twice && index_ == stratified_index)) {
                atom_->vars(vars);
            }
            break;
        }
        case VarSelectMode::depend: {
            if (sign_ == Sign::once || (sign_ == Sign::twice && index_ != stratified_index)) {
                atom_->vars(vars);
            }
            break;
        }
    }
}

auto LitProject::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                            std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    class MatcherProject : public Matcher {
      public:
        MatcherProject(State &state, UMatcher matcher) : state_{&state}, matcher_{std::move(matcher)} {}

      private:
        void do_init(SymbolStore &store, size_t gen) override {
            state_->init(store, gen);
            matcher_->init(store, gen);
        }
        void do_match(InstantiationContext &ctx) override { matcher_->match(ctx); }
        auto do_next(InstantiationContext &ctx) -> bool override { return matcher_->next(ctx); }
        void do_print(std::ostream &out) const override { matcher_->print(out); }

        State *state_;
        UMatcher matcher_;
    };
    auto m = [this]<class T>(T &&matcher) {
        return std::make_unique<MatcherProject>(*state_, std::forward<T>(matcher));
    };
    offset_ = invalid_offset;
    if (sign_ == Sign::once) {
        return {m(make_non_fact_matcher(state_->p_base(), *p_atom_, symbol_)), std::nullopt};
    }
    if (sign_ == Sign::twice && index_ != stratified_index) {
        return {m(make_once_matcher()), std::nullopt};
    }
    auto index = std::optional<size_t>{};
    if (index_ != stratified_index && type == MatcherType::new_atoms) {
        index = index_;
    }
    return {m(make_atom_matcher(mbr, bound, state_->p_base(), *p_atom_, type, offset_)), index};
}

auto LitProject::do_score(std::vector<bool> const &bound) const -> double {
    if (sign_ != Sign::once) {
        return atom_->score(static_cast<double>(state_->base().size()), bound);
    }
    return 0;
}

auto LitProject::do_hash() const -> size_t { return Util::value_hash_record<LitProject>(sign_, atom_); }

auto LitProject::do_equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitProject const *>(&other);
    return x != nullptr && std::tie(sign_, *atom_) == std::tie(x->sign_, *x->atom_);
}

auto LitProject::do_compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *x = dynamic_cast<LitProject const *>(&other); x != nullptr) {
        return std::tie(sign_, *atom_) <=> std::tie(x->sign_, *x->atom_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// definition of LitTuple

namespace {

class MatcherLitTuple : public OnceMatcher {
  public:
    MatcherLitTuple(VariableVec bind, VariableVec const &vars, SymbolVec const &syms)
        : bind_{std::move(bind)}, vars_{&vars}, syms_{&syms} {}

  private:
    auto do_once(InstantiationContext &ctx) -> bool override {
        auto &ass = ctx.ass();
        for (auto const &var : bind_) {
            ass[var] = std::nullopt;
        }
        auto it = syms_->begin();
        for (auto const &var : *vars_) {
            auto &val = ass[var];
            if (!val) {
                ass[var] = *it;
            } else if (*val != *it) {
                return false;
            }
            ++it;
        }
        return true;
    }

    void do_print(std::ostream &out) const override {
        out << "#once(" << Util::p_range(*vars_, [](std::ostream &out, auto var) { out << "X_" << var; }) << ")";
    }

    VariableVec bind_;
    VariableVec const *vars_;
    SymbolVec const *syms_;
};

} // namespace

void LitTuple::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::depend) {
        vars.insert(vars_.begin(), vars_.end());
    }
}

auto LitTuple::do_domain() const -> bool { return true; }

auto LitTuple::do_single_pass() const -> bool { return true; }

auto LitTuple::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr, [[maybe_unused]] MatcherType type,
                          std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    VariableVec bind;
    for (auto const &var : vars_) {
        if (!bound[var]) {
            bind.emplace_back(var);
        }
    }
    return {std::make_unique<MatcherLitTuple>(std::move(bind), vars_, *syms_), std::nullopt};
}

auto LitTuple::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return 0; }

void LitTuple::do_print(std::ostream &out) const {
    out << "#once(" << Util::p_range(vars_, [](std::ostream &out, auto var) { out << "X_" << var; }) << ")";
}

auto LitTuple::do_output([[maybe_unused]] InstantiationContext &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitTuple::do_copy() const -> ULit { return std::make_unique<LitTuple>(vars_, *syms_); }

auto LitTuple::do_hash() const -> size_t {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return Util::value_hash_record<LitTuple>(vars_, reinterpret_cast<uintptr_t>(syms_));
}

auto LitTuple::do_equal_to(Lit const &other) const -> bool {
    auto const *lit = dynamic_cast<LitTuple const *>(&other);
    return lit != nullptr && vars_ == lit->vars_ && syms_ == lit->syms_;
}

auto LitTuple::do_compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *x = dynamic_cast<LitTuple const *>(&other); x != nullptr) {
        return std::tie(vars_, syms_) <=> std::tie(x->vars_, x->syms_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// LitFailCheck

void LitFailCheck::do_print(std::ostream &out) const {
    out << "#not_fail" << Util::p_range(terms_, [](std::ostream &out, auto const &x) { out << *x; });
}

auto LitFailCheck::do_output([[maybe_unused]] InstantiationContext &ctx,
                             [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitFailCheck::do_copy() const -> ULit {
    UTermVec terms;
    terms.reserve(terms_.size());
    for (auto const &term : terms_) {
        terms.emplace_back(term->copy());
    }
    return std::make_unique<LitFailCheck>(std::move(terms));
}

auto LitFailCheck::do_domain() const -> bool { return true; }

auto LitFailCheck::do_single_pass() const -> bool { return true; }

void LitFailCheck::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        for (auto const &term : terms_) {
            term->vars(vars);
        }
    }
}

auto LitFailCheck::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr,
                              [[maybe_unused]] MatcherType type, [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    class FailCheckMatcher : public OnceMatcher {
      public:
        FailCheckMatcher(UTermVec &terms) : terms_{&terms} {}

      private:
        auto do_once(InstantiationContext &ctx) -> bool override {
            for (auto const &term : *terms_) {
                if (!term->eval(ctx.store(), ctx.ass())) {
                    return false;
                }
            }
            return true;
        }
        void do_print(std::ostream &out) const override {
            out << "#not_fail" << Util::p_range(*terms_, [](std::ostream &out, auto const &x) { out << *x; });
        }

        UTermVec *terms_;
    };
    return {std::make_unique<FailCheckMatcher>(terms_), std::nullopt};
}

auto LitFailCheck::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return -1; }

auto LitFailCheck::do_hash() const -> size_t { return Util::value_hash_record<LitFailCheck>(terms_); }

auto LitFailCheck::do_equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitFailCheck const *>(&other);
    return x != nullptr && Util::value_equal_to{}(terms_, x->terms_);
}

auto LitFailCheck::do_compare_to(Lit const &other) const -> std::weak_ordering {
    auto const *x = dynamic_cast<LitFailCheck const *>(&other);
    if (x != nullptr) {
        return std::lexicographical_compare_three_way(terms_.begin(), terms_.end(), x->terms_.begin(), x->terms_.end(),
                                                      [](auto const &a, auto const &b) { return *a <=> *b; });
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

} // namespace Gringo::Ground
