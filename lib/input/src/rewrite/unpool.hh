#pragma once

#include <clingo/util/algorithm.hh>
#include <clingo/util/immutable_value.hh>

#include <cstddef>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppClingo::Input {

namespace Detail {

template <class R, class E, class B, class... Ps>
auto unpool_cross(R &res, [[maybe_unused]] E const &expr, B &build, Ps &&...pools) {
    res.emplace_back(build(std::forward<Ps>(pools)...));
}

template <auto tag, auto... tags, class R, class E, class B, class P, class... Es>
auto unpool_cross(R &res, E const &expr, B &build, P const &pool, Es const &...elems) {
    if (pool) {
        for (auto const &elem : *pool) {
            unpool_cross<tags...>(res, expr, build, elems..., elem);
        }
    } else {
        unpool_cross<tags...>(res, expr, build, elems..., expr.template get_value<tag>());
    }
}

template <class R, auto n, class B, class U, class E, class... Es>
auto unpool_apply(B build, U const &unpool, E const &elem, Es const &...elems) -> std::optional<std::vector<R>> {
    if constexpr (n == 0) {
        if ((elem || ... || elems)) {
            std::vector<R> res;
            auto get_size = [](auto const &elem) { return elem ? elem->size() : 1; };
            res.reserve((get_size(elem) * ... * get_size(elems)));
            std::invoke(std::move(build), res, elem, elems...);
            return res;
        }
        return std::nullopt;
    } else {
        return unpool_apply<R, n - 1>(std::forward<B>(build), unpool, elems..., unpool(elem));
    }
}

} // namespace Detail

template <typename Span>
auto unpool_crossproduct(Span const &elems, auto unpool)
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
auto unpool_union(Span const &elems, auto unpool) -> std::optional<std::vector<typename Span::value_type>> {
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

template <class E, class B, class U, class... A>
auto unpool_build(E const &expr, B build, U const &unpool, [[maybe_unused]] A... args) {
    using R = std::invoke_result_t<B, std::decay_t<decltype(expr.template get_value<A::tag>())>...>;
    return Detail::unpool_apply<R, sizeof...(A)>(
        [&](auto &res, auto const &...elems) { return Detail::unpool_cross<A::tag...>(res, expr, build, elems...); },
        unpool, expr.template get_value<A::tag>()...);
}

template <class T, class E, class U, class... A> auto unpool_rewrite(E const &expr, U const &unpool, A... args) {
    return unpool_build(
        expr, [&]<class... V>(V &&...vals) -> T { return expr.update((args = std::forward<V>(vals))...); }, unpool,
        std::move(args)...);
}

} // namespace CppClingo::Input
