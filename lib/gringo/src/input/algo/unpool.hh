#pragma once

#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gringo/util/algorithm.hh>
#include <gringo/util/immutable_value.hh>

#define FWD(x) std::forward<decltype(x)>(x)

namespace Gringo::Input {

namespace Detail {

template <size_t i, size_t n>
auto unpool_crossproducts_build(auto &build, auto &vec, auto &orig, auto &unpooled, auto &&...cs) {
    if constexpr (i < n) {
        if (std::get<i>(unpooled).has_value()) {
            for (auto &elem : std::get<i>(unpooled).value()) {
                using elem_type = std::decay_t<decltype(elem)>;
                unpool_crossproducts_build<i + 1, n>(build, vec, orig, unpooled, cs...,
                                                     n == 1 ? std::move(elem) : static_cast<elem_type>(elem));
            }
        } else {
            using orig_type = std::decay_t<decltype(std::get<i>(orig))>;
            unpool_crossproducts_build<i + 1, n>(build, vec, orig, unpooled, cs...,
                                                 static_cast<orig_type>(std::get<i>(orig)));
        }
    }
    if constexpr (i == n) {
        vec.emplace_back(build(std::forward<decltype(cs)>(cs)...));
    }
}

template <class... As, size_t... Is>
auto unpool_crossproducts(auto &build, std::tuple<As const &...> orig, auto unpooled, std::index_sequence<Is...> seq) {
    using return_type = std::vector<std::decay_t<decltype(build(std::declval<As>()...))>>;
    if ((std::get<Is>(unpooled).has_value() || ...)) {
        return_type vec;
        unpool_crossproducts_build<0, seq.size()>(build, vec, orig, unpooled);
        return std::optional<return_type>(std::move(vec));
    }
    return std::optional<return_type>(std::nullopt);
}

} // namespace Detail

struct Unpooler {
    template <class T>
    auto operator()(Util::immutable_value<T> const &elem) const
        -> std::optional<std::vector<Util::immutable_value<T>>> {
        return elem->unpool();
    }
};

template <typename Span>
auto unpool_crossproduct(Span const &elems, auto &&unpool = Unpooler{})
    -> std::optional<std::vector<std::vector<typename Span::value_type>>> {
    // setup values to unpool + offsets
    std::vector<std::tuple<size_t, size_t, size_t>> offsets;
    std::vector<typename Span::value_type> pool;
    bool has_value = false;
    size_t n = 0;
    for (auto const &elem : elems) {
        auto unpooled = unpool(elem);
        if (unpooled.has_value() && !has_value) {
            has_value = true;
            pool.reserve(elems.size());
            offsets.reserve(elems.size());
            for (auto it = elems.begin(), ie = it + n; it != ie; ++it) {
                offsets.emplace_back(pool.size(), pool.size(), pool.size() + 1);
                pool.emplace_back(*it);
            }
        }
        if (has_value) {
            offsets.emplace_back(pool.size(), pool.size(), pool.size());
            if (!unpooled.has_value()) {
                pool.emplace_back(elem);
            } else {
                std::move(unpooled->begin(), unpooled->end(), std::back_inserter(pool));
            }
            std::get<2>(offsets.back()) = pool.size();
        }
        ++n;
    }
    if (!has_value) {
        return std::nullopt;
    }
    // unpool if at least one element changed
    std::vector<std::vector<typename Span::value_type>> ret;
    for (bool cont = true; cont;) {
        std::vector<typename Span::value_type> res;
        for (auto const &[cur, begin, end] : offsets) {
            if (begin == end) {
                return ret;
            }
            res.emplace_back(pool[cur]);
        }
        ret.emplace_back(std::move(res));
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
    return ret;
}

template <typename Span>
auto unpool_union(Span const &elems, auto &&unpool = Unpooler{})
    -> std::optional<std::vector<typename Span::value_type>> {
    size_t n = 0;
    std::optional<std::vector<typename Span::value_type>> ret;
    for (auto const &elem : elems) {
        auto unpooled = unpool(elem);
        if (unpooled.has_value() && !ret.has_value()) {
            ret = Util::copy_n(elems, n);
        }
        if (ret.has_value()) {
            if (!unpooled.has_value()) {
                ret->emplace_back(elem);
            } else {
                for (auto &&unpooled_elem : unpooled.value()) {
                    ret->emplace_back(std::move(unpooled_elem));
                }
            }
        }
        ++n;
    }
    return ret;
}

auto unpool_crossproducts(auto build, auto unpool, auto const &...args) {
    return Detail::unpool_crossproducts(build, std::forward_as_tuple(args...), std::forward_as_tuple(unpool(args)...),
                                        std::index_sequence_for<decltype(args)...>());
}

template <class R, class T, class... Args> constexpr auto builder(T const &x, Args const &...args) {
    return [&]<class... V>(V &&...vals) -> R { return x.update((args = std::forward<V>(vals))...); };
}

} // namespace Gringo::Input
