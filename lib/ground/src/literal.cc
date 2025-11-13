#include <clingo/ground/literal.hh>
#include <clingo/ground/matcher.hh>

#include <clingo/util/print.hh>

#include <typeindex>

namespace CppClingo::Ground {

static constexpr double score_fast = -1.0;
static constexpr double score_maybe_fast = -0.1;

void LitInterval::do_print(std::ostream &out) const {
    out << *lhs_ << "=" << *lower_ << ".." << *upper_;
}

auto LitInterval::do_output([[maybe_unused]] EvalContext const &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitInterval::do_copy() const -> ULit {
    return std::make_unique<LitInterval>(lhs_->copy(), lower_->copy(), upper_->copy());
}

auto LitInterval::do_domain() const -> bool {
    return true;
}

auto LitInterval::do_single_pass() const -> bool {
    return true;
}

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
                             [[maybe_unused]] MatcherType type, std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    return {make_interval_matcher(bound, *lhs_, *lower_, *upper_), std::nullopt};
}

auto LitInterval::do_score(std::vector<bool> const &bound) const -> double {
    if (auto *l = dynamic_cast<TermSymbol *>(lower_.get()), *r = dynamic_cast<TermSymbol *>(lower_.get());
        l != nullptr && r != nullptr) {
        VariableSet vars;
        lhs_->vars(vars);
        if (std::ranges::all_of(vars, [&bound](auto var) { return bound[var]; })) {
            return score_fast;
        }
        auto sl = l->symbol();
        auto sr = r->symbol();
        if (sl.type() != SymbolType::number || sr.type() != SymbolType::number) {
            return score_fast;
        }
        auto const &nl = sl.num();
        auto const &nr = sr.num();
        if (nl > nr) {
            return score_fast;
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

auto LitInterval::do_hash() const -> size_t {
    return Util::value_hash_record<LitInterval>(lhs_, lower_, upper_);
}

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

// LitComparison

void LitComparison::do_print(std::ostream &out) const {
    out << *lhs_ << cmp_ << *rhs_;
}

auto LitComparison::do_output([[maybe_unused]] EvalContext const &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitComparison::do_copy() const -> ULit {
    return std::make_unique<LitComparison>(lhs_->copy(), cmp_, rhs_->copy());
}

auto LitComparison::do_domain() const -> bool {
    return true;
}

auto LitComparison::do_single_pass() const -> bool {
    return true;
}

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

auto LitComparison::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    return score_fast;
}

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

// LitExternal

void LitExternal::do_print(std::ostream &out) const {
    out << *lhs_ << "=@" << name_;
    if (!args_.empty()) {
        out << "(" << Util::p_range(args_, [](auto &out, auto &term) { out << *term; }) << ")";
    }
}

auto LitExternal::do_output([[maybe_unused]] EvalContext const &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitExternal::do_copy() const -> ULit {
    return std::make_unique<LitExternal>(*ctx_, loc_, name_, lhs_->copy(), copy_uvec(args_));
}

auto LitExternal::do_domain() const -> bool {
    return true;
}

auto LitExternal::do_single_pass() const -> bool {
    return true;
}

void LitExternal::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode == VarSelectMode::all) {
        lhs_->vars(vars);
    }
    if (mode == VarSelectMode::provide) {
        VariableSet provide;
        lhs_->vars(vars);
        VariableSet depend;
        for (auto const &arg : args_) {
            arg->vars(vars);
        }
        for (auto const &var : depend) {
            provide.erase(var);
        }
        vars.insert(provide.begin(), provide.end());
    } else {
        for (auto const &arg : args_) {
            arg->vars(vars);
        }
    }
}

auto LitExternal::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr,
                             [[maybe_unused]] MatcherType type, std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    class ExternalMatcher : public Matcher {
      public:
        ExternalMatcher(LitExternal &lit, VariableVec free) : lit_{&lit}, free_{std::move(free)} {
            syms_.resize(lit_->args_.size());
        }

      private:
        void do_init([[maybe_unused]] InstantiationContext const &ctx, [[maybe_unused]] size_t gen) override {}
        void do_match(EvalContext const &ctx) override {
            syms_.clear();
            matches_.clear();
            for (auto const &arg : lit_->args_) {
                if (auto sym = arg->eval(ctx)) {
                    syms_.emplace_back(*sym);
                } else {
                    return;
                }
            }
            lit_->ctx_->call(lit_->loc_, lit_->name_.view(), syms_, matches_);
            cur_ = matches_.begin();
        }
        auto do_next(EvalContext const &ctx) -> bool override {
            auto &ass = ctx.ass();
            for (auto const &var : free_) {
                ass[var] = std::nullopt;
            }
            while (cur_ != matches_.end()) {
                if (lit_->lhs_->match(ctx, *cur_++)) {
                    return true;
                }
            }
            return false;
        }
        void do_print(std::ostream &out) const override {
            out << *lit_->lhs_ << "=@" << lit_->name_;
            if (!lit_->args_.empty()) {
                out << "(" << Util::p_range(lit_->args_, [](auto &out, auto &term) { out << *term; }) << ")";
            }
        }

        LitExternal *lit_;
        SymbolVec syms_;
        VariableVec free_;
        SymbolVec matches_;
        SymbolVec::const_iterator cur_;
    };

    VariableSet vars;
    lhs_->vars(vars);
    erase_if(vars, [&bound](auto const &var) { return bound[var]; });
    return {std::make_unique<ExternalMatcher>(*this, vars.release()), std::nullopt};
}

auto LitExternal::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    return score_maybe_fast;
}

auto LitExternal::do_hash() const -> size_t {
    return Util::value_hash(std::hash<LitExternal const *>{}(this));
}

auto LitExternal::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitExternal::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

// LitSymbolic

void LitSymbolic::do_print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
}

namespace {

auto get_atom(AtomBase &base, OutputStm &out, Sign sign, size_t index, Symbol symbol, size_t offset)
    -> std::optional<AtomBase::MapAtom::iterator> {
    // avoid lookups if the literal is known to be true
    if ((index == stratified_index || sign == Sign::none) && base.domain()) {
        return std::nullopt;
    }
    if (offset != invalid_offset) {
        if (auto it = base.nth(offset); it->second.state != StateAtom::fact) {
            return it;
        }
        // Cannot happen because the literal would not have matched.
        assert(sign != Sign::once);
        return std::nullopt;
    }
    // Avoid adding ids for literals whose atoms are false.
    if (sign == Sign::once && index == stratified_index) {
        return base.find(symbol);
    }
    return base.add(symbol, StateAtom::unknown, [&out]() { return out.uid(); }).first;
}

} // namespace

auto LitSymbolic::do_output([[maybe_unused]] EvalContext const &ctx, OutputLit &out) const -> bool {
    assert(offset_ != invalid_offset || Symbol::to_rep(symbol_) != 0);
    if (auto atom = get_atom(*base_, ctx.out(), sign_, index_, symbol_, offset_)) {
        out.lit(sign_, atom->key(), atom->value().id);
        return true;
    }
    return false;
}

auto LitSymbolic::do_copy() const -> ULit {
    return std::make_unique<LitSymbolic>(*base_, sign_, atom_->copy(), index_, domain_);
}

auto LitSymbolic::do_domain() const -> bool {
    return domain_ || (index_ == stratified_index && base_->domain());
}

auto LitSymbolic::do_single_pass() const -> bool {
    return sign_ != Sign::none || index_ == stratified_index;
}

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

auto LitSymbolic::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type, std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    offset_ = invalid_offset;
    if (sign_ == Sign::once) {
        return {make_non_fact_matcher(*base_, *atom_, symbol_), std::nullopt};
    }
    if (sign_ == Sign::twice && index_ != stratified_index) {
        return {make_once_matcher(*atom_, symbol_), std::nullopt};
    }

    auto index = std::optional<size_t>{};
    if (index_ != stratified_index && type == MatcherType::new_atoms) {
        index = index_;
    }
    return {make_atom_matcher(mbr, bound, *base_, *atom_, type, offset_), index};
}

auto LitSymbolic::do_score(std::vector<bool> const &bound) const -> double {
    if (sign_ != Sign::once) {
        auto size = base_->size();
        if (single_pass() && size == 0) {
            return -1;
        }
        return atom_->score(static_cast<double>(size), bound);
    }
    return 0;
}

auto LitSymbolic::do_hash() const -> size_t {
    return Util::value_hash_record<LitSymbolic>(sign_, atom_);
}

auto LitSymbolic::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitSymbolic::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

void LitProject::do_print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
}

auto LitProject::do_output(EvalContext const &ctx, OutputLit &out) const -> bool {
    if (auto atom = get_atom(state_->p_base(), ctx.out(), sign_, index_, symbol_, offset_)) {
        // evaluation cannot fail by construction
        auto sym = atom_->eval(ctx);
        assert(sym);
        out.lit(sign_, *sym, atom->value().id);
        return true;
    }
    return false;
}

auto LitProject::do_copy() const -> ULit {
    return std::make_unique<LitProject>(*state_, sign_, atom_->copy(), p_atom_->copy(), index_, domain_);
}

auto LitProject::do_domain() const -> bool {
    return domain_ || (index_ == stratified_index && state_->base().domain());
}

auto LitProject::do_single_pass() const -> bool {
    return sign_ != Sign::none || index_ == stratified_index;
}

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

auto LitProject::do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type, std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    class MatcherProject : public Matcher {
      public:
        MatcherProject(ProjectState &state, UMatcher matcher) : state_{&state}, matcher_{std::move(matcher)} {}

      private:
        void do_init(InstantiationContext const &ctx, size_t gen) override {
            state_->init(ctx, gen);
            matcher_->init(ctx, gen);
        }
        void do_match(EvalContext const &ctx) override { matcher_->match(ctx); }
        auto do_next(EvalContext const &ctx) -> bool override { return matcher_->next(ctx); }
        void do_print(std::ostream &out) const override { matcher_->print(out); }
        [[nodiscard]] auto do_type() const -> MatcherType override { return matcher_->type(); }

        ProjectState *state_;
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
        return {m(make_once_matcher(*p_atom_, symbol_)), std::nullopt};
    }
    auto index = std::optional<size_t>{};
    if (index_ != stratified_index && type == MatcherType::new_atoms) {
        index = index_;
    }
    return {m(make_atom_matcher(mbr, bound, state_->p_base(), *p_atom_, type, offset_)), index};
}

auto LitProject::do_score(std::vector<bool> const &bound) const -> double {
    if (sign_ != Sign::once) {
        auto size = state_->base().size();
        if (single_pass() && size == 0) {
            return -1;
        }
        return atom_->score(static_cast<double>(size), bound);
    }
    return 0;
}

auto LitProject::do_hash() const -> size_t {
    return Util::value_hash_record<LitProject>(sign_, atom_);
}

auto LitProject::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitProject::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

// definition of LitTuple

namespace {

class MatcherLitTuple : public OnceMatcher {
  public:
    MatcherLitTuple(VariableVec bind, VariableVec const &vars, SymbolVec const &syms)
        : bind_{std::move(bind)}, vars_{&vars}, syms_{&syms} {}

  private:
    auto do_once(EvalContext const &ctx) -> bool override {
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

auto LitTuple::do_domain() const -> bool {
    return true;
}

auto LitTuple::do_single_pass() const -> bool {
    return true;
}

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

auto LitTuple::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    return 0;
}

void LitTuple::do_print(std::ostream &out) const {
    out << "#once(" << Util::p_range(vars_, [](std::ostream &out, auto var) { out << "X_" << var; }) << ")";
}

auto LitTuple::do_output([[maybe_unused]] EvalContext const &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitTuple::do_copy() const -> ULit {
    return std::make_unique<LitTuple>(vars_, *syms_);
}

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

// LitCheck

auto LitCheck::do_single_pass() const -> bool {
    return true;
}

auto LitCheck::do_domain() const -> bool {
    return true;
}

auto LitCheck::do_output([[maybe_unused]] EvalContext const &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitCheck::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr, [[maybe_unused]] MatcherType type,
                          [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    class CheckMatcher : public OnceMatcher {
      public:
        CheckMatcher(LitCheck &lit) : lit_{&lit} {}

      private:
        auto do_once(EvalContext const &ctx) -> bool override { return lit_->do_check(ctx); }
        void do_print(std::ostream &out) const override { out << *lit_; }

        LitCheck *lit_;
    };
    return {std::make_unique<CheckMatcher>(*this), std::nullopt};
}

auto LitCheck::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    // we give a high score here because most of these checks will match anyway
    // their purpose is mostly to set the member storing the evaluation result
    return std::numeric_limits<double>::max();
}

auto LitCheck::do_hash() const -> size_t {
    return typeid(this).hash_code();
}

auto LitCheck::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitCheck::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

// definition of LitBool

void LitBool::do_print(std::ostream &out) const {
    out << (value_ ? "#true" : "#false");
}

auto LitBool::do_copy() const -> ULit {
    return std::make_unique<LitBool>(value_);
}

void LitBool::do_vars([[maybe_unused]] VariableSet &vars, [[maybe_unused]] VarSelectMode mode) const {
}

auto LitBool::do_check([[maybe_unused]] EvalContext const &ctx) -> bool {
    return value_;
}

// LitFactCheck

auto LitFactCheck::do_check(EvalContext const &ctx) -> bool {
    if (auto sym = atom_->eval(ctx)) {
        *target_ = *sym;
        return !base_->is_fact(*sym);
    }
    return false;
}

void LitFactCheck::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        atom_->vars(vars);
    }
}

void LitFactCheck::do_print(std::ostream &out) const {
    out << "#not_fact " << *atom_;
}

auto LitFactCheck::do_copy() const -> ULit {
    return std::make_unique<LitFactCheck>(*base_, *atom_, *target_);
}

// LitFailCheck

auto LitFailCheck::do_check(EvalContext const &ctx) -> bool {
    for (auto const &term : terms_) {
        if (!term->eval(ctx)) {
            return false;
        }
    }
    return true;
}

void LitFailCheck::do_print(std::ostream &out) const {
    out << "#check(" << Util::p_range(terms_, [](std::ostream &out, auto const &x) { out << *x; }) << ")";
}

auto LitFailCheck::do_copy() const -> ULit {
    return std::make_unique<LitFailCheck>(copy_uvec(terms_));
}

void LitFailCheck::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        for (auto const &term : terms_) {
            term->vars(vars);
        }
    }
}

// LitSimpleAggregate

void LitSimpleAggr::do_print(std::ostream &out) const {
    out << *lhs_ << " " << cmp_ << " " << fun_ << " { "
        << Util::p_range(tuples_, "; ",
                         [](auto &out, auto const &tuple) {
                             if (tuple.empty()) {
                                 out << ":";
                             } else {
                                 out << Util::p_range(tuple, [](auto &out, auto const &term) { out << *term; });
                             }
                         })
        << (tuples_.empty() ? "}" : " }");
}

auto LitSimpleAggr::do_output([[maybe_unused]] EvalContext const &ctx, [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitSimpleAggr::do_copy() const -> ULit {
    return std::make_unique<LitSimpleAggr>(lhs_->copy(), cmp_, fun_, copy_uvec(tuples_));
}

auto LitSimpleAggr::do_domain() const -> bool {
    return true;
}

auto LitSimpleAggr::do_single_pass() const -> bool {
    return true;
}

void LitSimpleAggr::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (cmp_ != Relation::equal) {
        mode = VarSelectMode::all;
    }
    switch (mode) {
        case VarSelectMode::all: {
            lhs_->vars(vars);
            for (auto const &tuple : tuples_) {
                for (auto const &term : tuple) {
                    term->vars(vars);
                }
            }
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
            for (auto const &tuple : tuples_) {
                for (auto const &term : tuple) {
                    term->vars(vars);
                }
            }
            break;
        }
    }
}

auto LitSimpleAggr::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr,
                               [[maybe_unused]] MatcherType type, std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    class AggrMatcher : public OnceMatcher {
      public:
        AggrMatcher(LitSimpleAggr &lit, VariableVec free) : lit_{&lit}, free_{std::move(free)} {
            free_.shrink_to_fit();
            syms_.resize(lit_->tuples_.size());
            tuples_.reserve(lit_->tuples_.size());
        }

      private:
        auto do_once(EvalContext const &ctx) -> bool override {
            for (auto const &var : free_) {
                ctx.ass()[var] = std::nullopt;
            }
            tuples_.clear();
            auto rhs_num = Number(0);
            auto rhs_sym = neutral_val(lit_->fun_);
            auto it = syms_.begin();
            for (auto const &tuple : lit_->tuples_) {
                auto &syms = *it;
                syms.clear();
                for (auto const &term : tuple) {
                    if (auto sym = term->eval(ctx)) {
                        syms.emplace_back(*sym);
                    } else {
                        syms.clear();
                        break;
                    }
                }
                // Note: maybe report messages...
                if (!syms.empty()) {
                    auto weight = syms.front();
                    switch (lit_->fun_) {
                        case AggregateFunction::max: {
                            rhs_sym = std::max(rhs_sym, weight);
                            break;
                        }
                        case AggregateFunction::min: {
                            rhs_sym = std::min(rhs_sym, weight);
                            break;
                        }
                        case AggregateFunction::sum: {
                            if (tuples_.emplace(&syms).second && weight.type() == SymbolType::number) {
                                rhs_num += weight.num();
                            }
                            break;
                        }
                        case AggregateFunction::sump: {
                            if (tuples_.emplace(&syms).second && weight.type() == SymbolType::number &&
                                weight.num() > 0) {
                                rhs_num += weight.num();
                            }
                            break;
                        }
                        case AggregateFunction::count: {
                            Util::unreachable();
                        }
                    }
                }
            }
            if (lit_->fun_ != AggregateFunction::max && lit_->fun_ != AggregateFunction::min) {
                rhs_sym = ctx.store().num_ref(rhs_num);
            }
            if (lit_->cmp_ == Relation::equal) {
                return lit_->lhs_->match(ctx, rhs_sym);
            }
            if (auto lhs = lit_->lhs_->eval(ctx)) {
                return evaluate(*lhs, lit_->cmp_, rhs_sym);
            }
            return false;
        }

        void do_print(std::ostream &out) const override { lit_->do_print(out); }

        LitSimpleAggr *lit_;
        VariableVec free_;
        std::vector<SymbolVec> syms_;
        Util::unordered_set<SymbolVec *> tuples_;
    };

    VariableSet vars;
    lhs_->vars(vars);
    VariableVec free;
    for (auto const &var : vars) {
        if (!bound[var]) {
            free.emplace_back(var);
        }
    }
    return {std::make_unique<AggrMatcher>(*this, std::move(free)), std::nullopt};
}

auto LitSimpleAggr::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    return score_fast;
}

auto LitSimpleAggr::do_hash() const -> size_t {
    return Util::value_hash(std::hash<LitSimpleAggr const *>{}(this));
}

auto LitSimpleAggr::do_equal_to(Lit const &other) const -> bool {
    return this == &other;
}

auto LitSimpleAggr::do_compare_to(Lit const &other) const -> std::weak_ordering {
    return this <=> &other;
}

} // namespace CppClingo::Ground
