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

////////// LiteralBoolean //////////

void LiteralBoolean::print(std::ostream &out) const { out << sign_ << (value_ ? "#true" : "#false"); }

void LiteralBoolean::add_sign(Sign s) { sign_ += s; }

void LiteralBoolean::unpool(PoolLiteral &pool) { pool.append(this); }

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
