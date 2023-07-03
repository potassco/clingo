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

auto map_opt(auto &&opt, auto &&map) -> decltype(std::make_optional(map(std::forward<decltype(opt)>(opt).value()))) {
    if (opt.has_value()) {
        return std::make_optional(map(std::forward<decltype(opt)>(opt).value()));
    }
    return std::nullopt;
}

auto map_opt_vec(auto &&vec, auto &&map) {
    using return_type = std::vector<std::decay_t<decltype(map(std::move(vec.value().front())))>>;
    return map_opt(std::move(vec), [&map](auto &&vec) {
        return_type ret;
        ret.reserve(vec.size());
        for (auto &&elem : vec) {
            // Note we assume that the vector own the values and move them out
            // if the input is not an lvalue reference.
            if constexpr (std::is_lvalue_reference_v<decltype(vec)>) {
                ret.emplace_back(map(elem));
            } else {
                ret.emplace_back(map(std::move(elem)));
            }
        }
        return ret;
    });
}
