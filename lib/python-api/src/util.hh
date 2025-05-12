#pragma once

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <compare> // IWYU pragma: keep
#include <ranges>
#include <type_traits>
#include <variant>

namespace Clingo::Python {

namespace py = pybind11;

namespace Detail {

template <class T, class... Ts> constexpr bool is_variant_member_v = (std::is_same_v<T, Ts> || ...);

template <class... Ts, class T>
    requires is_variant_member_v<T, Ts...>
auto equal(T const &t, std::variant<Ts...> const &v) -> bool {
    return std::holds_alternative<T>(v) && t == std::get<T>(v);
}

template <class... Ts, class T>
    requires is_variant_member_v<T, Ts...>
auto compare(T const &t, std::variant<Ts...> const &v) {
    if (std::holds_alternative<T>(v)) {
        return t <=> std::get<T>(v);
    }
    return v.index() <=> std::variant<Ts...>(t).index();
}

} // namespace Detail

template <typename> struct funptr_traits;

template <typename R, typename... As> struct funptr_traits<R (*)(As...)> {
    using return_type = R;
    using args_tuple = std::tuple<As...>;
    template <std::size_t N> using arg = std::tuple_element_t<N, args_tuple>;

    static constexpr std::size_t arity = sizeof...(As);
};

template <class T> struct value_handle {
  public:
    using pointer = typename T::pointer;

    value_handle() = default;

    value_handle(const value_handle &other) : ptr_{other ? T::copy(other.ptr_) : nullptr} {}

    value_handle(value_handle &&other) noexcept : ptr_{std::exchange(other.ptr_, nullptr)} {}

    explicit value_handle(pointer ptr, bool copy) : ptr_{copy && ptr != nullptr ? T::copy(ptr) : ptr} {}

    ~value_handle() {
        if (*this) {
            T::free(ptr_);
        }
    }

    auto operator=(const value_handle &other) -> value_handle & {
        if (this != &other) {
            if (*this) {
                T::free(ptr_);
            }
            ptr_ = other ? T::copy(other.ptr_) : nullptr;
        }
        return *this;
    }

    auto operator=(value_handle &&other) noexcept -> value_handle & {
        if (this != &other) {
            if (*this) {
                T::free(ptr_);
            }
            ptr_ = std::exchange(other.ptr_, nullptr);
        }
        return *this;
    }

    [[nodiscard]] auto get() const -> pointer { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }

  private:
    pointer ptr_ = nullptr;
};

template <class T, class P> class registered_handle {
  private:
    friend T;

    registered_handle() = default;

    registered_handle(registered_handle const &other) = delete; // NOLINT

    auto operator=(registered_handle const &other) -> registered_handle & = delete; // NOLINT

    registered_handle(registered_handle &&other) noexcept : ptr_{std::exchange(other.ptr_, nullptr)} {
        auto it = registry.find(ptr_);
        assert(it != registry.end());
        it->second = static_cast<T *>(this);
    }

    auto operator=(registered_handle &&other) noexcept -> registered_handle & {
        if (this != &other) {
            assert(ptr_ == nullptr || ptr_ != other.ptr_);
            registry.erase(ptr_);
            T::release(ptr_);
            ptr_ = std::exchange(other.ptr_, nullptr);
            if (ptr_ != nullptr) {
                auto it = registry.find(get());
                it->second = static_cast<T *>(this);
            }
        }
        return *this;
    }
    ~registered_handle() noexcept {
        registry.erase(ptr_);
        T::release(ptr_);
    }
    explicit registered_handle(P *ptr, bool inc = true) : ptr_{ptr} {
        T::acquire(ptr_, inc);
        if (ptr_ != nullptr) {
            registry.emplace(ptr_, static_cast<T *>(this));
        }
    }

    [[nodiscard]] auto get() const noexcept -> P * { return ptr_; }
    void reset(std::nullptr_t = nullptr) noexcept {
        registry.erase(ptr_);
        T::release(ptr_);
        ptr_ = nullptr;
    }
    void reset(P *ptr, bool inc = true) {
        registry.erase(ptr_);
        T::release(ptr_);
        ptr_ = ptr;
        T::acquire(ptr_, inc);
        if (ptr_ != nullptr) {
            registry.emplace(ptr_, static_cast<T *>(this));
        }
    }

    static auto from_registry(P *ptr) -> T * {
        if (auto it = registry.find(ptr); it != registry.end()) {
            return it->second;
        }
        return nullptr;
    }

    static std::unordered_map<P *, T *> registry;
    P *ptr_ = nullptr;
};

template <class T, class P> std::unordered_map<P *, T *> registered_handle<T, P>::registry;

template <class T> class reference_keeper {
  public:
    void tie(py::handle obj) { list_.append(obj); }

    static void setup(PyHeapTypeObject *heap_type) {
        auto *type = &heap_type->ht_type;
        type->tp_flags |= Py_TPFLAGS_HAVE_GC;
        type->tp_traverse = [](PyObject *self_base, visitproc visit, void *arg) -> int {
            auto &self = py::cast<T &>(py::handle(self_base));
            Py_VISIT(self.list_.ptr());
            return 0;
        };
        type->tp_clear = [](PyObject *self_base) -> int {
            auto &self = py::cast<T &>(py::handle(self_base));
            Py_CLEAR(self.list_.ptr());
            return 0;
        };
    }

  private:
    friend T;

    reference_keeper() = default;

    py::list list_;
};

// NOLINTBEGIN
// A compile time string literal.
template <unsigned N> struct FixedString {
    char buf[N];
    constexpr FixedString(char const (&s)[N]) { std::copy_n(s, N, buf); }
};

// Docstring operator that simply drops the first newline.
//
// This facilitates writing nicely aligned docstrings.
template <FixedString S> consteval auto operator""_d() -> char const * {
    static_assert(S.buf[0] == '\n', "string must start with a newline");
    return S.buf + 1;
}
// NOLINTEND

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

template <class T, class P> class ManagedPtr {
  public:
    ManagedPtr() = default;
    explicit ManagedPtr(P *ptr, bool inc = true) : ptr_{ptr} { T::acquire(ptr, inc); }
    ManagedPtr(ManagedPtr const &other) : ManagedPtr{other.ptr_} {}
    ManagedPtr(ManagedPtr &&other) noexcept : ptr_{std::exchange(other.ptr_, nullptr)} {}
    ~ManagedPtr() noexcept { T::release(ptr_); }
    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    auto operator=(ManagedPtr const &other) -> ManagedPtr & {
        T::acquire(other.ptr_);
        T::release(ptr_);
        ptr_ = other.ptr_;
        return *this;
    }
    auto operator=(ManagedPtr &&other) noexcept -> ManagedPtr & {
        T::release(ptr_);
        ptr_ = std::exchange(other.ptr_, nullptr);
        return *this;
    }
    [[nodiscard]] auto get() const noexcept -> P * { return ptr_; }
    void reset(std::nullptr_t = nullptr) noexcept {
        T::release(ptr_);
        ptr_ = nullptr;
    }
    void reset(P *ptr, bool inc = true) {
        T::release(ptr_);
        ptr_ = ptr;
        T::acquire(ptr_, inc);
    }
    friend auto operator==(ManagedPtr const &p, std::nullptr_t) noexcept -> bool { return p.ptr_ == nullptr; }
    friend auto operator!=(ManagedPtr const &p, std::nullptr_t) noexcept -> bool { return p.ptr_ != nullptr; }
    friend auto operator==(std::nullptr_t, ManagedPtr const &p) noexcept -> bool { return p.ptr_ == nullptr; }
    friend auto operator!=(std::nullptr_t, ManagedPtr const &p) noexcept -> bool { return p.ptr_ != nullptr; }

  private:
    P *ptr_ = nullptr;
};

//! Smart pointer whose copies model references.
template <class T, void (*deleter)(T *) noexcept> class owner_ptr {
  public:
    //! Create a reference to the given pointer.
    explicit constexpr owner_ptr(T *ptr) noexcept : ptr_{ptr}, own_{false} {}
    //! Depending on the given flag create an owning pointer or a reference.
    explicit constexpr owner_ptr(T *ptr, bool own) noexcept : ptr_{ptr}, own_{own && ptr != nullptr} {}
    //! Create a null pointer.
    constexpr owner_ptr() noexcept : ptr_{nullptr}, own_{false} {}
    //! Create a reference to another pointer.
    constexpr owner_ptr(owner_ptr const &other) noexcept : ptr_{other.ptr_}, own_{false} {};
    //! Take ownership of another pointer.
    constexpr owner_ptr(owner_ptr &&other) noexcept
        : ptr_{std::exchange(other.ptr_, nullptr)}, own_{std::exchange(other.own_, false)} {};
    //! Copy assign another pointer.
    //!
    //! Does not take ownership of the source pointer. Frees the held pointer
    //! ownership is held.
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
    //! Move assign another pointer.
    //!
    //! Takes ownership if the source pointer had ownership. Frees the held
    //! pointer ownership is held.
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
    //! Free the held pointer ownership is held.
    ~owner_ptr() noexcept {
        if (own_) {
            deleter(ptr_);
        }
    }

    //! Get the held pointer.
    [[nodiscard]] auto get() const -> T * { return ptr_; }

    //! Member pointer operator.
    auto operator->() const -> T * { return ptr_; }

    //! Take ownership of the given pointer.
    //!
    //! Frees the held pointer if it had ownership.
    void reset(T *ptr) noexcept {
        if (own_) {
            deleter(ptr_);
        }
        ptr_ = ptr;
        own_ = ptr != nullptr;
    }

    //! Clear the pointer.
    //!
    //! Frees the held pointer if it had ownership.
    void reset(std::nullptr_t = nullptr) noexcept {
        if (own_) {
            deleter(ptr_);
        }
        ptr_ = nullptr;
        own_ = false;
    }

    //! Check if the pointer is owning.
    [[nodiscard]] auto is_owner() const -> bool { return own_; }

  private:
    T *ptr_;
    bool own_;
};

//! Backport of std::unreachable from C++23.
[[noreturn]] inline void unreachable() {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else // GCC, Clang
    __builtin_unreachable();
#endif
}

//! Transform the given range into a vector applying the given function to
//! eache element.
template <std::ranges::range R, class F> auto transform_vec(R &&rng, F &&fun) {
    // NOTE:
    // - can be written better in C++23 as
    //   return std::vector{std::from_range, std::ranges::views::transform(std::forward<R>(rng), std::forward<F>(fun))};
    // - the iterator based vector constructor should not be used in C++20 because
    //   it relies on C++17 iterator_traits, which do not play nicely with ranges
    // - see https://stackoverflow.com/questions/67606563/
    auto result = std::vector<std::decay_t<std::invoke_result_t<F, std::ranges::range_value_t<R>>>>{};
    if constexpr (std::ranges::sized_range<R>) {
        result.reserve(std::ranges::size(rng));
    }
    std::ranges::transform(std::forward<R>(rng), std::back_inserter(result), std::forward<F>(fun));
    return result;
}

//! Compute the hash for the given type.
//!
//! This is a convenience wrapper around the std::hash struct.
template <class T> auto hash_value(T const &x) {
    return std::hash<T>{}(x);
}

//! Combine the given hash values.
inline auto hash_combine(size_t a, size_t b) -> size_t {
    // NOLINTBEGIN
    auto p = std::make_pair(a, b);
    return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<char const *>(&p), sizeof(decltype(p))));
    // NOLINTEND
}

//! Adds the hash and equality dunders to the given class.
//!
//! Assumes that type T provides a hash member function and corresponding
//! comparison operators.
template <class T, typename... O> auto make_hashable(pybind11::class_<T, O...> cls) -> pybind11::class_<T, O...> {
    cls.def("__hash__", &T::hash, "Compute a hash for the object.").def(py::self == py::self).def(py::self != py::self);
    return cls;
}

//! Adds the hash and comparision dunders to the given class.
//!
//! Assumes that type T provides a hash member function and corresponding
//! comparison operators.
template <class T, typename... O> auto make_comparable(pybind11::class_<T, O...> cls) -> pybind11::class_<T, O...> {
    cls.def(py::self < py::self).def(py::self <= py::self).def(py::self > py::self).def(py::self >= py::self);
    return make_hashable(std::move(cls));
}

//! Adds the hash and comparision dunders to the given class.
//!
//! Additional comparison dunders are added for the given variant type S. A
//! variant S holding type T is compared using T's comparision operators and
//! otherwise the indices of type T and the one in the variant are
//! compared.
//!
//! Assumes that T is a variant member of S.
template <class S, typename T, typename... O>
auto make_comparable_base(pybind11::class_<T, O...> cls) -> pybind11::class_<T, O...> {
    cls.def("__eq__", [](T const &a, S const &b) -> bool { return Detail::equal(a, b); })
        .def("__ne__", [](T const &a, S const &b) -> bool { return !Detail::equal(a, b); })
        .def("__le__", [](T const &a, S const &b) -> bool { return Detail::compare(a, b) <= 0; })
        .def("__ge__", [](T const &a, S const &b) -> bool { return Detail::compare(a, b) >= 0; })
        .def("__lt__", [](T const &a, S const &b) -> bool { return Detail::compare(a, b) < 0; })
        .def("__gt__", [](T const &a, S const &b) -> bool { return Detail::compare(a, b) > 0; });
    return make_comparable<T>(std::move(cls));
}

} // namespace Clingo::Python
