#include <optional>
#include <utility>

#include <util/print.hh>

#include <aggregate.hh>

#include "transform.hh"
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

void SetAggregate::unpool(PoolLiteral &pool, std::function<void(std::optional<SetAggregate>)> cb) const {
    // unpool the aggregate elements
    std::optional<ElementVec> elems;
    size_t i = 0;
    for (auto const &elem : elems_) {
        unpool_with(
            [&](std::optional<SLiteral> &lit, std::optional<SLiteralVec> &cond) {
                if (!lit.has_value() && !cond.has_value() && !elems.has_value()) {
                    return;
                }
                if (!elems.has_value()) {
                    elems = ElementVec{elems_.begin(), elems_.begin() + i};
                }
                auto const &[e_lit, e_cond] = elem;
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

auto SetAggregate::unpool_v2() const -> std::optional<std::vector<SetAggregate>> {
    return unpool_crossproducts(
        [&](auto lhs, auto elems, auto rhs) {
            return SetAggregate{std::move(lhs), std::move(elems), std::move(rhs)};
        },
        overloaded{
            [](ElementVec const &elems) -> std::optional<std::vector<ElementVec>> {
                return map_opt(unpool_union_v2(elems,
                                               [](auto elem) {
                                                   return unpool_crossproducts(
                                                       [](auto lit, auto cond) {
                                                           return Element{std::move(lit), std::move(cond)};
                                                       },
                                                       overloaded{[](SLiteral const &lit) { return lit->unpool_v2(); },
                                                                  [](SLiteralVec const &lits) {
                                                                      return unpool_crossproduct_v2(lits);
                                                                  }},
                                                       std::get<0>(elem), std::get<1>(elem));
                                               }),
                               [](auto elems) { return make_vec<ElementVec>(std::move(elems)); });
            },
            [](LGuard const &lhs) -> std::optional<std::vector<LGuard>> {
                return and_then_opt(lhs, [](auto const &lhs) {
                    return map_opt_vec(lhs.first->unpool_v2(), [&lhs](auto term) {
                        return std::make_optional<LGuard::value_type>(std::move(term), lhs.second);
                    });
                });
            },
            [](RGuard const &rhs) -> std::optional<std::vector<RGuard>> {
                return and_then_opt(rhs, [](auto const &rhs) {
                    return map_opt_vec(rhs.second->unpool_v2(), [&rhs](auto term) {
                        return std::make_optional<RGuard::value_type>(rhs.first, std::move(term));
                    });
                });
            },
        },
        lhs_, elems_, rhs_);
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

auto SetAggregate::project(Projection project, bool in_negative_scope) const -> std::optional<SetAggregate> {
    if (!in_negative_scope && reduct_is_nonmonotone(lhs_, AggregateFunction::count, rhs_)) {
        return std::nullopt;
    }
    auto fun = [project](Element const &elem) -> std::optional<Element> {
        auto const &[lit, cond] = elem;

        // add counts of local variables
        VarCounter counter{project.counts()};
        counter.add(lit);
        counter.add(cond);
        auto sub_project = Projection{counter};

        // project literals in condition
        auto fun = [sub_project](SLiteral const &lit) { return lit->project(sub_project); };
        return transform_construct<Element>(lit, Trans(cond, fun));
    };
    return transform_construct<SetAggregate>(lhs_, Trans{elems_, fun}, rhs_);
}

auto SetAggregate::rewrite_anonymous(NameGen &gen) const -> std::optional<SetAggregate> {
    auto fun = overloaded{[&gen](STerm const &term) { return term->rewrite_anonymous(gen); },
                          [&gen](SLiteral const &lit) { return lit->rewrite_anonymous(gen); }};
    return transform_construct<SetAggregate>(Trans{lhs_, fun}, Trans{elems_, fun}, Trans{rhs_, fun});
}
