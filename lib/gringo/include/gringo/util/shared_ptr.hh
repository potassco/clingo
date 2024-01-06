#pragma once

#include <cassert>
#include <cstddef>
#include <tuple>
#include <utility>

namespace Gringo::Util {

//! @defgroup core_util Utility
//! Utility data structures and functions.
//!
//! @ingroup core

//! @defgroup core_shared_ptr Shared Pointers
//! A shared and single owner pointer implementation.
//!
//! @ingroup core_util
//!
//! @{

//! Alternative shared_ptr implementation.
//!
//! This implementation aims to ease C integration
//! making it is easy to extend to provide full control over the reference count.
//! It should also be faster than the STL implementation.
//! However, it cannot be used safely in multi-threaded applications.
//!
//! The shared pointer cannot be downcasted. Support could be added.
template <typename T> class shared_ptr {
  public:
    //! The type of the stored pointer.
    using element_type = T;

    //! Construct a null pointer.
    constexpr shared_ptr() noexcept : data_{nullptr} {}

    //! Explicitly construct a null pointer.
    constexpr shared_ptr(std::nullptr_t) noexcept : data_{nullptr} {}

    //! Copy a shared pointer.
    shared_ptr(shared_ptr const &other) noexcept : data_{other.data_} { inc_(); }

    //! Move construct a shared pointer.
    shared_ptr(shared_ptr &&other) noexcept : data_{other.data_} { other.data_ = nullptr; }

    //! Copy assign a shared pointer.
    auto operator=(shared_ptr const &other) noexcept -> shared_ptr & {
        if (data_ != other.data_) {
            dec_();
            data_ = other.data_;
            inc_();
        }
        return *this;
    }

    //! Move assign a shared pointer.
    auto operator=(shared_ptr &&other) noexcept -> shared_ptr & {
        std::swap(data_, other.data_);
        return *this;
    }

    //! Check if the shared pointer is null.
    [[nodiscard]] explicit operator bool() const noexcept { return data_ != nullptr; }

    //! Get the reference count of the shared pointer.
    [[nodiscard]] auto use_count() const noexcept -> size_t { return data_->refs; }

    //! Get the raw pointer.
    [[nodiscard]] auto get() const noexcept -> element_type * { return &data_->value; }

    //! Dereference the pointer.
    [[nodiscard]] auto operator*() const noexcept -> element_type & { return *get(); }

    //! Get the member of pointer.
    auto operator->() const noexcept -> element_type * { return get(); }

    //! Decrement reference count and delete contained pointer if zero.
    ~shared_ptr() noexcept { dec_(); }

  private:
    struct data_type {
        template <class... Args> data_type(Args &&...args) : value{std::forward<Args>(args)...} {}
        size_t refs = 1;
        element_type value;
    };

    template <typename U, typename... Args> friend auto construct_shared(Args &&...args);

    template <typename... Args> static auto construct_(Args &&...args) -> shared_ptr;

    shared_ptr(data_type *data) noexcept : data_{data} {}

    void inc_() noexcept;
    void dec_() noexcept;

    data_type *data_;
};

//! Construct a shared pointer.
//!
//! @related shared_ptr
template <typename U, typename... Args> auto construct_shared(Args &&...args) {
    static_assert(std::is_constructible_v<U, Args...>);
    return shared_ptr<U>::construct_(std::forward<Args>(args)...);
}

#ifndef __clang_analyzer__

template <class T> template <typename... Args> auto shared_ptr<T>::construct_(Args &&...args) -> shared_ptr {
    return {new data_type(std::forward<Args>(args)...)};
}

template <class T> void shared_ptr<T>::inc_() noexcept {
    if (data_ != nullptr) {
        ++data_->refs;
    }
}

template <class T> void shared_ptr<T>::dec_() noexcept {
    if (data_ != nullptr) {
        --data_->refs;
        if (data_->refs == 0) {
            delete data_;
            data_ = nullptr;
        }
    }
}

#endif

//! Equality compare two shared pointers.
//!
//! @related shared_ptr
template <class X, class Y>
[[nodiscard]] auto operator==(const shared_ptr<X> &lhs, const shared_ptr<Y> &rhs) noexcept -> bool {
    return lhs.get() == rhs.get();
}

//! Inequality compare two shared pointers.
//!
//! @related shared_ptr
template <class X, class Y>
[[nodiscard]] auto operator!=(const shared_ptr<X> &lhs, const shared_ptr<Y> &rhs) noexcept -> bool {
    return lhs.get() != rhs.get();
}

//! @}

} // namespace Gringo::Util
