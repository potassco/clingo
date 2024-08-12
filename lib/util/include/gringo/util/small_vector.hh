#pragma once

#include <gringo/util/macro.hh>

#include <cstdint>
#include <memory>
#include <utility>

namespace Gringo::Util {

// NOLINTBEGIN

//! A vector that misuses the begin, end and capacity pointers to store
//! elements.
//!
//! This optimization makes it well-suited to store a large number of vectors
//! with up to N elements witout allocating. Value N is best chosen to not
//! extend the size of the vector.
template <class T, size_t N = 2>
    requires(std::is_nothrow_destructible_v<T> && std::is_nothrow_move_constructible_v<T>)
class small_vector {
  public:
    small_vector() = default;
    small_vector(small_vector &&other) noexcept { *this = std::move(other); }
    auto operator=(small_vector &&other) noexcept -> small_vector & {
        clear();
        if (other.is_small_()) {
            std::uninitialized_move_n(other.small_(), other.size_small_(), small_());
            std::destroy_n(other.small_(), other.size_small_());
            std::swap(other.begin_, begin_);
        } else {
            std::swap(begin_, other.begin_);
            std::swap(end_, other.end_);
            std::swap(cap_, other.cap_);
        }
        return *this;
    }

    small_vector(small_vector const &) = delete;
    auto operator=(small_vector const &other) = delete;

    auto size() const -> size_t {
        if (is_small_()) {
            return size_small_();
        }
        return end_ - large_();
    }

    auto empty() const -> bool { return size() == 0; }

    auto capacity() const -> size_t {
        if (is_small_()) {
            return N;
        }
        return cap_ - large_();
    }

    auto begin() const -> T const * { return const_cast<small_vector *>(this)->begin(); }

    auto begin() -> T * { return is_small_() ? small_() : large_(); }

    auto end() const -> T const * { return const_cast<small_vector *>(this)->end(); }

    auto end() -> T * {
        if (is_small_()) {
            return begin() + size_small_();
        }
        return end_;
    }

    auto operator[](size_t i) -> T & { return *(begin() + i); }

    auto operator[](size_t i) const -> T const & { return *(begin() + i); }

    void reserve(size_t n) {
        if (auto m = capacity(); m < n) {
            if (n < 2 * m) {
                n = 2 * m;
            }
            auto l = size();
            auto *data = ::operator new[](sizeof(T) * n);
            std::uninitialized_move(begin(), end(), static_cast<T *>(data));
            destroy_();
            begin_ = reinterpret_cast<uintptr_t>(data);
            end_ = large_() + l;
            cap_ = large_() + n;
        }
    }

    template <class... U> void emplace_back(U &&...args) {
        reserve(size() + 1);
        new (end()) T{std::forward<U>(args)...};
        if (is_small_()) {
            size_small_(size_small_() + 1);
        } else {
            ++end_;
        }
    }

    void pop_back() {
        if (is_small_()) {
            size_small_(size_small_() - 1);
        } else {
            --end_;
        }
    }

    void clear() {
        destroy_();
        begin_ = 1;
    }

    ~small_vector() { destroy_(); }

  private:
    auto large_() const -> T * { return reinterpret_cast<T *>(begin_); }
    auto small_() -> T * { return reinterpret_cast<T *>(buf_); }
    auto is_small_() const -> bool { return (begin_ & 1) != 0; }
    auto size_small_() const -> size_t { return begin_ >> 1; }

    auto size_small_(size_t n) { begin_ = (n << 1) | 1; }

    void destroy_() {
        std::destroy(begin(), end());
        if (!is_small_()) {
            ::operator delete[](large_());
        }
    }

    GRINGO_IGNORE_UNION_B
    uintptr_t begin_ = 1;
    union {
        struct {
            alignas(T) unsigned char buf_[N * sizeof(T)];
        };
        struct {
            T *end_;
            T *cap_;
        };
    };
    GRINGO_IGNORE_UNION_E
};

// NOLINTEND

} // namespace Gringo::Util
