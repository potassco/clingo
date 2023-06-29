#pragma once

#include <algorithm>

#include <literal.hh>

#include "unpool.hh"

namespace {

template <class T> struct MapLiteral {
    static void unpool(PoolLiteral &pool, SLiteral &lit) { lit->unpool(pool); }
    static auto vec(typename T::Element &elem) { return elem.first; }
    static auto map(SLiteral &orig, SLiteral lit) { return std::move(lit); }
    static auto equal(SLiteral &a, SLiteral &b) -> bool { return a == b; }
};

template <class T, class P>
void unpool_cond_lits(T *self, P &pool, VariableVec const &global, typename T::ElementVec &elems) {
    using Conds = std::vector<SLiteralVec>;
    using OConds = std::optional<Conds>;
    using ElemConds = std::vector<OConds>;
    using OElemConds = std::optional<ElemConds>;

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
        [&](std::optional<std::vector<SLiteralVec>> &elem_lits) {
            if (!elem_lits.has_value() && !conds.has_value()) {
                pool.append(self);
                return;
            }
            typename T::ElementVec unpooled;
            for (size_t i = 0; i < elems.size(); ++i) {
                auto lits = elem_lits.has_value() ? std::move(elem_lits->at(i)) : elems[i].first;
                if (conds.has_value() && conds->at(i).has_value()) {
                    for (auto &cond : conds->at(i).value()) {
                        unpooled.emplace_back(lits, elem_lits.has_value() ? cond : std::move(cond));
                    }
                } else {
                    unpooled.emplace_back(std::move(lits), elems[i].second);
                }
            }
            auto var_set = VariableSet{};
            for (auto const &elem : unpooled) {
                for (auto const &lit : elem.first) {
                    select_variables(*lit, var_set, VariableSelectMode::add);
                }
                for (auto const &lit : elem.second) {
                    select_variables(*lit, var_set, VariableSelectMode::add);
                }
            }
            auto var_vec = std::vector<std::string>{};
            std::copy_if(global.begin(), global.end(), std::back_inserter(var_vec),
                         [&](auto const &var) { return var_set.contains(var); });
            pool.template append_shared<T>(std::move(var_vec), std::move(unpooled));
        },
        unpool_union_crossproduct<PoolLiteral, typename T::Element, MapLiteral<T>>(pool, elems));
}

auto is_simple_cond_lits(auto const &elems, auto const &global) -> bool {
    auto lit_vars = VariableSet{};
    auto cond_vars = VariableSet{};
    return std::all_of(elems.begin(), elems.end(), [&](auto const &elem) {
        if (elem.first.size() != 1) {
            return false;
        }
        lit_vars.clear();
        cond_vars.clear();
        select_variables(*elem.first.front(), lit_vars, VariableSelectMode::add);
        for (auto const &lit : elems.front().second) {
            select_variables(*lit, cond_vars, VariableSelectMode::add);
        }
        if (std::any_of(global.begin(), global.end(), [&](auto const &var) { return cond_vars.contains(var); })) {
            return false;
        }
        std::erase_if(lit_vars, [&](auto const &var) { return cond_vars.contains(var); });
        for (auto const &var : global) {
            lit_vars.erase(var);
        }
        return lit_vars.empty();
    });
}

void print_cond_lits(auto const &elems, auto const &global, std::ostream &out, char const *kw, bool simple_empty) {
    if (elems.empty() ? simple_empty : is_simple_cond_lits(elems, global)) {
        out << p_range_with(elems, "; ", [](std::ostream &out, auto const &elem) {
            auto cs = elem.second.empty() ? "" : ": ";
            out << *elem.first.front() << cs << p_range(elem.second, ", ");
        });
    } else {
        char const *lp = global.empty() ? "" : "(";
        char const *rp = global.empty() ? "" : ")";
        char const *sp = elems.empty() ? "" : " ";
        out << kw << lp << p_range(global) << rp << " { "
            << p_range_with(elems, "; ",
                            [&](std::ostream &out, auto const &elem) {
                                char const *cs = !elem.second.empty() ? ": " : elem.first.empty() ? ":" : "";
                                out << p_range(elem.first, ", ") << cs << p_range(elem.second, ", ");
                            })
            << sp << "}";
    }
}

void cond_visit_variables(auto const &elems, auto const &global, std::function<void(std::string const &var)> fun,
                          VariableContext ctx) {
    auto visit = [&](auto const &expr) {
        switch (ctx) {
            case VariableContext::global: {
                expr.visit_variables([&global, fun](std::string const &var) {
                    if (std::binary_search(global.begin(), global.end(), var)) {
                        fun(var);
                    }
                });
            }
            case VariableContext::all: {
                expr.visit_variables(fun);
            }
        }
    };
    for (auto const &elem : elems) {
        for (auto const &lit : elem.first) {
            visit(*lit);
        }
        for (auto const &lit : elem.second) {
            visit(*lit);
        }
    }
}

} // namespace
