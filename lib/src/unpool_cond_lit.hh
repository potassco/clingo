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
    using SLitVecVec = std::vector<SLiteralVec>;
    using SLitVecOVec = std::optional<SLitVecVec>;
    using SLitVecOVecVec = std::vector<std::optional<std::vector<SLiteralVec>>>;
    using SLitVecOVecOVec = std::optional<std::vector<std::optional<std::vector<SLiteralVec>>>>;

    // unpool the conditions
    SLitVecOVecOVec conds;
    size_t i = 0;
    for (auto &elem : elems) {
        unpool_with(
            [&](std::optional<SLiteralVec> &cond) {
                if (cond.has_value()) {
                    if (!conds.has_value()) {
                        conds = SLitVecOVecVec(elems.size());
                    }
                    if (!conds->at(i).has_value()) {
                        conds->at(i) = SLitVecVec{};
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
                    for (auto &cond : conds->at(i).value()) {
                        unpooled.emplace_back(lit, lits.has_value() ? cond : std::move(cond));
                    }
                } else {
                    unpooled.emplace_back(std::move(lit), elems[i].second);
                }
            }
            pool.template append_shared<T>(std::move(unpooled));
        },
        unpool_crossproduct<PoolLiteral, typename T::Element, MapLiteral<T>>(pool, elems));
}
