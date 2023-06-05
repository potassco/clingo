#pragma once

#include <cstddef>
#include <iterator>
#include <tuple>
#include <vector>

#define FWD(x) std::forward<decltype(x)>(x)

namespace detail {

using OffsetVec = std::vector<std::tuple<size_t, size_t, size_t>>;

template <typename P, typename Inserter> void crossproduct_with(OffsetVec &offsets, P &pool, Inserter ins) {
    for (bool cont = true; cont;) {
        std::vector<typename P::element_type> res;
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

struct stop_tag {};

template <class F, class... Args> void combine_r(F ins, bool unchanged, stop_tag t, Args &&...args) {
    static_cast<void>(t);
    ins(std::forward<Args>(args)..., unchanged);
}
template <class F, class Pool, class... Pools> void combine_r(F ins, bool unchanged, Pool &&pool, Pools &&...pools) {
    pool.combine([&](auto &&elem, bool elem_unchanged) {
        combine_r(ins, unchanged && elem_unchanged, std::forward<Pools>(pools)..., std::forward<decltype(elem)>(elem));
    });
}

void unpool_r() {}

template <class Pool, class... Pools> void unpool_r(Pool &&pool, Pools &&...pools) {
    pool.unpool();
    unpool_r(std::forward<Pools>(pools)...);
}

/// Unpool a single element.
template <class P, class T> class UnpoolElement {
  public:
    UnpoolElement(P &pool, T &elem) : pool_{pool}, elem_{elem} {}

    void unpool() {
        begin_ = pool_.size();
        elem_->unpool(pool_);
        end_ = pool_.size();
        unchanged_ = end_ - begin_ == 1 && pool_.back() == elem_;
        if (unchanged_) {
            pool_.pop();
        }
    }

    template <class F> void combine(F cont) {
        if (unchanged_) {
            cont(elem_, true);
        } else {
            for (size_t i = begin_; i < end_; ++i) {
                cont(pool_[i], false);
            }
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

/// Unpool a vector computing the cross product of its elements.
template <class P, class T> class UnpoolCrossproduct {
  public:
    UnpoolCrossproduct(P &pool, std::vector<T> &vec) : pool_{pool}, vec_{vec} {}

    void unpool() {
        unchanged_ = true;
        offsets_.reserve(vec_.size());
        for (const auto &val : vec_) {
            offsets_.emplace_back(pool_.size(), pool_.size(), pool_.size());
            auto &[cur, begin, end] = offsets_.back();
            val->unpool(pool_);
            end = pool_.size();
            unchanged_ = unchanged_ && end - begin == 1 && pool_.back() == val;
        }
        if (unchanged_) {
            pool_.resize(std::get<1>(offsets_.front()));
        }
    }

    template <class F> void combine(F cont) {
        if (unchanged_) {
            cont(static_cast<std::vector<T> const &>(vec_), true);
        } else {
            crossproduct_with(offsets_, pool_, [&](std::vector<T> res) { cont(std::move(res), unchanged_); });
            if (!offsets_.empty()) {
                pool_.erase(std::get<1>(offsets_.front()), std::get<2>(offsets_.back()));
            }
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
            cont(vec_.back(), true);
        } else {
            for (size_t i = begin_; i < end_; ++i) {
                cont(pool_[i], false);
            }
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

} // namespace detail

template <class T> class Pool {
  public:
    using element_type = T;

    Pool(std::vector<T> &pool) : pool_{pool} {}

    [[nodiscard]] auto element(T &elem) -> detail::UnpoolElement<Pool, T> { return {*this, elem}; }
    [[nodiscard]] auto crossproduct(std::vector<T> &vec) -> detail::UnpoolCrossproduct<Pool, T> { return {*this, vec}; }
    [[nodiscard]] auto union_(std::vector<T> &vec) -> detail::UnpoolUnion<Pool, T> { return {*this, vec}; };

    template <typename E> void append(E &&elem) { pool_.emplace_back(std::forward<E>(elem)); };

    template <typename E, typename... Args> void append_shared(Args &&...args) {
        pool_.emplace_back(construct_shared<E, typename T::element_type>(std::forward<Args>(args)...));
    }

    [[nodiscard]] auto size() const -> size_t { return pool_.size(); }
    [[nodiscard]] auto back() -> T & { return pool_.back(); }
    [[nodiscard]] auto operator[](size_t pos) -> T & { return pool_[pos]; }
    void resize(size_t n) { pool_.resize(n); }
    void pop() { pool_.pop_back(); }
    void erase(size_t begin, size_t end) { pool_.erase(pool_.begin() + begin, pool_.begin() + end); }

  private:
    std::vector<T> &pool_;
};

template <typename F, typename... Pools> void unpool_with(F ins, Pools &&...pools) {
    unpool_r(pools...);
    detail::combine_r(ins, true, pools..., detail::stop_tag{});
}
