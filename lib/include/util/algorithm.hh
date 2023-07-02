#pragma once

#include <optional>
#include <vector>

auto copy_n(auto const &vec, size_t n) {
    std::decay_t<decltype(vec)> ret;
    ret.reserve(vec.size());
    for (auto it = vec.begin(), ie = it + n; it != ie; ++it) {
        ret.emplace_back(*it);
    }
    return ret;
}

template <class T, class F>
auto map_opt(std::optional<T> &&opt, F &&f) -> std::optional<std::decay_t<decltype(f(opt.value()))>> {
    if (opt.has_value()) {
        return std::make_optional(f(std::move(opt).value()));
    }
    return std::nullopt;
}
