#include <optional>
#include <sstream>
#include <utility>

#include <util/print.hh>

#include <literal.hh>

#include "transform.hh"
#include "unpool.hh"

////////// Literal //////////

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

auto operator<<(std::ostream &out, Sign op) -> std::ostream & {
    switch (op) {
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

auto Literal::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, Literal const &literal) -> std::ostream & {
    literal.print(out);
    return out;
}

auto Literal::is_atom() const -> bool { return false; }

auto Literal::is_test() const -> bool { return true; }

////////// LiteralRelation //////////

auto operator<<(std::ostream &out, Relation op) -> std::ostream & {
    switch (op) {
        case Relation::less: {
            out << "<";
            break;
        }
        case Relation::less_equal: {
            out << "<=";
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
        case Relation::equal: {
            out << "=";
            break;
        }
        case Relation::inequal: {
            out << "!=";
            break;
        }
    }
    return out;
}

void LiteralRelation::print(std::ostream &out) const {
    out << sign_ << *lhs_;
    for (auto const &guard : rhs_) {
        out << guard.first << *guard.second;
    }
}

void LiteralRelation::add_sign(Sign s) { sign_ += s; }

#include <iostream>

auto LiteralRelation::unpool() const -> std::optional<SLiteralVec> {
    return unpool_crossproducts(
        [this](auto lhs, auto rhs) {
            return construct_shared<LiteralRelation, Literal>(sign_, std::move(lhs), std::move(rhs));
        },
        overloaded{
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
    return d != nullptr && value_equal(sign_, d->sign_, lhs_, d->lhs_, rhs_, d->rhs_);
}

auto LiteralRelation::hash() const -> size_t { return value_hash(typeid(LiteralRelation), sign_, lhs_, rhs_); }

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

auto LiteralRelation::rewrite_anonymous(NameGen &gen) const -> std::optional<SLiteral> {
    auto fun = [&gen](STerm const &term) { return term->rewrite_anonymous(gen); };
    return transform_construct_shared<LiteralRelation, Literal>(Trans{lhs_, fun}, Trans{rhs_, fun});
}

////////// LiteralBoolean //////////

void LiteralBoolean::print(std::ostream &out) const { out << sign_ << (value_ ? "#true" : "#false"); }

void LiteralBoolean::add_sign(Sign s) { sign_ += s; }

auto LiteralBoolean::unpool() const -> std::optional<SLiteralVec> { return std::nullopt; }

auto LiteralBoolean::is_equal(Literal const &other) const -> bool {
    auto const *d = dynamic_cast<LiteralBoolean const *>(&other);
    return d != nullptr && value_equal(sign_, d->sign_, value_, d->value_);
}

auto LiteralBoolean::hash() const -> size_t { return value_hash(typeid(LiteralSymbolic), sign_, value_); }

void LiteralBoolean::visit_variables(VarVisitFun const &fun) const { static_cast<void>(fun); }

auto LiteralBoolean::project(Projection project) const -> std::optional<SLiteral> {
    static_cast<void>(project);
    return std::nullopt;
}

auto LiteralBoolean::rewrite_anonymous(NameGen &gen) const -> std::optional<SLiteral> {
    static_cast<void>(gen);
    return std::nullopt;
}

////////// LiteralSymbolic //////////

void LiteralSymbolic::print(std::ostream &out) const { out << sign_ << *term_; }

void LiteralSymbolic::add_sign(Sign s) { sign_ += s; }

auto LiteralSymbolic::unpool() const -> std::optional<SLiteralVec> {
    return map_opt_vec(term_->unpool(), [this](auto term) {
        return construct_shared<LiteralSymbolic, Literal>(sign_, std::move(term));
    });
}

auto LiteralSymbolic::is_equal(Literal const &other) const -> bool {
    auto const *d = dynamic_cast<LiteralSymbolic const *>(&other);
    return d != nullptr && value_equal(sign_, d->sign_, term_, d->term_);
}

auto LiteralSymbolic::hash() const -> size_t { return value_hash(typeid(LiteralSymbolic), sign_, term_); }

void LiteralSymbolic::visit_variables(VarVisitFun const &fun) const { term_->visit_variables(fun); }

auto LiteralSymbolic::project(Projection project) const -> std::optional<SLiteral> {
    if (sign_ == Sign::none) {
        auto fun = [&project](STerm const &term) { return term->project(project); };
        return transform_construct_shared<LiteralSymbolic, Literal>(sign_, Trans{term_, fun});
    }
    return std::nullopt;
}

auto LiteralSymbolic::is_atom() const -> bool { return sign_ == Sign::none; }

auto LiteralSymbolic::is_test() const -> bool { return false; }

auto LiteralSymbolic::rewrite_anonymous(NameGen &gen) const -> std::optional<SLiteral> {
    auto fun = [&gen](STerm const &term) { return term->rewrite_anonymous(gen); };
    return transform_construct_shared<LiteralSymbolic, Literal>(sign_, Trans{term_, fun});
}
