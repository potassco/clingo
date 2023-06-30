#include <optional>
#include <utility>

#include <util/print.hh>

#include <aggregate.hh>

#include "unpool.hh"
#include "variables.hh"

auto operator<<(std::ostream &out, AggregateFunction fun) -> std::ostream & {
    switch (fun) {
        case AggregateFunction::count: {
            out << "#count";
            break;
        }
        case AggregateFunction::sum: {
            out << "#sum";
            break;
        }
        case AggregateFunction::sump: {
            out << "#sum+";
            break;
        }
        case AggregateFunction::min: {
            out << "#min";
            break;
        }
        case AggregateFunction::max: {
            out << "#max";
            break;
        }
    }
    return out;
}

void SetAggregate::set_rhs(STerm lhs, Relation rel) { lhs_ = std::make_pair(std::move(lhs), rel); }

void SetAggregate::unpool(PoolLiteral &pool, std::function<void(std::optional<SetAggregate>)> cb) {
    // unpool the aggregate elements
    std::optional<ElementVec> elems;
    size_t i = 0;
    for (auto &elem : elems_) {
        unpool_with(
            [&](std::optional<SLiteral> &lit, std::optional<SLiteralVec> &cond) {
                if (!lit.has_value() && !cond.has_value() && !elems.has_value()) {
                    return;
                }
                if (!elems.has_value()) {
                    elems = ElementVec{elems_.begin(), elems_.begin() + i};
                }
                auto &[e_lit, e_cond] = elem;
                elems->emplace_back(lit.value_or(e_lit), cond.value_or(e_cond));
            },
            unpool_element(pool, elem.first), unpool_crossproduct(pool, elem.second));
        ++i;
    }

    // unpool the guards and combine with the elements
    unpool_with(
        [&](std::optional<STerm> &lhs, std::optional<STerm> &rhs) {
            if (!lhs.has_value() && !rhs.has_value() && !elems.has_value()) {
                cb(std::nullopt);
                return;
            }
            auto aggr = SetAggregate(elems.value_or(elems_));
            if (lhs.has_value() || lhs_.has_value()) {
                aggr.lhs_ = std::make_pair(lhs.value() ? lhs.value() : lhs_->first, lhs_->second);
            }
            if (rhs.has_value() || rhs_.has_value()) {
                aggr.rhs_ = std::make_pair(rhs_->first, rhs.value() ? rhs.value() : rhs_->second);
            }
            cb(std::move(aggr));
        },
        unpool_element<PoolTerm, LGuard, UnpoolGuards>(pool, lhs_),
        unpool_element<PoolTerm, RGuard, UnpoolGuards>(pool, rhs_));
}

auto operator<<(std::ostream &out, SetAggregate const &aggr) -> std::ostream & {
    if (aggr.lhs_) {
        out << *aggr.lhs_->first << " " << aggr.lhs_->second << " ";
    }
    out << "{ " << p_range_with(aggr.elems_, "; ", [](std::ostream &out, auto const &elem) {
        out << *std::get<0>(elem);
        if (!std::get<1>(elem).empty()) {
            out << ": " << p_range{std::get<1>(elem), ", "};
        }
    }) << (aggr.elems_.empty() ? "}" : " }");
    if (aggr.rhs_) {
        out << " " << aggr.rhs_->first << " " << *aggr.rhs_->second;
    }
    return out;
}

void SetAggregate::visit_variables(std::function<void(std::string const &var)> fun, VariableContext ctx) const {
    VarVisitor visit{std::move(fun)};
    visit.add(lhs_, rhs_);
    if (ctx == VariableContext::all) {
        visit.add(elems_);
    }
}

auto SetAggregate::project(Projection project, bool in_negative_scope) -> std::optional<SetAggregate> {
    if (!in_negative_scope && reduct_is_nonmonotone(lhs_, AggregateFunction::count, rhs_)) {
        return std::nullopt;
    }
    std::optional<ElementVec> elems;
    size_t n = 0;
    for (auto const &[lit, cond] : elems_) {
        // add counts of local variables
        VarCounter counter{project.counts()};
        counter.add(lit);
        counter.add(cond);
        auto sub_project = Projection{counter};

        // project literal if requested
        size_t m = 0;
        if (elems.has_value()) {
            elems->emplace_back(lit, copy_n(cond, m));
        }
        // project literals in condition
        for (auto const &lit_c : cond) {
            auto projected_lit = lit_c->project(sub_project);
            if (projected_lit != lit_c && !elems.has_value()) {
                elems = copy_n(elems_, n);
                elems->emplace_back(lit, copy_n(cond, m));
            }
            if (elems.has_value()) {
                std::get<1>(elems->back()).emplace_back(projected_lit);
            }
            ++m;
        }
        ++n;
    }
    if (elems.has_value()) {
        return SetAggregate{lhs_, std::move(elems).value(), rhs_};
    }
    return std::nullopt;
}
