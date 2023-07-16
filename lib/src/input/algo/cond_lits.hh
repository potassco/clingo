#pragma once

#include <algorithm>

#include <input/literal.hh>

#include <util/print.hh>

#include <input/algo/check_type.hh>
#include <input/algo/project.hh>

#include "transform.hh"
#include "unpool.hh"
#include "variables.hh"

namespace Gringo::Input::CondLits {

void visit_variables(auto const &elems, VarVisitFun const &fun, VariableContext ctx) {
    VarVisitor visit{fun};
    for (auto const &elem : elems) {
        visit.add(elem.first);
        if (ctx == VariableContext::all) {
            visit.add(elem.second);
        }
    }
}
template <class T, class B, class P>
auto project(typename T::ElementVec const &elems, P prj, bool project_lits, bool in_classical_scope)
    -> std::optional<Util::shared_ptr<B>> {
    using Gringo::Input::is_atom;
    using Gringo::Input::project;
    using Elem = typename T::ElementVec::value_type;
    auto fun = [&](Elem const &elem) -> std::optional<Elem> {
        auto const &[lits, cond] = elem;
        bool project_cond =
            in_classical_scope || std::all_of(lits.begin(), lits.end(), [](auto const &lit) { return !is_atom(lit); });
        // project conclusion
        std::optional<LiteralVec> projected_lits = std::nullopt;
        if (project_lits) {
            auto fun = [prj](Literal const &lit) { return project(lit, prj); };
            projected_lits = transform(fun, lits);
        }
        // project premise
        std::optional<LiteralVec> projected_cond = std::nullopt;
        if (project_cond) {
            // add counts of local variables
            VarCounter counter{prj.counts()};
            counter.add(lits);
            counter.add(cond);
            // Note that there can be no global variables with just one
            // occurrence in a condition. However, we can project local
            // variables.
            auto sub_prj = Projection{prj.mode(), counter};
            auto fun = [sub_prj](Literal const &lit) { return project(lit, sub_prj); };
            projected_cond = transform(fun, cond);
        }
        if (projected_lits.has_value() || projected_cond.has_value()) {
            return Elem{std::move(projected_lits).value_or(lits), std::move(projected_cond).value_or(cond)};
        }
        return std::nullopt;
    };
    return transform_construct_shared<T, B>(Trans{elems, fun});
}

auto is_atom(auto const &elems) -> bool {
    using Gringo::Input::is_atom;
    if (elems.size() != 1) {
        return false;
    }
    auto const &[lits, cond] = elems.front();
    return cond.empty() && lits.size() == 1 && is_atom(lits.front());
}

auto is_test(auto const &elems) -> bool {
    using Gringo::Input::is_test;
    return std::all_of(elems.begin(), elems.end(), [](auto const &elem) {
        auto const &[lits, cond] = elem;
        return cond.empty() && std::all_of(lits.begin(), lits.end(), [](auto const &lit) { return is_test(lit); });
    });
}

} // namespace Gringo::Input::CondLits
