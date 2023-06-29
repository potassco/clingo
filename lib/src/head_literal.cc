#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <util/print.hh>

#include <head_literal.hh>

#include "cond_lits.hh"

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

auto Disjunction::print_empty() const -> bool { return elems_.empty(); }

void Disjunction::print(std::ostream &out) const { print_cond_lits(elems_, global_, out, "#or", true); }

void Disjunction::unpool(PoolHeadLiteral &pool) { unpool_cond_lits(this, pool, global_, elems_); }

void Disjunction::visit_variables(std::function<void(std::string const &var)> fun, VariableContext ctx) const {
    cond_visit_variables(elems_, global_, fun, ctx);
}

auto Disjunction::project(Projection project) -> SHeadLiteral {
    // Note: that we can only project global variables in succeedents here.
    std::optional<ElementVec> elems;
    size_t n = 0;
    for (auto const &[lits, cond] : elems_) {
        // get counts of local variables
        std::unordered_map<std::string, size_t> counts;
        auto visit = [&project, &counts](std::string const &var) {
            if (!project.counts().contains(var)) {
                ++counts[var];
            }
        };
        for (auto const &lit : lits) {
            lit->visit_variables(visit);
        }
        for (auto const &lit : cond) {
            lit->visit_variables(visit);
        }
        auto local_project = Projection{counts};

        // project literals in antecedent
        size_t m = 0;
        if (elems.has_value()) {
            elems->emplace_back(lits, copy_n(cond, m));
        }
        for (auto const &lit : cond) {
            auto projected_lit = lit->project(local_project);
            if (projected_lit != lit && !elems.has_value()) {
                elems = copy_n(elems_, n);
                elems->emplace_back(lits, copy_n(cond, m));
            }
            if (elems.has_value()) {
                elems->back().second.emplace_back(projected_lit);
            }
            ++m;
        }
        ++n;
    }
    if (elems.has_value()) {
        return construct_shared<Disjunction, HeadLiteral>(global_, std::move(elems).value());
    }
    return SHeadLiteral{this};
}

////////// HeadTheoryAtom //////////

void HeadTheoryAtom::print(std::ostream &out) const { out << atom_; }

void HeadTheoryAtom::unpool(PoolHeadLiteral &pool) {
    atom_.unpool(pool, [&](std::optional<TheoryAtom> aggr) {
        if (!aggr.has_value()) {
            pool.append(this);
        } else {
            pool.append_shared<HeadTheoryAtom>(std::move(aggr).value());
        }
    });
}

void HeadTheoryAtom::visit_variables(std::function<void(std::string const &var)> fun, VariableContext ctx) const {
    atom_.visit_variables(fun, ctx);
}

auto HeadTheoryAtom::project(Projection project) -> SHeadLiteral { return SHeadLiteral{this}; }

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
            unpool_crossproduct<PoolTerm>(pool, std::get<0>(elem)),
            unpool_element<PoolLiteral>(pool, std::get<1>(elem)),
            unpool_crossproduct<PoolLiteral>(pool, std::get<2>(elem)));
        ++i;
    }

    // unpool the guards and combine with the elements
    unpool_with(
        [&](std::optional<STerm> &lhs, std::optional<STerm> &rhs) {
            if (!lhs.has_value() && !rhs.has_value() && !elems.has_value()) {
                pool.append(this);
                return;
            }
            pool.append_shared<HeadAggregate>(
                lhs ? LGuard(std::in_place, lhs.value(), lhs_->second) : (lhs_ ? lhs_ : std::nullopt), fun_,
                elems.value_or(elems_),
                rhs ? RGuard(std::in_place, rhs_->first, rhs.value()) : (rhs_ ? rhs_ : std::nullopt));
        },
        unpool_element<PoolTerm, LGuard, UnpoolGuards>(pool, lhs_),
        unpool_element<PoolTerm, RGuard, UnpoolGuards>(pool, rhs_));
}

void HeadAggregate::visit_variables(std::function<void(std::string const &var)> fun, VariableContext ctx) const {
    throw std::logic_error("implement me");
}

auto HeadAggregate::project(Projection project) -> SHeadLiteral {
    // Note: always project conditions
    throw std::logic_error("implement me");
}

////////// HeadSetAggregate //////////

void HeadSetAggregate::set_left_guard(STerm lhs, Relation rel) { aggr_.set_rhs(std::move(lhs), rel); }

void HeadSetAggregate::print(std::ostream &out) const { out << aggr_; }

void HeadSetAggregate::unpool(PoolHeadLiteral &pool) {
    aggr_.unpool(pool, [&](std::optional<SetAggregate> aggr) {
        if (!aggr.has_value()) {
            pool.append(this);
        } else {
            pool.append_shared<HeadSetAggregate>(std::move(aggr).value());
        }
    });
}

void HeadSetAggregate::visit_variables(std::function<void(std::string const &var)> fun, VariableContext ctx) const {
    throw std::logic_error("implement me");
}

auto HeadSetAggregate::project(Projection project) -> SHeadLiteral { throw std::logic_error("implement me"); }
