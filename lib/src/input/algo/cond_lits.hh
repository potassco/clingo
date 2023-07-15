#pragma once

#include <algorithm>

#include <input/literal.hh>

#include <util/print.hh>

#include "transform.hh"
#include "unpool.hh"
#include "variables.hh"

namespace Gringo::Input::CondLits {

template <class T, class B> auto unpool(typename T::ElementVec const &elems) {
    using Conds = std::vector<SLiteralVec>;
    using OConds = std::optional<Conds>;
    using ElemConds = std::vector<OConds>;
    using OElemConds = std::optional<ElemConds>;
    using Element = typename T::Element;
    using ElementVec = typename T::ElementVec;

    // unpool the conditions
    OElemConds elem_conds;
    size_t i = 0;
    for (auto const &elem : elems) {
        auto conds = unpool_crossproduct(elem.second);
        if (conds.has_value()) {
            if (!elem_conds.has_value()) {
                elem_conds = ElemConds(elems.size());
            }
            elem_conds->at(i) = std::move(conds).value();
        }
        ++i;
    }

    // unpool literals
    auto elem_lits = unpool_crossproduct(elems, [](auto const &elem) {
        return map_opt_vec(unpool_crossproduct(elem.first), [](auto lits) { return Element{std::move(lits), {}}; });
    });

    // copy literals if conditions have been unpooled
    if (elem_conds.has_value() && !elem_lits.has_value()) {
        elem_lits = Util::make_vec<ElementVec>(ElementVec{});
        elem_lits->back().reserve(elems.size());
        for (auto const &elem : elems) {
            elem_lits->back().emplace_back(Element{elem.first, {}});
        }
    }

    // set conditions of unpooled literals and build disjunctions
    return map_opt_vec(std::move(elem_lits), [&elem_conds, &elems](ElementVec elem_lits) {
        if (!elem_conds.has_value()) {
            size_t i = 0;
            for (auto &elem : elem_lits) {
                elem.second = elems[i].second;
                ++i;
            }
            return construct_shared<T, B>(elem_lits);
        }
        ElementVec unpooled;
        for (size_t i = 0; i < elem_conds->size(); ++i) {
            if (elem_conds->at(i).has_value()) {
                for (auto &cond : elem_conds->at(i).value()) {
                    unpooled.emplace_back(elem_lits[i].first, cond);
                }
            } else {
                unpooled.emplace_back(elem_lits[i].first, elems[i].second);
            }
        }
        return construct_shared<T, B>(std::move(unpooled));
    });
}

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
auto project(typename T::ElementVec const &elems, P project, bool project_lits, bool in_classical_scope)
    -> std::optional<Util::shared_ptr<B>> {
    using Elem = typename T::ElementVec::value_type;
    auto fun = [&](Elem const &elem) -> std::optional<Elem> {
        auto const &[lits, cond] = elem;
        bool project_cond = in_classical_scope ||
                            std::all_of(lits.begin(), lits.end(), [](auto const &lit) { return !lit->is_atom(); });
        // project conclusion
        std::optional<SLiteralVec> projected_lits = std::nullopt;
        if (project_lits) {
            auto fun = [project](SLiteral const &lit) { return lit->project(project); };
            projected_lits = transform(fun, lits);
        }
        // project premise
        std::optional<SLiteralVec> projected_cond = std::nullopt;
        if (project_cond) {
            // add counts of local variables
            VarCounter counter{project.counts()};
            counter.add(lits);
            counter.add(cond);
            // Note that there can be no global variables with just one
            // occurrence in a condition. However, we can project local
            // variables.
            auto sub_project = Projection{project.mode(), counter};
            auto fun = [sub_project](SLiteral const &lit) { return lit->project(sub_project); };
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
    if (elems.size() != 1) {
        return false;
    }
    auto const &[lits, cond] = elems.front();
    return cond.empty() && lits.size() == 1 && lits.front()->is_atom();
}

auto is_test(auto const &elems) -> bool {
    return std::all_of(elems.begin(), elems.end(), [](auto const &elem) {
        auto const &[lits, cond] = elem;
        return cond.empty() && std::all_of(lits.begin(), lits.end(), [](auto const &lit) { return lit->is_test(); });
    });
}

} // namespace Gringo::Input::CondLits
