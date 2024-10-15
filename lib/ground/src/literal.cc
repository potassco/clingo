#include <clingo/ground/literal.hh>
#include <clingo/ground/matcher.hh>

#include <clingo/util/print.hh>

#include <typeindex>

namespace Clingo::Ground {

void LitInterval::do_print(std::ostream &out) const { out << *lhs_ << "=" << *lower_ << ".." << *upper_; }

auto LitInterval::do_output([[maybe_unused]] InstantiationContext const &ctx,
                            [[maybe_unused]] OutputLit &out) const -> bool {
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

// LitComparison

void LitComparison::do_print(std::ostream &out) const { out << *lhs_ << cmp_ << *rhs_; }

auto LitComparison::do_output([[maybe_unused]] InstantiationContext const &ctx,
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

// LitExternal

void LitExternal::do_print(std::ostream &out) const {
    out << *lhs_ << "=@" << name_;
    if (!args_.empty()) {
        out << "(" << Util::p_range(args_, [](auto &out, auto &term) { out << *term; }) << ")";
    }
}

auto LitExternal::do_output([[maybe_unused]] InstantiationContext const &ctx,
                            [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitExternal::do_copy() const -> ULit {
    return std::make_unique<LitExternal>(*ctx_, name_, lhs_->copy(), copy_uvec(args_));
}

auto LitExternal::do_domain() const -> bool { return true; }

auto LitExternal::do_single_pass() const -> bool { return true; }

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

namespace {

class ExternalMatcher : public Matcher {
  public:
    ExternalMatcher(ScriptCallback &ctx, String name, Term const &lhs, UTermVec const &args, VariableVec free)
        : ctx_{&ctx}, name_{name}, lhs_{&lhs}, args_{&args}, free_{std::move(free)} {
        syms_.resize(args_->size());
    }

  private:
    void do_init([[maybe_unused]] InitContext const &ctx, [[maybe_unused]] size_t gen) override {}
    void do_match(InstantiationContext const &ctx) override {
        syms_.clear();
        matches_.clear();
        for (auto const &arg : *args_) {
            if (auto sym = arg->eval(ctx)) {
                syms_.emplace_back(*sym);
            } else {
                return;
            }
        }
        ctx_->call(name_.view(), syms_, matches_);
        cur_ = matches_.begin();
    }
    auto do_next(InstantiationContext const &ctx) -> bool override {
        auto &ass = ctx.ass();
        for (auto const &var : free_) {
            ass[var] = std::nullopt;
        }
        while (cur_ != matches_.end()) {
            if (lhs_->match(ctx, *cur_++)) {
                return true;
            }
        }
        return false;
    }
    void do_print(std::ostream &out) const override {
        out << *lhs_ << "=@" << name_;
        if (!args_->empty()) {
            out << "(" << Util::p_range(*args_, [](auto &out, auto &term) { out << *term; }) << ")";
        }
    }

    ScriptCallback *ctx_;
    String name_;
    Term const *lhs_;
    SymbolVec syms_;
    UTermVec const *args_;
    VariableVec free_;
    SymbolVec matches_;
    SymbolVec::const_iterator cur_;
};

} // namespace

auto LitExternal::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr,
                             [[maybe_unused]] MatcherType type,
                             std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    VariableSet vars;
    lhs_->vars(vars);
    erase_if(vars, [&bound](auto const &var) { return bound[var]; });
    return {std::make_unique<ExternalMatcher>(*ctx_, name_, *lhs_, args_, vars.release()), std::nullopt};
}

auto LitExternal::do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return -1; }

auto LitExternal::do_hash() const -> size_t { return Util::value_hash(std::hash<LitExternal const *>{}(this)); }

auto LitExternal::do_equal_to(Lit const &other) const -> bool { return this == &other; }

auto LitExternal::do_compare_to(Lit const &other) const -> std::weak_ordering { return this <=> &other; }

// LitSymbolic

void LitSymbolic::do_print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
}

auto LitSymbolic::do_output(InstantiationContext const &ctx, OutputLit &out) const -> bool {
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
        return atom_->eval(ctx);
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
        // TODO: Somehow clingo previously added 10,000,000 if all variables were
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

void LitProject::State::init(InitContext const &ctx, size_t gen) {
    base_->update(gen);
    for (size_t n = base_->end(MatcherType::all_atoms); imported_ != n; ++imported_) {
        auto atom = base_->nth(imported_);
        for (auto &sym : ass_) {
            sym = std::nullopt;
        }
        auto eval_ctx = EvalContext{ctx.log(), ctx.store(), ass_};
        if (p_body_->match(eval_ctx, atom->first)) {
            if (auto sym = p_head_->eval(eval_ctx); sym) {
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

auto LitProject::do_output(InstantiationContext const &ctx, OutputLit &out) const -> bool {
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
        return p_atom_->eval(ctx);
    };
    if (auto p_sym = get_symbol()) {
        if (sign_ == Sign::once ? index_ == stratified_index && !state_->p_base().contains(*p_sym)
                                : state_->p_base().is_fact(*p_sym)) {
            return false;
        }
    }
    if (auto sym = atom_->eval(ctx)) {
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
        void do_init(InitContext const &ctx, size_t gen) override {
            state_->init(ctx, gen);
            matcher_->init(ctx, gen);
        }
        void do_match(InstantiationContext const &ctx) override { matcher_->match(ctx); }
        auto do_next(InstantiationContext const &ctx) -> bool override { return matcher_->next(ctx); }
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
    auto do_once(InstantiationContext const &ctx) -> bool override {
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

auto LitTuple::do_output([[maybe_unused]] InstantiationContext const &ctx,
                         [[maybe_unused]] OutputLit &out) const -> bool {
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

// LitCheck

auto LitCheck::do_single_pass() const -> bool { return true; }

auto LitCheck::do_domain() const -> bool { return true; }

auto LitCheck::do_output([[maybe_unused]] InstantiationContext const &ctx,
                         [[maybe_unused]] OutputLit &out) const -> bool {
    return false;
}

auto LitCheck::do_matcher([[maybe_unused]] std::pmr::monotonic_buffer_resource &mbr, [[maybe_unused]] MatcherType type,
                          [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    class CheckMatcher : public OnceMatcher {
      public:
        CheckMatcher(LitCheck &lit) : lit_{&lit} {}

      private:
        auto do_once(InstantiationContext const &ctx) -> bool override { return lit_->do_check(ctx); }
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

auto LitCheck::do_hash() const -> size_t { return typeid(this).hash_code(); }

auto LitCheck::do_equal_to(Lit const &other) const -> bool { return this == &other; }

auto LitCheck::do_compare_to(Lit const &other) const -> std::weak_ordering { return this <=> &other; }

// definition of LitBool

void LitBool::do_print(std::ostream &out) const { out << (value_ ? "#true" : "#false"); }

auto LitBool::do_copy() const -> ULit { return std::make_unique<LitBool>(value_); }

void LitBool::do_vars([[maybe_unused]] VariableSet &vars, [[maybe_unused]] VarSelectMode mode) const {}

auto LitBool::do_check([[maybe_unused]] InstantiationContext const &ctx) -> bool { return value_; }

// LitFactCheck

auto LitFactCheck::do_check(InstantiationContext const &ctx) -> bool {
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

void LitFactCheck::do_print(std::ostream &out) const { out << "#not_fact " << *atom_; }

auto LitFactCheck::do_copy() const -> ULit { return std::make_unique<LitFactCheck>(*base_, *atom_, *target_); }

// LitFailCheck

auto LitFailCheck::do_check(InstantiationContext const &ctx) -> bool {
    if (result_ != nullptr) {
        result_->clear();
    }
    auto i = size_t{0};
    for (auto const &term : terms_) {
        if (auto res = term->eval(ctx)) {
            if (i < num_) {
                if (res->type() != SymbolType::number) {
                    return false;
                }
            }
            if (result_ != nullptr) {
                result_->emplace_back(*res);
            }
        } else {
            return false;
        }
        ++i;
    }
    return true;
}

void LitFailCheck::do_print(std::ostream &out) const {
    out << "#check(" << Util::p_range(terms_, [](std::ostream &out, auto const &x) { out << *x; }) << ")";
}

auto LitFailCheck::do_copy() const -> ULit { return std::make_unique<LitFailCheck>(copy_uvec(terms_), num_, result_); }

void LitFailCheck::do_vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        for (auto const &term : terms_) {
            term->vars(vars);
        }
    }
}

} // namespace Clingo::Ground
