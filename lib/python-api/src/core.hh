#pragma once

#include "util.hh"

#include <clingo.h>

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

inline auto operator==(clingo_location_t const &a, clingo_location_t const &b) -> bool {
    return clingo_location_equal(&a, &b);
}

inline auto operator<(clingo_location_t const &a, clingo_location_t const &b) -> bool {
    return clingo_location_less_than(&a, &b);
}

CLINGO_CPP_TOTAL_ORDER(inline, clingo_location_t)

namespace Clingo::Core {

namespace py = pybind11;

using LoggerCB = std::function<void(clingo_message_e, char const *)>;

static constexpr size_t default_message_limit = 25;

constexpr auto doc(char const *str) -> char const * {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return str + 1;
}

class Library {
  public:
    Library(bool shared, bool slotted, LoggerCB cb, size_t default_message_limit);
    Library(Library const &) = delete;
    Library(Library &&) = delete;
    ~Library() noexcept;
    void close() noexcept;

    operator clingo_lib_t *() const;

  private:
    static void logger_(clingo_message_t code, char const *message, void *self);

    clingo_lib_t *lib_ = nullptr;
    LoggerCB cb_;
};

inline void handle_error(clingo_lib_t *lib, bool success) {
    if (!success) {
        auto const *msg = clingo_error_message(lib);
        switch (static_cast<clingo_error_e>(clingo_error_code(lib))) {
            case clingo_error_success:
            case clingo_error_unknown:
            case clingo_error_runtime:
            case clingo_error_logic: {
                throw std::logic_error(msg);
            }
            case clingo_error_bad_alloc: {
                break;
            }
        }
        throw std::bad_alloc();
    }
}

struct Position {
    static auto construct(Library &lib, char const *file_name, size_t line, size_t column) -> Position;

    [[nodiscard]] auto str() const -> std::string;
    [[nodiscard]] auto repr() const -> std::string;
    [[nodiscard]] auto hash() const -> size_t;

    friend auto operator==(Position const &a, Position const &b) -> bool;
    friend auto operator<=>(Position const &a, Position const &b) -> std::strong_ordering;

    char const *file;
    size_t line;
    size_t column;
};

auto location_hash(clingo_location_t const &a) -> size_t;

[[nodiscard]] auto location_str(clingo_location_t const &loc) -> std::string;

[[nodiscard]] auto location_repr(clingo_location_t const &loc) -> std::string;

auto construct_location(Position const &begin, Position const &end) -> clingo_location_t;

void register_module(pybind11::module &m);

} // namespace Clingo::Core
