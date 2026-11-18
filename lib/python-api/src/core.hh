#pragma once

#include "iterable.hh"
#include "util.hh" // IWYU pragma: keep

#include <clingo/core.h>

#include <pybind11/pybind11.h>

#include <span>

// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

#define CLINGO_TRY try
#define CLINGO_CATCH                                                                                                   \
    catch (...) {                                                                                                      \
        return store_error();                                                                                          \
    }                                                                                                                  \
    return true

// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

namespace PyClingo {

namespace py = pybind11;

//! Store the active exception as a clingo error.
//!
//! Pybind11 version.
auto store_error() -> bool;
//! Store the active exception as a clingo error.
//!
//! Plain C API version.
auto store_python_error(std::string_view fallback) -> bool;
//! Raise the current clingo error as an exception.
void raise_error();
//! Raise the current clingo error as a python exception.
//!
//! Plain C API version.
void raise_python_error();

//! This function should be used to handle failing C API calls.
//!
//! It rethrows the current error as a python error.
inline void handle_error(bool res) {
    if (!res) {
        raise_error();
    }
}

//! Transform a clingo error into a python error and return nullptr.
auto py_handle_error() -> std::nullptr_t;

// Similar to handle_error but also reraises if the code is succsess.
void handle_error_no_code(bool res, int code);

//! Report an error to stderr.
//!
//! An error is reported if res is false or the code indicates an error and a
//! clingo error is set.
auto report_main_error(bool res, int code, clingo_string_t name) -> bool;

using Logger = std::function<void(clingo_message_e, std::string_view)>;

static constexpr size_t default_message_limit = 25;

class Library;
using PyLibrary = Annotation<Library>;

template <auto Copy, auto Free> struct value_handle_traits {
    using pointer = std::remove_pointer_t<typename funptr_traits<decltype(Copy)>::template arg<1>>;
    using const_pointer = funptr_traits<decltype(Copy)>::template arg<0>;
    static auto copy(pointer p) -> pointer {
        auto res = pointer{};
        handle_error(Copy(p, &res));
        return res;
    }
    static void free(const_pointer p) noexcept { Free(p); }
};

class Library : public managed_handle<Library, clingo_lib_t>, public reference_keeper<Library> {
  public:
    static constexpr char const *invalid_message = "library is no longer valid";

    Library(bool shared, bool slotted, bool fast_release, clingo_log_level_e level,
            Annotation<std::optional<Logger>> cb, size_t default_message_limit);

    void close() noexcept;
    auto capsule() -> py::capsule;

    //! Get the library and throw if the wrapper is no longer valid.
    // NOLINTNEXTLINE
    operator clingo_lib_t *() const;

    static void acquire(clingo_lib_t *lib, bool inc = true);
    static void release(clingo_lib_t *lib) noexcept;

    //! Create a wrapper that neither acquires nor releases the library.
    static auto borrow(clingo_lib_t *lib) -> py::object;
    //! Detach the wrapper from its library.
    void invalidate() noexcept;

  private:
    using Parent = managed_handle<Library, clingo_lib_t>;
    Library(clingo_lib_t *lib, Parent::borrow_t tag);

    static clingo_logger_t c_logger;
};

static constexpr auto code_base = 36;

class StringBuilder {
  public:
    [[nodiscard]] auto str() const -> std::string_view;

    explicit operator clingo_string_builder_t *() { return bld_.get(); };
    explicit operator clingo_string_builder_t const *() const { return bld_.get(); };

  private:
    using Traits = value_handle_traits<clingo_string_builder_copy, clingo_string_builder_free>;
    value_handle<Traits> bld_;
};

//! Get a thread local string builder.
auto string_builder() -> clingo_string_builder_t *;

class Position {
  public:
    explicit Position(clingo_position_t const *pos);
    Position(Library &lib, std::string_view, size_t line, size_t column);

    [[nodiscard]] auto file() const -> std::string_view;
    [[nodiscard]] auto line() const -> size_t;
    [[nodiscard]] auto column() const -> size_t;
    [[nodiscard]] auto str() const -> std::string_view;
    [[nodiscard]] auto repr() const -> std::string;
    [[nodiscard]] auto hash() const -> size_t;

    friend auto operator==(Position const &a, Position const &b) -> bool;
    friend auto operator<=>(Position const &a, Position const &b) -> std::strong_ordering;

    // NOLINTNEXTLINE
    operator clingo_position_t const *() const { return pos_.get(); };

  private:
    using Traits = value_handle_traits<clingo_position_copy, clingo_position_free>;
    value_handle<Traits> pos_;
};

class Location {
  public:
    explicit Location(clingo_location_t const *loc);
    Location(Position const &begin, Position const &end);

    [[nodiscard]] auto begin() const -> Position;
    [[nodiscard]] auto end() const -> Position;
    [[nodiscard]] auto str() const -> std::string_view;
    [[nodiscard]] auto repr() const -> std::string;
    [[nodiscard]] auto hash() const -> size_t;

    friend auto operator==(Location const &a, Location const &b) -> bool;
    friend auto operator<=>(Location const &a, Location const &b) -> std::strong_ordering;

    // NOLINTNEXTLINE
    operator clingo_location_t const *() const { return loc_.get(); };

  private:
    using Traits = value_handle_traits<clingo_location_copy, clingo_location_free>;
    value_handle<Traits> loc_;
};

void register_core(pybind11::module &m);

// C API to access `clingo.core.Library` objects.
//
// NOTE: The functions mimic the interface of the module once it is ported to
// the C API. Library objects release their clingo library in tp_dealloc and
// not in tp_clear so that objects in reference cycles with the library (e.g.,
// scripts holding symbols) are released before the clingo library is freed.

//! Get the `clingo.core.Library` type (new reference).
auto py_library_type() -> PyObject *;

//! Get the clingo library of a `clingo.core.Library` object.
//!
//! Returns nullptr with a Python error set if the object is not a library or
//! has been invalidated.
auto py_library_get(PyObject *type, PyObject *obj) -> clingo_lib_t *;

//! Create a `clingo.core.Library` object not owning the given library.
//!
//! The object must be invalidated with `py_library_invalidate` before the
//! clingo library is destroyed. Returns a new reference or nullptr with a
//! Python error set.
auto py_library_borrow(PyObject *type, clingo_lib_t *lib) -> PyObject *;

//! Detach a `clingo.core.Library` object from its clingo library.
//!
//! Further use from Python raises an error.
void py_library_invalidate(PyObject *obj);

//! Keep the given object alive as long as the library object.
//!
//! The reference is visible to the garbage collector. Returns false with a
//! Python error set on failure.
auto py_library_keep(PyObject *type, PyObject *library, PyObject *obj) -> bool;

using Lit_t = clingo_literal_t;
using LitVec = std::vector<Lit_t>;
using LitSpan = std::span<Lit_t const>;

using Atom_t = clingo_atom_t;
using AtomVec = std::vector<Atom_t>;
using AtomSpan = std::span<Atom_t const>;

using IdSpan = std::span<clingo_id_t>;

using WeightLitVec = std::vector<clingo_weighted_literal_t>;
using WeightLitSpan = std::span<clingo_weighted_literal_t const>;

} // namespace PyClingo

namespace pybind11::detail {

template <> struct type_caster<clingo_weighted_literal_t> {
  public:
    using literal_conv = make_caster<clingo_literal_t>;
    using weight_conv = make_caster<clingo_weight_t>;
    using type = clingo_weighted_literal_t;

    PYBIND11_TYPE_CASTER(type, _("Tuple[int, int]"));

    auto load(handle src, bool convert) -> bool {
        if (!isinstance<tuple>(src) || len(src) != 2) {
            return false;
        }

        literal_conv lc;
        weight_conv wc;
        if (!lc.load(src[pybind11::int_{0}], convert) || !wc.load(src[pybind11::int_{1}], convert)) {
            return false;
        }

        value = clingo_weighted_literal_t{cast_op<clingo_literal_t>(lc), cast_op<clingo_weight_t>(wc)};
        return true;
    }

    static auto cast(const type &src, return_value_policy policy, handle parent) -> handle {
        return make_tuple(literal_conv::cast(src.literal, policy, parent),
                          weight_conv::cast(src.weight, policy, parent))
            .release();
    }
};

} // namespace pybind11::detail
