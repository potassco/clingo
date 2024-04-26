#include <gringo/ground/literal.hh>
#include <gringo/ground/matcher.hh>

#include <typeindex>

#include <iostream>

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
    // TODO: compute proper score
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

auto LitComparison::score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return 0; }

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
    static_cast<void>(bound);
    // TODO: proper score computation
    return 2;
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

void LitProject::print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != std::numeric_limits<size_t>::max()) {
        out << "[" << index_ << "]";
    }
}

auto LitProject::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool {
    if (auto sym = atom_->eval(store, ass)) {
        out << sign_ << *sym;
        if (sign_ == Sign::once) {
            return index_ != std::numeric_limits<size_t>::max() || state_->base().contains(*sym);
        }
        return !state_->base().is_fact(*sym);
    }
    out << "#false";
    return true;
}

auto LitProject::domain(bool domain) const -> bool {
    // check if the base of the literal is domain
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
    std::cerr << "TODO: create projection matcher:\n";
    std::cerr << "- initialize projected domain on init\n";
    std::cerr << "- wrap the literal matchers\n";
    if (sign_ == Sign::once) {
        return {make_non_fact_matcher(state_->base(), *atom_), std::nullopt};
    }
    if (sign_ == Sign::twice && index_ != std::numeric_limits<size_t>::max()) {
        return {make_once_matcher(), std::nullopt};
    }

    auto index = std::optional<size_t>{};
    if (index_ != std::numeric_limits<size_t>::max() && type == MatcherType::new_atoms) {
        index = index_;
    }
    return {make_atom_matcher(bound, state_->base(), *atom_, type), index};
}

auto LitProject::score(std::vector<bool> const &bound) const -> double {
    static_cast<void>(bound);
    // TODO: proper score computation
    return 2;
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
