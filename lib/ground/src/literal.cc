#include <gringo/ground/literal.hh>

#include <typeindex>

// TODO
#include <iostream>

namespace Gringo::Ground {

namespace {

class MatchOnce {
  public:
    MatchOnce() = default;
    void reset() { matched_ = false; }
    auto operator*() -> bool {
        if (!matched_) {
            matched_ = true;
            return true;
        }
        return false;
    }

  private:
    bool matched_ = false;
};

class NonFactMatcher : public Matcher {
  public:
    NonFactMatcher(Base const &base, Term const &term) : base_{&base}, term_{&term} {}
    void init(size_t gen) override { base_->update(gen); }
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override { once_.reset(); }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        if (*once_) {
            auto sym = term_->eval(store, ass);
            return !sym || !base_->is_fact(*sym);
        }
        return false;
    }

  private:
    Base const *base_;
    Term const *term_;
    MatchOnce once_;
};

class OnceMatcher : public Matcher {
  public:
    OnceMatcher() = default;
    void init([[maybe_unused]] size_t gen) override {}
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override { once_.reset(); }
    auto next([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) -> bool override { return *once_; }

  private:
    MatchOnce once_;
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
        std::cerr << "match " << *term_ << " in range [" << base_->begin(type_) << "," << base_->end(type_) << "]\n";
    }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        // TODO: take into consideration type
        for (auto n = base_->end(type_); current_ < n;) {
            // unbind variables
            for (auto const &var : free_) {
                ass[var] = std::nullopt;
            }
            // match symbol and term
            std::cerr << "matching " << *term_ << " and " << base_->nth(current_)->first << ":";
            if (term_->match(store, base_->nth(current_++)->first, ass)) {
                std::cerr << " true\n";
                return true;
            }
            std::cerr << " false\n";
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

void LitSymbolic::print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != std::numeric_limits<size_t>::max()) {
        out << "[" << index_ << "]";
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
    std::cerr << "todo create a proper matcher\n";
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
    std::cerr << "todo proper score for literal\n";
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
