#include <optional>
#include <sstream>
#include <utility>

#include <util/print.hh>

#include <literal.hh>

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

[[nodiscard]] auto Literal::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, Literal const &literal) -> std::ostream & {
    literal.print(out);
    return out;
}

auto Literal::unpool() -> SLiteralVec {
    SLiteralVec lits;
    STermVec terms;
    PoolLiteral pool{lits, terms};
    unpool(pool);
    return lits;
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
    for (auto &&guard : rhs_) {
        out << guard.first << *guard.second;
    }
}

void LiteralRelation::add_sign(Sign s) { sign_ += s; }

namespace {

struct Mapper {
    static void unpool(PoolTerm &pool, Guard &elem) { elem.second->unpool(pool); }
    static auto map(Guard const &orig, STerm term) { return Guard{orig.first, std::move(term)}; }
    static auto equal(STerm &a, Guard &b) -> bool { return a == b.second; }
};

} // namespace

void LiteralRelation::unpool(PoolLiteral &pool) {
    unpool_with(
        [&](std::optional<STerm> &lhs, std::optional<GuardVec> &rhs) {
            if (!lhs.has_value() && !rhs.has_value()) {
                pool.append(this);
            } else {
                pool.append_shared<LiteralRelation>(lhs.value_or(lhs_), std::move(rhs).value_or(rhs_));
            }
        },
        unpool_element<PoolTerm>(pool, lhs_), unpool_crossproduct<PoolTerm, Guard, Mapper>(pool, rhs_));
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

[[nodiscard]] auto LiteralRelation::project(Projection project) -> SLiteral {
    static_cast<void>(project);
    return SLiteral{this};
}

////////// LiteralBoolean //////////

void LiteralBoolean::print(std::ostream &out) const { out << sign_ << (value_ ? "#true" : "#false"); }

void LiteralBoolean::add_sign(Sign s) { sign_ += s; }

void LiteralBoolean::unpool(PoolLiteral &pool) { pool.append(this); }

auto LiteralBoolean::is_equal(Literal const &other) const -> bool {
    auto const *d = dynamic_cast<LiteralBoolean const *>(&other);
    return d != nullptr && value_equal(sign_, d->sign_, value_, d->value_);
}

auto LiteralBoolean::hash() const -> size_t { return value_hash(typeid(LiteralSymbolic), sign_, value_); }

void LiteralBoolean::visit_variables(VarVisitFun const &fun) const { static_cast<void>(fun); }

[[nodiscard]] auto LiteralBoolean::project(Projection project) -> SLiteral {
    static_cast<void>(project);
    return SLiteral{this};
}

////////// LiteralSymbolic //////////

void LiteralSymbolic::print(std::ostream &out) const { out << sign_ << *term_; }

void LiteralSymbolic::add_sign(Sign s) { sign_ += s; }

void LiteralSymbolic::unpool(PoolLiteral &pool) {
    unpool_with(
        [&](std::optional<STerm> &term) {
            if (!term.has_value()) {
                pool.append(this);
            } else {
                pool.append_shared<LiteralSymbolic>(sign_, std::move(term).value());
            }
        },
        unpool_element<PoolTerm>(pool, term_));
}

auto LiteralSymbolic::is_equal(Literal const &other) const -> bool {
    auto const *d = dynamic_cast<LiteralSymbolic const *>(&other);
    return d != nullptr && value_equal(sign_, d->sign_, term_, d->term_);
}

auto LiteralSymbolic::hash() const -> size_t { return value_hash(typeid(LiteralSymbolic), sign_, term_); }

void LiteralSymbolic::visit_variables(VarVisitFun const &fun) const { term_->visit_variables(fun); }

[[nodiscard]] auto LiteralSymbolic::project(Projection project) -> SLiteral {
    if (sign_ == Sign::none) {
        auto term = term_->project(project);
        if (term != term_) {
            return construct_shared<LiteralSymbolic, Literal>(sign_, term);
        }
    }
    return SLiteral{this};
}

auto LiteralSymbolic::is_atom() const -> bool { return sign_ == Sign::none; }

auto LiteralSymbolic::is_test() const -> bool { return false; }
