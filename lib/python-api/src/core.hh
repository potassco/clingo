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

namespace Clingo::Python {

namespace py = pybind11;

//! Store the active exception as a clingo error.
auto store_error() -> bool;
//! Raise the current clingo error as an exception.
void raise_error();

//! This function should be used to handle failing C API calls.
//!
//! It rethrows the current error as a python error.
inline void handle_error(bool res) {
    if (!res) {
        raise_error();
    }
}

// Similar to handle_error but also reraises if the code is succsess.
void handle_error_no_code(bool res);

using Logger = std::function<void(clingo_message_e, std::string_view)>;

static constexpr size_t default_message_limit = 25;

auto user_data_slot() noexcept -> size_t;

class Library;
using PyLibrary = Annotation<Library>;

class Library {
  public:
    Library(bool shared, bool slotted, clingo_log_level_e level, Annotation<std::optional<Logger>> cb,
            size_t default_message_limit);
    ~Library();

    void close() noexcept;
    void tie(py::handle obj);

    operator clingo_lib_t *() const;

    static auto cast(clingo_lib_t *lib, bool convert = false) -> PyLibrary;
    static void acquire(clingo_lib_t *lib, bool inc = true);
    static void release(clingo_lib_t *lib) noexcept;
    static void setup(PyHeapTypeObject *heap_type);

  private:
    Library(clingo_lib_t *lib);

    static std::unordered_map<clingo_lib_t *, Library *> registry;
    static clingo_logger_t c_logger;

    ManagedPtr<Library, clingo_lib_t> lib_;
    py::list ref_;
};

static constexpr auto code_base = 36;

class StringBuilder {
  public:
    StringBuilder();
    StringBuilder(StringBuilder const &other);
    auto operator=(StringBuilder const &other) -> StringBuilder &;
    ~StringBuilder() noexcept;

    [[nodiscard]] auto str() const -> std::string_view;

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
    Position(Library &lib, std::string_view, size_t line, size_t column);
    Position(Position const &other);
    Position(Position &&other) noexcept;
    auto operator=(Position const &other) -> Position &;
    auto operator=(Position &&other) noexcept -> Position &;
    ~Position() noexcept;

    [[nodiscard]] auto file() const -> std::string_view;
    [[nodiscard]] auto line() const -> size_t;
    [[nodiscard]] auto column() const -> size_t;
    [[nodiscard]] auto str() const -> std::string_view;
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
    [[nodiscard]] auto str() const -> std::string_view;
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
