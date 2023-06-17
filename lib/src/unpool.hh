#pragma once

#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <util/shared_ptr.hh>

#define FWD(x) std::forward<decltype(x)>(x)

namespace detail {

using OffsetVec = std::vector<std::tuple<size_t, size_t, size_t>>;

template <typename P, typename Mapper, typename Inserter>
void crossproduct_with(OffsetVec &offsets, P &pool, Mapper map, Inserter ins) {
    for (bool cont = true; cont;) {
        std::vector<std::decay_t<decltype(map(0, pool[0]))>> res;
        size_t i = 0;
        for (auto const &[cur, begin, end] : offsets) {
            if (begin == end) {
                return;
            }
            res.emplace_back(map(i, pool[cur]));
            ++i;
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

struct stop_tag {};

template <class F, class... Pools> void combine_r(F ins, stop_tag &t, Pools &...pools) {
    static_cast<void>(t);
    ins(pools...);
}

template <class F, class Pool, class... Pools> void combine_r(F ins, Pool &pool, Pools &...pools) {
    pool.combine([&](auto &elem) { combine_r(ins, pools..., elem); });
}

inline void unpool_r() {}

template <class Pool, class... Pools> void unpool_r(Pool &pool, Pools &...pools) {
    pool.unpool();
    unpool_r(pools...);
}

inline void erase_r() {}

template <class Pool, class... Pools> void erase_r(Pool &pool, Pools &...pools) {
    erase_r(pools...);
    pool.erase();
}

struct Unpooler {
    template <class T> static auto is_empty_value(T &elem) { return false; }
    template <class T> static auto is_empty_value(std::optional<T> &elem) { return !elem.has_value(); }
    template <class P, class T> static void unpool(P &pool, T &elem) { elem.unpool(pool); }
    template <class P, class T> static void unpool(P &pool, shared_ptr<T> &elem) { unpool(pool, *elem); }
    template <class P, class T> static void unpool(P &pool, std::optional<T> &elem) {
        if (elem.has_value()) {
            unpool(pool, elem.value());
        }
    }
    template <class T, class U> static auto equal(T &a, U &b) -> bool { return a == b; }
};

/// Unpool a single element.
template <class P, class T, class U> class UnpoolElement : private U {
  public:
    UnpoolElement(P &pool, T &elem) : pool_{pool}, elem_{elem} {}

    void unpool() {
        begin_ = pool_.size();
        static_cast<U *>(this)->unpool(pool_, elem_);
        end_ = pool_.size();
        unchanged_ = end_ - begin_ == 1 && static_cast<U *>(this)->equal(pool_.back(), elem_);
        if (unchanged_) {
            pool_.pop();
        }
        if (begin_ == end_ && static_cast<U *>(this)->is_empty_value(elem_)) {
            unchanged_ = true;
        }
    }

    template <class F> void combine(F cont) {
        if (unchanged_) {
            std::optional<std::decay_t<decltype(pool_[0])>> opt = std::nullopt;
            cont(opt);
        } else {
            for (size_t i = begin_; i < end_; ++i) {
                auto opt = std::make_optional(pool_[i]);
                cont(opt);
            }
        }
    }

    void erase() {
        if (!unchanged_) {
            pool_.erase(begin_, end_);
        }
    }

  private:
    P &pool_;
    T &elem_;
    size_t begin_;
    size_t end_;
    bool unchanged_;
};

struct Mapper {
    template <class T, class P> static void unpool(P &pool, T &elem) { elem->unpool(pool); }
    template <class T, class U> static auto vec(U &val) { return val; }
    template <class T, class U> static auto map(T const &orig, U &val) { return val; }
    template <class T, class U> static auto equal(T &a, U &b) -> bool { return a == b; }
};

/// Unpool a vector computing the cross product of its elements.
template <class P, class T, class M> class UnpoolCrossproduct : private M {
  public:
    UnpoolCrossproduct(P &pool, std::vector<T> &vec) : pool_{pool}, vec_{vec} {}

    void unpool() {
        unchanged_ = true;
        offsets_.reserve(vec_.size());
        for (auto &val : vec_) {
            offsets_.emplace_back(pool_.size(), pool_.size(), pool_.size());
            auto &[cur, begin, end] = offsets_.back();
            static_cast<M *>(this)->unpool(pool_, val);
            end = pool_.size();
            unchanged_ = unchanged_ && end - begin == 1 && static_cast<M *>(this)->equal(pool_.back(), val);
        }
        if (unchanged_ && !offsets_.empty()) {
            pool_.resize(std::get<1>(offsets_.front()));
        }
    }

    template <class F> void combine(F cont) {
        if (unchanged_) {
            std::optional<std::vector<std::decay_t<decltype(static_cast<M *>(this)->map(vec_[0], pool_[0]))>>> opt =
                std::nullopt;
            cont(opt);
        } else {
            crossproduct_with(
                offsets_, pool_, [&](size_t i, auto &val) { return static_cast<M *>(this)->map(vec_[i], val); },
                [&](auto &&res) {
                    auto opt = std::make_optional(FWD(res));
                    cont(opt);
                });
        }
    }

    void erase() {
        if (!unchanged_ && !offsets_.empty()) {
            pool_.erase(std::get<1>(offsets_.front()), std::get<2>(offsets_.back()));
        }
    }

  private:
    P &pool_;
    std::vector<T> &vec_;
    OffsetVec offsets_;
    bool unchanged_;
};

/// Unpool a vector computing the union of its elements.
template <class P, class T> class UnpoolUnion {
  public:
    UnpoolUnion(P &pool, std::vector<T> &vec) : pool_{pool}, vec_{vec} {}

    void unpool() {
        begin_ = pool_.size();
        for (const auto &val : vec_) {
            val->unpool(pool_);
        }
        end_ = pool_.size();
        unchanged_ = vec_.size() == 1 && end_ - begin_ == 1 && pool_.back() == vec_.back();
        if (unchanged_) {
            pool_.resize(begin_);
        }
    }

    template <class F> void combine(F cont) {
        if (unchanged_) {
            std::optional<std::decay_t<decltype(pool_[0])>> opt = std::nullopt;
            cont(opt);
        } else {
            for (size_t i = begin_; i < end_; ++i) {
                auto opt = std::make_optional(pool_[i]);
                cont(opt);
            }
        }
    }

    void erase() {
        if (!unchanged_) {
            pool_.erase(begin_, end_);
        }
    }

  private:
    P &pool_;
    std::vector<T> &vec_;
    size_t begin_;
    size_t end_;
    bool unchanged_;
};

/// Unpool a vector of vectors computing the cross product of them.
template <class P, class T, class M> class UnpoolUnionCrossproduct : private M {
  public:
    UnpoolUnionCrossproduct(P &pool, std::vector<T> &vec) : pool_{pool}, vec_{vec} {}

    void unpool() {
        unchanged_ = true;
        for (auto &vec : vec_) {
            size_t i = 0;
            for (auto &val : static_cast<M *>(this)->vec(vec)) {
                union_.emplace_back(i);
                offsets_.emplace_back(pool_.size(), pool_.size(), pool_.size());
                auto &[cur, begin, end] = offsets_.back();
                static_cast<M *>(this)->unpool(pool_, val);
                end = pool_.size();
                unchanged_ = unchanged_ && end - begin == 1 && static_cast<M *>(this)->equal(pool_.back(), val);
                ++i;
            }
        }
        if (unchanged_ && !offsets_.empty()) {
            pool_.resize(std::get<1>(offsets_.front()));
        }
    }

    template <class F> void combine(F cont) {
        using mapped_type =
            std::decay_t<decltype(static_cast<M *>(this)->map(static_cast<M *>(this)->vec(vec_[0])[0], pool_[0]))>;
        using res_type = std::vector<std::vector<mapped_type>>;
        if (unchanged_) {
            std::optional<res_type> opt = std::nullopt;
            cont(opt);
        } else {
            crossproduct_with(
                offsets_, pool_,
                [&](size_t j, auto &val) {
                    return static_cast<M *>(this)->map(static_cast<M *>(this)->vec(vec_[union_[j]])[j], val);
                },
                [&](auto &&res) {
                    std::optional<res_type> opt = res_type{};
                    opt->resize(union_.size());
                    size_t j = 0;
                    for (auto &val : res) {
                        opt->at(union_[j]).emplace_back(std::move(val));
                        ++j;
                    }
                    cont(opt);
                });
        }
    }

    void erase() {
        if (!unchanged_ && !offsets_.empty()) {
            pool_.erase(std::get<1>(offsets_.front()), std::get<2>(offsets_.back()));
        }
    }

  private:
    P &pool_;
    std::vector<size_t> union_;
    OffsetVec offsets_;
    std::vector<T> &vec_;
    bool unchanged_;
};

} // namespace detail

/// Helper to unpool expressions and vectors of expressions.
///
/// This helper is to be used together with the unpool_with function.
template <class T> class Pool {
  public:
    using element_type = T;

    /// Construct a pool with a reference to a vector for storing unpooled expressions.
    Pool(std::vector<T> &pool) : pool_{pool} {}

    /// @{
    /// Vector-like interface to pool elements.
    template <typename... Args> void append(Args &&...args) { pool_.emplace_back(std::forward<Args>(args)...); };
    template <typename E, typename... Args> void append_shared(Args &&...args) {
        pool_.emplace_back(construct_shared<E, typename T::element_type>(std::forward<Args>(args)...));
    }
    [[nodiscard]] auto size() const -> size_t { return pool_.size(); }
    [[nodiscard]] auto back() -> T & { return pool_.back(); }
    [[nodiscard]] auto operator[](size_t pos) -> T & { return pool_[pos]; }
    void resize(size_t n) { pool_.resize(n); }
    void pop() { pool_.pop_back(); }
    void erase(size_t begin, size_t end) { pool_.erase(pool_.begin() + begin, pool_.begin() + end); }
    /// @}

  private:
    std::vector<T> &pool_;
};

/// Unpool an element.
template <class P, class T, class U = detail::Unpooler>
[[nodiscard]] auto unpool_element(P &pool, T &elem) -> detail::UnpoolElement<P, T, U> {
    return {pool, elem};
}

/// Unpool a vector computing it's crossproduct.
///
/// It is also possible to unpool a vector of different expressions as long
/// as they can be mapped back and forth.
template <class P, class U = P::element_type, class M = detail::Mapper>
[[nodiscard]] auto unpool_crossproduct(P &pool, std::vector<U> &vec) -> detail::UnpoolCrossproduct<P, U, M> {
    return {pool, vec};
}

/// Unpool a vector of vectors computing it's crossproduct.
///
/// It is also possible to unpool a vector of different expressions as long
/// as they can be mapped back and forth.
template <class P, class T = P::element_type, class M = detail::Mapper>
[[nodiscard]] auto unpool_union_crossproduct(P &pool, std::vector<T> &vec) -> detail::UnpoolUnionCrossproduct<P, T, M> {
    return {pool, vec};
}

/// Unpool a vector computing it's union.
template <class T, class P> [[nodiscard]] auto unpool_union(P &pool, std::vector<T> &vec) -> detail::UnpoolUnion<P, T> {
    return {pool, vec};
};

/// Helper to unpool sets of different expressions.
template <class T, class C> class PoolParent : public Pool<T> {
  public:
    PoolParent(std::vector<T> &vec, auto &...vecs) : Pool<T>{vec}, child_{std::forward<decltype(vecs)>(vecs)...} {}

    template <class U> operator U &() { return static_cast<U &>(child_); }

  private:
    C child_;
};

/// Unpool an expression composed of other expressions.
template <typename F, typename... Pools> void unpool_with(F ins, Pools &&...pools) {
    // Note: pools are deliberately passed by reference here!
    detail::unpool_r(pools...);
    detail::stop_tag stop;
    detail::combine_r(ins, pools..., stop);
    detail::erase_r(pools...);
}
