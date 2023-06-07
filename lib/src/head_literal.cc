#include <optional>
#include <sstream>

#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <util/print.hh>

#include <head_literal.hh>

#include "unpool.hh"

////////// HeadLiteral //////////

[[nodiscard]] auto HeadLiteral::print_empty() const -> bool { return false; }

[[nodiscard]] auto HeadLiteral::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, HeadLiteral const &literal) -> std::ostream & {
    literal.print(out);
    return out;
}

auto HeadLiteral::unpool() -> SHeadLiteralVec {
    SHeadLiteralVec head_lits;
    SLiteralVec lits;
    STermVec terms;
    PoolHeadLiteral pool{head_lits, lits, terms};
    unpool(pool);
    return head_lits;
}
////////// Disjunction //////////

[[nodiscard]] auto Disjunction::print_empty() const -> bool { return elems_.empty(); }

void Disjunction::print(std::ostream &out) const {
    out << p_range_with(elems_, "; ", [](std::ostream &out, auto const &elem) {
        out << *elem.first;
        if (!elem.second.empty()) {
            out << ": " << p_range(elem.second);
        }
    });
}

namespace {

struct MapLiteral {
    static void unpool(PoolLiteral &pool, Disjunction::Element &elem) { elem.first->unpool(pool); }
    static auto map(Disjunction::Element const &orig, SLiteral lit) { return std::move(lit); }
    static auto equal(SLiteral &a, Disjunction::Element &b) -> bool { return a == b.first; }
};

} // namespace

void Disjunction::unpool(PoolHeadLiteral &pool) {
    using SLitVecVec = std::vector<SLiteralVec>;
    using SLitVecOVec = std::optional<SLitVecVec>;
    using SLitVecOVecVec = std::vector<std::optional<std::vector<SLiteralVec>>>;
    using SLitVecOVecOVec = std::optional<std::vector<std::optional<std::vector<SLiteralVec>>>>;

    unpool_with(
        // combine literals and conditions
        [&](std::optional<SLiteralVec> &lits, SLitVecOVecOVec &conds) {
            if (!lits.has_value() && !conds.has_value()) {
                pool.append(this);
                return;
            }
            Disjunction::ElementVec elems;
            for (size_t i = 0; i < elems_.size(); ++i) {
                SLiteral lit = lits.has_value() ? std::move(lits->at(i)) : elems_[i].first;
                if (conds.has_value() && conds->at(i).has_value()) {
                    for (auto &cond : conds->at(i).value()) {
                        elems.emplace_back(lit, lits.has_value() ? cond : std::move(cond));
                    }
                } else {
                    elems.emplace_back(std::move(lit), elems_[i].second);
                }
            }
            pool.append_shared<Disjunction>(std::move(elems));
        },
        // unpool the literals
        unpool_crossproduct<PoolLiteral, Disjunction::Element, MapLiteral>(pool.child, elems_),
        // unpool the conditions
        unpool_map(pool.child, elems_, [](auto &pool, auto &elems) {
            SLitVecOVecOVec conds;
            for (auto &elem : elems) {
                unpool_with(
                    [&](std::optional<SLiteralVec> &cond) {
                        if (cond.has_value()) {
                            if (!conds.has_value()) {
                                conds = SLitVecOVecVec(elems.size());
                            }
                            if (!conds->back().has_value()) {
                                conds->back() = SLitVecVec{};
                            }
                            conds->back()->emplace_back(std::move(cond).value());
                        }
                    },
                    unpool_crossproduct(pool, elem.second));
            }
            return conds;
        }));
}

////////// HeadTheoryAtom //////////

void HeadTheoryAtom::print(std::ostream &out) const { out << atom_; }

void HeadTheoryAtom::unpool(PoolHeadLiteral &pool) { throw std::logic_error("implement me"); }

////////// HeadAggregate //////////

void HeadAggregate::set_left_guard(STerm lhs, Relation rel) { lhs_ = std::make_pair(std::move(lhs), rel); }

void HeadAggregate::print(std::ostream &out) const {
    if (lhs_) {
        out << *lhs_->first << " " << lhs_->second << " ";
    }
    out << fun_ << " { " << p_range_with(elems_, "; ", [](std::ostream &out, auto const &elem) {
        out << p_range{std::get<0>(elem), ","} << ": " << *std::get<1>(elem);
        if (!std::get<2>(elem).empty()) {
            out << ": " << p_range{std::get<2>(elem), ", "};
        }
    }) << (elems_.empty() ? "}" : " }");
    if (rhs_) {
        out << " " << rhs_->first << " " << *rhs_->second;
    }
}

void HeadAggregate::unpool(PoolHeadLiteral &pool) { throw std::logic_error("implement me"); }

////////// HeadSetAggregate //////////

void HeadSetAggregate::set_left_guard(STerm lhs, Relation rel) { aggr_.set_rhs(std::move(lhs), rel); }

void HeadSetAggregate::print(std::ostream &out) const { out << aggr_; }

void HeadSetAggregate::unpool(PoolHeadLiteral &pool) { throw std::logic_error("implement me"); }
