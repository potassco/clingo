#include <gringo/ground/literal.hh>

#include <typeindex>

#include <iostream>

namespace Gringo::Ground {

namespace {

class OnceMatcher : public Matcher {
  public:
    OnceMatcher() = default;
    virtual auto do_match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) -> bool {
        return true;
    }
    void init([[maybe_unused]] size_t gen) override {}
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override { match_ = true; }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        if (match_) {
            match_ = false;
            return do_match(store, ass);
        }
        return false;
    }

  private:
    bool match_ = false;
};

class CmpMatcher : public OnceMatcher {
  public:
    CmpMatcher(Term const &lhs, Relation cmp, Term const &rhs) : lhs_{&lhs}, rhs_{&rhs}, cmp_{cmp} {}
    auto do_match(SymbolStore &store, Assignment &ass) -> bool override {
        // std::cerr << "doing a cmp match: " << *lhs_ << " " << cmp_ << " " << *rhs_ << "\n";
        auto lhs = lhs_->eval(store, ass);
        if (!lhs) {
            return false;
        }
        auto rhs = rhs_->eval(store, ass);
        if (!rhs) {
            return false;
        }
        switch (cmp_) {
            case Relation::equal: {
                return *lhs == *rhs;
            }
            case Relation::greater: {
                return *lhs > *rhs;
            }
            case Relation::greater_equal: {
                return *lhs >= *rhs;
            }
            case Relation::less: {
                return *lhs < *rhs;
            }
            case Relation::less_equal: {
                return *lhs <= *rhs;
            }
            case Relation::not_equal: {
                return *lhs != *rhs;
            }
        }
        return false;
    }

  private:
    Term const *lhs_;
    Term const *rhs_;
    Relation cmp_;
};

class AssignMatcher : public OnceMatcher {
  public:
    AssignMatcher(Term const &lhs, Term const &rhs, VariableVec free)
        : lhs_{&lhs}, rhs_{&rhs}, free_{std::move(free)} {}
    auto do_match(SymbolStore &store, Assignment &ass) -> bool override {
        // unbind variables
        for (auto const &var : free_) {
            ass[var] = std::nullopt;
        }
        auto rhs = rhs_->eval(store, ass);
        // if (rhs) {
        //     std::cerr << "matching: " << *lhs_ << " and " << *rhs << "\n";
        // }
        return rhs && lhs_->match(store, *rhs, ass);
    }

  private:
    Term const *lhs_;
    Term const *rhs_;
    VariableVec free_;
};

class NonFactMatcher : public OnceMatcher {
  public:
    NonFactMatcher(Base const &base, Term const &term) : base_{&base}, term_{&term} {}
    void init(size_t gen) override { base_->update(gen); }
    auto do_match(SymbolStore &store, Assignment &ass) -> bool override {
        auto sym = term_->eval(store, ass);
        return !sym || !base_->is_fact(*sym);
    }

  private:
    Base const *base_;
    Term const *term_;
};

// TODO: this matcher is inefficient
// - However, matching like this is still a good idea for atoms of form
//   p(X,Y,Z) if no variables are bound.
// - Another interesting case is if all atoms are bound, then we can simply
//   lookup the symbol in the domain.
// - Otherwise, a lookup table should be build traversing the atoms in the
//   domain.
class DummyMatcher : public Matcher {
  public:
    DummyMatcher(Base const &base, Term const &term, VariableVec free, MatcherType type)
        : base_{&base}, term_{&term}, free_{std::move(free)}, type_{type} {}
    void init(size_t gen) override { base_->update(gen); }
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override {
        current_ = base_->begin(type_);
        // std::cerr << "matching: " << *term_ << " in range " << current_ << "-" << base_->end(type_) << "\n";
    }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        for (auto n = base_->end(type_); current_ < n;) {
            // std::cerr << "matching: " << *term_ << " and " << base_->nth(current_)->first << "\n";
            //  unbind variables
            for (auto const &var : free_) {
                ass[var] = std::nullopt;
            }
            // match symbol and term
            if (term_->match(store, base_->nth(current_++)->first, ass)) {
                return true;
            }
        }
        return false;
    }

  private:
    Base const *base_;
    Term const *term_;
    VariableVec free_;
    MatcherType type_;
    size_t current_ = 0;
};

class IntervalMatcher : public Matcher {
  public:
    IntervalMatcher(Term const &lhs, Term const &lower, Term const &upper, VariableVec free)
        : lhs_{&lhs}, lower_{&lower}, upper_{&upper}, free_{std::move(free)} {}
    void init([[maybe_unused]] size_t gen) override {}
    void match(SymbolStore &store, Assignment &ass) override {
        val_current_ = 1;
        val_upper_ = 0;
        if (auto lower = lower_->eval(store, ass), upper = upper_->eval(store, ass);
            lower && upper && lower->type() == SymbolType::number && upper->type() == SymbolType::number) {
            if (!free_.empty()) {
                val_current_ = lower->num();
                val_upper_ = upper->num();
            }
            // Note: that the case free is empty could be handled a little more
            // efficiently. I would not expect a big impact, though.
            else if (auto lhs = lhs_->eval(store, ass); lhs && lhs->type() == SymbolType::number &&
                                                        *lower->num() <= *lhs->num() && *lhs->num() <= *upper->num()) {
                val_current_ = lhs->num();
                val_upper_ = lhs->num();
            }
        }
    }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        while (val_current_ <= val_upper_) {
            for (auto const &var : free_) {
                ass[var] = std::nullopt;
            }
            auto num = val_current_;
            val_current_ += 1;
            if (lhs_->match(store, store.num(std::move(num)), ass)) {
                return true;
            }
        }
        return false;
    }

  private:
    Term const *lhs_;
    Term const *lower_;
    Term const *upper_;
    VariableVec free_;
    Number val_current_ = 0;
    Number val_upper_ = 0;
};

} // namespace

auto operator<<(std::ostream &out, Sign sign) -> std::ostream & {
    switch (sign) {
        case Sign::none: {
            break;
        }
        case Sign::once: {
            out << "not ";
            break;
        }
        case Sign::twice: {
            out << "not not ";
            break;
        }
    }
    return out;
}

auto operator<<(std::ostream &out, Relation rel) -> std::ostream & {
    switch (rel) {
        case Relation::equal: {
            out << "=";
            break;
        }
        case Relation::greater: {
            out << ">";
            break;
        }
        case Relation::greater_equal: {
            out << ">=";
            break;
        }
        case Relation::less: {
            out << "<";
            break;
        }
        case Relation::less_equal: {
            out << "<=";
            break;
        }
        case Relation::not_equal: {
            out << "!=";
            break;
        }
    }
    return out;
}

void LitInterval::print(std::ostream &out) const { out << *lhs_ << "=" << *lower_ << ".." << upper_; }

void LitInterval::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const {
    if (auto lhs = lhs_->eval(store, ass), lower = lower_->eval(store, ass), upper = upper_->eval(store, ass);
        lhs && lower && upper) {
        out << *lower << "<=" << *lhs << "<=" << *upper;
    } else {
        out << "#false";
    }
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

auto LitInterval::matcher([[maybe_unused]] MatcherType type, [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    VariableSet vars;
    lhs_->vars(vars);
    erase_if(vars, [&bound](auto const &var) { return bound[var]; });
    return {std::make_unique<IntervalMatcher>(*lhs_, *lower_, *upper_, vars.release()), std::nullopt};
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
        return std::tie(lhs_, lower_, upper_) == std::tie(x->lhs_, x->lower_, x->upper_);
    }
    return false;
}

auto LitInterval::compare_to(Lit const &other) const -> std::weak_ordering {
    auto const *x = dynamic_cast<LitInterval const *>(&other);
    if (x != nullptr) {
        return std::tie(lhs_, lower_, upper_) <=> std::tie(x->lhs_, x->lower_, x->upper_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

void LitComparison::print(std::ostream &out) const { out << *lhs_ << cmp_ << *rhs_; }

void LitComparison::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const {
    if (auto lhs = lhs_->eval(store, ass), rhs = rhs_->eval(store, ass); lhs && rhs) {
        out << *lhs << cmp_ << *rhs;
    } else {
        out << "#false";
    }
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
    if (cmp_ == Relation::equal) {
        VariableSet vars;
        lhs_->vars(vars, true);
        erase_if(vars, [&bound](auto const &var) { return bound[var]; });
        if (!vars.empty()) {
            return {std::make_unique<AssignMatcher>(*lhs_, *rhs_, vars.release()), std::nullopt};
        }
    }
    return {std::make_unique<CmpMatcher>(*lhs_, cmp_, *rhs_), std::nullopt};
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
        if (cmp_ == Relation::equal && x->cmp_ == Relation::equal && (*rhs_ < *lhs_ != *x->rhs_ < *x->lhs_)) {
            return std::tie(lhs_, rhs_) == std::tie(x->rhs_, x->lhs_);
        }
        return std::tie(lhs_, cmp_, rhs_) == std::tie(x->lhs_, x->cmp_, x->rhs_);
    }
    return false;
}

auto LitComparison::compare_to(Lit const &other) const -> std::weak_ordering {
    auto const *x = dynamic_cast<LitComparison const *>(&other);
    if (x != nullptr) {
        if (cmp_ == Relation::equal && x->cmp_ == Relation::equal && (*rhs_ < *lhs_ != *x->rhs_ < *x->lhs_)) {
            return std::tie(lhs_, rhs_) <=> std::tie(x->rhs_, x->lhs_);
        }
        return std::tie(lhs_, cmp_, rhs_) <=> std::tie(x->lhs_, x->cmp_, x->rhs_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

void LitSymbolic::print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != std::numeric_limits<size_t>::max()) {
        out << "[" << index_ << "]";
    }
}

void LitSymbolic::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const {
    if (auto sym = atom_->eval(store, ass)) {
        out << sign_ << *sym;
    } else {
        out << "#false";
    }
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

auto LitSymbolic::matcher(MatcherType type, std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    if (sign_ == Sign::once) {
        return {std::make_unique<NonFactMatcher>(*base_, *atom_), std::nullopt};
    }
    if (sign_ == Sign::twice && index_ != std::numeric_limits<size_t>::max()) {
        return {std::make_unique<OnceMatcher>(), std::nullopt};
    }
    // TODO: proper matcher creation
    VariableSet vars;
    atom_->vars(vars);
    erase_if(vars, [&bound](auto const &var) { return bound[var]; });
    auto index = std::optional<size_t>{};
    if (index_ != std::numeric_limits<size_t>::max() && type == MatcherType::new_atoms) {
        index = index_;
    }
    return {std::make_unique<DummyMatcher>(*base_, *atom_, vars.release(), type), index};
}

auto LitSymbolic::score(std::vector<bool> const &bound) const -> double {
    static_cast<void>(bound);
    // TODO: proper score computation
    return 2;
}

auto LitSymbolic::hash() const -> size_t { return Util::value_hash_record<LitSymbolic>(sign_, atom_); }

auto LitSymbolic::equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitSymbolic const *>(&other);
    return x != nullptr && std::tie(sign_, atom_) == std::tie(x->sign_, x->atom_);
}

auto LitSymbolic::compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *x = dynamic_cast<LitSymbolic const *>(&other); x != nullptr) {
        return std::tie(sign_, atom_) <=> std::tie(x->sign_, x->atom_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

} // namespace Gringo::Ground
