#pragma once

#include <optional>
#include <vector>

namespace detail {

using OffsetVec = std::vector<std::tuple<size_t, size_t, size_t>>;

template <typename T, typename Inserter>
void crossproduct_with(OffsetVec &offsets, std::vector<T> &pool, Inserter ins) {
    for (bool cont = true; cont;) {
        std::vector<T> res;
        for (auto &[cur, begin, end] : offsets) {
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
void unpool_vec_with(std::optional<T> unchanged, std::vector<T> &vec, std::vector<T> &pool, Inserter &&ins) {
    size_t m = pool.size();
    OffsetVec offsets;
    offsets.reserve(vec.size());
    for (const auto &val : vec) {
        offsets.emplace_back(pool.size(), pool.size(), pool.size());
        auto &[cur, begin, end] = offsets.back();
        val->unpool(pool);
        end = pool.size();
        if (end - begin != 1 || pool.back() != val) {
            unchanged = std::nullopt;
        }
    }
    if (unchanged) {
        pool.erase(pool.begin() + m, pool.end());
        pool.emplace_back(unchanged.value());
    } else {
        size_t n = pool.size();
        crossproduct_with(offsets, pool, std::forward<Inserter>(ins));
        pool.erase(pool.begin() + m, pool.begin() + n);
    }
}

} // namespace detail
