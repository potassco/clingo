#pragma once

#include <clingo/core.h>

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <compare> // IWYU pragma: keep
#include <concepts>
#include <limits>
#include <ranges>
#include <type_traits>
#include <utility>
#include <variant>

namespace PyClingo {

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
    using pointer = T::pointer;

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

    auto keeper_ref() -> PyObject * { return list_.ptr(); }

    static void setup(PyHeapTypeObject *heap_type) {
        auto *type = &heap_type->ht_type;
        type->tp_flags |= Py_TPFLAGS_HAVE_GC;
        type->tp_traverse = [](PyObject *self_base, visitproc visit, void *arg) -> int {
            auto *self = py::cast<T *>(py::handle(self_base));
            if (constructed_.contains(self)) {
                Py_VISIT(self->list_.ptr());
            }
            return 0;
        };
        type->tp_clear = [](PyObject *self_base) -> int {
            auto *self = py::cast<T *>(py::handle(self_base));
            if (constructed_.contains(self)) {
                Py_CLEAR(self->list_.ptr());
            }
            return 0;
        };
    }

    reference_keeper(reference_keeper const &other) = delete;
    reference_keeper(reference_keeper &&other) noexcept = delete;
    auto operator=(reference_keeper const &other) -> reference_keeper & = delete;
    auto operator=(reference_keeper &&other) noexcept -> reference_keeper & = delete;
    ~reference_keeper() { constructed_.erase(static_cast<T *>(this)); }

  private:
    friend T;

    // NOTE: we keep a set of constructed objects to avoid traversing/clearing
    // objects whose constructor has not been called. This is a bug in pybind11
    // tracked in issue: <https://github.com/pybind/pybind11/issues/4869>.
    static std::unordered_set<T *> constructed_;

    reference_keeper() { constructed_.emplace(static_cast<T *>(this)); }

    py::list list_;
};

template <class T> inline std::unordered_set<T *> reference_keeper<T>::constructed_ = {};

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

template <class T, auto Destroy> class Guard {
  public:
    constexpr explicit Guard(T v = {}) noexcept : value_{v} {}

    Guard(Guard const &other) = delete;
    auto operator=(Guard const &other) -> Guard & = delete;
    constexpr Guard(Guard &&other) noexcept : value_{std::exchange(other.value_, T{})} {}
    constexpr auto operator=(Guard &&other) noexcept -> Guard & {
        if (this != &other) {
            destroy_();
            value_ = std::exchange(other.value_, T{});
        }
        return *this;
    }

    ~Guard() noexcept { destroy_(); }

    // NOLINTNEXTLINE
    constexpr operator T &() noexcept { return value_; }
    [[nodiscard]] constexpr auto get() noexcept -> T & { return value_; }

    auto release() noexcept -> T {
        auto v = value_;
        value_ = T{};
        return v;
    }

  private:
    void destroy_() {
        if (value_ != T{}) {
            Destroy(value_);
        }
    }
    T value_{};
};

using PyGuard = Guard<PyObject *, Py_DECREF>;

class GILGuard {
  public:
    GILGuard() : state_{PyGILState_Ensure()} {}
    ~GILGuard() { PyGILState_Release(state_); }

    GILGuard(GILGuard const &) = delete;
    auto operator=(GILGuard const &) -> GILGuard & = delete;

  private:
    PyGILState_STATE state_;
};

class GILReleaseGuard {
  public:
    GILReleaseGuard() : state_(PyEval_SaveThread()) {}
    GILReleaseGuard(GILReleaseGuard const &) = delete;
    auto operator=(GILReleaseGuard const &) -> GILReleaseGuard & = delete;
    GILReleaseGuard(GILReleaseGuard &&) = delete;
    auto operator=(GILReleaseGuard &&) -> GILReleaseGuard & = delete;
    ~GILReleaseGuard() { PyEval_RestoreThread(state_); }

  private:
    PyThreadState *state_;
};

template <class F> auto with_gil_released(F &&f) -> decltype(auto) {
    auto guard = GILReleaseGuard{};
    return std::invoke(std::forward<F>(f));
}

inline auto steal(PyObject *obj) -> PyGuard {
    return PyGuard(obj);
}

inline auto borrow(PyObject *obj) -> PyGuard {
    Py_XINCREF(obj);
    return PyGuard(obj);
}

template <size_t N> auto cast_kw(std::array<char const *, N> const &kwlist) -> char ** {
    // NOLINTNEXTLINE
    return const_cast<char **>(kwlist.data());
}

inline auto py_c_function_cast(PyCFunctionWithKeywords fun) noexcept -> PyCFunction {
    return reinterpret_cast<PyCFunction>(reinterpret_cast<void *>(fun)); // NOLINT
}

using IntEnumMember = std::tuple<char const *, int>;

auto add_int_enum(PyObject *module, char const *enum_name, std::span<IntEnumMember const> members, PyObject **store,
                  std::span<PyObject *> cache) -> bool;

auto add_type(PyObject *module, const char *name, PyType_Spec *spec, PyObject **store, PyObject *base = nullptr)
    -> bool;

auto add_submodule(PyObject *module, PyModuleDef *def, PyObject **store) -> bool;

inline void free_object(PyObject *self) {
    PyTypeObject *type = Py_TYPE(self);

    if (PyType_HasFeature(type, Py_TPFLAGS_HAVE_GC) != 0) {
        PyObject_GC_UnTrack(self);
    }
    if (type->tp_clear != nullptr) {
        type->tp_clear(self);
    }
    type->tp_free(self);
    if (PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE) != 0) {
        Py_DECREF(type);
    }
}

template <std::integral T> auto parse_num(PyObject *obj, T *out, char const *what) -> bool {
    if (obj == nullptr) {
        return true;
    }
    if constexpr (std::is_unsigned_v<T>) {
        auto i = steal(PyNumber_Index(obj));
        if (i.get() == nullptr) {
            return false;
        }
        auto v = PyLong_AsUnsignedLongLong(i.get());
        if (v == std::numeric_limits<unsigned long long>::max() && PyErr_Occurred() != nullptr) {
            if (PyErr_ExceptionMatches(PyExc_OverflowError) != 0) {
                PyErr_Clear();
                PyErr_Format(PyExc_OverflowError, "%s is out of range", what);
            }
            return false;
        }
        if (std::cmp_greater(v, std::numeric_limits<T>::max())) {
            PyErr_Format(PyExc_OverflowError, "%s is out of range", what);
            return false;
        }
        *out = static_cast<T>(v);
    } else {
        auto v = PyLong_AsLongLong(obj);
        if (v == -1 && PyErr_Occurred() != nullptr) {
            if (PyErr_ExceptionMatches(PyExc_OverflowError) != 0) {
                PyErr_Clear();
                PyErr_Format(PyExc_OverflowError, "%s is out of range", what);
            }
            return false;
        }
        if (std::cmp_less(v, std::numeric_limits<T>::min()) || std::cmp_greater(v, std::numeric_limits<T>::max())) {
            PyErr_Format(PyExc_OverflowError, "%s is out of range", what);
            return false;
        }
        *out = static_cast<T>(v);
    }
    return true;
}

template <class T> auto py_cast(PyObject *obj) -> T * {
    try {
        auto res = py::handle{obj}.cast<T *>();
        if (res == nullptr) {
            PyErr_SetString(PyExc_TypeError, "object is not of the expected type");
        }
        return res;
    } catch (py::error_already_set &e) {
        e.restore();
        return nullptr;
    } catch (py::cast_error &) {
        PyErr_SetString(PyExc_TypeError, "object is not of the expected type");
        return nullptr;
    } catch (std::exception const &e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return nullptr;
    }
}

template <class T> struct PyBuffer {
    PyBuffer() = default;
    PyBuffer(PyBuffer const &) = delete;
    auto operator=(PyBuffer const &) -> PyBuffer & = delete;
    PyBuffer(PyBuffer &&) = delete;
    auto operator=(PyBuffer &&) -> PyBuffer & = delete;

    ~PyBuffer() {
        if (ptr_ != nullptr) {
            PyMem_Free(ptr_);
        }
    }

    auto alloc(size_t n) -> bool {
        T *new_ptr = nullptr;
        if (n > 0) {
            new_ptr = static_cast<T *>(PyMem_Calloc(n, sizeof(T)));
            if (new_ptr == nullptr) {
                PyErr_NoMemory();
                return false;
            }
        }
        if (ptr_ != nullptr) {
            PyMem_Free(ptr_);
        }
        ptr_ = new_ptr;
        n_ = n;
        return true;
    }

    auto data() -> T * { return ptr_; }
    [[nodiscard]] auto data() const -> T const * { return ptr_; }
    [[nodiscard]] auto size() const -> size_t { return n_; }

    auto operator[](size_t i) -> T & { return ptr_[i]; }
    auto operator[](size_t i) const -> T const & { return ptr_[i]; }

  private:
    T *ptr_{nullptr};
    size_t n_{0};
};

inline auto parse_string_sequence(PyObject *obj, PyGuard &seq, PyBuffer<clingo_string_t> &out, char const *what)
    -> bool {
    if (PyUnicode_Check(obj) != 0 || PyBytes_Check(obj) != 0) {
        PyErr_SetString(PyExc_TypeError, what);
        return false;
    }
    seq = steal(PySequence_Tuple(obj));
    if (seq.get() == nullptr) {
        return false;
    }
    auto n = PyTuple_Size(seq.get());
    if (n < 0) {
        return false;
    }
    if (!out.alloc(static_cast<size_t>(n))) {
        return false;
    }
    auto *it = out.data();
    for (Py_ssize_t i = 0; i < n; ++i) {
        auto *item = PyTuple_GetItem(seq.get(), i);
        if (item == nullptr) {
            return false;
        }
        if (PyUnicode_Check(item) == 0) {
            PyErr_SetString(PyExc_TypeError, what);
            return false;
        }
        Py_ssize_t len = 0;
        const auto *s = PyUnicode_AsUTF8AndSize(item, &len);
        if (s == nullptr) {
            return false;
        }
        *it++ = clingo_string_t{s, static_cast<size_t>(len)};
    }

    return true;
}

} // namespace PyClingo
