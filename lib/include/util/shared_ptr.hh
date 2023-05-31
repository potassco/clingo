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

    constexpr shared_ptr() noexcept : ptr_{nullptr} {}

    constexpr shared_ptr(std::nullptr_t) noexcept : ptr_{nullptr} {}

    shared_ptr(shared_ptr<element_type> const &other) noexcept : ptr_{other.ptr_} { inc_(); }

    shared_ptr(shared_ptr<element_type> &&other) noexcept : ptr_{other.ptr_} { other.ptr_ = nullptr; }

    auto operator=(shared_ptr<element_type> const &other) -> shared_ptr<element_type> & {
        if (ptr_ != other.ptr_) {
            dec_();
            ptr_ = other.ptr_;
            inc_();
        }
        return *this;
    }

    auto operator=(shared_ptr<element_type> &&other) noexcept -> shared_ptr<element_type> & {
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

template <typename T, typename B, typename... Args> auto construct_shared(Args &&...args) {
    return shared_ptr<B>{new T{std::forward<Args>(args)...}};
}
