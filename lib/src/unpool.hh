#pragma once

#include <cstddef>
#include <tuple>
#include <vector>

namespace detail {

using OffsetVec = std::vector<std::tuple<size_t, size_t, size_t>>;

template <typename T, typename Inserter>
void crossproduct_with(OffsetVec &offsets, std::vector<T> &pool, Inserter ins) {
    for (bool cont = true; cont;) {
        std::vector<T> res;
        for (auto const &[cur, begin, end] : offsets) {
            if (begin == end) {
                return;
            }
            res.emplace_back(pool[cur]);
        }
        ins(std::move(res));
        cont = false;
        for (auto &[cur, begin, end] : offsets) {
            ++cur;
            if (cur == end) {
                cur = begin;
            } else {
                cont = true;
                break;
            }
        }
    }
}

template <typename T, typename Inserter>
void unpool_vec_with(std::vector<T> const &vec, std::vector<T> &pool, Inserter ins) {
    bool unchanged = true;
    size_t m = pool.size();
    OffsetVec offsets;
    offsets.reserve(vec.size());
    for (const auto &val : vec) {
        offsets.emplace_back(pool.size(), pool.size(), pool.size());
        auto &[cur, begin, end] = offsets.back();
        val->unpool(pool);
        end = pool.size();
        unchanged = unchanged && end - begin == 1 && pool.back() == val;
    }
    size_t n = pool.size();
    if (unchanged) {
        std::vector<T> res;
        res.reserve(n - m);
        std::move(pool.begin() + m, pool.end(), std::back_inserter(res));
        pool.erase(pool.begin() + m, pool.end());
        ins(std::move(res), unchanged);
    } else {
        crossproduct_with(offsets, pool, [&](std::vector<T> res) { ins(std::move(res), unchanged); });
        pool.erase(pool.begin() + m, pool.begin() + n);
    }
}

} // namespace detail
