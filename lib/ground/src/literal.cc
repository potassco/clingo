#include <gringo/ground/literal.hh>
#include <gringo/ground/matcher.hh>

#include <typeindex>

namespace Gringo::Ground {

void LitInterval::print(std::ostream &out) const { out << *lhs_ << "=" << *lower_ << ".." << *upper_; }

auto LitInterval::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool {
    if (auto lhs = lhs_->eval(store, ass), lower = lower_->eval(store, ass), upper = upper_->eval(store, ass);
        lhs && lower && upper) {
        out << *lower << "<=" << *lhs << "<=" << *upper;
    } else {
        out << "#false";
    }
    return false;
}

auto LitInterval::domain([[maybe_unused]] bool domain) const -> bool { return true; }

auto LitInterval::recursive() const -> bool { return false; }

void LitInterval::vars(VariableSet &vars, VarSelectMode mode) const {
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

auto LitInterval::matcher([[maybe_unused]] MatcherType type,
                          std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    return {make_interval_matcher(bound, *lhs_, *lower_, *upper_), std::nullopt};
}

auto LitInterval::score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
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
        auto nl = sl.num();
        auto nr = sr.num();
        if (*nl > *nr) {
            return -1;
        }
        auto d = *nr - *nl;
        if (auto id = d.as_int(); id) {
            return *id;
        }
        return std::numeric_limits<double>::max();
    }
    // NOLINTNEXTLINE(readability-magic-numbers)
    return 100;
}

auto LitInterval::hash() const -> size_t { return Util::value_hash_record<LitInterval>(lhs_, lower_, upper_); }

auto LitInterval::equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitInterval const *>(&other);
    if (x != nullptr) {
        return std::tie(*lhs_, *lower_, *upper_) == std::tie(*x->lhs_, *x->lower_, *x->upper_);
    }
    return false;
}

auto LitInterval::compare_to(Lit const &other) const -> std::weak_ordering {
    auto const *x = dynamic_cast<LitInterval const *>(&other);
    if (x != nullptr) {
        return std::tie(*lhs_, *lower_, *upper_) <=> std::tie(*x->lhs_, *x->lower_, *x->upper_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

void LitComparison::print(std::ostream &out) const { out << *lhs_ << cmp_ << *rhs_; }

auto LitComparison::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool {
    if (auto lhs = lhs_->eval(store, ass), rhs = rhs_->eval(store, ass); lhs && rhs) {
        out << *lhs << cmp_ << *rhs;
    } else {
        out << "#false";
    }
    return false;
}

auto LitComparison::domain([[maybe_unused]] bool domain) const -> bool { return true; }

auto LitComparison::recursive() const -> bool { return false; }

void LitComparison::vars(VariableSet &vars, VarSelectMode mode) const {
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

auto LitComparison::matcher([[maybe_unused]] MatcherType type, [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    return {make_comp_matcher(bound, *lhs_, cmp_, *rhs_), std::nullopt};
}

auto LitComparison::score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return -1; }

auto LitComparison::hash() const -> size_t {
    if (cmp_ == Relation::equal && *rhs_ < *lhs_) {
        return Util::value_hash_record<LitComparison>(rhs_, cmp_, lhs_);
    }
    return Util::value_hash_record<LitComparison>(lhs_, cmp_, rhs_);
}

auto LitComparison::equal_to(Lit const &other) const -> bool {
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

auto LitComparison::compare_to(Lit const &other) const -> std::weak_ordering {
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

void LitFactCheck::vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::provide) {
        atom_->vars(vars);
    }
}

auto LitFactCheck::domain([[maybe_unused]] bool domain) const -> bool { return true; }

auto LitFactCheck::recursive() const -> bool { return false; }

auto LitFactCheck::matcher([[maybe_unused]] MatcherType type, [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    return {make_non_fact_matcher(*base_, *atom_), std::nullopt};
}

auto LitFactCheck::score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return 0; }

void LitFactCheck::print(std::ostream &out) const { out << "#not_fact " << *atom_; }

auto LitFactCheck::output([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment const &ass,
                          std::ostream &out) const -> bool {
    out << "#true";
    return false;
}

auto LitFactCheck::hash() const -> size_t { return Util::value_hash_record<LitFactCheck>(*atom_); }

auto LitFactCheck::equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitFactCheck const *>(&other);
    return x != nullptr && *atom_ == *x->atom_;
}

auto LitFactCheck::compare_to(Lit const &other) const -> std::weak_ordering {
    auto const *x = dynamic_cast<LitFactCheck const *>(&other);
    if (x != nullptr) {
        return *atom_ <=> *x->atom_;
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// LitSymbolic

void LitSymbolic::print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != std::numeric_limits<size_t>::max()) {
        out << "[" << index_ << "]";
    }
}

auto LitSymbolic::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool {
    if (auto sym = atom_->eval(store, ass)) {
        out << sign_ << *sym;
        if (sign_ == Sign::once) {
            return index_ != std::numeric_limits<size_t>::max() || base_->contains(*sym);
        }
        return !base_->is_fact(*sym);
    }
    out << "#false";
    return true;
}

auto LitSymbolic::domain(bool domain) const -> bool {
    // check if the base of the literal is domain
    if (!base_->domain()) {
        return false;
    }
    // stratifed literals with a domain base can be completely evaluated
    if (index_ == std::numeric_limits<size_t>::max()) {
        return true;
    }
    // return true if the literal is in a domain component
    // noting that a domain component cannot contain negative literals
    return domain;
}

auto LitSymbolic::recursive() const -> bool {
    return sign_ == Sign::none && index_ != std::numeric_limits<size_t>::max();
}

void LitSymbolic::vars(VariableSet &vars, VarSelectMode mode) const {
    switch (mode) {
        case VarSelectMode::all: {
            atom_->vars(vars);
            break;
        }
        case VarSelectMode::provide: {
            if (sign_ == Sign::none || (sign_ == Sign::twice && index_ == std::numeric_limits<size_t>::max())) {
                atom_->vars(vars);
            }
            break;
        }
        case VarSelectMode::depend: {
            if (sign_ == Sign::once || (sign_ == Sign::twice && index_ != std::numeric_limits<size_t>::max())) {
                atom_->vars(vars);
            }
            break;
        }
    }
}

auto LitSymbolic::matcher(MatcherType type,
                          std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    if (sign_ == Sign::once) {
        return {make_non_fact_matcher(*base_, *atom_), std::nullopt};
    }
    if (sign_ == Sign::twice && index_ != std::numeric_limits<size_t>::max()) {
        return {make_once_matcher(), std::nullopt};
    }

    auto index = std::optional<size_t>{};
    if (index_ != std::numeric_limits<size_t>::max() && type == MatcherType::new_atoms) {
        index = index_;
    }
    return {make_atom_matcher(bound, *base_, *atom_, type), index};
}

auto LitSymbolic::score(std::vector<bool> const &bound) const -> double {
    if (sign_ != Sign::once) {
        // TODO: Somehow gringo previously added 10,000,000 if all variables were
        // bound. I don't see the point of this?
        return atom_->score(static_cast<double>(base_->size()), bound);
    }
    return 0;
}

auto LitSymbolic::hash() const -> size_t { return Util::value_hash_record<LitSymbolic>(sign_, atom_); }

auto LitSymbolic::equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitSymbolic const *>(&other);
    return x != nullptr && std::tie(sign_, *atom_) == std::tie(x->sign_, *x->atom_);
}

auto LitSymbolic::compare_to(Lit const &other) const -> std::weak_ordering {
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
            auto state = atom->second.state;
            if (state == AtomState::external) {
                state = AtomState::unknown;
            }
            if (auto sym = p_head_->eval(store, ass_); sym) {
                p_base_.add(*sym, state);
            }
        }
    }
    p_base_.update(gen);
}

void LitProject::print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != std::numeric_limits<size_t>::max()) {
        out << "[" << index_ << "]";
    }
}

auto LitProject::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool {
    if (auto p_sym = p_atom_->eval(store, ass)) {
        if (auto sym = atom_->eval(store, ass)) {
            out << sign_ << *sym;
        }
        if (sign_ == Sign::once) {
            return index_ != std::numeric_limits<size_t>::max() || state_->p_base().contains(*p_sym);
        }
        return !state_->p_base().is_fact(*p_sym);
    }
    out << "#false";
    return true;
}

auto LitProject::domain(bool domain) const -> bool {
    // check if the base of the literal is domain
    // Note: This check could be stronger in principle. However, this would
    // require to import the base into the p_base at this point. Hence, we only
    // use an approximation here. The test should work well in practice
    if (!state_->base().domain()) {
        return false;
    }
    // stratifed literals with a domain base can be completely evaluated
    if (index_ == std::numeric_limits<size_t>::max()) {
        return true;
    }
    // return true if the literal is in a domain component
    // noting that a domain component cannot contain negative literals
    return domain;
}

auto LitProject::recursive() const -> bool {
    return sign_ == Sign::none && index_ != std::numeric_limits<size_t>::max();
}

void LitProject::vars(VariableSet &vars, VarSelectMode mode) const {
    switch (mode) {
        case VarSelectMode::all: {
            atom_->vars(vars);
            break;
        }
        case VarSelectMode::provide: {
            if (sign_ == Sign::none || (sign_ == Sign::twice && index_ == std::numeric_limits<size_t>::max())) {
                atom_->vars(vars);
            }
            break;
        }
        case VarSelectMode::depend: {
            if (sign_ == Sign::once || (sign_ == Sign::twice && index_ != std::numeric_limits<size_t>::max())) {
                atom_->vars(vars);
            }
            break;
        }
    }
}

auto LitProject::matcher(MatcherType type,
                         std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    class MatcherProject : public Matcher {
      public:
        MatcherProject(State &state, UMatcher matcher) : state_{&state}, matcher_{std::move(matcher)} {}
        void init(SymbolStore &store, size_t gen) override {
            state_->init(store, gen);
            matcher_->init(store, gen);
        }
        void match(SymbolStore &store, Assignment &ass) override { matcher_->match(store, ass); }
        auto next(SymbolStore &store, Assignment &ass) -> bool override { return matcher_->next(store, ass); }

      private:
        State *state_;
        UMatcher matcher_;
    };
    auto m = [this]<class T>(T &&matcher) {
        return std::make_unique<MatcherProject>(*state_, std::forward<T>(matcher));
    };
    if (sign_ == Sign::once) {
        return {m(make_non_fact_matcher(state_->p_base(), *p_atom_)), std::nullopt};
    }
    if (sign_ == Sign::twice && index_ != std::numeric_limits<size_t>::max()) {
        return {m(make_once_matcher()), std::nullopt};
    }
    auto index = std::optional<size_t>{};
    if (index_ != std::numeric_limits<size_t>::max() && type == MatcherType::new_atoms) {
        index = index_;
    }
    return {m(make_atom_matcher(bound, state_->p_base(), *p_atom_, type)), index};
}

auto LitProject::score(std::vector<bool> const &bound) const -> double {
    if (sign_ != Sign::once) {
        return atom_->score(static_cast<double>(state_->base().size()), bound);
    }
    return 0;
}

auto LitProject::hash() const -> size_t { return Util::value_hash_record<LitProject>(sign_, atom_); }

auto LitProject::equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitProject const *>(&other);
    return x != nullptr && std::tie(sign_, *atom_) == std::tie(x->sign_, *x->atom_);
}

auto LitProject::compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *x = dynamic_cast<LitProject const *>(&other); x != nullptr) {
        return std::tie(sign_, *atom_) <=> std::tie(x->sign_, *x->atom_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

} // namespace Gringo::Ground
