#pragma once

#include "iterable.hh"
#include "util.hh"

#include <charconv>
#include <clingo/core.h>

#include <pybind11/pybind11.h>

#include <span>

// NOLINTBEGIN(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

#define CLINGO_TRY try
#define CLINGO_CATCH(x)                                                                                                \
    catch (...) {                                                                                                      \
        return handle_error(x);                                                                                        \
    }                                                                                                                  \
    return clingo_result_success

// NOLINTEND(cppcoreguidelines-macro-usage,bugprone-macro-parentheses)

namespace Clingo::Python {

namespace py = pybind11;

using Logger = std::function<void(clingo_message_e, char const *)>;

static constexpr size_t default_message_limit = 25;

class Library {
  public:
    Library(bool shared, bool slotted, clingo_log_level_e level, Annotation<std::optional<Logger>> cb,
            size_t default_message_limit);
    Library(clingo_lib_t *lib) : lib_{lib} {}
    void close() noexcept;
    auto add_object(py::object script) -> PyObject * {
        objs_.emplace_front(std::move(script));
        return objs_.front().ptr();
    }
    static void setup(PyHeapTypeObject *heap_type);

    operator clingo_lib_t *() const;

  private:
    static void free_lib_(clingo_lib_t *lib) noexcept {
        if (lib != nullptr) {
            clingo_lib_free(lib, false);
        }
    }
    static void logger_(clingo_message_t code, char const *message, void *log) noexcept;

    owner_ptr<clingo_lib_t, free_lib_> lib_;
    std::forward_list<py::object> objs_;
};

static constexpr auto code_base = 36;

class PyClingoError : public std::exception {
  public:
    PyClingoError(clingo_result_t code) {
        std::to_chars(msg_.begin(), msg_.end(), static_cast<unsigned char>(code), code_base);
    }

    [[nodiscard]] auto what() const noexcept -> char const * override { return msg_.data(); }
    [[nodiscard]] auto code() const noexcept -> clingo_result_t {
        unsigned char res = 0;
        std::from_chars(msg_.begin(), msg_.end(), res, code_base);
        return res;
    }

  private:
    std::array<char, 3> msg_{};
};

//! This function should be used to handle failing C API calls.
//!
//! If the given exception pointer is not null and the code is unknown, the
//! exception is rethrown. Otherwise, the code is wrapped in a PyClingoError,
//! which has an empty error message. This error is intended to just forward
//! the code but is otherwise ignored by exception handling.
inline void handle_error(clingo_result_t code, std::exception_ptr const &ptr = nullptr) {
    switch (static_cast<clingo_result_e>(code)) {
        case clingo_result_success: {
            break;
        }
        case clingo_result_unknown: {
            if (ptr != nullptr) {
                std::rethrow_exception(ptr);
            }
            [[fallthrough]];
        }
        default: {
            throw PyClingoError{code};
        }
    }
}

//! Rethrows the current exception and stores it in the given exception
//! pointer.
//!
//! Must be called in a catch clause.
//!
//! If a pybind11 exception is to be stored, python's error indicator is
//! cleared to allow further execution of python code in the current thread.
//! Always returns result code unknown.
inline auto handle_error(std::exception_ptr &ptr) -> clingo_result_t {
    try {
        throw;
    } catch (py::error_already_set const &e) {
        auto gil = py::gil_scoped_acquire{};
        ptr = std::current_exception();
        PyErr_Clear();
    } catch (...) {
        ptr = std::current_exception();
    }
    return clingo_result_unknown;
}

//! Outputs the current exception via the libraries logger.
//!
//! Must be called in a catch clause.
//!
//! This function does not report PyClingoErrors, which are assumed to be
//! handled earlier. Their code is forwarded; other exceptions are reported and
//! mapped to a suitable code.
auto handle_error(clingo_lib_t *lib) -> clingo_result_t;

class StringBuilder {
  public:
    StringBuilder();
    StringBuilder(StringBuilder const &other);
    auto operator=(StringBuilder const &other) -> StringBuilder &;
    ~StringBuilder() noexcept;

    [[nodiscard]] auto str() const -> std::string;

    operator clingo_string_builder_t *() { return bld_; };
    operator clingo_string_builder_t const *() const { return bld_; };

  private:
    clingo_string_builder_t *bld_ = nullptr;
};

//! Get a thread local string builder.
auto string_builder() -> clingo_string_builder_t *;

class Position {
  public:
    explicit Position(clingo_position_t const *pos);
    Position(Library &lib, char const *file, size_t line, size_t column);
    Position(Position const &other);
    Position(Position &&other) noexcept;
    auto operator=(Position const &other) -> Position &;
    auto operator=(Position &&other) noexcept -> Position &;
    ~Position() noexcept;

    [[nodiscard]] auto file() const -> char const *;
    [[nodiscard]] auto line() const -> size_t;
    [[nodiscard]] auto column() const -> size_t;
    [[nodiscard]] auto str() const -> std::string;
    [[nodiscard]] auto repr() const -> std::string;
    [[nodiscard]] auto hash() const -> size_t;

    friend auto operator==(Position const &a, Position const &b) -> bool;
    friend auto operator<=>(Position const &a, Position const &b) -> std::strong_ordering;

    operator clingo_position_t const *() const { return pos_; };

  private:
    clingo_position_t const *pos_ = nullptr;
};

class Location {
  public:
    explicit Location(clingo_location_t const *loc);
    Location(Position const &begin, Position const &end);
    Location(Location const &other);
    Location(Location &&other) noexcept;
    auto operator=(Location const &other) -> Location &;
    auto operator=(Location &&other) noexcept -> Location &;
    ~Location() noexcept;

    [[nodiscard]] auto begin() const -> Position;
    [[nodiscard]] auto end() const -> Position;
    [[nodiscard]] auto str() const -> std::string;
    [[nodiscard]] auto repr() const -> std::string;
    [[nodiscard]] auto hash() const -> size_t;

    friend auto operator==(Location const &a, Location const &b) -> bool;
    friend auto operator<=>(Location const &a, Location const &b) -> std::strong_ordering;

    operator clingo_location_t const *() const { return loc_; };

  private:
    clingo_location_t const *loc_ = nullptr;
};

void register_core(pybind11::module &m);

using Lit_t = clingo_literal_t;
using LitVec = std::vector<Lit_t>;
using LitSpan = std::span<Lit_t const>;

using Atom_t = clingo_atom_t;
using AtomVec = std::vector<Atom_t>;
using AtomSpan = std::span<Atom_t const>;

using IdSpan = std::span<clingo_id_t>;

using WeightLitVec = std::vector<clingo_weighted_literal_t>;
using WeightLitSpan = std::span<clingo_weighted_literal_t const>;

} // namespace Clingo::Python

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
