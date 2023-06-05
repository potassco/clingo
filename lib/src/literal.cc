#include <sstream>

#include <util/print.hh>

#include <literal.hh>
#include <utility>

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
    STermVec terms;
    SLiteralVec lits;
    GuardVec guards;
    PoolLiteral pool{lits, guards, terms};
    unpool(pool);
    return lits;
}

namespace {

struct Mapper {
    static void map(Pool<STerm> &pool, Guard &elem) {
        // TODO: can this static cast be somehow avoided
        // Should be doable by having a "two way" mapper for the UnpoolCrossproduct helper.
        // This would also avoid the need to have a separate vector.
        elem.second->unpool(pool);
    }
    // TODO: consider mapping elements individually
    static auto unmap(GuardVec const &orig, STermVec vec) {
        GuardVec res;
        res.reserve(orig.size());
        for (size_t i = 0; i < orig.size(); ++i) {
            res.emplace_back(orig[i].first, std::move(vec[i]));
        }
        return res;
    }
    static auto equal(STerm &a, Guard &b) -> bool { return a == b.second; }
};

} // namespace

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

void LiteralRelation::unpool(PoolLiteral &pool) {
    unpool_with(
        [&](auto &&lhs, auto &&rhs, bool unchanged) {
            if (unchanged) {
                pool.get<SLiteral>().append(this);
            } else {
                pool.get<SLiteral>().append_shared<LiteralRelation>(FWD(lhs), FWD(rhs_));
            }
        },
        pool.get<STerm>().element(lhs_), pool.get<STerm>().crossproduct<Guard, Mapper>(rhs_));
}

////////// LiteralBoolean //////////

void LiteralBoolean::print(std::ostream &out) const { out << sign_ << (value_ ? "#true" : "#false"); }

void LiteralBoolean::add_sign(Sign s) { sign_ += s; }

void LiteralBoolean::unpool(PoolLiteral &pool) { pool.get<SLiteral>().append(this); }

////////// LiteralSymbolic //////////

void LiteralSymbolic::print(std::ostream &out) const { out << sign_ << *term_; }

void LiteralSymbolic::add_sign(Sign s) { sign_ += s; }

void LiteralSymbolic::unpool(PoolLiteral &pool) {
    unpool_with(
        [&](auto &&term, bool unchanged) {
            if (unchanged) {
                pool.get<SLiteral>().append(this);
            } else {
                pool.get<SLiteral>().append_shared<LiteralSymbolic>(sign_, FWD(term));
            }
        },
        pool.get<STerm>().element(term_));
}
