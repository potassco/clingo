#include <optional>
#include <utility>

#include <util/print.hh>

#include <aggregate.hh>

#include "transform.hh"
#include "unpool.hh"
#include "variables.hh"

namespace Gringo::Input {

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

auto SetAggregate::unpool() const -> std::optional<std::vector<SetAggregate>> {
    return unpool_crossproducts(
        [&](auto lhs, auto elems, auto rhs) {
            return SetAggregate{std::move(lhs), std::move(elems), std::move(rhs)};
        },
        overloaded{
            [](ElementVec const &elems) -> std::optional<std::vector<ElementVec>> {
                return map_opt(
                    unpool_union(elems,
                                 [](auto elem) {
                                     return unpool_crossproducts(
                                         [](auto lit, auto cond) {
                                             return Element{std::move(lit), std::move(cond)};
                                         },
                                         overloaded{[](SLiteral const &lit) { return lit->unpool(); },
                                                    [](SLiteralVec const &lits) { return unpool_crossproduct(lits); }},
                                         std::get<0>(elem), std::get<1>(elem));
                                 }),
                    [](auto elems) { return make_vec<ElementVec>(std::move(elems)); });
            },
            [](LGuard const &lhs) -> std::optional<std::vector<LGuard>> {
                return and_then_opt(lhs, [](auto const &lhs) {
                    return map_opt_vec(lhs.first->unpool(), [&lhs](auto term) {
                        return std::make_optional<LGuard::value_type>(std::move(term), lhs.second);
                    });
                });
            },
            [](RGuard const &rhs) -> std::optional<std::vector<RGuard>> {
                return and_then_opt(rhs, [](auto const &rhs) {
                    return map_opt_vec(rhs.second->unpool(), [&rhs](auto term) {
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
        auto sub_project = Projection{project.mode(), counter};

        // project literals in condition
        auto fun = [sub_project](SLiteral const &lit) { return lit->project(sub_project); };
        return Util::transform_construct<Element>(lit, Util::Trans(cond, fun));
    };
    return Util::transform_construct<SetAggregate>(lhs_, Util::Trans{elems_, fun}, rhs_);
}

auto SetAggregate::project_anonymous() const -> std::optional<SetAggregate> {
    auto fun = [](SLiteral const &lit) { return lit->project_anonymous(); };
    return Util::transform_construct<SetAggregate>(lhs_, Util::Trans{elems_, fun}, rhs_);
}

auto SetAggregate::rewrite_anonymous(NameGen &gen) const -> std::optional<SetAggregate> {
    auto fun = overloaded{[&gen](STerm const &term) { return term->rewrite_anonymous(gen); },
                          [&gen](SLiteral const &lit) { return lit->rewrite_anonymous(gen); }};
    return Util::transform_construct<SetAggregate>(Util::Trans{lhs_, fun}, Util::Trans{elems_, fun},
                                                   Util::Trans{rhs_, fun});
}

} // namespace Gringo::Input
