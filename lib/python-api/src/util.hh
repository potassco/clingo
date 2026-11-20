#pragma once

// NOTE: Python.h has to be included before any standard headers
#include <Python.h>

#include <clingo/core.h>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <limits>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace PyClingo {

//! Check whether the pointer stored at the given address is not null.
//!
//! The pointer can be of any pointer type; it is read via memcpy to not
//! violate aliasing rules.
inline auto pointer_valid(void const *slot) noexcept -> bool {
    void *ptr = nullptr;
    std::memcpy(static_cast<void *>(&ptr), slot, sizeof(ptr));
    return ptr != nullptr;
}

//! Get the address of a pointer as a slot for pointer_valid().
template <class P> auto pointer_slot(P *const *slot) noexcept -> void const * {
    return static_cast<void const *>(slot);
}

//! The pointer observed via invalid_slot().
inline void *const invalid_pointer = nullptr;

//! Get a slot for pointer_valid() that is never valid.
inline auto invalid_slot() noexcept -> void const * {
    return pointer_slot(&invalid_pointer);
}

//! Backport of std::unreachable from C++23.
[[noreturn]] inline void unreachable() {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else // GCC, Clang
    __builtin_unreachable();
#endif
}

//! Convert a hash value into a Python hash value.
//!
//! Python reserves -1 to indicate errors, so it is mapped to -2 (like Python does for integers).
inline auto py_hash(size_t value) -> Py_hash_t {
    auto res = static_cast<Py_hash_t>(value);
    return res == -1 ? -2 : res;
}

//! Combine the given hash values.
inline auto hash_combine(size_t a, size_t b) -> size_t {
    // NOLINTBEGIN
    auto p = std::make_pair(a, b);
    return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<char const *>(&p), sizeof(decltype(p))));
    // NOLINTEND
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

using PyCFunctionFastWithKeywords = PyObject *(*)(PyObject *, PyObject *const *, Py_ssize_t, PyObject *);

inline auto method_cast(PyCFunctionWithKeywords fun) noexcept -> PyCFunction {
    return reinterpret_cast<PyCFunction>(reinterpret_cast<void *>(fun)); // NOLINT
}

inline auto method_cast(PyCFunctionFastWithKeywords fun) noexcept -> PyCFunction {
    return reinterpret_cast<PyCFunction>(reinterpret_cast<void *>(fun)); // NOLINT
}

using IntEnumMember = std::tuple<char const *, int>;

auto add_int_enum(PyObject *module, char const *enum_name, std::span<IntEnumMember const> members, PyObject **store,
                  std::span<PyObject *> cache) -> bool;

auto add_type(PyObject *module, const char *name, PyType_Spec *spec, PyObject **store, PyObject *base = nullptr)
    -> bool;

//! Create a submodule, add it to the parent module and register it in sys.modules.
auto add_submodule(PyObject *module, PyModuleDef *def, PyObject **store) -> bool;

//! Register a type with an abstract base class from collections.abc.
//!
//! Registering with Sequence or Mapping sets Py_TPFLAGS_SEQUENCE or
//! Py_TPFLAGS_MAPPING, which are not part of the limited API, so that
//! instances match sequence or mapping patterns.
auto register_abc(PyObject *type, char const *abc) -> bool;

//! A state member and a function returning a new reference to the type to store in it.
template <class State> using TypeImport = std::pair<PyObject * State::*, PyObject *(*)()>;

//! Store types of other modules in a module state.
//!
//! This is the second registration phase. It runs after all modules have been
//! created, so the order in which modules are created does not matter.
template <class State, std::size_t N>
auto import_types(State *state, std::array<TypeImport<State>, N> const &imports) -> bool {
    // NOLINTNEXTLINE(readability-use-anyofallof)
    for (auto [member, get] : imports) {
        state->*member = get();
        if (state->*member == nullptr) {
            return false;
        }
    }
    return true;
}

inline void free_object(PyObject *self) {
    auto *type = Py_TYPE(self);

    if (PyType_HasFeature(type, Py_TPFLAGS_HAVE_GC) != 0) {
        PyObject_GC_UnTrack(self);
    }
    if (auto *clear = reinterpret_cast<inquiry>(PyType_GetSlot(type, Py_tp_clear)); clear != nullptr) {
        clear(self);
    }
    reinterpret_cast<freefunc>(PyType_GetSlot(type, Py_tp_free))(self);
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

//! A growable array of trivially copyable values.
//!
//! The container does not depend on Python and does not throw: allocation
//! failures are reported via return values and have to be handled by the
//! caller.
template <class T> class TrivialVector {
    static_assert(std::is_trivially_copyable_v<T>);

  public:
    TrivialVector() = default;
    TrivialVector(TrivialVector const &) = delete;
    auto operator=(TrivialVector const &) -> TrivialVector & = delete;
    TrivialVector(TrivialVector &&other) noexcept
        : data_{std::exchange(other.data_, nullptr)}, size_{std::exchange(other.size_, 0)},
          capacity_{std::exchange(other.capacity_, 0)} {}
    auto operator=(TrivialVector &&other) noexcept -> TrivialVector & {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        return *this;
    }
    ~TrivialVector() {
        std::free(data_); // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
    }

    //! Ensure capacity for at least n values.
    [[nodiscard]] auto reserve(size_t n) noexcept -> bool {
        if (n <= capacity_) {
            return true;
        }
        if (n > std::numeric_limits<size_t>::max() / sizeof(T)) {
            return false;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
        auto *data = static_cast<T *>(std::realloc(static_cast<void *>(data_), n * sizeof(T)));
        if (data == nullptr) {
            return false;
        }
        data_ = data;
        capacity_ = n;
        return true;
    }
    //! Append a value.
    [[nodiscard]] auto push_back(T const &value) noexcept -> bool {
        static constexpr size_t min_capacity = 8;
        if (size_ == capacity_ && !reserve(std::max(2 * capacity_, min_capacity))) {
            return false;
        }
        data_[size_++] = value;
        return true;
    }
    //! Remove all values keeping the capacity.
    void clear() noexcept { size_ = 0; }

    [[nodiscard]] auto data() const noexcept -> T * { return data_; }
    [[nodiscard]] auto size() const noexcept -> size_t { return size_; }
    [[nodiscard]] auto capacity() const noexcept -> size_t { return capacity_; }
    [[nodiscard]] auto empty() const noexcept -> bool { return size_ == 0; }

  private:
    T *data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

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

    auto resize(size_t n) -> bool {
        if (n == n_) {
            return true;
        }
        T *new_ptr = nullptr;
        if (n > 0) {
            new_ptr = static_cast<T *>(PyMem_Realloc(ptr_, n * sizeof(T)));
            if (new_ptr == nullptr) {
                PyErr_NoMemory();
                return false;
            }
        } else {
            PyMem_Free(ptr_);
        }
        ptr_ = new_ptr;
        n_ = n;
        return true;
    }

    [[nodiscard]]

    auto data() -> T * {
        return ptr_;
    }
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

//! Get a placeholder for required arguments set by parse_args() that fails every type check.
//!
//! Initializing the outputs with it instead of nullptr keeps clang-analyzer
//! from assuming that they are still null after parsing.
inline auto unset_arg() -> PyObject * {
    return Py_Ellipsis;
}

//! Parse required and optional arguments passed with the METH_FASTCALL | METH_KEYWORDS convention.
inline auto parse_args(char const *fname, std::initializer_list<char const *> req,
                       std::initializer_list<char const *> opt, PyObject *const *args, Py_ssize_t nargs,
                       PyObject *kwnames, std::initializer_list<PyObject **> out) -> bool {
    assert(req.size() + opt.size() == out.size());
    auto n = out.size();
    auto const *outs = out.begin();
    auto name = [&](size_t j) { return j < req.size() ? req.begin()[j] : opt.begin()[j - req.size()]; };
    for (auto *x : out) {
        *x = nullptr;
    }
    if (std::cmp_greater(nargs, n)) {
        PyErr_Format(PyExc_TypeError, "%s() takes at most %zu arguments (%zd given)", fname, n, nargs);
        return false;
    }
    for (Py_ssize_t i = 0; i < nargs; ++i) {
        *outs[i] = args[i];
    }
    if (kwnames != nullptr) {
        auto nkw = PyTuple_Size(kwnames);
        if (nkw < 0) {
            return false;
        }
        for (Py_ssize_t k = 0; k < nkw; ++k) {
            auto *key = PyTuple_GetItem(kwnames, k);
            if (key == nullptr) {
                return false;
            }
            size_t j = 0;
            while (j < n && PyUnicode_CompareWithASCIIString(key, name(j)) != 0) {
                ++j;
            }
            if (j == n) {
                PyErr_Format(PyExc_TypeError, "%s() got an unexpected keyword argument '%U'", fname, key);
                return false;
            }
            if (*outs[j] != nullptr) {
                PyErr_Format(PyExc_TypeError, "%s() got multiple values for argument '%s'", fname, name(j));
                return false;
            }
            *outs[j] = args[nargs + k];
        }
    }
    for (size_t j = 0; j < req.size(); ++j) {
        if (*outs[j] == nullptr) {
            PyErr_Format(PyExc_TypeError, "%s() missing required argument '%s'", fname, name(j));
            return false;
        }
    }
    return true;
}

} // namespace PyClingo
