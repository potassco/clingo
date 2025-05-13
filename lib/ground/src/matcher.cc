#include <clingo/ground/matcher.hh>

namespace CppClingo::Ground {

namespace {

class CmpMatcher : public OnceMatcher {
  public:
    CmpMatcher(Term const &lhs, Relation cmp, Term const &rhs) : lhs_{&lhs}, rhs_{&rhs}, cmp_{cmp} {}

  private:
    auto do_once(EvalContext const &ctx) -> bool override {
        // std::cerr << "doing a cmp match: " << *lhs_ << " " << cmp_ << " " << *rhs_ << "\n";
        auto lhs = lhs_->eval(ctx);
        if (!lhs) {
            return false;
        }
        auto rhs = rhs_->eval(ctx);
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
    auto do_once(EvalContext const &ctx) -> bool override {
        // unbind variables
        for (auto const &var : free_) {
            ctx.ass()[var] = std::nullopt;
        }
        auto rhs = rhs_->eval(ctx);
        // if (rhs) {
        //     std::cerr << "matching: " << *lhs_ << " and " << *rhs << "\n";
        // }
        return rhs && lhs_->match(ctx, *rhs);
    }
    void do_print(std::ostream &out) const override { out << *lhs_ << ":=" << *rhs_; }

    Term const *lhs_;
    Term const *rhs_;
    VariableVec free_;
};

class IntervalMatcher : public Matcher {
  public:
    IntervalMatcher(Term const &lhs, Term const &lower, Term const &upper, VariableVec free)
        : lhs_{&lhs}, lower_{&lower}, upper_{&upper}, free_{std::move(free)} {}

  private:
    void do_init([[maybe_unused]] InstantiationContext const &ctx, [[maybe_unused]] size_t gen) override {}
    void do_match(EvalContext const &ctx) override {
        val_current_ = 1;
        val_upper_ = 0;
        if (auto lower = lower_->eval(ctx), upper = upper_->eval(ctx);
            lower && upper && lower->type() == SymbolType::number && upper->type() == SymbolType::number) {
            if (!free_.empty()) {
                val_current_ = lower->num();
                val_upper_ = upper->num();
            }
            // Note: that the case free is empty could be handled a little more
            // efficiently. I would not expect a big impact, though.
            else if (auto lhs = lhs_->eval(ctx); lhs && lhs->type() == SymbolType::number &&
                                                 lower->num() <= lhs->num() && lhs->num() <= upper->num()) {
                val_current_ = lhs->num();
                val_upper_ = lhs->num();
            }
        }
    }
    auto do_next(EvalContext const &ctx) -> bool override {
        while (val_current_ <= val_upper_) {
            for (auto const &var : free_) {
                ctx.ass()[var] = std::nullopt;
            }
            auto num = val_current_;
            val_current_ += 1;
            if (lhs_->match(ctx, ctx.store().num_ref(std::move(num)))) {
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

auto make_once_matcher() -> UMatcher {
    return std::make_unique<OnceMatcher>();
}

auto make_interval_matcher(std::vector<bool> const &bound, Term const &lhs, Term const &lower, Term const &upper)
    -> UMatcher {
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

} // namespace CppClingo::Ground
