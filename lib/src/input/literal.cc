#include <optional>
#include <utility>

#include <input/literal.hh>

#include "transform.hh"
#include "unpool.hh"

namespace Gringo::Input {

////////// Literal //////////

namespace {

auto tra(auto const &x, NameGen &gen) {
    return Trans(x, [&gen](STerm const &term) { return term->rewrite_anonymous(gen); });
}

auto tpa(auto const &x) {
    return Trans(x, [](STerm const &term) { return term->project_anonymous(); });
}

auto tp(auto const &x, Projection project) {
    return Trans(x, [project](STerm const &term) { return term->project(project); });
}

} // namespace

auto operator-(Sign a) -> Sign {
    switch (a) {
        case Sign::none: {
            return Sign::once;
        }
        case Sign::once: {
            return Sign::twice;
        }
        case Sign::twice: {
            break;
        }
    }
    return Sign::once;
}

auto operator+(Sign a, Sign b) -> Sign {
    switch (a) {
        case Sign::none: {
            return b;
        }
        case Sign::once: {
            return -b;
        }
        case Sign::twice: {
            break;
        }
    }
    return -(-b);
}

auto operator+=(Sign &a, Sign b) -> Sign & {
    a = a + b;
    return a;
}

auto Literal::is_atom() const -> bool { return false; }

auto Literal::is_test() const -> bool { return true; }

////////// LiteralRelation //////////

void LiteralRelation::add_sign(Sign s) { sign_ += s; }

auto LiteralRelation::unpool() const -> std::optional<SLiteralVec> {
    return unpool_crossproducts(
        [this](auto lhs, auto rhs) {
            return construct_shared<LiteralRelation, Literal>(sign_, std::move(lhs), std::move(rhs));
        },
        Util::overloaded{
            [](STerm const &term) { return term->unpool(); },
            [](GuardVec const &guard) {
                return unpool_crossproduct(guard, [](Guard const &guard) {
                    return map_opt_vec(guard.second->unpool(), [&guard](auto term) {
                        return Guard{guard.first, std::move(term)};
                    });
                });
            },
        },
        lhs_, rhs_);
}

auto LiteralRelation::is_equal(Literal const &other) const -> bool {
    auto const *d = dynamic_cast<LiteralRelation const *>(&other);
    return d != nullptr && Util::value_equal(sign_, d->sign_, lhs_, d->lhs_, rhs_, d->rhs_);
}

auto LiteralRelation::hash() const -> size_t { return Util::value_hash(typeid(LiteralRelation), sign_, lhs_, rhs_); }

void LiteralRelation::visit_variables(VarVisitFun const &fun) const {
    lhs_->visit_variables(fun);
    for (auto const &guard : rhs_) {
        guard.second->visit_variables(fun);
    }
}

auto LiteralRelation::project(Projection project) const -> std::optional<SLiteral> {
    static_cast<void>(project);
    return std::nullopt;
}

auto LiteralRelation::project_anonymous() const -> std::optional<SLiteral> { return std::nullopt; }

auto LiteralRelation::rewrite_anonymous(NameGen &gen) const -> std::optional<SLiteral> {
    return transform_construct_shared<LiteralRelation, Literal>(tra(lhs_, gen), tra(rhs_, gen));
}

void LiteralRelation::accept(LiteralVisitor const &visitor) const { visitor.visit(*this); }

////////// LiteralBoolean //////////

void LiteralBoolean::add_sign(Sign s) { sign_ += s; }

auto LiteralBoolean::unpool() const -> std::optional<SLiteralVec> { return std::nullopt; }

auto LiteralBoolean::is_equal(Literal const &other) const -> bool {
    auto const *d = dynamic_cast<LiteralBoolean const *>(&other);
    return d != nullptr && Util::value_equal(sign_, d->sign_, value_, d->value_);
}

auto LiteralBoolean::hash() const -> size_t { return Util::value_hash(typeid(LiteralSymbolic), sign_, value_); }

void LiteralBoolean::visit_variables(VarVisitFun const &fun) const { static_cast<void>(fun); }

auto LiteralBoolean::project(Projection project) const -> std::optional<SLiteral> {
    static_cast<void>(project);
    return std::nullopt;
}

auto LiteralBoolean::project_anonymous() const -> std::optional<SLiteral> { return std::nullopt; }

auto LiteralBoolean::rewrite_anonymous(NameGen &gen) const -> std::optional<SLiteral> {
    static_cast<void>(gen);
    return std::nullopt;
}

void LiteralBoolean::accept(LiteralVisitor const &visitor) const { visitor.visit(*this); }

////////// LiteralSymbolic //////////

void LiteralSymbolic::add_sign(Sign s) { sign_ += s; }

auto LiteralSymbolic::unpool() const -> std::optional<SLiteralVec> {
    return map_opt_vec(term_->unpool(), [this](auto term) {
        return construct_shared<LiteralSymbolic, Literal>(sign_, std::move(term));
    });
}

auto LiteralSymbolic::is_equal(Literal const &other) const -> bool {
    auto const *d = dynamic_cast<LiteralSymbolic const *>(&other);
    return d != nullptr && Util::value_equal(sign_, d->sign_, term_, d->term_);
}

auto LiteralSymbolic::hash() const -> size_t { return Util::value_hash(typeid(LiteralSymbolic), sign_, term_); }

void LiteralSymbolic::visit_variables(VarVisitFun const &fun) const { term_->visit_variables(fun); }

auto LiteralSymbolic::project(Projection project) const -> std::optional<SLiteral> {
    if (sign_ == Sign::none) {
        return transform_construct_shared<LiteralSymbolic, Literal>(sign_, tp(term_, project));
    }
    return std::nullopt;
}

auto LiteralSymbolic::project_anonymous() const -> std::optional<SLiteral> {
    if (sign_ != Sign::none) {
        return transform_construct_shared<LiteralSymbolic, Literal>(sign_, tpa(term_));
    }
    return std::nullopt;
}

auto LiteralSymbolic::is_atom() const -> bool { return sign_ == Sign::none; }

auto LiteralSymbolic::is_test() const -> bool { return false; }

auto LiteralSymbolic::rewrite_anonymous(NameGen &gen) const -> std::optional<SLiteral> {
    return transform_construct_shared<LiteralSymbolic, Literal>(sign_, tra(term_, gen));
}

void LiteralSymbolic::accept(LiteralVisitor const &visitor) const { visitor.visit(*this); }

} // namespace Gringo::Input
