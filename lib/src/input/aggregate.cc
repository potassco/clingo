#include <optional>
#include <utility>

#include <util/print.hh>

#include <input/aggregate.hh>

#include "transform.hh"
#include "unpool.hh"
#include "variables.hh"

namespace Gringo::Input {

void SetAggregate::set_rhs(STerm lhs, Relation rel) { lhs_ = std::make_pair(std::move(lhs), rel); }

auto SetAggregate::unpool() const -> std::optional<std::vector<SetAggregate>> {
    return unpool_crossproducts(
        [&](auto lhs, auto elems, auto rhs) {
            return SetAggregate{std::move(lhs), std::move(elems), std::move(rhs)};
        },
        Util::overloaded{
            [](ElementVec const &elems) -> std::optional<std::vector<ElementVec>> {
                return map_opt(unpool_union(elems,
                                            [](auto elem) {
                                                return unpool_crossproducts(
                                                    [](auto lit, auto cond) {
                                                        return Element{std::move(lit), std::move(cond)};
                                                    },
                                                    Util::overloaded{[](SLiteral const &lit) { return lit->unpool(); },
                                                                     [](SLiteralVec const &lits) {
                                                                         return unpool_crossproduct(lits);
                                                                     }},
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
        return transform_construct<Element>(lit, Trans(cond, fun));
    };
    return transform_construct<SetAggregate>(lhs_, Trans{elems_, fun}, rhs_);
}

auto SetAggregate::project_anonymous() const -> std::optional<SetAggregate> {
    auto fun = [](SLiteral const &lit) { return lit->project_anonymous(); };
    return transform_construct<SetAggregate>(lhs_, Trans{elems_, fun}, rhs_);
}

} // namespace Gringo::Input
