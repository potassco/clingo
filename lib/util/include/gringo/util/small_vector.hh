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
    small_vector() : size_{0} {}
    small_vector(small_vector &&other) noexcept : size_{0} { *this = std::move(other); }
    auto operator=(small_vector &&other) noexcept -> small_vector & {
        clear();
        if (other.size_ <= N) {
            for (auto it = other.begin(), ie = other.end(), jt = begin(); it != ie; ++it, ++jt) {
                new (jt) T(std::move(*it));
            }
            std::swap(other.size_, size_);
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
        if (size_ <= N) {
            return size_;
        }
        return end_ - begin_;
    }

    auto empty() const -> bool { return size() == 0; }

    auto capacity() const -> size_t {
        if (size_ <= N) {
            return N;
        }
        return cap_ - begin_;
    }

    auto begin() const -> T const * { return const_cast<small_vector *>(this)->begin(); }

    auto begin() -> T * {
        if (size_ <= N) {
            return small_;
        }
        return begin_;
    }

    auto end() const -> T const * { return const_cast<small_vector *>(this)->end(); }

    auto end() -> T * {
        if (size_ <= N) {
            return begin() + size_;
        }
        return end_;
    }

    auto operator[](size_t i) -> T & { return *(begin() + i); }

    auto operator[](size_t i) const -> T const & { return *(begin() + i); }

    void reserve(size_t n) {
        if (auto m = capacity(); m < n) {
            // should check overflow
            if (n < 2 * m) {
                n = 2 * m;
            }
            auto l = size();
            auto *data = ::operator new[](sizeof(T) * n);
            for (auto it = begin(), ie = end(), jt = static_cast<T *>(data); it != ie; ++it, ++jt) {
                new (jt) T(std::move(*it));
            }
            destroy_();
            begin_ = static_cast<T *>(data);
            end_ = begin_ + l;
            cap_ = begin_ + n;
        }
    }

    template <class... U> void emplace_back(U &&...args) {
        reserve(size() + 1);
        new (end()) T{std::forward<U>(args)...};
        if (size_ <= N) {
            ++size_;
        } else {
            ++end_;
        }
    }

    void pop_back() {
        if (size_ <= N) {
            --size_;
        } else {
            --end_;
        }
    }

    void clear() {
        destroy_();
        size_ = 0;
    }

    ~small_vector() { destroy_(); }

  private:
    void destroy_() {
        std::destroy(begin(), end());
        if (size_ > N) {
            ::operator delete[](begin_);
        }
    }

    // Note that technically, writing to begin_ and then reading though size_
    // is undefined behavior.
    GRINGO_IGNORE_UNION_B
    union {
        uintptr_t size_ = 0;
        T *begin_;
    };
    union {
        struct {
            T small_[2];
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
