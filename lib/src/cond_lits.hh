#pragma once

#include <algorithm>

#include <literal.hh>

#include "transform.hh"
#include "unpool.hh"
#include "variables.hh"

namespace CondLits {

template <class T> struct MapLiteral {
    static void unpool(PoolLiteral &pool, SLiteral &lit) { lit->unpool(pool); }
    static auto vec(typename T::Element &elem) { return elem.first; }
    static auto map(SLiteral &orig, SLiteral lit) { return std::move(lit); }
    static auto equal(SLiteral &a, SLiteral &b) -> bool { return a == b; }
};

template <class T, class P> void unpool(T *self, P &pool, typename T::ElementVec &elems) {
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
            pool.template append_shared<T>(std::move(unpooled));
        },
        unpool_union_crossproduct<PoolLiteral, typename T::Element, MapLiteral<T>>(pool, elems));
}

auto is_simple(auto const &elems) -> bool {
    return !elems.empty() &&
           std::all_of(elems.begin(), elems.end(), [&](auto const &elem) { return elem.first.size() == 1; });
}

void print(auto const &elems, std::ostream &out, char const *kw, bool simple_empty) {
    if (elems.empty() ? simple_empty : is_simple(elems)) {
        out << p_range_with(elems, "; ", [](std::ostream &out, auto const &elem) {
            auto cs = elem.second.empty() ? "" : ": ";
            out << *elem.first.front() << cs << p_range(elem.second, ", ");
        });
    } else {
        char const *sp = elems.empty() ? "" : " ";
        out << kw << " { "
            << p_range_with(elems, "; ",
                            [&](std::ostream &out, auto const &elem) {
                                char const *cs = !elem.second.empty() ? ": " : elem.first.empty() ? ":" : "";
                                out << p_range(elem.first, ", ") << cs << p_range(elem.second, ", ");
                            })
            << sp << "}";
    }
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
    -> std::optional<shared_ptr<B>> {
    using Elem = typename T::ElementVec::value_type;
    auto fun = [&](Elem const &elem) -> std::optional<Elem> {
        auto const &[lits, cond] = elem;
        bool project_cond = in_classical_scope ||
                            std::all_of(lits.begin(), lits.end(), [](auto const &lit) { return !lit->is_atom(); });
        auto fun = [project](SLiteral const &lit) { return lit->project(project); };
        // project conclusion
        std::optional<SLiteralVec> projected_lits = std::nullopt;
        if (project_lits) {
            projected_lits = transform(fun, lits);
        }
        // project premise
        std::optional<SLiteralVec> projected_cond = std::nullopt;
        if (project_cond) {
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

} // namespace CondLits
