#pragma once

#include <algorithm>
#include <clingo/util/macro.hh>

#include <cassert>
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
    using value_type = T;
    using iterator = T *;
    using const_iterator = T const *;
    using reference = T &;
    using const_reference = T const &;
    using pointer = T *;
    using const_pointer = T const *;

    //! Construct an empty vector.
    small_vector() = default;
    //! Copy construct the vector.
    small_vector(small_vector const &other) : small_vector{} {
        reserve(other.size());
        std::ranges::copy(other, std::back_inserter(*this));
    }
    //! Move construct the vector.
    small_vector(small_vector &&other) noexcept { *this = std::move(other); }

    //! Copy assign the vector.
    auto operator=(small_vector const &other) {
        if (this != &other) {
            clear();
            reserve(other.size());
            std::ranges::copy(other, std::back_inserter(*this));
        }
        return *this;
    }
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
    auto begin() -> iterator { return is_small_() ? small_() : large_(); }

    //! Get a const iterator to the beginning of the vector.
    auto begin() const -> const_iterator { return const_cast<small_vector *>(this)->begin(); }

    //! Get a const iterator to the beginning of the vector.
    auto cbegin() const -> const_iterator { return begin(); }

    //! Get an iterator to the end of the vector.
    auto end() -> iterator {
        if (is_small_()) {
            return begin() + size_small_();
        }
        return end_;
    }

    //! Get a const iterator to the end of the vector.
    auto end() const -> const_iterator { return const_cast<small_vector *>(this)->end(); }

    //! Get a const iterator to the end of the vector.
    auto cend() const -> const_iterator { return end(); }

    //! Get a pointer to the stored C array.
    auto data() -> iterator { return begin(); }

    //! Get a const pointer to the stored C array.
    auto data() const -> const_iterator { return cbegin(); }

    //! Get a const pointer to the stored C array.
    auto cdata() const -> const_iterator { return cbegin(); }

    //! Get the element at the given index.
    auto operator[](size_t i) -> reference { return *(begin() + i); }

    //! Get the element at the given index.
    auto operator[](size_t i) const -> const_reference { return *(begin() + i); }

    //! Reserve space for at least n elements.
    void reserve(size_t n) {
        if (auto m = capacity(); m < n) {
            if (n < 2 * m) {
                n = 2 * m;
            }
            auto l = size();
            auto *data = ::operator new[](sizeof(value_type) * n);
            std::uninitialized_move(begin(), end(), static_cast<pointer>(data));
            destroy_();
            begin_ = reinterpret_cast<uintptr_t>(data);
            end_ = large_() + l;
            cap_ = large_() + n;
        }
    }

    //! Get the first element in the vector.
    auto front() -> reference {
        assert(!empty());
        return *begin();
    }

    //! Get the last element in the vector.
    auto back() -> reference {
        assert(!empty());
        return *(end() - 1);
    }

    //! Erase the given element.
    auto erase(iterator it) -> iterator {
        assert(it != end());
        std::destroy_at(std::move(it + 1, end(), it));
        if (is_small_()) {
            size_small_(size_small_() - 1);
        } else {
            --end_;
        }
        return it;
    }

    //! Erase the given range of elements.
    auto erase(iterator first, iterator last) -> iterator {
        assert(first <= last);
        if (ssize_t n = last - first; n > 0) {
            std::destroy(std::move(last, end(), first), end());
            if (is_small_()) {
                size_small_(size_small_() - n);
            } else {
                end_ -= n;
                last -= n;
            }
        }
        return last;
    }

    //! Emplace an element before the given iterator.
    template <class... U> void emplace(const_iterator loc, U &&...args) {
        // TODO: moves can be avoided when reallocating
        auto i = std::distance(cbegin(), loc);
        reserve(size() + 1);
        auto it = std::next(begin(), i);
        if (auto ie = end(), ip = std::prev(ie); it != ie) {
            std::construct_at(ie, std::move(*ip));
            std::move_backward(it, ip, ie);
        }
        std::construct_at(it, std::forward<U>(args)...);
        if (is_small_()) {
            size_small_(size_small_() + 1);
        } else {
            std::advance(end_, 1);
        }
    }

    //! Emplace an element after the last element.
    template <class... U> void emplace_back(U &&...args) {
        reserve(size() + 1);
        std::construct_at(end(), std::forward<U>(args)...);
        if (is_small_()) {
            size_small_(size_small_() + 1);
        } else {
            ++end_;
        }
    }

    //! Append an element after the last element.
    void push_back(value_type const &x) { return emplace_back(x); }

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
    auto large_() const -> pointer { return reinterpret_cast<T *>(begin_); }
    auto small_() -> pointer { return reinterpret_cast<T *>(buf_); }
    auto is_small_() const -> bool { return (begin_ & 1) != 0; }
    auto size_small_() const -> size_t { return begin_ >> 1; }

    auto size_small_(size_t n) { begin_ = (n << 1) | 1; }

    void destroy_() noexcept {
        std::destroy(begin(), end());
        if (!is_small_()) {
            ::operator delete[](large_());
        }
    }

    GRINGO_IGNORE_UNION_B
    uintptr_t begin_ = 1;
    union {
        struct {
            alignas(value_type) unsigned char buf_[N * sizeof(value_type)];
        };
        struct {
            pointer end_;
            pointer cap_;
        };
    };
    GRINGO_IGNORE_UNION_E
};

// NOLINTEND

//! @}

} // namespace Clingo::Util
