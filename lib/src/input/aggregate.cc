#include <optional>
#include <utility>

#include <util/print.hh>

#include <input/aggregate.hh>

#include <input/algo/project.hh>
#include <input/algo/project_anonymous.hh>
#include <input/algo/unpool.hh>
#include <input/algo/visit_variables.hh>

#include "algo/transform.hh"
#include "algo/unpool.hh"
#include "algo/variables.hh"

namespace Gringo::Input {

void SetAggregate::visit_variables(std::function<void(std::string const &var)> fun, VariableContext ctx) const {
    using Gringo::Input::visit_variables;
    VarVisitor visit{std::move(fun)};
    visit.add(lhs_, rhs_);
    if (ctx == VariableContext::all) {
        visit.add(elems_);
    }
}

auto SetAggregate::project(Projection prj, bool in_negative_scope) const -> std::optional<SetAggregate> {
    using Gringo::Input::project;
    if (!in_negative_scope && reduct_is_nonmonotone(lhs_, AggregateFunction::count, rhs_)) {
        return std::nullopt;
    }
    auto fun = [prj](Element const &elem) -> std::optional<Element> {
        auto const &[lit, cond] = elem;

        // add counts of local variables
        VarCounter counter{prj.counts()};
        counter.add(lit);
        counter.add(cond);
        auto sub_project = Projection{prj.mode(), counter};

        // project literals in condition
        auto fun = [sub_project](Literal const &lit) { return project(lit, sub_project); };
        return transform_construct<Element>(lit, Trans(cond, fun));
    };
    return transform_construct<SetAggregate>(lhs_, Trans{elems_, fun}, rhs_);
}

auto SetAggregate::project_anonymous() const -> std::optional<SetAggregate> {
    using Gringo::Input::project_anonymous;
    auto fun = [](Literal const &lit) { return project_anonymous(lit); };
    return transform_construct<SetAggregate>(lhs_, Trans{elems_, fun}, rhs_);
}

} // namespace Gringo::Input
