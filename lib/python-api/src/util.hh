#pragma once

#include <clingo/core.h>

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

#define CLINGO_PY_TOTAL_ORDER                                                                                          \
    .def(py::self == py::self)                                                                                         \
        .def(py::self != py::self)                                                                                     \
        .def(py::self < py::self)                                                                                      \
        .def(py::self <= py::self)                                                                                     \
        .def(py::self > py::self)                                                                                      \
        .def(py::self >= py::self)

#define CLINGO_CPP_TOTAL_ORDER(type, T)                                                                                \
    [[maybe_unused]] type auto operator!=(T const &a, T const &b)->bool { return !(a == b); }                          \
    [[maybe_unused]] type auto operator<=(T const &a, T const &b)->bool { return !(b < a); }                           \
    [[maybe_unused]] type auto operator>(T const &a, T const &b)->bool { return b < a; }                               \
    [[maybe_unused]] type auto operator>=(T const &a, T const &b)->bool { return !(a < b); }

// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

namespace Clingo::Python {

namespace py = pybind11;

constexpr auto doc(char const *str) -> char const * {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return str + 1;
}

inline void handle_error(clingo_result_t code) {
    switch (static_cast<clingo_result_e>(code)) {
        case clingo_result_success: {
            break;
        }
        case clingo_result_unknown: {
            throw std::runtime_error("unknown error");
        }
        case clingo_result_runtime: {
            throw std::runtime_error("runtime error");
        }
        case clingo_result_logic: {
            throw std::logic_error("logic error");
        }
        case clingo_result_invalid: {
            throw std::invalid_argument("invalid argumets");
        }
        case clingo_result_range: {
            throw std::range_error("range error");
        }
        case clingo_result_bad_alloc: {
            throw std::bad_alloc();
        }
    }
}

//! Use std::transform to build a vector.
template <class It, class Pred> auto transform(It begin, It end, Pred pred) {
    auto p = std::vector<std::invoke_result_t<Pred, typename std::iterator_traits<It>::value_type>>{};
    p.reserve(std::distance(begin, end));
    std::transform(begin, end, std::back_inserter(p), pred);
    return p;
}

//! Use std::transform to build a vector.
template <class Rng, class Pred> auto transform(Rng const &rng, Pred pred) {
    using std::begin;
    using std::end;
    return transform(begin(rng), end(rng), pred);
}

//! Smart pointer whose copies model references.
template <class T, void (*deleter)(T *) noexcept> class owner_ptr {
  public:
    explicit constexpr owner_ptr(T *ptr) noexcept : ptr_{ptr}, own_{false} {}
    explicit constexpr owner_ptr(T *ptr, bool own) noexcept : ptr_{ptr}, own_{own && ptr != nullptr} {}
    constexpr owner_ptr() noexcept : ptr_{nullptr}, own_{false} {}
    constexpr owner_ptr(owner_ptr const &other) noexcept : ptr_{other.ptr_}, own_{false} {};
    constexpr owner_ptr(owner_ptr &&other) noexcept
        : ptr_{std::exchange(other.ptr_, nullptr)}, own_{std::exchange(other.own_, false)} {};
    // NOLINTNEXTLINE
    auto operator=(owner_ptr const &other) noexcept -> owner_ptr & {
        if (ptr_ != other.ptr_) {
            if (own_) {
                deleter(ptr_);
            }
            ptr_ = other.ptr_;
            own_ = false;
        }
        return *this;
    }
    auto operator=(owner_ptr &&other) noexcept -> owner_ptr & {
        if (ptr_ != other.ptr_) {
            if (own_) {
                deleter(ptr_);
            }
            ptr_ = std::exchange(other.ptr_, nullptr);
            own_ = std::exchange(other.own_, false);
        } else if (other.own_) {
            own_ = std::exchange(other.own_, false);
        }
        return *this;
    }
    ~owner_ptr() noexcept {
        if (own_) {
            deleter(ptr_);
        }
    }

    [[nodiscard]] auto get() const -> T * { return ptr_; }

    auto operator->() const -> T * { return ptr_; }

    void reset(T *ptr) noexcept {
        if (own_) {
            deleter(ptr_);
        }
        ptr_ = ptr;
        own_ = ptr != nullptr;
    }

    void reset(std::nullptr_t = nullptr) noexcept {
        if (own_) {
            deleter(ptr_);
        }
        ptr_ = nullptr;
        own_ = false;
    }

  private:
    T *ptr_;
    bool own_;
};

} // namespace Clingo::Python
