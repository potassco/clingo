#pragma once

#include <cstddef>
#include <utility>

namespace Gringo::Util {

//! @defgroup core_util Utility
//! Utility data structures and functions.
//!
//! @ingroup core

//! @defgroup core_immutable Immutable Values
//! Immutable values and arrays with reference counting.
//!
//! @ingroup core_util
//!
//! @{

//! An immutable value imlementation.
//!
//! This implementation aims to ease C integration
//! making it is easy to extend to provide full control over the reference count.
//! It should also be faster than the STL implementation.
//! However, it cannot be used safely in multi-threaded applications.
template <typename T> class immutable_value {
  public:
    //! The type of the stored pointer.
    using element_type = T;

    //! Construct a null pointer.
    constexpr immutable_value() noexcept : data_{nullptr} {}

    //! Explicitly construct a null pointer.
    constexpr immutable_value(std::nullptr_t) noexcept : data_{nullptr} {}

    //! Copy a immutable value.
    immutable_value(immutable_value const &other) noexcept : data_{other.data_} { inc_(); }

    //! Move construct a immutable value.
    immutable_value(immutable_value &&other) noexcept : data_{other.data_} { other.data_ = nullptr; }

    //! Copy assign a immutable value.
    auto operator=(immutable_value const &other) noexcept -> immutable_value & {
        if (data_ != other.data_) {
            dec_();
            data_ = other.data_;
            inc_();
        }
        return *this;
    }

    //! Move assign a immutable value.
    auto operator=(immutable_value &&other) noexcept -> immutable_value & {
        std::swap(data_, other.data_);
        return *this;
    }

    //! Check if the immutable value is null.
    [[nodiscard]] explicit operator bool() const noexcept { return data_ != nullptr; }

    //! Get the reference count of the immutable value.
    [[nodiscard]] auto use_count() const noexcept -> size_t { return data_->refs; }

    //! Get the raw pointer.
    [[nodiscard]] auto get() const noexcept -> element_type const * { return &data_->value; }

    //! Dereference the pointer.
    [[nodiscard]] auto operator*() const noexcept -> element_type const & { return *get(); }

    //! Get the member of pointer.
    auto operator->() const noexcept -> element_type const * { return get(); }

    //! Decrement reference count and delete contained pointer if zero.
    ~immutable_value() noexcept { dec_(); }

  private:
    struct data_type {
        template <class... Args> data_type(Args &&...args) : value{std::forward<Args>(args)...} {}
        size_t refs = 1;
        element_type value;
    };

    template <typename U, typename... Args> friend auto make_immutable(Args &&...args);

    template <typename... Args> static auto construct_(Args &&...args) -> immutable_value;

    immutable_value(data_type *data) noexcept : data_{data} {}

    void inc_() noexcept;
    void dec_() noexcept;

    data_type *data_;
};

//! Construct a immutable value.
//!
//! @related immutable_value
template <typename U, typename... Args> auto make_immutable(Args &&...args) {
    static_assert(std::is_constructible_v<U, Args...>);
    return immutable_value<U>::construct_(std::forward<Args>(args)...);
}

#ifndef __clang_analyzer__

template <class T> template <typename... Args> auto immutable_value<T>::construct_(Args &&...args) -> immutable_value {
    return {new data_type(std::forward<Args>(args)...)};
}

template <class T> void immutable_value<T>::inc_() noexcept {
    if (data_ != nullptr) {
        ++data_->refs;
    }
}

template <class T> void immutable_value<T>::dec_() noexcept {
    if (data_ != nullptr) {
        --data_->refs;
        if (data_->refs == 0) {
            delete data_;
        }
        data_ = nullptr;
    }
}

#endif

//! Compare two immutable values.
//!
//! @related immutable_value
template <class X, class Y>
[[nodiscard]] auto operator==(const immutable_value<X> &lhs, const immutable_value<Y> &rhs) noexcept -> bool {
    if (lhs && rhs) {
        return *lhs == *rhs;
    }
    return lhs.get() == rhs.get();
}

//! Compare two immutable values.
//!
//! @related immutable_value
template <class X, class Y>
[[nodiscard]] auto operator<(const immutable_value<X> &lhs, const immutable_value<Y> &rhs) noexcept -> bool {
    if (lhs && rhs) {
        return *lhs < *rhs;
    }
    return lhs.get() < rhs.get();
}

//! @}

} // namespace Gringo::Util
