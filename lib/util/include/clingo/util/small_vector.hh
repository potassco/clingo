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

//! A vector that misuses the begin, end and capacity pointers to store
//! elements.
//!
//! This optimization makes it well-suited to store a large number of vectors
//! with up to N elements witout allocating. Value N is best chosen to not
//! extend the size of the vector.
template <class T, size_t N = 2>
    requires(std::is_nothrow_destructible_v<T> && std::is_nothrow_move_constructible_v<T>)
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
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
    small_vector(small_vector &&other) noexcept : small_vector{} { *this = std::move(other); }

    //! Copy assign the vector.
    auto operator=(small_vector const &other) -> small_vector & {
        if (this != &other) {
            clear();
            reserve(other.size());
            std::ranges::copy(other, std::back_inserter(*this));
        }
        return *this;
    }
    //! Move assign the vector.
    auto operator=(small_vector &&other) noexcept -> small_vector & {
        if (this != &other) {
            clear();
            if (other.is_small_()) {
                std::uninitialized_move_n(other.begin_small_(), other.size_small_(), begin_small_());
                std::ranges::destroy_n(other.begin_small_(), other.size_small_());
                std::ranges::swap(other.begin_, begin_);
            } else {
                std::ranges::swap(begin_, other.begin_);
                std::ranges::swap(end_large_(), other.end_large_());
                std::ranges::swap(cap_large_(), other.cap_large_());
            }
        }
        return *this;
    }

    //! Get the size of the vector.
    [[nodiscard]] auto size() const -> size_t {
        if (is_small_()) {
            return size_small_();
        }
        return std::ranges::distance(begin_large_(), end_large_());
    }

    //! Check if the vector is empty.
    [[nodiscard]] auto empty() const -> bool { return size() == 0; }

    //! Get the capacity of the vector.
    [[nodiscard]] auto capacity() const -> size_t {
        if (is_small_()) {
            return N;
        }
        return std::ranges::distance(begin_large_(), end_large_());
    }

    //! Get an iterator to the beginning of the vector.
    [[nodiscard]] auto begin() -> iterator { return is_small_() ? begin_small_() : begin_large_(); }

    //! Get a const iterator to the beginning of the vector.
    [[nodiscard]] auto begin() const -> const_iterator {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        return const_cast<small_vector *>(this)->begin();
    }

    //! Get a const iterator to the beginning of the vector.
    [[nodiscard]] auto cbegin() const -> const_iterator { return begin(); }

    //! Get an iterator to the end of the vector.
    [[nodiscard]] auto end() -> iterator {
        if (is_small_()) {
            return std::ranges::next(begin(), size_small_());
        }
        return end_large_();
    }

    //! Get a const iterator to the end of the vector.
    [[nodiscard]] auto end() const -> const_iterator {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        return const_cast<small_vector *>(this)->end();
    }

    //! Get a const iterator to the end of the vector.
    [[nodiscard]] auto cend() const -> const_iterator { return end(); }

    //! Get a pointer to the stored C array.
    [[nodiscard]] auto data() -> iterator { return begin(); }

    //! Get a const pointer to the stored C array.
    [[nodiscard]] auto data() const -> const_iterator { return cbegin(); }

    //! Get a const pointer to the stored C array.
    [[nodiscard]] auto cdata() const -> const_iterator { return cbegin(); }

    //! Get the element at the given index.
    [[nodiscard]] auto operator[](size_t i) -> reference {
        assert(i < size());
        return *std::ranges::next(begin(), i);
    }

    //! Get the element at the given index.
    [[nodiscard]] auto operator[](size_t i) const -> const_reference {
        assert(i < size());
        return *std::ranges::next(begin(), i);
    }

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
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            begin_ = reinterpret_cast<uintptr_t>(data);
            end_large_() = std::ranges::next(begin_large_(), l);
            cap_large_() = std::ranges::next(begin_large_(), n);
        }
    }

    //! Get the first element in the vector.
    auto front() -> reference {
        assert(!empty());
        return *begin();
    }

    //! Get the first element in the vector.
    auto front() const -> const_reference {
        assert(!empty());
        return *begin();
    }

    //! Get the last element in the vector.
    auto back() -> reference {
        assert(!empty());
        return *std::ranges::prev(end());
    }

    //! Get the last element in the vector.
    auto back() const -> const_reference {
        assert(!empty());
        return *std::ranges::prev(end());
    }

    //! Erase the given element.
    auto erase(iterator it) -> iterator {
        assert(it != end());
        std::ranges::destroy_at(std::move(std::ranges::next(it), end(), it));
        if (is_small_()) {
            size_small_(size_small_() - 1);
        } else {
            std::ranges::advance(end_large_(), -1);
        }
        return it;
    }

    //! Erase the given range of elements.
    auto erase(iterator first, iterator last) -> iterator {
        assert(first <= last);
        if (auto n = std::ranges::distance(first, last); n > 0) {
            std::ranges::destroy(std::move(last, end(), first), end());
            if (is_small_()) {
                size_small_(size_small_() - n);
            } else {
                std::ranges::advance(end_large_(), -n);
                std::ranges::advance(last, -n);
            }
        }
        return last;
    }

    //! Emplace an element before the given iterator.
    template <class... U> void emplace(const_iterator loc, U &&...args) {
        // TODO: moves can be avoided when reallocating
        auto i = std::ranges::distance(cbegin(), loc);
        reserve(size() + 1);
        auto it = std::next(begin(), i);
        if (auto ie = end(), ip = std::ranges::prev(ie); it != ie) {
            std::ranges::construct_at(ie, std::move(*ip));
            std::ranges::move_backward(it, ip, ie);
        }
        std::ranges::construct_at(it, std::forward<U>(args)...);
        if (is_small_()) {
            size_small_(size_small_() + 1);
        } else {
            std::ranges::advance(end_large_(), 1);
        }
    }

    //! Emplace an element after the last element.
    template <class... U> void emplace_back(U &&...args) {
        reserve(size() + 1);
        std::ranges::construct_at(end(), std::forward<U>(args)...);
        if (is_small_()) {
            size_small_(size_small_() + 1);
        } else {
            std::advance(end_large_(), 1);
        }
    }

    //! Append an element after the last element.
    void push_back(value_type const &x) { return emplace_back(x); }

    //! Pop the last element.
    void pop_back() {
        assert(!empty());
        std::destroy_at(std::prev(end()));
        if (is_small_()) {
            size_small_(size_small_() - 1);
        } else {
            std::advance(end_large_(), -1);
        }
    }

    //! Clear the vector.
    //!
    //! Note that this frees allocated storage.
    void clear() noexcept {
        destroy_();
        begin_ = 1;
    }

    //! Deconstruct the vector.
    ~small_vector() noexcept { destroy_(); }

  private:
    [[nodiscard]] auto begin_large_() const -> pointer {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<T *>(begin_);
    }
    [[nodiscard]] auto end_large_() -> pointer & {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        return end_;
    }
    [[nodiscard]] auto end_large_() const -> pointer {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        return end_;
    }
    [[nodiscard]] auto cap_large_() const -> pointer {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        return cap_;
    }
    [[nodiscard]] auto cap_large_() -> pointer & {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        return cap_;
    }
    [[nodiscard]] auto begin_small_() -> pointer {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-type-union-access)
        return reinterpret_cast<T *>(buf_);
    }
    [[nodiscard]] auto is_small_() const -> bool { return (begin_ & 1) != 0; }
    [[nodiscard]] auto size_small_() const -> size_t { return begin_ >> 1; }
    [[nodiscard]] auto size_small_(size_t n) { begin_ = (n << 1) | 1; }

    void destroy_() noexcept {
        std::destroy(begin(), end());
        if (!is_small_()) {
            ::operator delete[](begin_large_());
        }
    }

    GRINGO_IGNORE_UNION_B
    uintptr_t begin_ = 1;
    union {
        struct {
            // NOLINTNEXTLINE(modernize-avoid-c-arrays)
            alignas(value_type) unsigned char buf_[N * sizeof(value_type)];
        };
        struct {
            pointer end_;
            pointer cap_;
        };
    };
    GRINGO_IGNORE_UNION_E
};

//! @}

} // namespace Clingo::Util
