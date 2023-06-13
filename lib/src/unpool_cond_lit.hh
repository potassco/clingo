#pragma once

#include <literal.hh>

#include "unpool.hh"

namespace {

template <class T> struct MapLiteral {
    static void unpool(PoolLiteral &pool, typename T::Element &elem) { elem.first->unpool(pool); }
    static auto map(typename T::Element const &orig, SLiteral lit) { return std::move(lit); }
    static auto equal(SLiteral &a, typename T::Element &b) -> bool { return a == b.first; }
};

} // namespace

template <class T, class P> void unpool_cond_lits(T *self, P &pool, typename T::ElementVec &elems) {
    using Conds = std::vector<SLiteralVec>;
    using OConds = std::optional<Conds>;
    using ElemConds = std::vector<OConds>;
    using OElemConds = std::optional<ElemConds>;

    // Note because conditions have to stand on their own, we protect variables
    // from becoming global here:
    // - global before unpooling:
    //   - G = getvars(lit) - getvars(cond)
    // - global after unpooling:
    //   - H = getvars(lit) - getvars(cond)
    // - if var in H and var not in G:
    //   - add var=var to condition to make it local again
    // Furthermore, we use that variables can only loose the local status, if a
    // literal in the condition is unpooled.

    auto get_global = [](auto const &lit, auto const &cond) {
        VariableSet vars_lit;
        lit->variables(vars_lit, VariableSelectMode::all);
        VariableSet vars_cond;
        for (auto const &lit : cond) {
            lit->variables(vars_cond, VariableSelectMode::all);
        }
        std::erase_if(vars_lit, [&](auto &var) { return vars_cond.contains(var); });
        return vars_lit;
    };

    // unpool the conditions
    OElemConds conds;
    size_t i = 0;
    for (auto &elem : elems) {
        unpool_with(
            [&](std::optional<SLiteralVec> &cond) {
                if (cond.has_value()) {
                    if (!conds.has_value()) {
                        conds = ElemConds(elems.size());
                    }
                    if (!conds->at(i).has_value()) {
                        conds->at(i) = Conds{};
                    }
                    conds->at(i)->emplace_back(std::move(cond).value());
                }
            },
            unpool_crossproduct<PoolLiteral>(pool, elem.second));
        ++i;
    }

    // unpool literals and combine with conditions
    unpool_with(
        [&](std::optional<SLiteralVec> &lits) {
            if (!lits.has_value() && !conds.has_value()) {
                pool.append(self);
                return;
            }
            typename T::ElementVec unpooled;
            for (size_t i = 0; i < elems.size(); ++i) {
                SLiteral lit = lits.has_value() ? std::move(lits->at(i)) : elems[i].first;
                if (conds.has_value() && conds->at(i).has_value()) {
                    auto global = get_global(elems[i].first, elems[i].second);
                    for (auto &cond : conds->at(i).value()) {
                        auto copy = lits.has_value() ? cond : std::move(cond);
                        // protect global variables
                        std::vector<std::string> vars;
                        for (auto const &var : get_global(lit, copy)) {
                            if (!global.contains(var)) {
                                vars.emplace_back(var);
                            }
                        }
                        std::sort(vars.begin(), vars.end());
                        for (auto const &var : vars) {
                            auto var_term = construct_shared<TermVariable, Term>(var);
                            auto rhs = GuardVec{Guard{Relation::less_equal, var_term}};
                            copy.emplace_back(
                                construct_shared<LiteralRelation, Literal>(std::move(var_term), std::move(rhs)));
                        }
                        unpooled.emplace_back(lit, std::move(copy));
                    }
                } else {
                    unpooled.emplace_back(std::move(lit), elems[i].second);
                }
            }
            pool.template append_shared<T>(std::move(unpooled));
        },
        unpool_crossproduct<PoolLiteral, typename T::Element, MapLiteral<T>>(pool, elems));
}
