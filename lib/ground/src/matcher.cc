#include <gringo/ground/matcher.hh>

namespace Gringo::Ground {

namespace {

class CmpMatcher : public OnceMatcher {
  public:
    CmpMatcher(Term const &lhs, Relation cmp, Term const &rhs) : lhs_{&lhs}, rhs_{&rhs}, cmp_{cmp} {}

  private:
    auto do_once(InstantiationContext &ctx) -> bool override {
        // std::cerr << "doing a cmp match: " << *lhs_ << " " << cmp_ << " " << *rhs_ << "\n";
        auto lhs = lhs_->eval(ctx.store(), ctx.ass());
        if (!lhs) {
            return false;
        }
        auto rhs = rhs_->eval(ctx.store(), ctx.ass());
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
    void do_print(std::ostream &out) const override { out << *lhs_ << cmp_ << *rhs_; }

    Term const *lhs_;
    Term const *rhs_;
    Relation cmp_;
};

class AssignMatcher : public OnceMatcher {
  public:
    AssignMatcher(Term const &lhs, Term const &rhs, VariableVec free)
        : lhs_{&lhs}, rhs_{&rhs}, free_{std::move(free)} {}

  private:
    auto do_once(InstantiationContext &ctx) -> bool override {
        // unbind variables
        for (auto const &var : free_) {
            ctx.ass()[var] = std::nullopt;
        }
        auto rhs = rhs_->eval(ctx.store(), ctx.ass());
        // if (rhs) {
        //     std::cerr << "matching: " << *lhs_ << " and " << *rhs << "\n";
        // }
        return rhs && lhs_->match(ctx.store(), *rhs, ctx.ass());
    }
    void do_print(std::ostream &out) const override { out << *lhs_ << ":=" << *rhs_; }

    Term const *lhs_;
    Term const *rhs_;
    VariableVec free_;
};

class NonFactMatcher : public OnceMatcher {
  public:
    NonFactMatcher(Base &base, Term const &term, Symbol *target) : base_{&base}, term_{&term}, target_{target} {}

  private:
    void do_init([[maybe_unused]] SymbolStore &store, size_t gen) override { base_->update(gen); }
    auto do_once(InstantiationContext &ctx) -> bool override {
        if (auto sym = term_->eval(ctx.store(), ctx.ass())) {
            if (target_ != nullptr) {
                *target_ = *sym;
            }
            return !base_->is_fact(*sym);
        }
        return false;
    }
    void do_print(std::ostream &out) const override { out << "#not fact " << *term_; }

    Base *base_;
    Term const *term_;
    Symbol *target_;
};

class IntervalMatcher : public Matcher {
  public:
    IntervalMatcher(Term const &lhs, Term const &lower, Term const &upper, VariableVec free)
        : lhs_{&lhs}, lower_{&lower}, upper_{&upper}, free_{std::move(free)} {}

  private:
    void do_init([[maybe_unused]] SymbolStore &store, [[maybe_unused]] size_t gen) override {}
    void do_match(InstantiationContext &ctx) override {
        val_current_ = 1;
        val_upper_ = 0;
        if (auto lower = lower_->eval(ctx.store(), ctx.ass()), upper = upper_->eval(ctx.store(), ctx.ass());
            lower && upper && lower->type() == SymbolType::number && upper->type() == SymbolType::number) {
            if (!free_.empty()) {
                val_current_ = lower->num();
                val_upper_ = upper->num();
            }
            // Note: that the case free is empty could be handled a little more
            // efficiently. I would not expect a big impact, though.
            else if (auto lhs = lhs_->eval(ctx.store(), ctx.ass()); lhs && lhs->type() == SymbolType::number &&
                                                                    *lower->num() <= *lhs->num() &&
                                                                    *lhs->num() <= *upper->num()) {
                val_current_ = lhs->num();
                val_upper_ = lhs->num();
            }
        }
    }
    auto do_next(InstantiationContext &ctx) -> bool override {
        while (val_current_ <= val_upper_) {
            for (auto const &var : free_) {
                ctx.ass()[var] = std::nullopt;
            }
            auto num = val_current_;
            val_current_ += 1;
            if (lhs_->match(ctx.store(), ctx.store().num_ref(std::move(num)), ctx.ass())) {
                return true;
            }
        }
        return false;
    }
    void do_print(std::ostream &out) const override { out << *lhs_ << ":=" << *lower_ << ".." << *upper_; }

    Term const *lhs_;
    Term const *lower_;
    Term const *upper_;
    VariableVec free_;
    Number val_current_ = 0;
    Number val_upper_ = 0;
};

} // namespace

auto make_once_matcher() -> UMatcher { return std::make_unique<OnceMatcher>(); }

auto make_interval_matcher(std::vector<bool> const &bound, Term const &lhs, Term const &lower,
                           Term const &upper) -> UMatcher {
    VariableSet vars;
    lhs.vars(vars);
    erase_if(vars, [&bound](auto const &var) { return bound[var]; });
    return std::make_unique<IntervalMatcher>(lhs, lower, upper, vars.release());
}

auto make_comp_matcher(std::vector<bool> const &bound, Term const &lhs, Relation rel, Term const &rhs) -> UMatcher {
    if (rel == Relation::equal) {
        VariableSet vars;
        lhs.vars(vars, true);
        erase_if(vars, [&bound](auto const &var) { return bound[var]; });
        if (!vars.empty()) {
            return std::make_unique<AssignMatcher>(lhs, rhs, vars.release());
        }
    }
    return std::make_unique<CmpMatcher>(lhs, rel, rhs);
}

auto make_non_fact_matcher(Base &base, Term const &term, Symbol *target) -> UMatcher {
    return std::make_unique<NonFactMatcher>(base, term, target);
}

} // namespace Gringo::Ground
