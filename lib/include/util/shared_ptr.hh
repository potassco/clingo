#pragma once

#include <cassert>
#include <cstddef>
#include <tuple>
#include <utility>

namespace Gringo::Util {

//! @defgroup core_util Utility
//! @ingroup core
//!
//! Utility data structures and functions.

//! @defgroup core_shared_ptr Shared Pointers
//! @ingroup core_util
//!
//! A shared and single owner pointer implementation.
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
    auto operator=(shared_ptr const &other) -> shared_ptr & {
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
    ~shared_ptr() { dec_(); }

  private:
    struct data_type {
        template <class... Args> data_type(Args &&...args) : value{std::forward<Args>(args)...} {}
        size_t refs = 1;
        element_type value;
    };

    template <typename Y> friend class shared_ptr;

    template <typename U, typename... Args> friend auto construct_shared(Args &&...args);

    constexpr shared_ptr(data_type *data) noexcept : data_{data} {}

    void inc_() noexcept {
        if (data_ != nullptr) {
            ++data_->refs;
        }
    }

    void dec_() {
        if (data_ != nullptr) {
            --data_->refs;
            if (data_->refs == 0) {
                delete data_;
                data_ = nullptr;
            }
        }
    }

    data_type *data_;
};

//! Construct a shared pointer.
//!
//! @related shared_ptr
template <typename U, typename... Args> auto construct_shared(Args &&...args) {
    using data_type = typename shared_ptr<U>::data_type;
    return shared_ptr<U>{new data_type(std::forward<Args>(args)...)};
}

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

//! A smart pointer with exactly one owner.
template <typename T> class single_owner_ptr {
  public:
    //! The type of the pointer.
    using element_type = T;

    //! Construct a null pointer.
    constexpr single_owner_ptr() noexcept : ptr_{nullptr}, owner_{false} {}

    //! Construct a null pointer.
    constexpr single_owner_ptr(std::nullptr_t ptr) noexcept : ptr_{ptr}, owner_{false} {}

    //! Copy another single owner pointer.
    single_owner_ptr(single_owner_ptr const &other) noexcept : ptr_{other.ptr_}, owner_{false} {}

    //! Copy a compatible single owner pointer.
    template <typename Y, typename = std::enable_if_t<std::is_base_of_v<T, Y>>>
    single_owner_ptr(single_owner_ptr<Y> const &other) noexcept : ptr_{other.ptr_}, owner_{false} {}

    //! Move construct a single owner pointer.
    //!
    //! This newly constructed object takes ownership if the target had ownership.
    single_owner_ptr(single_owner_ptr &&other) noexcept : ptr_{other.ptr_}, owner_{other.owner_} {
        other.ptr_ = nullptr;
        other.owner_ = false;
    }

    //! Move construct from a compatible a single owner pointer.
    //!
    //! This newly constructed object takes ownership if the target had ownership.
    template <typename Y, typename = std::enable_if_t<std::is_base_of_v<T, Y>>>
    single_owner_ptr(single_owner_ptr<Y> &&other) noexcept : ptr_{other.ptr_}, owner_{other.owner_} {
        other.ptr_ = nullptr;
        other.owner_ = false;
    }

    //! Copy assign a single owner pointer.
    auto operator=(single_owner_ptr const &other) -> single_owner_ptr & {
        if (ptr_ != other.ptr_) {
            ~single_owner_ptr();
            ptr_ = other.ptr_;
            owner_ = false;
        }
        return *this;
    }

    //! Copy assign from a compatible single owner pointer.
    template <typename Y, typename = std::enable_if_t<std::is_base_of_v<T, Y>>>
    auto operator=(single_owner_ptr<Y> const &other) -> single_owner_ptr & {
        if (ptr_ != other.ptr_) {
            ~single_owner_ptr();
            ptr_ = other.ptr_;
            owner_ = false;
        }
        return *this;
    }

    //! Move assign a single owner pointer.
    auto operator=(single_owner_ptr &&other) noexcept -> single_owner_ptr & {
        std::swap(ptr_, other.ptr_);
        return *this;
    }

    //! Move assign from a compatible single owner pointer.
    template <typename Y, typename = std::enable_if_t<std::is_base_of_v<T, Y>>>
    auto operator=(single_owner_ptr<Y> &&other) noexcept -> single_owner_ptr & {
        std::swap(ptr_, other.ptr_);
        return *this;
    }

    //! Check whether the pointer is null.
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    //! Check whether the pointer is owned.
    [[nodiscard]] auto is_owner() const noexcept -> bool { return owner_; }

    //! Get the pointer.
    [[nodiscard]] auto get() const noexcept -> element_type * { return ptr_; }

    //! Dereference the pointer.
    [[nodiscard]] auto operator*() const noexcept -> element_type & { return *ptr_; }

    //! Get the member of pointer.
    auto operator->() const noexcept -> element_type * { return ptr_; }

    //! Delete the pointer if it is owned.
    ~single_owner_ptr() {
        if (owner_) {
            delete ptr_;
        }
    }

  private:
    template <typename Y> friend class single_owner_ptr;
    template <typename Y, typename B, typename... Args> friend auto construct_single(Args &&...args);

    explicit single_owner_ptr(element_type *ptr) noexcept : ptr_{ptr}, owner_{true} {}

    element_type *ptr_;
    bool owner_;
};

//! Equality compare two single owner pointers.
//!
//! @related single_owner_ptr
template <class X, class Y>
[[nodiscard]] auto operator==(const single_owner_ptr<X> &lhs, const single_owner_ptr<Y> &rhs) noexcept -> bool {
    return lhs.get() == rhs.get();
}

//! Inequality compare two single owner pointers.
//!
//! @related single_owner_ptr
template <class X, class Y>
[[nodiscard]] auto operator!=(const single_owner_ptr<X> &lhs, const single_owner_ptr<Y> &rhs) noexcept -> bool {
    return lhs.get() != rhs.get();
}

//! Construct a single owner pointer.
//!
//! @related single_owner_ptr
template <typename T, typename B = T, typename... Args> auto construct_single(Args &&...args) {
    return single_owner_ptr<B>{new T{std::forward<Args>(args)...}};
}

//! @}

} // namespace Gringo::Util
