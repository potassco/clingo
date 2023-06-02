#pragma once

#include <cstddef>
#include <utility>

// Alternative intrusive shared_ptr implementation. It is intrusive to ease C
// integration and also avoid some complexity in view of an optimal make_shared
// implementation. This implementation should be faster than the STL
// implementation. However, it cannot be used safely in multi-threaded
// applications.
template <typename T> class shared_ptr {
  public:
    using element_type = T;

    explicit shared_ptr(element_type *ptr) noexcept : ptr_{ptr} { inc_(); }

    template <typename Y, typename = std::enable_if_t<std::is_base_of_v<T, Y>>>
    explicit shared_ptr(Y *ptr) noexcept : ptr_{ptr} {
        inc_();
    }

    constexpr shared_ptr() noexcept : ptr_{nullptr} {}

    constexpr shared_ptr(std::nullptr_t) noexcept : ptr_{nullptr} {}

    shared_ptr(shared_ptr const &other) noexcept : ptr_{other.ptr_} { inc_(); }

    template <typename Y, typename = std::enable_if_t<std::is_base_of_v<T, Y>>>
    shared_ptr(shared_ptr<Y> const &other) noexcept : ptr_{other.ptr_} {
        inc_();
    }

    shared_ptr(shared_ptr &&other) noexcept : ptr_{other.ptr_} { other.ptr_ = nullptr; }

    template <typename Y, typename = std::enable_if_t<std::is_base_of_v<T, Y>>>
    shared_ptr(shared_ptr<Y> &&other) noexcept : ptr_{other.ptr_} {
        other.ptr_ = nullptr;
    }

    auto operator=(shared_ptr const &other) -> shared_ptr & {
        if (ptr_ != other.ptr_) {
            dec_();
            ptr_ = other.ptr_;
            inc_();
        }
        return *this;
    }

    auto operator=(shared_ptr &&other) noexcept -> shared_ptr & {
        std::swap(ptr_, other.ptr_);
        return *this;
    }

    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    [[nodiscard]] auto use_count() const noexcept -> size_t { return ptr_->refs; }

    [[nodiscard]] auto get() const noexcept -> element_type * { return ptr_; }

    [[nodiscard]] auto operator*() const noexcept -> element_type & { return *ptr_; }

    auto operator->() const noexcept -> element_type * { return ptr_; }

    ~shared_ptr() { dec_(); }

  private:
    template <typename Y> friend class shared_ptr;

    void inc_() noexcept {
        if (ptr_ != nullptr) {
            ++ptr_->refs;
        }
    }

    void dec_() {
        if (ptr_ != nullptr) {
            --ptr_->refs;
            if (use_count() == 0) {
                delete ptr_;
                ptr_ = nullptr;
            }
        }
    }

    element_type *ptr_;
};

template <class X, class Y>
[[nodiscard]] auto operator==(const shared_ptr<X> &lhs, const shared_ptr<Y> &rhs) noexcept -> bool {
    return lhs.get() == rhs.get();
}

template <class X, class Y>
[[nodiscard]] auto operator!=(const shared_ptr<X> &lhs, const shared_ptr<Y> &rhs) noexcept -> bool {
    return lhs.get() != rhs.get();
}

template <typename T, typename B = T, typename... Args> auto construct_shared(Args &&...args) {
    return shared_ptr<B>{new T{std::forward<Args>(args)...}};
}
