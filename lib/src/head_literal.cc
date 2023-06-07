#include <iterator>
#include <optional>
#include <sstream>
#include <utility>

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

    // unpool the conditions
    SLitVecOVecOVec conds;
    size_t i = 0;
    for (auto &elem : elems_) {
        unpool_with(
            [&](std::optional<SLiteralVec> &cond) {
                if (cond.has_value()) {
                    if (!conds.has_value()) {
                        conds = SLitVecOVecVec(elems_.size());
                    }
                    if (!conds->at(i).has_value()) {
                        conds->at(i) = SLitVecVec{};
                    }
                    conds->at(i)->emplace_back(std::move(cond).value());
                }
            },
            unpool_crossproduct(pool.child, elem.second));
        ++i;
    }

    // unpool literals and combine with conditions
    unpool_with(
        [&](std::optional<SLiteralVec> &lits) {
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
        unpool_crossproduct<PoolLiteral, Disjunction::Element, MapLiteral>(pool.child, elems_));
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

void HeadAggregate::unpool(PoolHeadLiteral &pool) {
    // unpool the aggregate elements
    std::optional<ElementVec> elems;
    size_t i = 0;
    for (auto &elem : elems_) {
        unpool_with(
            [&](std::optional<STermVec> &tuple, std::optional<SLiteral> &lit, std::optional<SLiteralVec> &cond) {
                if (!tuple.has_value() && !lit.has_value() && !cond.has_value() && !elems.has_value()) {
                    return;
                }
                if (!elems.has_value()) {
                    elems = ElementVec{elems_.begin(), elems_.begin() + i};
                }
                auto &[e_tuple, e_lit, e_cond] = elem;
                elems->emplace_back(tuple.value_or(e_tuple), lit.value_or(e_lit), cond.value_or(e_cond));
            },
            unpool_crossproduct(pool.child.child, std::get<0>(elem)), unpool_element(pool.child, std::get<1>(elem)),
            unpool_crossproduct(pool.child, std::get<2>(elem)));
        ++i;
    }

    // unpool the guards and combine with the elements
    unpool_with(
        [&](std::optional<STerm> &lhs, std::optional<STerm> &rhs) {
            if (!lhs.has_value() && !rhs.has_value() && !elems.has_value()) {
                pool.append(this);
                return;
            }
            auto aggr = construct_shared<HeadAggregate, HeadAggregate>(fun_, elems.value_or(elems_));
            if (lhs.has_value() || lhs_.has_value()) {
                aggr->set_left_guard(lhs.value() ? lhs.value() : lhs_->first, lhs_->second);
            }
            if (rhs.has_value() || rhs_.has_value()) {
                aggr->rhs_ = std::make_pair(rhs_->first, rhs.value() ? rhs.value() : rhs_->second);
            }
            pool.append(std::move(aggr));
        },
        unpool_element<PoolTerm, LGuard, UnpoolGuards>(pool.child.child, lhs_),
        unpool_element<PoolTerm, RGuard, UnpoolGuards>(pool.child.child, rhs_));
}

////////// HeadSetAggregate //////////

void HeadSetAggregate::set_left_guard(STerm lhs, Relation rel) { aggr_.set_rhs(std::move(lhs), rel); }

void HeadSetAggregate::print(std::ostream &out) const { out << aggr_; }

void HeadSetAggregate::unpool(PoolHeadLiteral &pool) {
    aggr_.unpool(pool.child, [&](std::optional<SetAggregate> aggr) {
        if (!aggr.has_value()) {
            pool.append(this);
        } else {
            pool.append_shared<HeadSetAggregate>(std::move(aggr).value());
        }
    });
}
