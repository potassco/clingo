#pragma once

#include <Python.h>

#include <clingo/core.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <string_view>
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
    return PyGuard(Py_XNewRef(obj));
}

template <size_t N> auto cast_kw(std::array<char const *, N> const &kwlist) -> char ** {
    // NOLINTNEXTLINE
    return const_cast<char **>(kwlist.data());
}

using PyCFunctionFast = PyObject *(*)(PyObject *, PyObject *const *, Py_ssize_t);
using PyCFunctionFastWithKeywords = PyObject *(*)(PyObject *, PyObject *const *, Py_ssize_t, PyObject *);

inline auto method_cast(PyCFunctionWithKeywords fun) noexcept -> PyCFunction {
    return reinterpret_cast<PyCFunction>(reinterpret_cast<void *>(fun)); // NOLINT
}

inline auto method_cast(PyCFunctionFast fun) noexcept -> PyCFunction {
    return reinterpret_cast<PyCFunction>(reinterpret_cast<void *>(fun)); // NOLINT
}

inline auto method_cast(PyCFunctionFastWithKeywords fun) noexcept -> PyCFunction {
    return reinterpret_cast<PyCFunction>(reinterpret_cast<void *>(fun)); // NOLINT
}

struct IntEnumMember {
    char const *name;
    int value;
};

auto add_int_enum(PyObject *module, char const *enum_name, std::span<IntEnumMember const> members, PyObject **store,
                  std::span<PyObject *> cache) -> bool;

auto add_type(PyObject *module, const char *name, PyType_Spec *spec, PyObject **store, PyObject *base = nullptr)
    -> bool;

// asyncio integration
//
// The helpers only use the methods of asyncio's event loop and future
// interfaces, so that asyncio does not have to be imported.

//! Take the current Python error as an exception object (new reference).
auto take_error() -> PyObject *;

//! Create a future using the create_future method of the event loop (new reference).
auto future_new(PyObject *loop) -> PyObject *;

//! Check whether a future is done; returns -1 on error.
auto future_done(PyObject *fut) -> int;

//! Check whether a future was cancelled; returns -1 on error.
auto future_cancelled(PyObject *fut) -> int;

//! Set the result of a future unless it is already done (e.g., cancelled).
auto future_set_result(PyObject *fut, PyObject *value) -> bool;

//! Set the exception of a future unless it is already done (e.g., cancelled).
auto future_set_exception(PyObject *fut, PyObject *exc) -> bool;

//! Fail a future with StopAsyncIteration unless it is already done.
auto future_stop(PyObject *fut) -> bool;

//! Call the function bound to self with the future once it is done.
auto future_add_done(PyObject *fut, PyMethodDef *def, PyObject *self) -> bool;

//! Pass the outcome of the source future to the target future once it is done.
//!
//! The source future must not be cancelled. Nothing is passed if the target
//! future is already done.
auto future_chain(PyObject *src, PyObject *dst) -> bool;

//! Call the function bound to self on the thread of the event loop.
//!
//! The post argument is the call_soon_threadsafe method of the event loop.
//! The function must be called with the GIL held but can be called from any
//! thread.
auto loop_post(PyObject *post, PyMethodDef *def, PyObject *self) -> bool;

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

//! Parse a Boolean argument; absent arguments keep the output untouched.
inline auto parse_bool(PyObject *obj, bool *out) -> bool {
    if (obj == nullptr) {
        return true;
    }
    auto res = PyObject_IsTrue(obj);
    if (res < 0) {
        return false;
    }
    *out = res != 0;
    return true;
}

//! Parse a string argument; absent arguments keep the output untouched.
inline auto parse_str(PyObject *obj, std::string_view *out, char const *what) -> bool {
    if (obj == nullptr) {
        return true;
    }
    if (PyUnicode_Check(obj) == 0) {
        PyErr_Format(PyExc_TypeError, "%s must be a str", what);
        return false;
    }
    Py_ssize_t size = 0;
    auto const *str = PyUnicode_AsUTF8AndSize(obj, &size);
    if (str == nullptr) {
        return false;
    }
    *out = std::string_view{str, static_cast<size_t>(size)};
    return true;
}

//! A growable array.
//!
//! The container does not depend on Python and does not throw: allocation
//! failures are reported via return values and have to be handled by the
//! caller.
template <class T> class Vector {
    static_assert(std::is_nothrow_move_constructible_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);

  public:
    Vector() noexcept = default;

    Vector(Vector const &) = delete;
    auto operator=(Vector const &) -> Vector & = delete;

    Vector(Vector &&other) noexcept
        : data_{std::exchange(other.data_, nullptr)}, size_{std::exchange(other.size_, 0)},
          capacity_{std::exchange(other.capacity_, 0)} {}
    auto operator=(Vector &&other) noexcept -> Vector & {
        if (this != &other) {
            clear();
            deallocate(data_);

            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
        }
        return *this;
    }

    ~Vector() noexcept {
        clear();
        deallocate(data_);
    }

    [[nodiscard]] auto resize(std::size_t n) noexcept -> bool
        requires std::is_nothrow_default_constructible_v<T>
    {
        if (n > max_size()) {
            return false;
        }
        if (n > capacity_ && !reserve(n)) {
            return false;
        }

        if (n < size_) {
            std::destroy_n(data_ + n, size_ - n);
        } else {
            std::uninitialized_value_construct_n(data_ + size_, n - size_);
        }
        size_ = n;
        return true;
    }

    [[nodiscard]] auto reserve(std::size_t n) noexcept -> bool {
        if (n <= capacity_) {
            return true;
        }
        if (n > max_size()) {
            return false;
        }

        auto *new_data = allocate(n);
        if (new_data == nullptr) {
            return false;
        }

        std::uninitialized_move_n(data_, size_, new_data);
        std::destroy_n(data_, size_);
        deallocate(data_);

        data_ = new_data;
        capacity_ = n;
        return true;
    }

    template <class... Args>
        requires std::is_nothrow_constructible_v<T, Args &&...>
    [[nodiscard]] auto emplace_back(Args &&...args) noexcept -> bool {
        if (size_ < capacity_) {
            std::construct_at(data_ + size_, std::forward<Args>(args)...);
            ++size_;
            return true;
        }

        auto const max = max_size();
        if (capacity_ == max) {
            return false;
        }
        auto new_capacity = capacity_;
        if (new_capacity == 0) {
            new_capacity = std::min<std::size_t>(8, max); // NOLINT
        } else if (new_capacity > max / 2) {
            new_capacity = max;
        } else {
            new_capacity *= 2;
        }

        auto *new_data = allocate(new_capacity);
        if (new_data == nullptr) {
            return false;
        }

        std::construct_at(new_data + size_, std::forward<Args>(args)...);
        std::uninitialized_move_n(data_, size_, new_data);
        std::destroy_n(data_, size_);
        deallocate(data_);

        data_ = new_data;
        capacity_ = new_capacity;
        ++size_;
        return true;
    }

    void clear() noexcept {
        std::destroy_n(data_, size_);
        size_ = 0;
    }

    auto back() noexcept -> T & {
        assert(size_ > 0);
        return data_[size_ - 1];
    }
    auto front() noexcept -> T & {
        assert(size_ > 0);
        return data_[0];
    }

    auto operator[](std::size_t i) noexcept -> T & {
        assert(i < size_);
        return data_[i];
    }
    auto operator[](std::size_t i) const noexcept -> T const & {
        assert(i < size_);
        return data_[i];
    }

    [[nodiscard]] auto data() const noexcept -> T const * { return data_; }
    [[nodiscard]] auto data() noexcept -> T * { return data_; }
    [[nodiscard]] auto size() const noexcept -> std::size_t { return size_; }
    [[nodiscard]] auto capacity() const noexcept -> std::size_t { return capacity_; }
    [[nodiscard]] auto empty() const noexcept -> bool { return size_ == 0; }

  private:
    [[nodiscard]] static constexpr auto max_size() noexcept -> std::size_t {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }

    [[nodiscard]] static auto allocate(std::size_t n) noexcept -> T * {
        auto const bytes = n * sizeof(T);
        return static_cast<T *>(::operator new(bytes, std::align_val_t{alignof(T)}, std::nothrow));
    }

    static void deallocate(T *ptr) noexcept {
        if (ptr != nullptr) {
            ::operator delete(ptr, std::align_val_t{alignof(T)});
        }
    }

    T *data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
};

template <class T> struct PyBuffer {
    PyBuffer() = default;
    PyBuffer(PyBuffer const &) = delete;
    auto operator=(PyBuffer const &) -> PyBuffer & = delete;
    PyBuffer(PyBuffer &&other) noexcept : ptr_{std::exchange(other.ptr_, nullptr)}, n_{std::exchange(other.n_, 0)} {}
    auto operator=(PyBuffer &&other) noexcept -> PyBuffer & {
        if (this != &other) {
            if (ptr_ != nullptr) {
                PyMem_Free(ptr_);
            }
            ptr_ = std::exchange(other.ptr_, nullptr);
            n_ = std::exchange(other.n_, 0);
        }
        return *this;
    }

    ~PyBuffer() noexcept {
        if (ptr_ != nullptr) {
            PyMem_Free(ptr_);
        }
    }

    auto alloc(size_t n) noexcept -> bool {
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

    auto resize(size_t n) noexcept -> bool {
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

    auto data() noexcept -> T * {
        return ptr_;
    }
    [[nodiscard]] auto data() const noexcept -> T const * { return ptr_; }
    [[nodiscard]] auto size() const noexcept -> size_t { return n_; }

    auto operator[](size_t i) noexcept -> T & { return ptr_[i]; }
    auto operator[](size_t i) const noexcept -> T const & { return ptr_[i]; }

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
//!
//! Outputs of absent optional arguments keep their values, so they can be
//! initialized with defaults.
inline auto parse_args(char const *fname, std::initializer_list<char const *> req,
                       std::initializer_list<char const *> opt, PyObject *const *args, Py_ssize_t nargs,
                       PyObject *kwnames, std::initializer_list<PyObject **> out) -> bool {
    assert(req.size() + opt.size() == out.size());
    auto n = out.size();
    assert(n <= 64);
    auto const *outs = out.begin();
    auto name = [&](size_t j) { return j < req.size() ? req.begin()[j] : opt.begin()[j - req.size()]; };
    uint64_t seen = 0;
    if (std::cmp_greater(nargs, n)) {
        PyErr_Format(PyExc_TypeError, "%s() takes at most %zu arguments (%zd given)", fname, n, nargs);
        return false;
    }
    for (Py_ssize_t i = 0; i < nargs; ++i) {
        *outs[i] = args[i];
        seen |= uint64_t{1} << static_cast<size_t>(i);
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
            if ((seen & (uint64_t{1} << j)) != 0) {
                PyErr_Format(PyExc_TypeError, "%s() got multiple values for argument '%s'", fname, name(j));
                return false;
            }
            *outs[j] = args[nargs + k];
            seen |= uint64_t{1} << j;
        }
    }
    for (size_t j = 0; j < req.size(); ++j) {
        if ((seen & (uint64_t{1} << j)) == 0) {
            PyErr_Format(PyExc_TypeError, "%s() missing required argument '%s'", fname, name(j));
            return false;
        }
    }
    return true;
}

//! Parse required and optional arguments passed with the METH_FASTCALL convention.
inline auto parse_args(char const *fname, std::initializer_list<char const *> req,
                       std::initializer_list<char const *> opt, PyObject *const *args, Py_ssize_t nargs,
                       std::initializer_list<PyObject **> out) -> bool {
    return parse_args(fname, req, opt, args, nargs, nullptr, out);
}

} // namespace PyClingo
