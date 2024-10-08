#pragma once

#include <algorithm>
#include <clingo/util/macro.hh>

#include <cstdint>
#include <memory>
#include <utility>

namespace Clingo::Util {

//! @addtogroup util_container
//! @{

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
    //! Construct an empty vector.
    small_vector() = default;
    //! For the time being, copy construction is disabled.
    small_vector(small_vector const &) = delete;
    //! Move construct the vector.
    small_vector(small_vector &&other) noexcept { *this = std::move(other); }

    //! For the time being, copy assignment is disabled.
    auto operator=(small_vector const &other) = delete;
    //! Move assign the vector.
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

    //! Get the size of the vector.
    auto size() const -> size_t {
        if (is_small_()) {
            return size_small_();
        }
        return end_ - large_();
    }

    //! Check if the vector is empty.
    auto empty() const -> bool { return size() == 0; }

    //! Get the capacity of the vector.
    auto capacity() const -> size_t {
        if (is_small_()) {
            return N;
        }
        return cap_ - large_();
    }

    //! Get an iterator to the beginning of the vector.
    auto begin() const -> T const * { return const_cast<small_vector *>(this)->begin(); }

    //! Get an iterator to the beginning of the vector.
    auto begin() -> T * { return is_small_() ? small_() : large_(); }

    //! Get a pointer to the stored C array.
    auto data() -> T * { return begin(); }

    //! Get a pointer to the stored C array.
    auto data() const -> T const * { return begin(); }

    //! Get an iterator to the end of the vector.
    auto end() const -> T const * { return const_cast<small_vector *>(this)->end(); }

    //! Get an iterator to the end of the vector.
    auto end() -> T * {
        if (is_small_()) {
            return begin() + size_small_();
        }
        return end_;
    }

    //! Get the element at the given index.
    auto operator[](size_t i) -> T & { return *(begin() + i); }

    //! Get the element at the given index.
    auto operator[](size_t i) const -> T const & { return *(begin() + i); }

    //! Reserve space for at least n elements.
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

    //! Get the first element in the vector.
    auto front() -> T & {
        assert(!empty());
        return *begin();
    }

    //! Get the last element in the vector.
    auto back() -> T & {
        assert(!empty());
        return *(end() - 1);
    }

    //! Erase the given range of elements.
    auto erase(T *first, T *last) -> T * {
        if (ssize_t n = last - first; n > 0) {
            std::destroy(first, last);
            std::move(last, this->end(), first);
            if (is_small_()) {
                size_small_(size_small_() - n);
            } else {
                last -= n;
            }
        }
        return last;
    }

    //! Emplace an element before the given iterator.
    template <class... U> void emplace(T *it, U &&...args) {
        reserve(size() + 1);
        auto ie = end();
        std::move_backward(it, ie, ie + 1);
        new (it) T{std::forward<U>(args)...};
        if (is_small_()) {
            size_small_(size_small_() + 1);
        } else {
            ++end_;
        }
    }

    //! Emplace an element after the last element.
    template <class... U> void emplace_back(U &&...args) {
        reserve(size() + 1);
        new (end()) T{std::forward<U>(args)...};
        if (is_small_()) {
            size_small_(size_small_() + 1);
        } else {
            ++end_;
        }
    }

    //! Pop the last element.
    void pop_back() {
        if (is_small_()) {
            size_small_(size_small_() - 1);
        } else {
            --end_;
        }
    }

    //! Clear the vector.
    //!
    //! Note that this frees allocated storage.
    void clear() {
        destroy_();
        begin_ = 1;
    }

    //! Deconstruct the vector.
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

//! @}

} // namespace Clingo::Util
