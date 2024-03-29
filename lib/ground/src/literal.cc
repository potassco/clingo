#include <gringo/ground/literal.hh>

#include <typeindex>

namespace Gringo::Ground {

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

auto LitSymbolic::matcher(MatcherType type) -> UMatcher {
    static_cast<void>(type);
    // TODO:
    // - distinguish matcher types
    // - the first matcher can just iterate over the base to do the matching
    // - to construct the matcher the bound variables are still missing
    // - how to track new/old/all generations:
    //   - before accessing a domain we update the generation
    //     - this has the advantage of storing limits/generations only once
    //     - if the current generation is equal to generation:
    //       - do nothing
    //     - if the current generation is equal to generation - 1:
    //       - limit of the old generation is the previous all limit
    //       - limit of the all generation is the current domain size
    //       - update current generation
    //     - if the current generation is less than generation - 1:
    //       - limits of the old and all generations is the current domain size
    //       - update current generation
    //   - the above has to be performed when
    //     - accessing the limits
    //     - adding symbols to the base
    // - matchers track offsets of atoms in the base, they can determine old/new/all based on the old/all limits
    class DummyMatcher : public Matcher {
      public:
        void match(Assignment &ass) override {
            static_cast<void>(ass);
            printf("todo start match\n");
        }

        auto next(Assignment &ass) -> bool override {
            static_cast<void>(ass);
            printf("todo next match\n");
            return false;
        }
    };
    printf("todo create a proper matcher\n");
    return std::make_unique<DummyMatcher>();
}

auto LitSymbolic::score(std::vector<bool> const &bound) const -> double {
    static_cast<void>(bound);
    printf("todo proper score for literal\n");
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
