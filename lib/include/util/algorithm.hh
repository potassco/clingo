#pragma once

#include <vector>

auto copy_n(auto const &vec, size_t n) {
    std::decay_t<decltype(vec)> ret;
    ret.reserve(vec.size());
    for (auto it = vec.begin(), ie = it + n; it != ie; ++it) {
        ret.emplace_back(*it);
    }
    return ret;
}
