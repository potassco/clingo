#pragma once

#include <cstddef>
#include <utility>

#ifdef __clang_analyzer__
#include <memory>
#endif

namespace CppClingo::Util {

//! @addtogroup util_immutable
//! @{

//! An immutable value imlementation.
//!
//! It is faster than the std::shared_ptr implementation.
//! However, it cannot be used safely in multi-threaded applications.
template <typename T> class immutable_value {
  public:
    //! The type of the stored pointer.
    using element_type = T;

    //! Construct a null pointer.
    constexpr immutable_value() noexcept = default;

    //! Explicitly construct a null pointer.
    constexpr immutable_value(std::nullptr_t) noexcept {}

    //! Construct a value.
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    template <class U> immutable_value(U &&value) : immutable_value{std::in_place, std::forward<U>(value)} {}

#ifdef __clang_analyzer__
    template <class... Args>
    immutable_value([[maybe_unused]] std::in_place_t tag, Args &&...args)
        : data_{std::make_shared<T>(std::forward<Args>(args)...)} {}
    immutable_value(immutable_value const &other) noexcept = default;
    immutable_value(immutable_value &&other) noexcept = default;
    auto operator=(immutable_value const &other) noexcept -> immutable_value & = default;
    auto operator=(immutable_value &&other) noexcept -> immutable_value & = default;
    ~immutable_value() noexcept = default;
#else
    //! Construct a value in place.
    template <class... Args>
    immutable_value([[maybe_unused]] std::in_place_t tag, Args &&...args)
        : data_{new data_type(std::forward<Args>(args)...)} {}

    //! Copy an immutable value.
    immutable_value(immutable_value const &other) noexcept : data_{other.data_} { inc_(); }

    //! Move construct an immutable value.
    immutable_value(immutable_value &&other) noexcept : data_{std::exchange(other.data_, nullptr)} {}

    //! Copy assign an immutable value.
    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    auto operator=(immutable_value const &other) noexcept -> immutable_value & {
        other.inc_();
        dec_();
        data_ = other.data_;
        return *this;
    }

    //! Move assign an immutable value.
    auto operator=(immutable_value &&other) noexcept -> immutable_value & {
        if (this != &other) {
            dec_();
            data_ = std::exchange(other.data_, nullptr);
        }
        return *this;
    }

    //! Decrement reference count and delete contained pointer if zero.
    ~immutable_value() noexcept { dec_(); }
#endif

    //! Check if the value is engaged.
    [[nodiscard]] auto has_value() const noexcept -> bool { return data_ != nullptr; }

    //! Check if the value is engaged.
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    //! Get the value.
    [[nodiscard]] auto get() const noexcept -> element_type const & {
#ifdef __clang_analyzer__
        return *data_;
#else
        return data_->value;
#endif
    }

    //! Get the value.
    [[nodiscard]] auto operator*() const noexcept -> element_type const & { return get(); }

    //! Get the member of pointer.
    auto operator->() const noexcept -> element_type const * { return &get(); }

    //! Conversion operator.
    [[nodiscard]] operator T const &() const noexcept { return get(); }

  private:
#ifdef __clang_analyzer__
    std::shared_ptr<T> data_;
#else
    struct data_type {
        template <class... Args> data_type(Args &&...args) : value{std::forward<Args>(args)...} {}
        size_t refs = 1;
        element_type value;
    };

    void inc_() const noexcept {
        if (data_ != nullptr) {
            ++data_->refs;
        }
    }

    void dec_() noexcept {
        if (data_ != nullptr) {
            --data_->refs;
            if (data_->refs == 0) {
                delete data_;
            }
            data_ = nullptr;
        }
    }

    data_type *data_ = nullptr;
#endif
};

//! Construct an immutable value.
template <typename U, typename... Args> auto make_immutable(Args &&...args) -> immutable_value<U> {
    static_assert(std::is_constructible_v<U, Args...>);
    return immutable_value<U>{std::in_place, std::forward<Args>(args)...};
}

//! Compare two immutable values.
//!
//! @related immutable_value
template <class X, class Y>
[[nodiscard]] auto operator==(const immutable_value<X> &lhs, const immutable_value<Y> &rhs) -> bool {
    if (lhs && rhs) {
        return *lhs == *rhs;
    }
    return lhs.get() == rhs.get();
}

//! Compare two immutable values.
//!
//! @related immutable_value
template <class X, class Y>
[[nodiscard]] auto operator<=>(const immutable_value<X> &lhs, const immutable_value<Y> &rhs) {
    if (lhs && rhs) {
        return *lhs <=> *rhs;
    }
    return lhs.get() <=> rhs.get();
}

//! @}

} // namespace CppClingo::Util
